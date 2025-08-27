#!/usr/bin/env python3

"""
Coverage Validator
Verify coverage improvements for INTG-001
Target: 75%+ coverage
"""

import os
import sys
import json
import subprocess
import re
from datetime import datetime
from pathlib import Path

class CoverageValidator:
    def __init__(self, target_coverage=75.0):
        self.target_coverage = target_coverage
        self.base_dir = "/Users/jqwang/98-ghosttyAI/cache/week1/QA-001/p0-verification"
        self.coverage_dir = f"{self.base_dir}/coverage"
        self.report_dir = f"{self.base_dir}/reports"
        self.timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        
        # Ensure directories exist
        Path(self.coverage_dir).mkdir(parents=True, exist_ok=True)
        Path(self.report_dir).mkdir(parents=True, exist_ok=True)
    
    def run_lcov(self, source_dir, output_file):
        """Run lcov to generate coverage data"""
        try:
            # Capture coverage data
            cmd = [
                "lcov",
                "--capture",
                "--directory", source_dir,
                "--output-file", output_file,
                "--quiet"
            ]
            
            result = subprocess.run(cmd, capture_output=True, text=True)
            if result.returncode != 0:
                print(f"❌ lcov failed: {result.stderr}")
                return False
            
            return True
        except Exception as e:
            print(f"❌ Error running lcov: {e}")
            return False
    
    def parse_coverage_info(self, info_file):
        """Parse lcov info file to extract coverage metrics"""
        if not os.path.exists(info_file):
            return None
        
        metrics = {
            'lines_found': 0,
            'lines_hit': 0,
            'functions_found': 0,
            'functions_hit': 0,
            'branches_found': 0,
            'branches_hit': 0,
            'files': {}
        }
        
        current_file = None
        
        try:
            with open(info_file, 'r') as f:
                for line in f:
                    line = line.strip()
                    
                    if line.startswith('SF:'):
                        current_file = line[3:]
                        metrics['files'][current_file] = {
                            'lines_found': 0,
                            'lines_hit': 0,
                            'functions_found': 0,
                            'functions_hit': 0
                        }
                    
                    elif line.startswith('LF:'):  # Lines found
                        count = int(line[3:])
                        metrics['lines_found'] += count
                        if current_file:
                            metrics['files'][current_file]['lines_found'] = count
                    
                    elif line.startswith('LH:'):  # Lines hit
                        count = int(line[3:])
                        metrics['lines_hit'] += count
                        if current_file:
                            metrics['files'][current_file]['lines_hit'] = count
                    
                    elif line.startswith('FNF:'):  # Functions found
                        count = int(line[4:])
                        metrics['functions_found'] += count
                        if current_file:
                            metrics['files'][current_file]['functions_found'] = count
                    
                    elif line.startswith('FNH:'):  # Functions hit
                        count = int(line[4:])
                        metrics['functions_hit'] += count
                        if current_file:
                            metrics['files'][current_file]['functions_hit'] = count
                    
                    elif line.startswith('BRF:'):  # Branches found
                        count = int(line[4:])
                        metrics['branches_found'] += count
                    
                    elif line.startswith('BRH:'):  # Branches hit
                        count = int(line[4:])
                        metrics['branches_hit'] += count
        
        except Exception as e:
            print(f"❌ Error parsing coverage info: {e}")
            return None
        
        # Calculate percentages
        if metrics['lines_found'] > 0:
            metrics['line_coverage'] = (metrics['lines_hit'] / metrics['lines_found']) * 100
        else:
            metrics['line_coverage'] = 0
        
        if metrics['functions_found'] > 0:
            metrics['function_coverage'] = (metrics['functions_hit'] / metrics['functions_found']) * 100
        else:
            metrics['function_coverage'] = 0
        
        if metrics['branches_found'] > 0:
            metrics['branch_coverage'] = (metrics['branches_hit'] / metrics['branches_found']) * 100
        else:
            metrics['branch_coverage'] = 0
        
        return metrics
    
    def validate_coverage(self, metrics):
        """Validate coverage against targets"""
        validation = {
            'passed': True,
            'line_coverage': {
                'value': metrics['line_coverage'],
                'target': self.target_coverage,
                'passed': metrics['line_coverage'] >= self.target_coverage
            },
            'function_coverage': {
                'value': metrics['function_coverage'],
                'target': 80.0,
                'passed': metrics['function_coverage'] >= 80.0
            },
            'branch_coverage': {
                'value': metrics['branch_coverage'],
                'target': 70.0,
                'passed': metrics['branch_coverage'] >= 70.0
            },
            'critical_paths': []
        }
        
        # Check critical paths (files that must have 100% coverage)
        critical_files = [
            'ffi_bridge.c',
            'callback_dispatcher.c',
            'memory_manager.c'
        ]
        
        for file_path, file_metrics in metrics['files'].items():
            filename = os.path.basename(file_path)
            if filename in critical_files:
                if file_metrics['lines_found'] > 0:
                    file_coverage = (file_metrics['lines_hit'] / file_metrics['lines_found']) * 100
                    critical_validation = {
                        'file': filename,
                        'coverage': file_coverage,
                        'passed': file_coverage == 100.0
                    }
                    validation['critical_paths'].append(critical_validation)
                    if not critical_validation['passed']:
                        validation['passed'] = False
        
        # Overall pass/fail
        if not validation['line_coverage']['passed']:
            validation['passed'] = False
        
        return validation
    
    def generate_report(self, metrics, validation):
        """Generate detailed coverage report"""
        report_file = f"{self.report_dir}/coverage_validation_{self.timestamp}.txt"
        
        with open(report_file, 'w') as f:
            f.write("=" * 60 + "\n")
            f.write("COVERAGE VALIDATION REPORT\n")
            f.write(f"Generated: {datetime.now().isoformat()}\n")
            f.write("=" * 60 + "\n\n")
            
            # Overall metrics
            f.write("OVERALL COVERAGE METRICS\n")
            f.write("-" * 40 + "\n")
            f.write(f"Line Coverage:     {metrics['line_coverage']:.2f}% ")
            f.write(f"({metrics['lines_hit']}/{metrics['lines_found']})\n")
            f.write(f"Function Coverage: {metrics['function_coverage']:.2f}% ")
            f.write(f"({metrics['functions_hit']}/{metrics['functions_found']})\n")
            f.write(f"Branch Coverage:   {metrics['branch_coverage']:.2f}% ")
            f.write(f"({metrics['branches_hit']}/{metrics['branches_found']})\n")
            f.write("\n")
            
            # Validation results
            f.write("VALIDATION RESULTS\n")
            f.write("-" * 40 + "\n")
            
            # Line coverage validation
            status = "✅ PASS" if validation['line_coverage']['passed'] else "❌ FAIL"
            f.write(f"Line Coverage:     {status}\n")
            f.write(f"  Current: {validation['line_coverage']['value']:.2f}%\n")
            f.write(f"  Target:  {validation['line_coverage']['target']:.2f}%\n")
            
            # Function coverage validation
            status = "✅ PASS" if validation['function_coverage']['passed'] else "❌ FAIL"
            f.write(f"Function Coverage: {status}\n")
            f.write(f"  Current: {validation['function_coverage']['value']:.2f}%\n")
            f.write(f"  Target:  {validation['function_coverage']['target']:.2f}%\n")
            
            # Branch coverage validation
            status = "✅ PASS" if validation['branch_coverage']['passed'] else "⚠️  WARN"
            f.write(f"Branch Coverage:   {status}\n")
            f.write(f"  Current: {validation['branch_coverage']['value']:.2f}%\n")
            f.write(f"  Target:  {validation['branch_coverage']['target']:.2f}%\n")
            f.write("\n")
            
            # Critical paths
            if validation['critical_paths']:
                f.write("CRITICAL PATH COVERAGE (100% Required)\n")
                f.write("-" * 40 + "\n")
                for critical in validation['critical_paths']:
                    status = "✅" if critical['passed'] else "❌"
                    f.write(f"{status} {critical['file']}: {critical['coverage']:.2f}%\n")
                f.write("\n")
            
            # File-by-file breakdown
            f.write("FILE-BY-FILE BREAKDOWN\n")
            f.write("-" * 40 + "\n")
            
            # Sort files by coverage (lowest first)
            sorted_files = sorted(
                metrics['files'].items(),
                key=lambda x: (x[1]['lines_hit'] / x[1]['lines_found'] 
                              if x[1]['lines_found'] > 0 else 0)
            )
            
            for file_path, file_metrics in sorted_files[:10]:  # Top 10 worst
                if file_metrics['lines_found'] > 0:
                    coverage = (file_metrics['lines_hit'] / file_metrics['lines_found']) * 100
                    filename = os.path.basename(file_path)
                    f.write(f"  {filename:30} {coverage:6.2f}% ")
                    f.write(f"({file_metrics['lines_hit']}/{file_metrics['lines_found']})\n")
            
            f.write("\n")
            
            # Final verdict
            f.write("=" * 60 + "\n")
            if validation['passed']:
                f.write("✅ COVERAGE VALIDATION: PASSED\n")
                f.write("All coverage targets have been met.\n")
            else:
                f.write("❌ COVERAGE VALIDATION: FAILED\n")
                f.write("Coverage targets not met. DO NOT DEPLOY.\n")
            f.write("=" * 60 + "\n")
        
        print(f"Report saved to: {report_file}")
        return report_file
    
    def compare_with_baseline(self, current_metrics, baseline_file):
        """Compare current coverage with baseline"""
        if not os.path.exists(baseline_file):
            print(f"⚠️  No baseline file found: {baseline_file}")
            return None
        
        try:
            baseline_metrics = self.parse_coverage_info(baseline_file)
            if not baseline_metrics:
                return None
            
            comparison = {
                'line_coverage_diff': current_metrics['line_coverage'] - baseline_metrics['line_coverage'],
                'function_coverage_diff': current_metrics['function_coverage'] - baseline_metrics['function_coverage'],
                'branch_coverage_diff': current_metrics['branch_coverage'] - baseline_metrics['branch_coverage'],
                'improved': current_metrics['line_coverage'] > baseline_metrics['line_coverage']
            }
            
            return comparison
        except Exception as e:
            print(f"❌ Error comparing with baseline: {e}")
            return None
    
    def run_validation(self, source_dir, component="INTG-001"):
        """Main validation workflow"""
        print("=" * 60)
        print(f"COVERAGE VALIDATION FOR {component}")
        print("=" * 60)
        
        # Generate coverage info
        coverage_file = f"{self.coverage_dir}/coverage_{component}_{self.timestamp}.info"
        print(f"Generating coverage data...")
        
        if not self.run_lcov(source_dir, coverage_file):
            print("❌ Failed to generate coverage data")
            return False
        
        # Parse coverage metrics
        print(f"Parsing coverage metrics...")
        metrics = self.parse_coverage_info(coverage_file)
        
        if not metrics:
            print("❌ Failed to parse coverage metrics")
            return False
        
        # Validate coverage
        print(f"Validating coverage targets...")
        validation = self.validate_coverage(metrics)
        
        # Compare with baseline if exists
        baseline_file = f"{self.coverage_dir}/baseline_{component}.info"
        comparison = self.compare_with_baseline(metrics, baseline_file)
        
        # Generate report
        report_file = self.generate_report(metrics, validation)
        
        # Print summary
        print("\n" + "=" * 60)
        print("VALIDATION SUMMARY")
        print("=" * 60)
        print(f"Line Coverage:     {metrics['line_coverage']:.2f}% ", end="")
        print(f"(Target: {self.target_coverage}%)")
        print(f"Function Coverage: {metrics['function_coverage']:.2f}% ", end="")
        print("(Target: 80%)")
        print(f"Branch Coverage:   {metrics['branch_coverage']:.2f}% ", end="")
        print("(Target: 70%)")
        
        if comparison:
            print("\nCoverage Change from Baseline:")
            print(f"  Line:     {comparison['line_coverage_diff']:+.2f}%")
            print(f"  Function: {comparison['function_coverage_diff']:+.2f}%")
            print(f"  Branch:   {comparison['branch_coverage_diff']:+.2f}%")
        
        print("\n" + "=" * 60)
        if validation['passed']:
            print("✅ COVERAGE VALIDATION: PASSED")
            print(f"Component {component} meets all coverage requirements.")
            
            # Save as new baseline
            subprocess.run(["cp", coverage_file, baseline_file], check=False)
            print(f"New baseline saved.")
            
            return True
        else:
            print("❌ COVERAGE VALIDATION: FAILED")
            print(f"Component {component} does not meet coverage requirements.")
            print("Issues:")
            
            if not validation['line_coverage']['passed']:
                print(f"  - Line coverage below {self.target_coverage}%")
            
            if not validation['function_coverage']['passed']:
                print("  - Function coverage below 80%")
            
            for critical in validation['critical_paths']:
                if not critical['passed']:
                    print(f"  - Critical file {critical['file']} below 100%")
            
            return False

def main():
    # Parse arguments
    if len(sys.argv) < 2:
        print("Usage: coverage_validator.py <source_directory> [target_coverage]")
        sys.exit(1)
    
    source_dir = sys.argv[1]
    target_coverage = float(sys.argv[2]) if len(sys.argv) > 2 else 75.0
    
    # Create validator
    validator = CoverageValidator(target_coverage)
    
    # Run validation
    if validator.run_validation(source_dir):
        sys.exit(0)
    else:
        sys.exit(1)

if __name__ == "__main__":
    main()