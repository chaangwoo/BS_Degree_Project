# Directory Description
- `CryptoMMU-sim-artifact/CryptoMMU_ReadAcc/` is the provided artifact in the **CryptoMMU** paper
  - Some variables are added for the simulation metric
  - `archive/` consists of the simulation results (`stats.txt` files) of each workload
- `custom_sim_cryptommu_ra/` is the customized directory with the baseline of `CryptoMMU-sim-artifact/CryptoMMU_ReadAcc/`
  - `archive/` consists of the simulation results or each workload and the `DMAController.cc` and `DMAController.hh` files

# Workload Breakdown
- Each workload consists of multiple *jobs*
- Each *job* consists of multiple *tasks*
- Each *task* consists of consecutive **READ**, **COMPUTE**, and **WRITE** *actions*

# What I Have Done
### Add/modify some metrics 
- To (correctly) measure the performance of the workloads
- Modified parts are marked with `// changed` comment in the following files:
    - `src/mem/ruby/profiler/Profiler.cc`
    - `src/mem/ruby/profiler/Profiler.hh`
    - `src/modules/LCAcc/DMAController.cc`
    - `src/modules/LCAcc/DMAController.hh`
    - `src/modules/LCAcc/LCAccDevice.cc`
    - `src/modules/LCAcc/LCAccDevice.hh`
    - `src/modules/linked-prefetch-tile/DMAEngine.cc`
    - `src/modules/linked-prefetch-tile/DMAEngine.hh`
#### Detailed description of the modified parts
1. `src/modules/LCAcc/LCAccDevice.cc`
  - `numJobs` measures the number of jobs in the workload
  - `jobTimeStamp` and `jobCycles` measures the total cycles spent on doing jobs
  - `numTasks` measures the number of total tasks
  - `readCycles`, `computeCycles`, and `writeCycles` measures the cycles spent on read, compute, and write actions, respectively
    - These metrics sum up all spent cycles for each action, which may allow overlapping
    - Measured with the `std::map` container `actionTimeStamp`, which stores the timestamps of corresponding actions
  - `readActionTime`, `computeActionTime`, `writeActionTime`, and `RWActionTime` measures the cycles spent on read, compute, write, and read+write actions, respectively
    - Cycles with at least one pending actions (without overlapping)
    - Measured with the `std::set` container `pendingReadActions`, `pendingComputeActions`, `pendingWriteActions`, and `pendingRWActions`
  - `numHandleTLBMissCall_n` counts the number of `onTLBMiss` calls in the `LCAcc::DMAController::translateTiming()` with the `MAC_dma` of `n` 
    - `0` means the private TLB miss and the MSHR miss, which needs the PTW
    - `1` means the private TLB write hit
    - `2` means the private TLB read hit  
2. `src/modules/LCAcc/DMAController.cc`
  - `hits`, `misses`, and `mshrhits` to be properly incremented
  - `numReads`, `numWrites`, `numReadHits`, `numReadMisses`, `numWriteHits`, and `numWriteMisses` are added  
3. `src/modules/link-prefetch-tile/DMAEngine.cc`
  - `busyCycles` measures the number of cycles with `inflight > 0`
  - `idleCycles` measures the number of cycles with `inflight == 0`
  - `stallCycles` measures the stalled cycles
    - Cycles that failed executing `DMAEngine::TryTransfers()`
---
### Optimize the baseline
- Modified parts are marked with `// changed_optimized_baseline` comment in the following files:
  - `src/modules/LCAcc/DMAController.cc` (Archived in `custom_sim_cryptommu_ra/archive/optimized_baseline/`)
- In the provided artifact, there are some non-reasonable parts
  - There is the redundant PTW even in the *write hit* case
  - Checks MSHR only when the *read miss*, which means *write miss* always do the PTW
- Optimized the code to **prevent the redundant PTW** on the consecutive write misses
  - By checking MSHR in both *read miss* and the *write miss* cases, and allocate entry if possible
  - Do the PTW only when the MSHR is full
- Comparing the statistics in the `CryptoMMU-sim-artifact/CryptoMMU_ReadAcc/archive/` and the `custom_sim_cryptommu_ra/archive/optimized_baseline` yields that this optimization performs well in the *write miss-rich* workload (e.g. Denoise)
---
### Set the new baseline with correctly behaving MSHR
- Modified parts are marked with `// changed_stalling_mshr` comment in the following files:
  - `src/mem/request.hh`
  - `src/modules/LCAcc/DMAController.cc` (Archived in `custom_sim_cryptommu_ra/archive/non_stalling_mshr/`)
  - `src/modules/LCAcc/DMAController.hh` (Archived in `custom_sim_cryptommu_ra/archive/non_stalling_mshr/`)
  - `src/modules/NetworkInterrupt/NetworkInterrupts.cc`
  - `src/modules/NetworkInterrupt/NetworkInterrupts.hh`
  - `src/modules/TaskDistributor/TD.cc`
  - `src/modules/TaskDistributor/TD.hh`
#### Detailed description of the modified parts
1. `src/mem/request.hh`
   - .
2. `src/modules/LCAcc/DMAController.cc`
   - .
3. `src/modules/NetworkInterrupt/NetworkInterrupts.cc`
   - .
4. `src/modules/TaskDistributor/TD.cc`
   - .
---
### Implement the *stall* mechanism to the MSHR
- Modified parts are marked with `// changed_stalling_mshr` comment in the following files:
  - `src/modules/LCAcc/DMAController.cc` (Archived in `custom_sim_cryptommu_ra/archive/stalling_mshr/`)
  - `src/modules/LCAcc/DMAController.hh` (Archived in `custom_sim_cryptommu_ra/archive/stalling_mshr/`)
- In the simulator code, the data structure `MSHR` is more like *Request Merging Buffer* than a conventional MSHR
- It not only coalesces the TLB misses, but also store the requests which are *write hit* in the private TLB to wait for the MAC verification
- *Stalling* mechanism for the memory requests when the `MSHR` structure is full is needed due to the following reasons:
  - Doing PTW and MAC generation whenever MSHR is full puts a heavy load to the host's page table walker
  - Can Pioritize the read miss to the write request (both write hit and miss) when the `MSHR` is available; Note that the existing mechanism is just FIFO policy
#### Detailed description of the modified parts
1. `src/modules/LCAcc/DMAController.cc`
- `translateTiming()` function is modified to be called without arguments
- Pop the one `td` from the queue (`waitingReadTransfers` or `waitingWriteTransfers`) in each cycle and try the translation
- If it is blocked due to the limited MSHR size, re-push the `td` into the queue


  
