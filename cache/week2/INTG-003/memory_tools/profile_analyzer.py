#!/usr/bin/env python3
# profile_analyzer.py - Parse and analyze profiling data
# Author: INTG-003 (performance-eng)
# Date: 2025-08-25

import sys
import json
import statistics
from collections import defaultdict

def parse_perf_output(filename):
    """Parse perf report output"""
    results = defaultdict(float)
    
    try:
        with open(filename, 'r') as f:
            for line in f:
                if '%' in line and not line.startswith('#'):
                    parts = line.strip().split()
                    if len(parts) >= 5:
                        percentage = float(parts[0].rstrip('%'))
                        function = parts[-1]
                        results[function] = percentage
    except Exception as e:
        print(f"Error parsing {filename}: {e}")
    
    return results

def analyze_latency_distribution(data):
    """Analyze latency distribution from benchmark data"""
    if not data:
        return {}
    
    sorted_data = sorted(data)
    
    return {
        'min': sorted_data[0],
        'p50': sorted_data[int(len(data) * 0.50)],
        'p90': sorted_data[int(len(data) * 0.90)],
        'p95': sorted_data[int(len(data) * 0.95)],
        'p99': sorted_data[int(len(data) * 0.99)],
        'p999': sorted_data[int(len(data) * 0.999)],
        'max': sorted_data[-1],
        'mean': statistics.mean(data),
        'stddev': statistics.stdev(data) if len(data) > 1 else 0
    }

def check_performance_targets(stats):
    """Check if performance targets are met"""
    results = {
        'throughput_target_met': False,
        'p99_latency_target_met': False,
        'memory_growth_acceptable': False
    }
    
    # Check throughput (target: 200k ops/s)
    if 'ops_per_sec' in stats:
        results['throughput_target_met'] = stats['ops_per_sec'] >= 200000
    
    # Check P99 latency (target: <0.5ms = 500µs)
    if 'p99' in stats:
        results['p99_latency_target_met'] = stats['p99'] < 500
    
    # Check memory growth (target: <10%)
    if 'memory_growth_percent' in stats:
        results['memory_growth_acceptable'] = stats['memory_growth_percent'] < 10
    
    return results

def generate_optimization_recommendations(hotspots):
    """Generate optimization recommendations based on hotspots"""
    recommendations = []
    
    for function, percentage in sorted(hotspots.items(), key=lambda x: x[1], reverse=True)[:5]:
        if percentage > 10:
            recommendations.append(f"HIGH: {function} consuming {percentage:.1f}% CPU - consider optimization")
        elif percentage > 5:
            recommendations.append(f"MEDIUM: {function} consuming {percentage:.1f}% CPU - review for improvements")
        else:
            recommendations.append(f"LOW: {function} consuming {percentage:.1f}% CPU - acceptable")
    
    return recommendations

def main():
    print("=== Performance Profile Analyzer ===")
    print()
    
    # Example analysis (would read actual data in production)
    sample_latencies = [250, 280, 300, 320, 350, 400, 450, 480, 520, 1200]  # microseconds
    
    stats = analyze_latency_distribution(sample_latencies)
    stats['ops_per_sec'] = 250000  # Example
    stats['memory_growth_percent'] = 8  # Example
    
    print("Latency Distribution (microseconds):")
    for key, value in stats.items():
        if key not in ['ops_per_sec', 'memory_growth_percent']:
            print(f"  {key}: {value:.2f}")
    
    print()
    print(f"Throughput: {stats['ops_per_sec']:.0f} ops/sec")
    print(f"Memory Growth: {stats['memory_growth_percent']:.1f}%")
    
    print()
    print("Target Validation:")
    targets = check_performance_targets(stats)
    for target, met in targets.items():
        status = "✓ PASS" if met else "✗ FAIL"
        print(f"  {target}: {status}")
    
    print()
    print("Optimization Recommendations:")
    # Example hotspots
    hotspots = {
        'event_loop_dispatch': 15.2,
        'ffi_boundary_cross': 12.8,
        'grid_update': 8.5,
        'memory_allocate': 6.2,
        'mutex_lock': 4.1
    }
    
    recommendations = generate_optimization_recommendations(hotspots)
    for rec in recommendations:
        print(f"  - {rec}")
    
    # Output JSON for further processing
    output = {
        'statistics': stats,
        'targets': targets,
        'hotspots': hotspots,
        'recommendations': recommendations
    }
    
    with open('profile_analysis.json', 'w') as f:
        json.dump(output, f, indent=2)
    
    print()
    print("Analysis saved to profile_analysis.json")

if __name__ == "__main__":
    main()