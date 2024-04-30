import argparse
import re
import csv
import os

FILES = [
        "TDLCA_BlackScholes/stats.txt",
        "TDLCA_Deblur_Modified/stats.txt",
        "TDLCA_Denoise/stats.txt",
        "TDLCA_Disparity_Map/stats.txt",
        "TDLCA_EKF_SLAM/stats.txt",
        "TDLCA_LPCIP_Desc/stats.txt",
        "TDLCA_Robot_Localization/stats.txt",
        "TDLCA_Segmentation/stats.txt",
        "TDLCA_StreamCluster/stats.txt",
        "TDLCA_Swaptions/stats.txt",
        ]
CSV = "results.csv"

def write_to_csv(comps):
    with open(CSV, 'ab') as csvfile:
        writer = csv.writer(csvfile)
        # writer.writerow(['config', 'workload', 'sim_seconds', 'tlb_hits', 'tlb_misses', 'memory_requests'])
        writer.writerow(comps)

if __name__ == '__main__':
    dirs = next(os.walk('.'))[1]
    # print(dirs)
    write_to_csv(['workload', 'tlb_hits', 'read_hits', 'write_hits', 'tlb_misses', 'read_misses', 'write_misses', 'mshr_hits', 'mshr_stall_cycles', 'num_jobs', 'total_cycles', 'num_tasks', 'read_action_cycles', 'compute_action_cycles', 'write_action_cycles'])
    for sub_dir in dirs:
        print(sub_dir)
        file = open("{}/stats.txt".format(sub_dir), 'r')
        lines = file.readlines()

        file_name = sub_dir
        comps = [file_name]
        tlb_hits = 0
        read_hits = 0
        write_hits = 0
        tlb_misses = 0
        read_misses = 0
        write_misses = 0
        mshr_hits = 0
        mshr_stall_cycles = 0
        num_jobs = 0
        total_cycles = 0 # cycles doing jobs
        num_tasks = 0
        read_action_cycles = 0 # processing at least one action
        compute_action_cycles = 0
        write_action_cycles = 0
        
        acc_count = 0
        for idx, line in enumerate(lines):
            # Search in line with 'in'
            # if "sim_seconds" in line:
            #     comps.append(line.split()[1])
            if "tlb_hits" in line and "lcacc" in line:
                tlb_hits = tlb_hits + int(line.split()[1])
                acc_count = acc_count + 1
            if "tlb_readhits" in line and "lcacc" in line:
                read_hits = read_hits + int(line.split()[1])
            if "tlb_writehits" in line and "lcacc" in line:
                write_hits = write_hits + int(line.split()[1])
            if "tlb_misses" in line and "lcacc" in line:
                tlb_misses = tlb_misses + int(line.split()[1])
            if "tlb_readmisses" in line and "lcacc" in line:
                read_misses = read_misses + int(line.split()[1])
            if "tlb_writemisses" in line and "lcacc" in line:
                write_misses = write_misses + int(line.split()[1])
            if "mshrhits" in line and "lcacc" in line:
                mshr_hits = mshr_hits + int(line.split()[1])
            if "mshr_stall_cycles" in line and "lcacc" in line:
                mshr_stall_cycles = mshr_stall_cycles + int(line.split()[1])
            if "num_jobs" in line and "lcacc" in line:
                num_jobs = num_jobs + int(line.split()[1])
            if "job_cycles" in line and "lcacc" in line:
                total_cycles = total_cycles + int(line.split()[1])
            if "num_tasks" in line and "lcacc" in line:
                num_tasks = num_tasks + int(line.split()[1])
            if "read_action_time" in line and "lcacc" in line:
                read_action_cycles = read_action_cycles + int(line.split()[1])
            if "compute_action_time" in line and "lcacc" in line:
                compute_action_cycles = compute_action_cycles + int(line.split()[1])
            if "write_action_time" in line and "lcacc" in line:
                write_action_cycles = write_action_cycles + int(line.split()[1])

        comps.append(tlb_hits)
        comps.append(read_hits)
        comps.append(write_hits)
        comps.append(tlb_misses)
        comps.append(read_misses)
        comps.append(write_misses)
        comps.append(mshr_hits)
        comps.append(mshr_stall_cycles)
        comps.append(num_jobs)
        comps.append(total_cycles)
        comps.append(num_tasks)
        comps.append(read_action_cycles)
        comps.append(compute_action_cycles)
        comps.append(write_action_cycles)

        write_to_csv(comps)

        # Write to csv if necessary
        """
        for i in range(1, acc_count):
            comps = [0, ... ]
            write_to_csv(comps)
        """ 
            
