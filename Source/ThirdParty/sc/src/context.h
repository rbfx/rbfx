/*
 * Copyright (c) 2016 Johan Sköld
 * License: https://opensource.org/licenses/ISC
 */

#pragma once

#include <stddef.h> /* size_t */

#include <sc/sc.h>

#if defined(_MSC_VER) && _MSC_VER < 1900
#   define SC_INLINE static
#else
#   define SC_INLINE static inline
#endif

typedef void* sc_context_sp_t;

typedef struct {
    sc_context_sp_t ctx;
    void* data;
} sc_transfer_t;

sc_transfer_t SC_CALL_DECL sc_jump_context (sc_context_sp_t to, void* vp);
sc_context_sp_t SC_CALL_DECL sc_make_context (void* sp, size_t size, void(*fn)(sc_transfer_t));
void SC_CALL_DECL sc_context_state (sc_state_t* state, sc_context_sp_t ctx);

/* For the provided fcontext implementations, there's no necessary work to
   be done for freeing a context, but some custom backends (for proprietary
   hardware) do. */

#if defined(SC_CUSTOM_FREE_CONTEXT)
    void SC_CALL_DECL sc_free_context (sc_context_sp_t);
#else
    SC_INLINE void sc_free_context (sc_context_sp_t ctx) { (void)ctx; }
#endif
