import argparse
import re
import csv
import os

FILES = [
        "TDLCA_BlackScholes/result.txt",
        "TDLCA_Deblur_Modified/result.txt",
        "TDLCA_Denoise/result.txt",
        "TDLCA_Disparity_Map/result.txt",
        "TDLCA_EKF_SLAM/result.txt",
        "TDLCA_LPCIP_Desc/result.txt",
        "TDLCA_Robot_Localization/result.txt",
        "TDLCA_Segmentation/result.txt",
        "TDLCA_StreamCluster/result.txt",
        "TDLCA_Swaptions/result.txt",
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
    write_to_csv(['workload', 'run_time'])
    for sub_dir in dirs:
        print(sub_dir)
        file = open("{}/result.txt".format(sub_dir), 'r')
        lines = file.readlines()

        file_name = sub_dir
        comps = [file_name]
        numCycles = 0
        acc_count = 0
        timeStamp = 0
        for idx, line in enumerate(lines):
            # Search in line with 'in'
            # if "sim_seconds" in line:
            #     comps.append(line.split()[1])
            # if "num_spm_reads" in line:
            #     comps.append(line.split()[1])
            # if "num_spm_writes" in line:
            #     comps.append(line.split()[1])
            # if "tlb_misses" in line and "lcacc" in line:
            #     tlb_misses = tlb_misses .append(line.split()[1])
            #     acc_count = acc_count + 1
            # if "tlb_hits" in line and "lcacc" in line:
            #     tlb_hits = tlb_hits .append(line.split()[1])
            if "END program read" in line:
                timeStamp = int(line.split()[1][6:-1])
            if "END Program Execute" in line:
                numCycles = numCycles + int(line.split()[1][6:-1]) - timeStamp
                
        # comps.append(tlb_misses)
        # comps.append(tlb_hits)
        # if len(comps) < 5:
            # comps = [config, file_name, -1, -1, -1, -1]
        # comps.append(int(comps[-1]) .append(comps[-2]))
        # comps.append(acc_count)
        if (sub_dir == "TDLCA_StreamCluster"):
            numCycles = 0
        comps.append(numCycles)
        write_to_csv(comps)
            
            
