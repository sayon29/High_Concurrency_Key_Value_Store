#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <numeric>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <algorithm>
#include <fstream>
#include <filesystem>
#include <sstream>
#include <iomanip>
#include <mutex>

using namespace std::chrono;
namespace fs = std::filesystem;

enum class ConnStatus { CONNECTING, SENDING, READING, DONE };

struct Metrics {
    int total_requests = 0;
    int successful_requests = 0;
    int connection_failures = 0;
    std::vector<double> latencies;
};

struct ConnState {
    int fd;
    int reqs_done = 0;
    ConnStatus status = ConnStatus::CONNECTING;
    high_resolution_clock::time_point last_start;
};

class BenchmarkClient {
private:
    std::string ip;
    int port;
    int total_conns;
    int reqs_per_thread;
    int num_threads;

    void set_nonblocking(int fd) {
        fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
    }

    void bucket_worker(int conns_to_manage, Metrics& m) {
        std::vector<ConnState> states;
        std::vector<struct pollfd> fds;

        for (int i = 0; i < conns_to_manage; ++i) {
            int fd = socket(AF_INET, SOCK_STREAM, 0);
            if (fd < 0) {
                m.connection_failures++;
                continue;
            }
            set_nonblocking(fd);

            sockaddr_in addr;
            addr.sin_family = AF_INET;
            addr.sin_port = htons(port);
            inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);

            connect(fd, (struct sockaddr*)&addr, sizeof(addr));
            
            states.push_back({fd, 0, ConnStatus::CONNECTING, {}});
            fds.push_back({fd, POLLOUT, 0}); 
        }

        int completed = 0;
        while (completed < (int)states.size()) {
            int ret = poll(fds.data(), fds.size(), 100);
            if (ret < 0) break;

            for (size_t i = 0; i < fds.size(); ++i) {
                if (fds[i].fd == -1) continue;

                if (fds[i].revents & (POLLERR | POLLHUP | POLLNVAL)) {
                    close(fds[i].fd);
                    fds[i].fd = -1;
                    completed++;
                    m.connection_failures++;
                    continue;
                }

                if (fds[i].revents & POLLOUT) {
                    if (states[i].status == ConnStatus::CONNECTING) {
                        states[i].status = ConnStatus::SENDING;
                    }

                    if (states[i].status == ConnStatus::SENDING) {
                        std::string req;
                        if (states[i].reqs_done < reqs_per_thread - 1) {
                            req = (states[i].reqs_done % 2 == 0) ? "SET k v\n" : "GET k\n";
                        } else {
                            req = "QUIT\n";
                        }

                        states[i].last_start = high_resolution_clock::now();
                        if (send(fds[i].fd, req.c_str(), req.length(), 0) > 0) {
                            if (req == "QUIT\n") {
                                states[i].status = ConnStatus::DONE;
                                close(fds[i].fd);
                                fds[i].fd = -1;
                                completed++;
                            } else {
                                states[i].status = ConnStatus::READING;
                                fds[i].events = POLLIN; 
                            }
                        }
                    }
                }
                // Ready to READ response
                else if (fds[i].revents & POLLIN) {
                    char buf[1024];
                    int r = recv(fds[i].fd, buf, sizeof(buf), 0);
                    if (r > 0) {
                        auto end = high_resolution_clock::now();
                        m.latencies.push_back(duration<double, std::milli>(end - states[i].last_start).count());
                        m.successful_requests++;
                        states[i].reqs_done++;
                        
                        states[i].status = ConnStatus::SENDING;
                        fds[i].events = POLLOUT; 
                        m.total_requests++;
                    } else if (r == 0) { 
                        close(fds[i].fd);
                        fds[i].fd = -1;
                        completed++;
                    }
                }
            }
        }
    }

public:
    BenchmarkClient(std::string ip, int port, int threads, int conns_per_t) 
        : ip(ip), port(port), num_threads(threads), total_conns(conns_per_t * threads), reqs_per_thread(100) {}

    void run() {
        std::vector<std::thread> workers;
        std::vector<Metrics> all_metrics(num_threads);
        int conns_per_thread = total_conns / num_threads;

        std::cout << "Starting V2 Benchmark: " << total_conns << " connections managed by " 
                  << num_threads << " threads." << std::endl;

        auto start_time = high_resolution_clock::now();

        for(int i = 0; i < num_threads; i++) {
            workers.emplace_back(&BenchmarkClient::bucket_worker, this, conns_per_thread, std::ref(all_metrics[i]));
        }

        for(auto& t : workers) if (t.joinable()) t.join();

        auto end_time = high_resolution_clock::now();
        double total_time_sec = duration<double>(end_time - start_time).count();

        aggregate_and_print(all_metrics, total_time_sec);
    }

    void aggregate_and_print(std::vector<Metrics>& all_metrics, double total_time) {
        std::vector<double> all_latencies;
        int total_reqs = 0, successful_reqs = 0, total_fails = 0;

        for(const auto& m : all_metrics) {
            total_reqs += m.total_requests;
            successful_reqs += m.successful_requests;
            total_fails += m.connection_failures;
            all_latencies.insert(all_latencies.end(), m.latencies.begin(), m.latencies.end());
        }

        if (all_latencies.empty()) {
            std::cerr << "Error: No successful requests recorded." << std::endl;
            return;
        }

        std::sort(all_latencies.begin(), all_latencies.end());

        double rps = successful_reqs / total_time;
        double avg_lat = std::accumulate(all_latencies.begin(), all_latencies.end(), 0.0) / all_latencies.size();
        double p50_lat = all_latencies[all_latencies.size() * 0.50];
        double p95_lat = all_latencies[all_latencies.size() * 0.95];
        double p99_lat = all_latencies[all_latencies.size() * 0.99];

        std::ostringstream report;
        report << "======================================\n";
        report << "       V2 STABILITY BENCHMARK         \n";
        report << "======================================\n";
        report << "Total Connections: " << total_conns << "\n";
        report << "Total Successful : " << successful_reqs << " requests\n";
        report << "Connection Fails : " << total_fails << "\n";
        report << "Time taken       : " << std::fixed << std::setprecision(2) << total_time << " seconds\n";
        report << "Throughput       : " << std::fixed << std::setprecision(2) << rps << " req/sec\n";
        report << "--------------------------------------\n";
        report << "Avg Latency      : " << std::fixed << std::setprecision(3) << avg_lat << " ms\n";
        report << "p50 Latency      : " << std::fixed << std::setprecision(3) << p50_lat << " ms\n";
        report << "p95 Latency      : " << std::fixed << std::setprecision(3) << p95_lat << " ms\n";
        report << "p99 Latency      : " << std::fixed << std::setprecision(3) << p99_lat << " ms\n";
        report << "======================================\n";

        std::cout << "\n" << report.str();
        save_to_file("results/v2_stability_report.txt", report.str());
    }

    void save_to_file(const std::string& filepath, const std::string& content) {
        fs::path p(filepath);
        if (p.has_parent_path()) fs::create_directories(p.parent_path());
        std::ofstream outfile(filepath, std::ios::app);
        if (outfile.is_open()) outfile << content << "\n";
    }
};

int main(int argc, char* argv[]) {
    int concurrency = 10;           // Threads in the benchmark tool
    int conns_per_thread = 2000;    // Conns managed per benchmark thread
    
    int opt;
    while ((opt = getopt(argc, argv, "c:r:")) != -1) {
        switch (opt) {
            case 'c': concurrency = std::stoi(optarg); break;
            case 'r': conns_per_thread = std::stoi(optarg); break;
        }
    }

    BenchmarkClient bench("127.0.0.1", 8080, concurrency, conns_per_thread);
    bench.run();
    return 0;
}