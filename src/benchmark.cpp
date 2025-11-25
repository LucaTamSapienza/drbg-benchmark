/**
 * @file benchmark.cpp
 * @brief Implementation of benchmarking utilities
 */

#include "benchmark.hpp"
#include <iomanip>
#include <sstream>
#include <cmath>

BenchmarkResult Benchmark::run(DRBG* drbg, size_t num_bits) {
    BenchmarkResult result;
    result.drbg_name = drbg->getName();
    result.num_bits = num_bits;
    result.state_size = drbg->getStateSize();
    
    Timer timer;
    
    // Generate random bits and measure time
    timer.start();
    auto data = drbg->generate(num_bits);
    result.generation_time_us = timer.elapsedMicroseconds();
    
    result.output_size = data.size();
    
    // Count bit distribution
    auto [zeros, ones] = countBits(data, num_bits);
    result.count_zeros = zeros;
    result.count_ones = ones;
    
    // Calculate ratio and bias
    result.ratio = (zeros > 0) ? static_cast<double>(ones) / zeros : 0;
    result.bias = std::abs(0.5 - (static_cast<double>(ones) / num_bits));
    
    // Calculate throughput
    result.bits_per_microsecond = (result.generation_time_us > 0) 
        ? num_bits / result.generation_time_us 
        : 0;
    
    return result;
}

std::pair<size_t, size_t> Benchmark::countBits(const std::vector<uint8_t>& data, size_t num_bits) {
    size_t zeros = 0;
    size_t ones = 0;
    size_t bits_counted = 0;
    
    for (size_t byte_idx = 0; byte_idx < data.size() && bits_counted < num_bits; ++byte_idx) {
        uint8_t byte = data[byte_idx];
        for (int bit = 7; bit >= 0 && bits_counted < num_bits; --bit) {
            if ((byte >> bit) & 1) {
                ones++;
            } else {
                zeros++;
            }
            bits_counted++;
        }
    }
    
    return {zeros, ones};
}

void Benchmark::exportToCSV(const std::vector<BenchmarkResult>& results, const std::string& filename) {
    std::ofstream file(filename);
    
    // Header
    file << "DRBG,NumBits,GenerationTimeUs,StateSize,OutputSize,"
         << "Zeros,Ones,Ratio,Bias,BitsPerMicrosecond\n";
    
    // Data
    for (const auto& r : results) {
        file << r.drbg_name << ","
             << r.num_bits << ","
             << std::fixed << std::setprecision(2) << r.generation_time_us << ","
             << r.state_size << ","
             << r.output_size << ","
             << r.count_zeros << ","
             << r.count_ones << ","
             << std::setprecision(6) << r.ratio << ","
             << std::setprecision(8) << r.bias << ","
             << std::setprecision(2) << r.bits_per_microsecond << "\n";
    }
    
    file.close();
}

void Benchmark::generatePlotScript(const std::string& csv_file, const std::string& output_file) {
    std::ofstream file(output_file);
    
    file << R"(#!/usr/bin/env python3
"""
DRBG Benchmark Visualization Script
Generates plots comparing DRBG performance metrics
"""

import pandas as pd
import matplotlib.pyplot as plt
import numpy as np

# Read the benchmark data
df = pd.read_csv(')" << csv_file << R"(')

# Get unique DRBG names
drbgs = df['DRBG'].unique()
colors = ['#2ecc71', '#3498db']

# Create figure with subplots
fig, axes = plt.subplots(2, 2, figsize=(14, 10))
fig.suptitle('DRBG Performance Comparison', fontsize=16, fontweight='bold')

# 1. Generation Time vs Sequence Length
ax1 = axes[0, 0]
for i, drbg in enumerate(drbgs):
    data = df[df['DRBG'] == drbg]
    ax1.plot(data['NumBits'], data['GenerationTimeUs'], 
             marker='o', label=drbg, color=colors[i % len(colors)], linewidth=2)
ax1.set_xscale('log')
ax1.set_yscale('log')
ax1.set_xlabel('Sequence Length (bits)', fontsize=11)
ax1.set_ylabel('Generation Time (μs)', fontsize=11)
ax1.set_title('Time Complexity', fontsize=12, fontweight='bold')
ax1.legend()
ax1.grid(True, alpha=0.3)

# 2. Throughput (bits per microsecond)
ax2 = axes[0, 1]
for i, drbg in enumerate(drbgs):
    data = df[df['DRBG'] == drbg]
    ax2.plot(data['NumBits'], data['BitsPerMicrosecond'], 
             marker='s', label=drbg, color=colors[i % len(colors)], linewidth=2)
ax2.set_xscale('log')
ax2.set_xlabel('Sequence Length (bits)', fontsize=11)
ax2.set_ylabel('Throughput (bits/μs)', fontsize=11)
ax2.set_title('Generation Throughput', fontsize=12, fontweight='bold')
ax2.legend()
ax2.grid(True, alpha=0.3)

# 3. Bit Distribution Bias
ax3 = axes[1, 0]
for i, drbg in enumerate(drbgs):
    data = df[df['DRBG'] == drbg]
    ax3.plot(data['NumBits'], data['Bias'] * 100, 
             marker='^', label=drbg, color=colors[i % len(colors)], linewidth=2)
ax3.set_xscale('log')
ax3.set_xlabel('Sequence Length (bits)', fontsize=11)
ax3.set_ylabel('Bias from 50% (%)', fontsize=11)
ax3.set_title('Bit Distribution Bias', fontsize=12, fontweight='bold')
ax3.legend()
ax3.grid(True, alpha=0.3)
ax3.axhline(y=0, color='gray', linestyle='--', alpha=0.5)

# 4. Memory Usage (State Size) - Bar chart
ax4 = axes[1, 1]
state_sizes = [df[df['DRBG'] == drbg]['StateSize'].iloc[0] for drbg in drbgs]
bars = ax4.bar(drbgs, state_sizes, color=colors[:len(drbgs)])
ax4.set_xlabel('DRBG Algorithm', fontsize=11)
ax4.set_ylabel('State Size (bytes)', fontsize=11)
ax4.set_title('Memory Footprint', fontsize=12, fontweight='bold')
ax4.grid(True, alpha=0.3, axis='y')

# Add value labels on bars
for bar, size in zip(bars, state_sizes):
    ax4.text(bar.get_x() + bar.get_width()/2, bar.get_height() + 1, 
             f'{size}', ha='center', va='bottom', fontsize=10)

plt.tight_layout()
plt.savefig('drbg_comparison.png', dpi=150, bbox_inches='tight')
plt.savefig('drbg_comparison.svg', format='svg', bbox_inches='tight')
print("Plots saved as 'drbg_comparison.png' and 'drbg_comparison.svg'")
plt.show()

# Additional: Create a summary table
print("\n" + "="*80)
print("BENCHMARK SUMMARY")
print("="*80)

for drbg in drbgs:
    data = df[df['DRBG'] == drbg]
    print(f"\n{drbg}:")
    print(f"  State Size: {data['StateSize'].iloc[0]} bytes")
    print(f"  Max Throughput: {data['BitsPerMicrosecond'].max():.2f} bits/μs")
    print(f"  Avg Bias: {data['Bias'].mean() * 100:.4f}%")
    print(f"  Time for 10^7 bits: {data[data['NumBits'] == 10000000]['GenerationTimeUs'].values[0]/1000:.2f} ms")
)";
    
    file.close();
}

void Benchmark::generateHTMLVisualization(const std::vector<BenchmarkResult>& results,
                                           const std::string& filename) {
    std::ofstream file(filename);
    
    file << R"(<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>DRBG Benchmark Results</title>
    <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body { 
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            background: linear-gradient(135deg, #1a1a2e 0%, #16213e 100%);
            color: #eee;
            min-height: 100vh;
            padding: 20px;
        }
        .container { max-width: 1400px; margin: 0 auto; }
        h1 { 
            text-align: center; 
            margin-bottom: 30px;
            font-size: 2.5em;
            background: linear-gradient(90deg, #00d2ff, #3a7bd5);
            -webkit-background-clip: text;
            -webkit-text-fill-color: transparent;
        }
        .charts-grid {
            display: grid;
            grid-template-columns: repeat(2, 1fr);
            gap: 20px;
            margin-bottom: 30px;
        }
        .chart-container {
            background: rgba(255, 255, 255, 0.05);
            border-radius: 15px;
            padding: 20px;
            box-shadow: 0 8px 32px rgba(0, 0, 0, 0.3);
        }
        .chart-container h2 {
            text-align: center;
            margin-bottom: 15px;
            font-size: 1.2em;
            color: #00d2ff;
        }
        canvas { max-height: 300px; }
        .summary-table {
            width: 100%;
            border-collapse: collapse;
            margin-top: 20px;
            background: rgba(255, 255, 255, 0.05);
            border-radius: 10px;
            overflow: hidden;
        }
        .summary-table th, .summary-table td {
            padding: 12px 15px;
            text-align: center;
            border-bottom: 1px solid rgba(255, 255, 255, 0.1);
        }
        .summary-table th {
            background: rgba(0, 210, 255, 0.2);
            font-weight: 600;
        }
        .summary-table tr:hover { background: rgba(255, 255, 255, 0.05); }
        .metric-card {
            display: inline-block;
            background: rgba(255, 255, 255, 0.1);
            padding: 15px 25px;
            border-radius: 10px;
            margin: 10px;
            text-align: center;
        }
        .metric-card h3 { font-size: 2em; color: #00d2ff; }
        .metric-card p { color: #aaa; }
    </style>
</head>
<body>
    <div class="container">
        <h1>🔐 DRBG Benchmark Results</h1>
        
        <div class="charts-grid">
            <div class="chart-container">
                <h2>⏱️ Generation Time (log scale)</h2>
                <canvas id="timeChart"></canvas>
            </div>
            <div class="chart-container">
                <h2>🚀 Throughput (bits/μs)</h2>
                <canvas id="throughputChart"></canvas>
            </div>
            <div class="chart-container">
                <h2>⚖️ Bit Distribution Bias</h2>
                <canvas id="biasChart"></canvas>
            </div>
            <div class="chart-container">
                <h2>💾 Memory Footprint</h2>
                <canvas id="memoryChart"></canvas>
            </div>
        </div>
        
        <h2 style="text-align: center; margin: 30px 0;">📊 Detailed Results</h2>
        <table class="summary-table">
            <thead>
                <tr>
                    <th>DRBG</th>
                    <th>Bits Generated</th>
                    <th>Time (μs)</th>
                    <th>Zeros</th>
                    <th>Ones</th>
                    <th>Bias (%)</th>
                    <th>Throughput (bits/μs)</th>
                </tr>
            </thead>
            <tbody>
)";

    // Add table rows
    for (const auto& r : results) {
        file << "                <tr>\n"
             << "                    <td>" << r.drbg_name << "</td>\n"
             << "                    <td>" << r.num_bits << "</td>\n"
             << "                    <td>" << std::fixed << std::setprecision(2) << r.generation_time_us << "</td>\n"
             << "                    <td>" << r.count_zeros << "</td>\n"
             << "                    <td>" << r.count_ones << "</td>\n"
             << "                    <td>" << std::setprecision(4) << (r.bias * 100) << "</td>\n"
             << "                    <td>" << std::setprecision(2) << r.bits_per_microsecond << "</td>\n"
             << "                </tr>\n";
    }

    file << R"(            </tbody>
        </table>
    </div>

    <script>
        const colors = {
            'CTR-DRBG': '#2ecc71',
            'Hash-DRBG': '#3498db'
        };

        // Prepare data from results
        const results = [
)";

    // Embed results as JavaScript array
    for (size_t i = 0; i < results.size(); ++i) {
        const auto& r = results[i];
        file << "            { name: '" << r.drbg_name << "', "
             << "bits: " << r.num_bits << ", "
             << "time: " << std::fixed << std::setprecision(2) << r.generation_time_us << ", "
             << "stateSize: " << r.state_size << ", "
             << "bias: " << std::setprecision(8) << r.bias << ", "
             << "throughput: " << std::setprecision(2) << r.bits_per_microsecond << " }";
        if (i < results.size() - 1) file << ",";
        file << "\n";
    }

    file << R"(        ];

        // Group by DRBG name
        const drbgNames = [...new Set(results.map(r => r.name))];
        const bitSizes = [...new Set(results.map(r => r.bits))].sort((a, b) => a - b);

        // Time Chart
        new Chart(document.getElementById('timeChart'), {
            type: 'line',
            data: {
                labels: bitSizes.map(b => b.toExponential(0)),
                datasets: drbgNames.map(name => ({
                    label: name,
                    data: bitSizes.map(bits => {
                        const r = results.find(x => x.name === name && x.bits === bits);
                        return r ? r.time : null;
                    }),
                    borderColor: colors[name],
                    backgroundColor: colors[name] + '33',
                    tension: 0.3
                }))
            },
            options: {
                responsive: true,
                scales: {
                    y: { type: 'logarithmic', title: { display: true, text: 'Time (μs)' } }
                }
            }
        });

        // Throughput Chart
        new Chart(document.getElementById('throughputChart'), {
            type: 'line',
            data: {
                labels: bitSizes.map(b => b.toExponential(0)),
                datasets: drbgNames.map(name => ({
                    label: name,
                    data: bitSizes.map(bits => {
                        const r = results.find(x => x.name === name && x.bits === bits);
                        return r ? r.throughput : null;
                    }),
                    borderColor: colors[name],
                    backgroundColor: colors[name] + '33',
                    tension: 0.3
                }))
            },
            options: { responsive: true }
        });

        // Bias Chart
        new Chart(document.getElementById('biasChart'), {
            type: 'line',
            data: {
                labels: bitSizes.map(b => b.toExponential(0)),
                datasets: drbgNames.map(name => ({
                    label: name,
                    data: bitSizes.map(bits => {
                        const r = results.find(x => x.name === name && x.bits === bits);
                        return r ? r.bias * 100 : null;
                    }),
                    borderColor: colors[name],
                    backgroundColor: colors[name] + '33',
                    tension: 0.3
                }))
            },
            options: {
                responsive: true,
                scales: {
                    y: { title: { display: true, text: 'Bias (%)' } }
                }
            }
        });

        // Memory Chart
        new Chart(document.getElementById('memoryChart'), {
            type: 'bar',
            data: {
                labels: drbgNames,
                datasets: [{
                    label: 'State Size (bytes)',
                    data: drbgNames.map(name => {
                        const r = results.find(x => x.name === name);
                        return r ? r.stateSize : 0;
                    }),
                    backgroundColor: drbgNames.map(name => colors[name])
                }]
            },
            options: { responsive: true }
        });
    </script>
</body>
</html>
)";

    file.close();
}
