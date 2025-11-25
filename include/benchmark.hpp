/**
 * @file benchmark.hpp
 * @brief Benchmarking utilities for DRBG comparison
 */

#ifndef BENCHMARK_HPP
#define BENCHMARK_HPP

#include "drbg.hpp"
#include <chrono>
#include <memory>
#include <string>
#include <vector>
#include <fstream>

/**
 * @struct BenchmarkResult
 * @brief Stores the results of a single benchmark run
 */
struct BenchmarkResult {
    std::string drbg_name;
    size_t num_bits;
    
    // Timing metrics (in microseconds)
    double generation_time_us;
    
    // Space metrics (in bytes)
    size_t state_size;
    size_t output_size;
    
    // Bit distribution metrics
    size_t count_zeros;
    size_t count_ones;
    double ratio;  // ones / zeros
    double bias;   // deviation from 0.5
    
    // Derived metrics
    double bits_per_microsecond;
};

/**
 * @class Benchmark
 * @brief Utility class for running DRBG benchmarks
 */
class Benchmark {
public:
    /**
     * @brief Run a complete benchmark on a DRBG
     * @param drbg Pointer to the DRBG to benchmark
     * @param num_bits Number of bits to generate
     * @return BenchmarkResult containing all metrics
     */
    static BenchmarkResult run(DRBG* drbg, size_t num_bits);
    
    /**
     * @brief Count zeros and ones in a byte array
     * @param data The byte array to analyze
     * @param num_bits Number of bits to count (may be less than data.size() * 8)
     * @return Pair of (zeros, ones) counts
     */
    static std::pair<size_t, size_t> countBits(const std::vector<uint8_t>& data, size_t num_bits);
    
    /**
     * @brief Export results to CSV format
     * @param results Vector of benchmark results
     * @param filename Output filename
     */
    static void exportToCSV(const std::vector<BenchmarkResult>& results, const std::string& filename);
    
    /**
     * @brief Generate a Python plotting script
     * @param csv_file Input CSV file name
     * @param output_file Output script filename
     */
    static void generatePlotScript(const std::string& csv_file, const std::string& output_file);
    
    /**
     * @brief Generate an HTML visualization
     * @param results Vector of benchmark results
     * @param filename Output HTML filename
     */
    static void generateHTMLVisualization(const std::vector<BenchmarkResult>& results, 
                                          const std::string& filename);
};

/**
 * @class Timer
 * @brief High-resolution timer for benchmarking
 */
class Timer {
private:
    std::chrono::high_resolution_clock::time_point start_time;
    
public:
    void start() {
        start_time = std::chrono::high_resolution_clock::now();
    }
    
    double elapsedMicroseconds() const {
        auto end_time = std::chrono::high_resolution_clock::now();
        return std::chrono::duration<double, std::micro>(end_time - start_time).count();
    }
    
    double elapsedMilliseconds() const {
        return elapsedMicroseconds() / 1000.0;
    }
};

#endif // BENCHMARK_HPP
