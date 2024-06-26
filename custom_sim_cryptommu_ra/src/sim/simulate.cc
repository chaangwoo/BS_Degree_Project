/*
 * Copyright (c) 2006 The Regents of The University of Michigan
 * Copyright (c) 2013 Advanced Micro Devices, Inc.
 * Copyright (c) 2013 Mark D. Hill and David A. Wood
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
 *
 * Authors: Nathan Binkert
 *          Steve Reinhardt
 */

#include <mutex>
#include <thread>

#include "base/misc.hh"
#include "base/pollevent.hh"
#include "base/types.hh"
#include "sim/async.hh"
#include "sim/eventq_impl.hh"
#include "sim/sim_events.hh"
#include "sim/sim_exit.hh"
#include "sim/simulate.hh"
#include "sim/stat_control.hh"

//0. SW version
//#define SIM_SW

#define SIM_NET_PORTS
#ifdef SIM_NET_PORTS
#include "modules/NetworkInterrupt/NetworkInterrupts.hh"
#include "modules/Synchronize/Synchronize.hh"
#include "mem/ruby/common/Global.hh"
#include "system.hh"
#endif

//! Mutex for handling async events.
std::mutex asyncEventMutex;

//! Global barrier for synchronizing threads entering/exiting the
//! simulation loop.
Barrier *threadBarrier;

//! forward declaration
Event *doSimLoop(EventQueue *);

/**
 * The main function for all subordinate threads (i.e., all threads
 * other than the main thread).  These threads start by waiting on
 * threadBarrier.  Once all threads have arrived at threadBarrier,
 * they enter the simulation loop concurrently.  When they exit the
 * loop, they return to waiting on threadBarrier.  This process is
 * repeated until the simulation terminates.
 */
static void
thread_loop(EventQueue *queue)
{
    while (true) {
        threadBarrier->wait();
        doSimLoop(queue);
    }
}

/** Simulate for num_cycles additional cycles.  If num_cycles is -1
 * (the default), do not limit simulation; some other event must
 * terminate the loop.  Exported to Python via SWIG.
 * @return The SimLoopExitEvent that caused the loop to exit.
 */
GlobalSimLoopExitEvent *
simulate(Tick num_cycles)
{
    // The first time simulate() is called from the Python code, we need to
    // create a thread for each of event queues referenced by the
    // instantiated sim objects.
    static bool threads_initialized = false;
    static std::vector<std::thread *> threads;

    if (!threads_initialized) {
        threadBarrier = new Barrier(numMainEventQueues);

        // the main thread (the one we're currently running on)
        // handles queue 0, so we only need to allocate new threads
        // for queues 1..N-1.  We'll call these the "subordinate" threads.
        for (uint32_t i = 1; i < numMainEventQueues; i++) {
            threads.push_back(new std::thread(thread_loop, mainEventQueue[i]));
        }

#ifdef SIM_NET_PORTS
        //Before Ruby starts, we need to initilize TLB interface and barrier interface
        g_TLBHack_interface = (TLBHackInterface *)malloc(sizeof(TLBHackInterface));
        memset(g_TLBHack_interface, 0, sizeof(TLBHackInterface));
        g_TLBHack_interface->PageKnown = PageKnownHandler;
        g_TLBHack_interface->Lookup = LookupHandler;
        g_TLBHack_interface->AddEntry = AddEntryHandler;
        inform("Initialize TLBHack_interface\n");//*/

        //This is the number of applications to run, to synchronize
        std::vector<System *>::iterator system_iterator = System::systemList.begin();
        System *m5_system = *system_iterator;
        assert(m5_system);
        int num_thread_contexts = m5_system->numContexts();
        inform("Number of threadcontexts is %d\n", num_thread_contexts);
        // num_thread_contexts = 2;
        SetBarrierWidth(num_thread_contexts);
        inform("Set Barrier Width to %d\n", num_thread_contexts);//*/
#endif

        threads_initialized = true;
    }

    inform("Entering event queue @ %d.  Starting simulation...\n", curTick());

    if (num_cycles < MaxTick - curTick())
        num_cycles = curTick() + num_cycles;
    else // counter would roll over or be set to MaxTick anyhow
        num_cycles = MaxTick;

    GlobalEvent *limit_event = new GlobalSimLoopExitEvent(num_cycles,
                                "simulate() limit reached", 0, 0);

    GlobalSyncEvent *quantum_event = NULL;
    if (numMainEventQueues > 1) {
        if (simQuantum == 0) {
            fatal("Quantum for multi-eventq simulation not specified");
        }

        quantum_event = new GlobalSyncEvent(curTick() + simQuantum, simQuantum,
                            EventBase::Progress_Event_Pri, 0);

        inParallelMode = true;
    }

    // all subordinate (created) threads should be waiting on the
    // barrier; the arrival of the main thread here will satisfy the
    // barrier, and all threads will enter doSimLoop in parallel
    threadBarrier->wait();
    Event *local_event = doSimLoop(mainEventQueue[0]);
    assert(local_event != NULL);

    inParallelMode = false;

    // locate the global exit event and return it to Python
    BaseGlobalEvent *global_event = local_event->globalEvent();
    assert(global_event != NULL);

    GlobalSimLoopExitEvent *global_exit_event =
        dynamic_cast<GlobalSimLoopExitEvent *>(global_event);
    assert(global_exit_event != NULL);

    // if we didn't hit limit_event, delete it.
    if (global_exit_event != limit_event) {
        assert(limit_event->scheduled());
        limit_event->deschedule();
        delete limit_event;
    }

    //! Delete the simulation quantum event.
    if (quantum_event != NULL) {
        quantum_event->deschedule();
        delete quantum_event;
    }

    return global_exit_event;
}

/**
 * Test and clear the global async_event flag, such that each time the
 * flag is cleared, only one thread returns true (and thus is assigned
 * to handle the corresponding async event(s)).
 */
static bool
testAndClearAsyncEvent()
{
    bool was_set = false;
    asyncEventMutex.lock();

    if (async_event) {
        was_set = true;
        async_event = false;
    }

    asyncEventMutex.unlock();
    return was_set;
}

/**
 * The main per-thread simulation loop. This loop is executed by all
 * simulation threads (the main thread and the subordinate threads) in
 * parallel.
 */
Event *
doSimLoop(EventQueue *eventq)
{
    // set the per thread current eventq pointer
    curEventQueue(eventq);
    eventq->handleAsyncInsertions();

#if defined(SIM_NET_PORTS) && !defined(SIM_SW)
    std::vector<System *>::iterator system_iterator = System::systemList.begin();
    System *m5_system = *system_iterator;
    assert(m5_system);
    Cycles lastCycle = m5_system->ticksToCycles(curTick());
#endif

    while (1) {
        // there should always be at least one event (the SimLoopExitEvent
        // we just scheduled) in the queue
        assert(!eventq->empty());
        assert(curTick() <= eventq->nextTick() &&
               "event scheduled in the past");

#if defined(SIM_NET_PORTS) && !defined(SIM_SW)
        //advance our private event queue for local CBs
	while( lastCycle < m5_system->ticksToCycles(eventq->nextTick()) ) {
	  while(localCBsForCycle(lastCycle)) {
	      std::queue<CBContainer>& cycleQueue = getCurrentCycleQueue(lastCycle);
	      CBContainer cb = cycleQueue.front();
	      cycleQueue.pop();
	      cb.call();
	      //don't retire so that the next check will retire this sequence.
	  }
	  ++lastCycle;
	  eventq->setCurTick(eventq->getCurTick() + m5_system->clockPeriod());
	}
#endif

        Event *exit_event = eventq->serviceOne();
        if (exit_event != NULL) {
            return exit_event;
        }

        if (async_event && testAndClearAsyncEvent()) {
            // Take the event queue lock in case any of the service
            // routines want to schedule new events.
            std::lock_guard<EventQueue> lock(*eventq);
            async_event = false;
            if (async_statdump || async_statreset) {
                Stats::schedStatEvent(async_statdump, async_statreset);
                async_statdump = false;
                async_statreset = false;
            }

            if (async_exit) {
                async_exit = false;
                exitSimLoop("user interrupt received");
            }

            if (async_io) {
                async_io = false;
                pollQueue.service();
            }

            if (async_exception) {
                async_exception = false;
                return NULL;
            }
        }
    }

    // not reached... only exit is return on SimLoopExitEvent
}
