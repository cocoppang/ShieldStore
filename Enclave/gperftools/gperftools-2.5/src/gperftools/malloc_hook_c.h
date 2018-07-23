/* Copyright (c) 2008, Google Inc.
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
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
 * --
 * Author: Craig Silverstein
 *
 * C shims for the C++ malloc_hook.h.  See malloc_hook.h for details
 * on how to use these.
 */

#ifndef _MALLOC_HOOK_C_H_
#define _MALLOC_HOOK_C_H_

#include <stddef.h>
#include <sys/types.h>

/* Annoying stuff for windows; makes sure clients can import these functions */
#ifndef PERFTOOLS_DLL_DECL
# ifdef _WIN32
#   define PERFTOOLS_DLL_DECL  __declspec(dllimport)
# else
#   define PERFTOOLS_DLL_DECL
# endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Get the current stack trace.  Try to skip all routines up to and
 * and including the caller of MallocHook_ocall::Invoke*.
 * Use "skip_count" (similarly to GetStackTrace from stacktrace.h)
 * as a hint about how many routines to skip if better information
 * is not available.
 */
PERFTOOLS_DLL_DECL
int MallocHook_ocall_GetCallerStackTrace(void** result, int max_depth,
                                   int skip_count);

/* The MallocHook_ocall_{Add,Remove}*Hook functions return 1 on success and 0 on
 * failure.
 */

typedef void (*MallocHook_ocall_NewHook)(const void* ptr, size_t size);
PERFTOOLS_DLL_DECL
int MallocHook_ocall_AddNewHook(MallocHook_ocall_NewHook hook);
PERFTOOLS_DLL_DECL
int MallocHook_ocall_RemoveNewHook(MallocHook_ocall_NewHook hook);

typedef void (*MallocHook_ocall_DeleteHook)(const void* ptr);
PERFTOOLS_DLL_DECL
int MallocHook_ocall_AddDeleteHook(MallocHook_ocall_DeleteHook hook);
PERFTOOLS_DLL_DECL
int MallocHook_ocall_RemoveDeleteHook(MallocHook_ocall_DeleteHook hook);

typedef void (*MallocHook_ocall_PreMmapHook)(const void *start,
                                       size_t size,
                                       int protection,
                                       int flags,
                                       int fd,
                                       off_t offset);
PERFTOOLS_DLL_DECL
int MallocHook_ocall_AddPreMmapHook(MallocHook_ocall_PreMmapHook hook);
PERFTOOLS_DLL_DECL
int MallocHook_ocall_RemovePreMmapHook(MallocHook_ocall_PreMmapHook hook);

typedef void (*MallocHook_ocall_MmapHook)(const void* result,
                                    const void* start,
                                    size_t size,
                                    int protection,
                                    int flags,
                                    int fd,
                                    off_t offset);
PERFTOOLS_DLL_DECL
int MallocHook_ocall_AddMmapHook(MallocHook_ocall_MmapHook hook);
PERFTOOLS_DLL_DECL
int MallocHook_ocall_RemoveMmapHook(MallocHook_ocall_MmapHook hook);

typedef int (*MallocHook_ocall_MmapReplacement)(const void* start,
                                          size_t size,
                                          int protection,
                                          int flags,
                                          int fd,
                                          off_t offset,
                                          void** result);
int MallocHook_ocall_SetMmapReplacement(MallocHook_ocall_MmapReplacement hook);
int MallocHook_ocall_RemoveMmapReplacement(MallocHook_ocall_MmapReplacement hook);

typedef void (*MallocHook_ocall_MunmapHook)(const void* ptr, size_t size);
PERFTOOLS_DLL_DECL
int MallocHook_ocall_AddMunmapHook(MallocHook_ocall_MunmapHook hook);
PERFTOOLS_DLL_DECL
int MallocHook_ocall_RemoveMunmapHook(MallocHook_ocall_MunmapHook hook);

typedef int (*MallocHook_ocall_MunmapReplacement)(const void* ptr,
                                            size_t size,
                                            int* result);
int MallocHook_ocall_SetMunmapReplacement(MallocHook_ocall_MunmapReplacement hook);
int MallocHook_ocall_RemoveMunmapReplacement(MallocHook_ocall_MunmapReplacement hook);

typedef void (*MallocHook_ocall_MremapHook)(const void* result,
                                      const void* old_addr,
                                      size_t old_size,
                                      size_t new_size,
                                      int flags,
                                      const void* new_addr);
PERFTOOLS_DLL_DECL
int MallocHook_ocall_AddMremapHook(MallocHook_ocall_MremapHook hook);
PERFTOOLS_DLL_DECL
int MallocHook_ocall_RemoveMremapHook(MallocHook_ocall_MremapHook hook);

typedef void (*MallocHook_ocall_PreSbrkHook)(ptrdiff_t increment);
PERFTOOLS_DLL_DECL
int MallocHook_ocall_AddPreSbrkHook(MallocHook_ocall_PreSbrkHook hook);
PERFTOOLS_DLL_DECL
int MallocHook_ocall_RemovePreSbrkHook(MallocHook_ocall_PreSbrkHook hook);

typedef void (*MallocHook_ocall_SbrkHook)(const void* result, ptrdiff_t increment);
PERFTOOLS_DLL_DECL
int MallocHook_ocall_AddSbrkHook(MallocHook_ocall_SbrkHook hook);
PERFTOOLS_DLL_DECL
int MallocHook_ocall_RemoveSbrkHook(MallocHook_ocall_SbrkHook hook);

/* The following are DEPRECATED. */
PERFTOOLS_DLL_DECL
MallocHook_ocall_NewHook MallocHook_ocall_SetNewHook(MallocHook_ocall_NewHook hook);
PERFTOOLS_DLL_DECL
MallocHook_ocall_DeleteHook MallocHook_ocall_SetDeleteHook(MallocHook_ocall_DeleteHook hook);
PERFTOOLS_DLL_DECL
MallocHook_ocall_PreMmapHook MallocHook_ocall_SetPreMmapHook(MallocHook_ocall_PreMmapHook hook);
PERFTOOLS_DLL_DECL
MallocHook_ocall_MmapHook MallocHook_ocall_SetMmapHook(MallocHook_ocall_MmapHook hook);
PERFTOOLS_DLL_DECL
MallocHook_ocall_MunmapHook MallocHook_ocall_SetMunmapHook(MallocHook_ocall_MunmapHook hook);
PERFTOOLS_DLL_DECL
MallocHook_ocall_MremapHook MallocHook_ocall_SetMremapHook(MallocHook_ocall_MremapHook hook);
PERFTOOLS_DLL_DECL
MallocHook_ocall_PreSbrkHook MallocHook_ocall_SetPreSbrkHook(MallocHook_ocall_PreSbrkHook hook);
PERFTOOLS_DLL_DECL
MallocHook_ocall_SbrkHook MallocHook_ocall_SetSbrkHook(MallocHook_ocall_SbrkHook hook);
/* End of DEPRECATED functions. */

#ifdef __cplusplus
}   // extern "C"
#endif

#endif /* _MALLOC_HOOK_C_H_ */
