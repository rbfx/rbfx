///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Functionality related to memory and code generation synchronization.
//
// Overview (partially taken from Usenet)
// On all modern hardware, a store instruction does not necessarily result
// in an immediate write to main memory, or even to the (processor specific)
// cache. A store instruction simply places a write request in a request
// queue, and continues. (Future reads in the same processor will check if
// there is a write to the same address in this queue, and fetch it, rather
// than reading from memory. Reads from another processor, however, can't
// see this queue.) Generally, the ordering of requests in this queue is
// not guaranteed, although some hardware offers stricter guarantees.
// Thus, you must do something to ensure that the writes actually occur.
// This is called a write barrier, and generally takes the form of a special
// instruction.
// 
// And of course, just because you have written the data to main memory
// doesn't mean that some other processor, executing a different thread,
// doesn't have a stale copy in its cache, and use that for a read. Before
// reading the variables, you need to ensure that the processor has the
// most recent copy in its cache. This is called a read barrier, and
// again, takes the form of a special hardware instruction. A number of
// architectures (e.g. Intel x86-32) still guarantee read consistency -- 
// all of the processors "listen" on the main memory bus, and if there is 
// a write, automatically purge the corresponding data from their cache. 
// But not all.
//
// Note that if you are writing data within a operating system-level 
// locked mutex, the lock and unlock of the mutex will synchronize memory
// for you, thus eliminating the need for you to execute read and/or write
// barriers. However, mutex locking and its associated thread stalling is 
// a potentially inefficient operation when in some cases you could simply 
// write the memory from one thread and read it from another without 
// using mutexes around the data access. Some systems let you write memory 
// from one thread and read it from another (without you using mutexes)
// without using memory barriers, but others (notably SMP) will not let you 
// get away with this, even if you put a mutex around the write. In these
// cases you need read/write barriers.
/////////////////////////////////////////////////////////////////////////////


#ifndef EATHREAD_EATHREAD_SYNC_H
#define EATHREAD_EATHREAD_SYNC_H


// Note
// These functions are not placed in a C++ namespace but instead are standalone.
// The reason for this is that these are usually implemented as #defines of 
// C or asm code or implemented as compiler intrinsics. We however document
// these functions here as if they are simply functions. The actual platform-
// specific declarations are in the appropriate platform-specific directory.

#include <EABase/eabase.h>
#include <eathread/internal/config.h>

#if !EA_THREADS_AVAILABLE
	// Do nothing.
#elif defined(EA_PROCESSOR_X86)
	#include <eathread/x86/eathread_sync_x86.h>
#elif defined(EA_PROCESSOR_X86_64)
	#include <eathread/x86-64/eathread_sync_x86-64.h>
#elif defined(EA_PROCESSOR_IA64)
	#include <eathread/ia64/eathread_sync_ia64.h>
#elif defined(EA_PLATFORM_APPLE)
	#include <eathread/apple/eathread_sync_apple.h>
#elif defined(EA_PROCESSOR_ARM) 
	#include <eathread/arm/eathread_sync_arm.h>
#endif

#if defined(EA_PRAGMA_ONCE_SUPPORTED)
	#pragma once // Some compilers (e.g. VC++) benefit significantly from using this. We've measured 3-4% build speed improvements in apps as a result.
#endif



// EA_THREAD_DO_SPIN
//     
// Provides a macro which maps to whatever processor idle functionality the given platform requires.
// 
// Example usage:
//     EA_THREAD_DO_SPIN();
// 
#ifndef EA_THREAD_DO_SPIN
	#ifdef EA_THREAD_COOPERATIVE  
		 #define EA_THREAD_DO_SPIN() ThreadSleep()               
	#else
		 #define EA_THREAD_DO_SPIN() EAProcessorPause() // We don't check for EA_TARGET_SMP here and instead sleep if not defined because you probably shouldn't be using a spinlock on a pre-emptive system unless it is a multi-processing system.     
	#endif
#endif



// The above header files would define EA_THREAD_SYNC_IMPLEMENTED.
#if !defined(EA_THREAD_SYNC_IMPLEMENTED)
   // Perhaps it should be considered too serious of an error to allow compilation 
   // to continue. If so, then we should enable the #error below.
   // #error EA_THREAD_SYNC_IMPLEMENTED not defined. 


   /// EAProcessorPause
   ///
   /// \Declaration
   ///    void EAProcessorPause();
   ///
   /// \Description
   ///    This statement causes the processor to efficiently (as much as possible)
   ///    execute a no-op (a.k.a nop or noop). These are particularly useful in 
   ///    spin-wait loops. Without a proper pause, some processors suffer severe
   ///    performance penalties while executing spin-wait loops such as those in 
   ///    simple spin locks. Many processors have specialized pause instructions 
   ///    (e.g. Intel x86 P4 'pause' or 'asm rep nop') that can be taken advantage 
   ///    of here.
   ///
   /// \Example
   ///    while (!flag) {
   ///       EAProcessorPause();
   ///    }
   #define EAProcessorPause()



   /// EAReadBarrier
   ///
   /// \Declaration
   ///    void EAReadBarrier();
   ///
   /// \Description
   ///    A read barrier ensures that neither software nor hardware perform a memory 
   ///    read prior to the read barrier and that recent writes to main memory are 
   ///    immediately seen (and not using stale cached data) by the processor executing
   ///    the read barrier. This generally does not mean a (performance draining) 
   ///    invalidation of the entire cache but does possibly mean invalidating any cache 
   ///    that refers to main memory which has changed. Thus, there is a performance 
   ///    cost but considering the use of this operation, this is the most efficient 
   ///    way of achieving the effect.
   ///
   /// \Example
   ///    The following function will operate fine on some multiprocessing systems but 
   ///    hang (possibly indefinitely) on other multiprocessing systems unless the 
   ///    EAReadBarrier call is present.
   ///
   ///    void ThreadFunction() {
   ///      extern volatile int gFlag;
   ///      while(gFlag == 0){ // Wait for separate thread to write to gSomeFlag.
   ///         EAProcessorPause(); 
   ///         EAReadBarrier();
   ///         // Do memory sharing operations with other threads here.
   ///      }
   ///    }
   #define EAReadBarrier()





   /// EAWriteBarrier
   ///
   /// \Declaration
   ///    void EAWriteBarrier();
   ///
   /// \Description
   ///    A write barrier ensures that neither software nor hardware delay a memory 
   ///    write operation past the barrier. If you want your memory write committed
   ///    to main memory immediately, this statement will have that effect. As such,
   ///    this is something like a flush of the current processor's write cache.
   ///    Note that flushing memory from a processor's cache to main memory like this
   ///    doesn't cause a second processor to immediately see the changed values in 
   ///    main memory, as the second processor has a read cache between it and main 
   ///    memory. Thus, a second processor would need to execute a read barrier if it
   ///    wants to see the updates immediately.
   #define EAWriteBarrier()





   /// EAReadWriteBarrier
   ///
   /// Declaration
   ///    void EAReadWriteBarrier();
   ///
   /// Description
   ///    A read/write barrier has the same effect as both a read barrier and a write
   ///    barrier at once. A read barrier ensures that neither software nor hardware 
   ///    perform a memory read prior to the read barrier, while a write barrier 
   ///    ensures that neither software nor hardware delay a memory write operation 
   ///    past the barrier. A ReadWriteBarrier specifically acts like a WriteBarrier
   ///    followed by a ReadBarrier, despite the name ReadWriteBarrier being the 
   ///    other way around.
   ///
   ///    EAReadWriteBarrier synchronizes both reads and writes to system memory 
   ///    between processors and their caches on multiprocessor systems, particulary 
   ///    SMP systems. This can be useful to ensure the state of global variables at 
   ///    a particular point in your code for multithreaded applications. Higher level
   ///    thread synchronization level primitives such as mutexes achieve the same 
   ///    effect (while providing the additional functionality of synchronizing code
   ///    execution) but at a significantly higher cost. 
   ///
   ///    A two-processor SMP system has two processors, each with its own instruction
   ///    and data caches. If the first processor writes to a memory location and the 
   ///    second processor needs to read from that location, the first procesor's 
   ///    write may still be in its cache and not committed to main memory and the 
   ///    second processor may thus would not see the newly written value. The value
   ///    will eventually get written from the first cache to main memory, but if you 
   ///    need to ensure that it is written at a particular time, you would use a 
   ///    ReadWrite barrier. 
   ///
   ///    This function is similar to the Linux kernel rwb() function and to the 
   ///    Windows kernel KeMemoryBarrier function.
   #define EAReadWriteBarrier()





   /// EACompilerMemoryBarrier
   ///
   /// \Declaration
   ///    void EACompilerMemoryBarrier();
   ///
   /// \Description
   ///    Provides a barrier for compiler optimization. The compiler will not make
   ///    assumptions about locations across an EACompilerMemoryBarrier statement.
   ///    For example, if a compiler has memory values temporarily cached in 
   ///    registers but you need them to be written to memory, you can execute the
   ///    EACompilerMemoryBarrier statement. This is somewhat similar in concept to 
   ///    the C volatile keyword except that it applies to all memory the compiler
   ///    is currently working with and applies its effect only where you specify
   ///    and not for every usage as with the volatile keyword. 
   ///
   ///    Under GCC, this statement is equivalent to the GCC `asm volatile("":::"memory")` 
   ///    statement. Under VC++, this is equivalent to a _ReadWriteBarrier statement  
   ///    (not to be confused with EAReadWriteBarrier above) and equivalent to the Windows
   ///    kernel function KeMemoryBarrierWithoutFence. This is also known as barrier()
   ///    undef Linux. 
   ///    
   ///    EACompilerMemoryBarrier is a compiler-level statement and not a 
   ///    processor-level statement. For processor-level memory barriers, 
   ///    use EAReadBarrier, etc.
   /// 
   /// \Example
   ///    Without the compiler memory barrier below, an optimizing compiler might
   ///    never assign 0 to gValue because gValue is reassigned to 1 later and 
   ///    because gValue is not declared volatile.
   ///
   ///    void ThreadFunction() {
   ///       extern int gValue; // Note that gValue is intentionally not declared volatile, 
   ///       gValue = 0;
   ///       EACompilerMemoryBarrier();
   ///       gValue = 1;
   ///    }
   #define EACompilerMemoryBarrier()


#endif // EA_THREAD_SYNC_IMPLEMENTED


#endif // #ifdef EATHREAD_EATHREAD_SYNC_H








