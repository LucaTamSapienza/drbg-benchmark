/**
 * @file main.cpp
 * @brief Main program for DRBG benchmarking and comparison
 * 
 * This program implements and compares two Deterministic Random Bit Generators:
 * 1. CTR-DRBG (Counter mode DRBG)
 * 2. Hash-DRBG (SHA-256 based)
 * 
 * Comparison metrics:
 * - Time: Generation time for different sequence lengths
 * - Space: Memory footprint (internal state size)
 * - Bit Distribution: Count of 0s and 1s, bias from 50%
 */

#include <iostream>
#include <iomanip>
#include <memory>
#include <vector>
#include <random>
#include <cmath>
#include "drbg.hpp"
#include "benchmark.hpp"

/**
 * @brief Generate initial seed using system entropy
 * @param size Number of bytes for the seed
 * @return Vector of random seed bytes
 */
std::vector<uint8_t> generateSeed(size_t size = 32) {
    std::random_device rd;
    std::vector<uint8_t> seed(size);
    for (size_t i = 0; i < size; i += 4) {
        uint32_t val = rd();
        for (size_t j = 0; j < 4 && i + j < size; ++j) {
            seed[i + j] = static_cast<uint8_t>((val >> (j * 8)) & 0xFF);
        }
    }
    return seed;
}

/**
 * @brief Print a formatted header
 */
void printHeader() {
    std::cout << "\n";
    std::cout << "╔══════════════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║          DRBG Benchmark - Cybersecurity Homework 5                       ║\n";
    std::cout << "║          Deterministic Random Bit Generator Comparison                   ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════════════════════╝\n";
    std::cout << "\n";
}

/**
 * @brief Print DRBG information
 */
void printDRBGInfo() {
    std::cout << "┌─────────────────────────────────────────────────────────────────────────┐\n";
    std::cout << "│                      DRBG Algorithms Implemented                        │\n";
    std::cout << "├─────────────────────────────────────────────────────────────────────────┤\n";
    std::cout << "│ 1. CTR-DRBG   : Counter mode DRBG based on AES-like block cipher       │\n";
    std::cout << "│ 2. Hash-DRBG  : NIST SP 800-90A compliant, uses SHA-256                │\n";
    std::cout << "└─────────────────────────────────────────────────────────────────────────┘\n\n";
}

/**
 * @brief Print benchmark progress
 */
void printProgress(const std::string& drbg_name, size_t bits, int current, int total) {
    std::cout << "\r  [" << current << "/" << total << "] "
              << std::setw(12) << drbg_name << " | "
              << std::setw(10) << bits << " bits" << std::flush;
}

/**
 * @brief Print a single benchmark result
 */
void printResult(const BenchmarkResult& r) {
    std::cout << "  │ " << std::setw(10) << r.drbg_name 
              << " │ " << std::setw(10) << r.num_bits
              << " │ " << std::setw(12) << std::fixed << std::setprecision(2) << r.generation_time_us
              << " │ " << std::setw(12) << r.count_zeros
              << " │ " << std::setw(12) << r.count_ones
              << " │ " << std::setw(10) << std::setprecision(6) << r.bias * 100
              << "% │\n";
}

int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[]) {
    printHeader();
    printDRBGInfo();
    
    // Generate seeds for all DRBGs
    auto seed = generateSeed(48);  // 384-bit seed
    
    std::cout << "📋 Seed generated: " << seed.size() << " bytes from system entropy\n\n";
    
    // Create DRBG instances
    std::vector<std::unique_ptr<DRBG>> drbgs;
    drbgs.push_back(std::make_unique<CTR_DRBG>(seed));
    drbgs.push_back(std::make_unique<Hash_DRBG>(seed));
    
    // Print state sizes
    std::cout << "💾 Internal State Sizes:\n";
    for (const auto& drbg : drbgs) {
        std::cout << "   • " << std::setw(12) << drbg->getName() 
                  << ": " << drbg->getStateSize() << " bytes\n";
    }
    std::cout << "\n";
    
    // Define test sequence lengths: 10^1 to 10^7
    std::vector<size_t> bit_lengths = {
        10,          // 10^1
        100,         // 10^2
        1000,        // 10^3
        10000,       // 10^4
        100000,      // 10^5
        1000000,     // 10^6
        10000000     // 10^7
    };
    
    std::vector<BenchmarkResult> all_results;
    int total_tests = static_cast<int>(drbgs.size() * bit_lengths.size());
    int current_test = 0;
    
    std::cout << "🚀 Running benchmarks...\n";
    
    // Run benchmarks
    for (const auto& drbg : drbgs) {
        // Reseed for each DRBG to ensure fair comparison
        drbg->reseed(seed);
        
        for (size_t bits : bit_lengths) {
            current_test++;
            printProgress(drbg->getName(), bits, current_test, total_tests);
            
            auto result = Benchmark::run(drbg.get(), bits);
            all_results.push_back(result);
        }
    }
    
    std::cout << "\n\n✅ Benchmarks completed!\n\n";
    
    // Print results table
    std::cout << "┌────────────────────────────────────────────────────────────────────────────────────┐\n";
    std::cout << "│                              BENCHMARK RESULTS                                     │\n";
    std::cout << "├────────────┬────────────┬──────────────┬──────────────┬──────────────┬────────────┤\n";
    std::cout << "│    DRBG    │    Bits    │   Time (μs)  │    Zeros     │    Ones      │   Bias     │\n";
    std::cout << "├────────────┼────────────┼──────────────┼──────────────┼──────────────┼────────────┤\n";
    
    for (const auto& r : all_results) {
        printResult(r);
    }
    
    std::cout << "└────────────┴────────────┴──────────────┴──────────────┴──────────────┴────────────┘\n\n";
    
    // Export results
    std::cout << "📁 Exporting results...\n";
    
    Benchmark::exportToCSV(all_results, "benchmark_results.csv");
    std::cout << "   ✓ CSV data saved to: benchmark_results.csv\n";
    
    Benchmark::generatePlotScript("benchmark_results.csv", "plot_results.py");
    std::cout << "   ✓ Python plot script saved to: plot_results.py\n";
    
    Benchmark::generateHTMLVisualization(all_results, "visualization.html");
    std::cout << "   ✓ HTML visualization saved to: visualization.html\n";
    
    // Print summary statistics
    std::cout << "\n";
    std::cout << "╔══════════════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║                           SUMMARY STATISTICS                             ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════════════════════╝\n\n";
    
    for (const auto& drbg : drbgs) {
        std::cout << "📊 " << drbg->getName() << ":\n";
        
        double total_time = 0;
        double total_bias = 0;
        double max_throughput = 0;
        int count = 0;
        
        for (const auto& r : all_results) {
            if (r.drbg_name == drbg->getName()) {
                total_time += r.generation_time_us;
                total_bias += r.bias;
                max_throughput = std::max(max_throughput, r.bits_per_microsecond);
                count++;
            }
        }
        
        std::cout << "   • State Size:      " << drbg->getStateSize() << " bytes\n";
        std::cout << "   • Total Time:      " << std::fixed << std::setprecision(2) 
                  << total_time / 1000.0 << " ms\n";
        std::cout << "   • Avg Bias:        " << std::setprecision(6) 
                  << (total_bias / count) * 100 << " %\n";
        std::cout << "   • Max Throughput:  " << std::setprecision(2) 
                  << max_throughput << " bits/μs\n\n";
    }
    
    std::cout << "═══════════════════════════════════════════════════════════════════════════\n";
    std::cout << "📖 To view visualizations:\n";
    std::cout << "   • Open 'visualization.html' in a web browser for interactive charts\n";
    std::cout << "   • Run 'python3 plot_results.py' to generate PNG/SVG plots\n";
    std::cout << "═══════════════════════════════════════════════════════════════════════════\n";
    
    return 0;
}
