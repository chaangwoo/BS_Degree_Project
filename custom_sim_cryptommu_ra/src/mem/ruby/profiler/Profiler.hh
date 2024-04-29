/*
 * Copyright (c) 1999-2008 Mark D. Hill and David A. Wood
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer;
 * redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution;
 * neither the name of the copyright holders nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
   This file has been modified by Kevin Moore and Dan Nussbaum of the
   Scalable Systems Research Group at Sun Microsystems Laboratories
   (http://research.sun.com/scalable/) to support the Adaptive
   Transactional Memory Test Platform (ATMTP).

   Please send email to atmtp-interest@sun.com with feedback, questions, or
   to request future announcements about ATMTP.

   ----------------------------------------------------------------------

   File modification date: 2008-02-23

   ----------------------------------------------------------------------
*/

#ifndef __MEM_RUBY_PROFILER_PROFILER_HH__
#define __MEM_RUBY_PROFILER_PROFILER_HH__

#include <map>
#include <string>
#include <vector>
#include "base/callback.hh"
#include "base/hashmap.hh"
#include "base/statistics.hh"
#include "mem/protocol/AccessType.hh"
#include "mem/protocol/PrefetchBit.hh"
#include "mem/protocol/RubyAccessMode.hh"
#include "mem/protocol/RubyRequestType.hh"
#include "mem/ruby/common/Global.hh"
#include "mem/ruby/common/MachineID.hh"
#include "params/RubySystem.hh"

#define SIM_NET_PORTS
#define SIM_VISUAL_TRACE
#ifdef SIM_VISUAL_TRACE
#include "sim/sim_object.hh"
#include <fstream>
#endif

class RubyRequest;
class AddressProfiler;

class Profiler : public SimObject
{
  public:
    Profiler(const RubySystemParams *);
    ~Profiler();

    void wakeup();
    void regStats(const std::string &name);
    void regStats() {};
    void collateStats();

#ifdef SIM_VISUAL_TRACE
  std::string trace_filename;
  std::ofstream traceFile;
  void reportStatEvent(const std::string& deviceName, const std::string& attributeTitle, int value);
  void reportStatEvent(const std::string& deviceName, const std::string& attributeTitle, float value);
  void reportStatEvent(const std::string& deviceName, const std::string& attributeTitle, double value);
  void reportStatEvent(const std::string& deviceName, const std::string& attributeTitle, const std::string& value);
  void controllerActive(MachineID machID);
  void traceProfile_L1Cache_active(int node);
  void traceProfile_L1Cache_read(int node);
  void traceProfile_L1Cache_write(int node);
  void traceProfile_L1Cache_miss(int node);
  void traceProfile_L2Cache_active(int node);
  void traceProfile_L2Cache_read(int node);
  void traceProfile_L2Cache_write(int node);
  void traceProfile_L2Cache_miss(int node);
  void traceProfile_MC_read(int node);
  void traceProfile_MC_write(int node);
#endif

    AddressProfiler* getAddressProfiler() { return m_address_profiler_ptr; }
    AddressProfiler* getInstructionProfiler() { return m_inst_profiler_ptr; }

    void addAddressTraceSample(const RubyRequest& msg, NodeID id);

    // added by SS
    bool getHotLines() { return m_hot_lines; }
    bool getAllInstructions() { return m_all_instructions; }

  private:
    // Private copy constructor and assignment operator
    Profiler(const Profiler& obj);
    Profiler& operator=(const Profiler& obj);

    AddressProfiler* m_address_profiler_ptr;
    AddressProfiler* m_inst_profiler_ptr;

    Stats::Histogram delayHistogram;
    std::vector<Stats::Histogram *> delayVCHistogram;

    //! Histogram for number of outstanding requests per cycle.
    Stats::Histogram m_outstandReqHist;

    //! Histogram for holding latency profile of all requests.
    Stats::Histogram m_latencyHist;
    std::vector<Stats::Histogram *> m_typeLatencyHist;

    //! Histogram for holding latency profile of all requests that
    //! hit in the controller connected to this sequencer.
    Stats::Histogram m_hitLatencyHist;
    std::vector<Stats::Histogram *> m_hitTypeLatencyHist;

    //! Histograms for profiling the latencies for requests that
    //! did not required external messages.
    std::vector<Stats::Histogram *> m_hitMachLatencyHist;
    std::vector< std::vector<Stats::Histogram *> > m_hitTypeMachLatencyHist;

    //! Histogram for holding latency profile of all requests that
    //! miss in the controller connected to this sequencer.
    Stats::Histogram m_missLatencyHist;
    std::vector<Stats::Histogram *> m_missTypeLatencyHist;

    //! Histograms for profiling the latencies for requests that
    //! required external messages.
    std::vector<Stats::Histogram *> m_missMachLatencyHist;
    std::vector< std::vector<Stats::Histogram *> > m_missTypeMachLatencyHist;

    //! Histograms for recording the breakdown of miss latency
    std::vector<Stats::Histogram *> m_IssueToInitialDelayHist;
    std::vector<Stats::Histogram *> m_InitialToForwardDelayHist;
    std::vector<Stats::Histogram *> m_ForwardToFirstResponseDelayHist;
    std::vector<Stats::Histogram *> m_FirstResponseToCompletionDelayHist;
    Stats::Scalar m_IncompleteTimes[MachineType_NUM];

#ifdef SIM_NET_PORTS
  Stats::Scalar m_spmReads;
  Stats::Scalar m_spmWrites;
#endif

    //added by SS
    bool m_hot_lines;
    bool m_all_instructions;

    std::vector<Stats::Scalar> m_lcacc_tlb_hits;
    std::vector<Stats::Scalar> m_lcacc_tlb_misses;
    std::vector<Stats::Scalar> m_lcacc_tlb_accesses;
    std::vector<Stats::Scalar> m_lcacc_tlb_flush;
    std::vector<Stats::Scalar> m_lcacc_tlbCycles;
    // changed
    std::vector<Stats::Scalar> m_lcacc_mshrhits;
    std::vector<Stats::Scalar> m_lcacc_dma_reads;
    std::vector<Stats::Scalar> m_lcacc_dma_writes;
    std::vector<Stats::Scalar> m_lcacc_tlb_readhits;
    std::vector<Stats::Scalar> m_lcacc_tlb_readmisses;
    std::vector<Stats::Scalar> m_lcacc_tlb_writehits;
    std::vector<Stats::Scalar> m_lcacc_tlb_writemisses;
    std::vector<Stats::Scalar> m_lcacc_num_tasks;
    std::vector<Stats::Scalar> m_lcacc_task_cycles;
    std::vector<Stats::Scalar> m_lcacc_num_actions;
    std::vector<Stats::Scalar> m_lcacc_read_cycles;
    std::vector<Stats::Scalar> m_lcacc_compute_cycles;
    std::vector<Stats::Scalar> m_lcacc_write_cycles;

    std::vector<Stats::Scalar> m_lcacc_read_action_time;
    std::vector<Stats::Scalar> m_lcacc_compute_action_time;
    std::vector<Stats::Scalar> m_lcacc_write_action_time;
    std::vector<Stats::Scalar> m_lcacc_rw_action_time;

    std::vector<Stats::Scalar> m_lcacc_dma_lifetime;
    std::vector<Stats::Scalar> m_lcacc_dma_busy_cycles;
    std::vector<Stats::Scalar> m_lcacc_dma_idle_cycles;
    std::vector<Stats::Scalar> m_lcacc_dma_stall_cycles;

    std::vector<Stats::Scalar> m_lcacc_onTLBMissCall_0;
    std::vector<Stats::Scalar> m_lcacc_onTLBMissCall_1;
    std::vector<Stats::Scalar> m_lcacc_onTLBMissCall_2;

    std::vector<Stats::Scalar> m_lcacc_mshr_stall_cycles;
    //

    Stats::Scalar m_td_tlb_hits;
    Stats::Scalar m_td_tlb_misses;
    Stats::Scalar m_td_tlb_mshrhits;
    Stats::Scalar m_td_tlb_accesses;
    Stats::Scalar m_td_tlb_bCCMshrhits;

    std::vector<Stats::Scalar> m_host_pagetable_walks;
    std::vector<Stats::Scalar> m_bcc_access;
    std::vector<Stats::Scalar> m_bcc_hits;
    std::vector<Stats::Scalar> m_host_pagetable_walk_time;

#ifdef SIM_VISUAL_TRACE
  uint64_t m_L1Cache_read;
  uint64_t m_L1Cache_write;
  uint64_t m_L1Cache_miss;
  uint64_t m_L2Cache_read;
  uint64_t m_L2Cache_write;
  uint64_t m_L2Cache_miss;
  uint64_t m_MC_read;
  uint64_t m_MC_write;
  std::vector<uint64_t> m_bank_specific_L1Cache_read;
  std::vector<uint64_t> m_bank_specific_L1Cache_write;
  std::vector<uint64_t> m_bank_specific_L1Cache_miss;
  std::vector<uint64_t> m_bank_specific_L2Cache_read;
  std::vector<uint64_t> m_bank_specific_L2Cache_write;
  std::vector<uint64_t> m_bank_specific_L2Cache_miss;
  std::vector<uint64_t> m_bank_specific_MC_read;
  std::vector<uint64_t> m_bank_specific_MC_write;
  std::vector<uint64_t> m_bank_specific_L1Cache_active;
  std::vector<uint64_t> m_bank_specific_L2Cache_active;

  protected:
    class ProfileEvent : public Event
    {
        public:
            ProfileEvent(Profiler *_profiler)
            {
                profiler = _profiler;
            }
        private:
            void process() { profiler->wakeup(); }
            Profiler *profiler;
    };
    ProfileEvent m_event;
#endif
};

#endif // __MEM_RUBY_PROFILER_PROFILER_HH__
