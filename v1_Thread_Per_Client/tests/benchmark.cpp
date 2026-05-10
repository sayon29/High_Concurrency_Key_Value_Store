#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <numeric>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <algorithm>
#include <fstream>
#include <filesystem>
#include <sstream>
#include <iomanip>

using namespace std::chrono;
namespace fs = std::filesystem;

struct Metrics {
    int total_requests = 0;
    int successful_requests = 0;
    std::vector<double> latencies;
};

class BenchmarkClient {
private:
    std::string ip;
    int port;
    int num_threads;
    int reqs_per_thread;

    // Connects to server, sends bytes, returns latency
    double send_request(int sock, const std::string& req) {
        auto start = high_resolution_clock::now();
        
        if (send(sock, req.c_str(), req.size(), 0) < 0) return -1.0;
        
        char buffer[1024];
        if (recv(sock, buffer, sizeof(buffer), 0) <= 0) return -1.0;
        
        auto end = high_resolution_clock::now();
        return duration<double, std::milli>(end - start).count();
    }

    void worker_task(Metrics& metrics) {
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) return;

        sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);
        inet_pton(AF_INET, ip.c_str(), &server_addr.sin_addr);

        if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            close(sock);
            return;
        }

        std::vector<std::string> workload;
        for(int i = 0; i < reqs_per_thread; i++) {
            if (i % 5 == 0) workload.push_back("SET key" + std::to_string(i) + " val\n");
            else workload.push_back("GET key" + std::to_string(i - 1) + "\n");
        }

        for(const auto& req : workload) {
            metrics.total_requests++;
            double lat = send_request(sock, req);
            if (lat >= 0.0) {
                metrics.latencies.push_back(lat);
                metrics.successful_requests++;
            }
        }

        close(sock);
    }

public:
    BenchmarkClient(std::string ip, int port, int threads, int reqs) 
        : ip(ip), port(port), num_threads(threads), reqs_per_thread(reqs) {}

    void run() {
        std::vector<std::thread> workers;
        std::vector<Metrics> thread_metrics(num_threads);

        std::cout << "Starting benchmark with " << num_threads << " threads, " 
                  << reqs_per_thread << " requests per thread..." << std::endl;

        auto start_time = high_resolution_clock::now();

        for(int i = 0; i < num_threads; i++) {
            workers.emplace_back(&BenchmarkClient::worker_task, this, std::ref(thread_metrics[i]));
        }

        for(auto& t : workers) {
            if (t.joinable()) t.join();
        }

        auto end_time = high_resolution_clock::now();
        double total_time_sec = duration<double>(end_time - start_time).count();

        aggregate_and_print(thread_metrics, total_time_sec);
    }

    void aggregate_and_print(std::vector<Metrics>& all_metrics, double total_time) {
        std::vector<double> all_latencies;
        int total_reqs = 0;
        int successful_reqs = 0;

        for(const auto& m : all_metrics) {
            total_reqs += m.total_requests;
            successful_reqs += m.successful_requests;
            all_latencies.insert(all_latencies.end(), m.latencies.begin(), m.latencies.end());
        }

        if (all_latencies.empty()) {
            std::cerr << "Error: No successful requests recorded." << std::endl;
            return;
        }

        std::sort(all_latencies.begin(), all_latencies.end());

        double rps = successful_reqs / total_time;
        double avg_lat = std::accumulate(all_latencies.begin(), all_latencies.end(), 0.0) / all_latencies.size();
        
        // Calculate p50 (Median) and p99
        double p50_lat = all_latencies[all_latencies.size() * 0.50];
        double p99_lat = all_latencies[all_latencies.size() * 0.99];

        // Format the output string
        std::ostringstream report;
        report << "======================================\n";
        report << "          BENCHMARK RESULTS           \n";
        report << "======================================\n";
        report << "Concurrency      : " << num_threads << " threads\n";
        report << "Total Attempted  : " << total_reqs << " requests\n";
        report << "Total Successful : " << successful_reqs << " requests\n";
        report << "Time taken       : " << std::fixed << std::setprecision(2) << total_time << " seconds\n";
        report << "Throughput       : " << std::fixed << std::setprecision(2) << rps << " req/sec\n";
        report << "--------------------------------------\n";
        report << "Avg Latency      : " << std::fixed << std::setprecision(3) << avg_lat << " ms\n";
        report << "p50 Latency      : " << std::fixed << std::setprecision(3) << p50_lat << " ms\n";
        report << "p99 Latency      : " << std::fixed << std::setprecision(3) << p99_lat << " ms\n";
        report << "======================================\n";

        // Print to console
        std::cout << "\n" << report.str();

        // Write to file
        save_to_file("../results/result.txt", report.str());
    }

private:
    void save_to_file(const std::string& filepath, const std::string& content) {
        try {
            fs::path pathObj(filepath);
            if (pathObj.has_parent_path()) {
                fs::create_directories(pathObj.parent_path());
            }

            std::ofstream outfile(filepath, std::ios::app); 
            if (outfile.is_open()) {
                outfile << content << "\n"; 
                outfile.close();
                std::cout << "Results successfully appended to " << filepath << std::endl;
            } else {
                std::cerr << "Failed to open file for writing: " << filepath << std::endl;
            }
        } catch (const std::exception& e) {
            std::cerr << "Error writing to file: " << e.what() << std::endl;
        }
    }
};

int main(int argc, char* argv[]) {
    int concurrency = 10;
    int requests_per_thread = 10000;
    
    int opt;
    while ((opt = getopt(argc, argv, "c:r:")) != -1) {
        switch (opt) {
            case 'c':
                concurrency = std::stoi(optarg);
                break;
            case 'r':
                requests_per_thread = std::stoi(optarg);
                break;
            default:
                std::cerr << "Usage: " << argv[0] << " [-c concurrency] [-r requests]\n";
                return 1;
        }
    }

    BenchmarkClient bench("127.0.0.1", 8080, concurrency, requests_per_thread);
    bench.run();
    
    return 0;
}