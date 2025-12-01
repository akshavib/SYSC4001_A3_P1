import os
import re
import csv

# --- CONFIGURATION ---
INPUT_DIR = "input_files"
RESULTS_FILE = "scheduler_metrics_from_existing.csv"

# Map scheduler names to your output directory structure
# Key: Scheduler Name, Value: Path to output files
OUTPUT_DIRS = {
    "EP": "output_files/EP_algorithm_tests",
    "RR": "output_files/RR_algorithm_tests",
    "EP_RR": "output_files/EP_RR_algorithm_tests"
}

# --- HELPER: PARSE INPUT FILE ---
def parse_trace_file(filepath):
    """Reads input trace to get static process details."""
    procs = {}
    with open(filepath, 'r') as f:
        for line in f:
            parts = [x.strip() for x in line.split(',')]
            if len(parts) < 6: continue
            pid = int(parts[0])
            procs[pid] = {
                'arrival': int(parts[2]),
                'cpu_burst': int(parts[3]),
                'io_freq': int(parts[4]),
                'io_dur': int(parts[5])
            }
    return procs

# --- HELPER: PARSE EXECUTION LOG ---
def parse_execution_log(filepath):
    """Parses execution.txt to find start and end times."""
    events = []
    regex = re.compile(r"\|\s*(\d+)\s*\|\s*(\d+)\s*\|\s*(\w+)\s*\|\s*(\w+)\s*\|")
    
    if not os.path.exists(filepath):
        print(f"Warning: {filepath} not found.")
        return []

    with open(filepath, 'r') as f:
        for line in f:
            match = regex.search(line)
            if match:
                time = int(match.group(1))
                pid = int(match.group(2))
                old_s = match.group(3)
                new_s = match.group(4)
                events.append((time, pid, old_s, new_s))
    return events

# --- CORE: CALCULATE METRICS ---
def calculate_metrics(procs, events):
    """Computes the 4 required metrics."""
    stats = {pid: {'start': None, 'end': None, 'io_count': 0} for pid in procs}
    
    for time, pid, old_s, new_s in events:
        if pid not in stats: continue
        if new_s == "RUNNING" and stats[pid]['start'] is None:
            stats[pid]['start'] = time
        if new_s == "TERMINATED":
            stats[pid]['end'] = time
        if new_s == "WAITING":
            stats[pid]['io_count'] += 1

    total_turnaround = 0
    total_wait = 0
    total_response = 0
    count = 0
    max_completion_time = 0

    for pid, data in stats.items():
        if data['end'] is None: continue 
        
        count += 1
        arrival = procs[pid]['arrival']
        burst = procs[pid]['cpu_burst']
        io_total_time = data['io_count'] * procs[pid]['io_dur']
        
        turnaround = data['end'] - arrival
        first_run = data['start'] if data['start'] is not None else arrival
        response = first_run - arrival
        wait = turnaround - burst - io_total_time
        if wait < 0: wait = 0

        total_turnaround += turnaround
        total_response += response
        total_wait += wait
        
        if data['end'] > max_completion_time:
            max_completion_time = data['end']

    if count == 0: return None

    return {
        'throughput': count / max_completion_time if max_completion_time > 0 else 0,
        'avg_turnaround': total_turnaround / count,
        'avg_wait': total_wait / count,
        'avg_response': total_response / count
    }

# --- MAIN EXECUTION ---
def main():
    print("--- Starting Metric Calculation from Existing Files ---")
    
    with open(RESULTS_FILE, 'w', newline='') as f:
        writer = csv.writer(f)
        writer.writerow(['Scheduler', 'Trace File', 'Throughput', 'Avg Turnaround', 'Avg Wait', 'Avg Response'])
        
        # Iterate through your 3 schedulers
        for sch, output_dir in OUTPUT_DIRS.items():
            print(f"\nProcessing {sch} results from {output_dir}...")
            
            if not os.path.exists(output_dir):
                print(f"  Directory not found: {output_dir}")
                continue

            # Look for all execution files in the output folder
            # Pattern: ep_test1_execution.txt or rr_test1_execution.txt
            out_files = sorted([f for f in os.listdir(output_dir) if f.endswith('_execution.txt')])
            
            for out_file in out_files:
                # Extract the test number to find the matching input file
                # Example: ep_test1_execution.txt -> test1
                match = re.search(r'(test\d+)_execution', out_file)
                if not match: continue
                
                test_name = match.group(1)
                input_file = f"{test_name}.txt"
                input_path = os.path.join(INPUT_DIR, input_file)
                output_path = os.path.join(output_dir, out_file)
                
                if not os.path.exists(input_path):
                    print(f"  Input file missing for {out_file}: {input_path}")
                    continue

                # Calculate
                proc_info = parse_trace_file(input_path)
                events = parse_execution_log(output_path)
                m = calculate_metrics(proc_info, events)
                
                if m:
                    writer.writerow([
                        sch, 
                        test_name, 
                        f"{m['throughput']:.6f}", 
                        f"{m['avg_turnaround']:.2f}", 
                        f"{m['avg_wait']:.2f}", 
                        f"{m['avg_response']:.2f}"
                    ])
                    print(f"  {test_name}: Turnaround={m['avg_turnaround']:.1f}, Wait={m['avg_wait']:.1f}")

    print(f"\n[DONE] Results saved to {RESULTS_FILE}")

if __name__ == "__main__":
    main()