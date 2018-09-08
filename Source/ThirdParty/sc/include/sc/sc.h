/*
 * Copyright (c) 2018 Johan Sköld
 * License: https://opensource.org/licenses/ISC
 */

#pragma once

#include <stddef.h> /* size_t */
#include <stdint.h> /* uint32_t, uint64_t */

#if defined(_MSC_VER)
#   define SC_CALL_DECL __cdecl
#else
#   define SC_CALL_DECL
#endif

#if defined(__cplusplus)
extern "C" {
#endif

/** Minimum supported stack size. */
enum { SC_MIN_STACK_SIZE = 2048 };

/** Context handle type. */
typedef struct sc_context* sc_context_t;

/** CPU types, for use with `sc_state_t`. */
typedef enum {
    SC_CPU_TYPE_UNKNOWN,
    SC_CPU_TYPE_X86,
    SC_CPU_TYPE_X64,
    SC_CPU_TYPE_ARM,
    SC_CPU_TYPE_ARM64,
} sc_cpu_type_t;

/** Context state, as captured by `sc_get_state`. */
typedef struct {
    sc_cpu_type_t type;

    union {
        /** Registers captured on an arm CPU (when `type` is
         ** `SC_CPU_TYPE_ARM`. */
        struct {
            uint32_t v1;
            uint32_t v2;
            uint32_t v3;
            uint32_t v4;
            uint32_t v5;
            uint32_t v6;
            uint32_t v7;
            uint32_t v8;
            uint32_t lr;
            uint32_t sp;
            uint32_t pc;
        } arm;

        /** Registers captured on an arm64 CPU (when `type` is
         ** `SC_CPU_TYPE_ARM`. */
        struct {
            uint64_t x19;
            uint64_t x20;
            uint64_t x21;
            uint64_t x22;
            uint64_t x23;
            uint64_t x24;
            uint64_t x25;
            uint64_t x26;
            uint64_t x27;
            uint64_t x28;
            uint64_t fp;
            uint64_t lr;
            uint64_t sp;
            uint64_t pc;
        } arm64;

        /** Registers captured on an x86 CPU (when `type` is
         ** `SC_CPU_TYPE_X86`. */
        struct {
            uint32_t edi;
            uint32_t esi;
            uint32_t ebx;
            uint32_t ebp;
            uint32_t eip;
            uint32_t esp;
        } x86;

        /** Registers captured on an x64 CPU (when `type` is
         ** `SC_CPU_TYPE_X64`. */
        struct {
            uint64_t r12;
            uint64_t r13;
            uint64_t r14;
            uint64_t r15;
            uint64_t rdi;
            uint64_t rsi;
            uint64_t rbx;
            uint64_t rbp;
            uint64_t rip;
            uint64_t rsp;
        } x64;
    } registers;
} sc_state_t;

/** Context procedure type. */
typedef void (SC_CALL_DECL * sc_context_proc_t) (void* param);

/** Create a context with the given stack and procedure.
 **
 ** * `stack_ptr`:  Pointer to the buffer the context should use as stack.
 **                 Must be a valid pointer (not NULL).
 ** * `stack_size`: Size of the stack buffer provided in `stack_ptr`. Must
 **                 be at least `SC_MIN_STACK_SIZE`.
 ** * `proc`:       Procedure to invoke inside the new context. The
 **                 parameter passed to the proc will be the first value
 **                 yielded to the context through `sc_yield`.
 **
 ** **Note:** If the proc is allowed to run to its end, it will cause the
 **           process to exit.
 **
 ** **Important:** The stack must be big enough to be able to contain the
 **                maximum stack size used by the procedure. As this is
 **                implementation specific, it is up to the caller (or
 **                possibly attached debuggers) to ensure this is true. */
sc_context_t SC_CALL_DECL sc_context_create (
    void* stack_ptr,
    size_t stack_size,
    sc_context_proc_t proc
);

/** Destroy a context created through `sc_context_create`.
 **
 ** * `context`: Context to destroy. Must not be the currently executing
 **              context, or the main context (retrieved by calling
 **                `sc_main_context`). */
void SC_CALL_DECL sc_context_destroy (sc_context_t context);

/** Switch execution to another context, returning control to it, and
 ** passing the given value to it. Returns the value passed to
 ** `sc_switch` or `sc_yield` when control is returned to this context.
 **
 ** * `target`: Context to switch control to. Must be a valid context
 **             created by `sc_context_create`, or returned by
 **             `sc_main_context`.
 ** * `value`: Value to pass to the target context. */
void* SC_CALL_DECL sc_switch (sc_context_t target, void* value);

/** Yields execution to the parent context, returning control to it, and
 ** passing the given value to it. Returns the value passed to `sc_switch`
 ** or `sc_yield` when control is returned to this context.
 **
 ** * `value`: Value to pass to the target context.
 **
 ** **Note:** The *parent context* is the context that created this
 **           context. It is up to the caller to ensure that the parent
 **           context is still valid.
 **
 ** **Important:** This should not be called from the main context
 **                (returned by `sc_main_context`), as it does not have a
 **                parent. Doing so triggers an assert in debug builds,
 **                and undefined behavior in release builds. */
void* SC_CALL_DECL sc_yield (void* value);

/** Assign a user-specified pointer to be attached to the given context.
 **
 ** * `context`: Context to attach the pointer to.
 ** * `data`:    Pointer to attach to the context. */
void SC_CALL_DECL sc_set_data (sc_context_t context, void* data);

/** Get the user-specified pointer assigned to the given context through
 ** `sc_set_data`.
 **
 ** * `context`: Context to get the attached pointer for. */
void* SC_CALL_DECL sc_get_data (sc_context_t context);

/** Get the current state of the given context.
 **
 ** * `context`: Context to get the state of.
 **
 ** **Note:** Currently only implemented for Windows, macOS, and Linux. Other
 **           platforms will have the type set to `SC_CPU_TYPE_UNKNOWN`. */
sc_state_t SC_CALL_DECL sc_get_state (sc_context_t context);

/** Get the handle for the currently executing context. */
sc_context_t SC_CALL_DECL sc_current_context (void);

/** Get the handle for the currently executing context's parent context.
 ** The parent context is the context that created this context. If the
 ** currently executing context is the main context, NULL is returned. */
sc_context_t SC_CALL_DECL sc_parent_context (void);

/** Get the handle for this thread's main context. */
sc_context_t SC_CALL_DECL sc_main_context (void);

#if defined(__cplusplus)
} // extern "C"
#endif
