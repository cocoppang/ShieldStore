// -*- Mode: C++; c-basic-offset: 2; indent-tabs-mode: nil -*-
// Copyright (c) 2005, Google Inc.
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
// 
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// ---
// Author: Sanjay Ghemawat <opensource@google.com>

#include <config.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#if defined HAVE_STDINT_H
#include <stdint.h>
#elif defined HAVE_INTTYPES_H
#include <inttypes.h>
#else
#include <sys/types.h>
#endif
#include <string>
#include "base/dynamic_annotations.h"
#ifndef TCMALLOC_SGX
#include "base/sysinfo.h"    // for FillProcSelfMaps
#endif
#ifndef NO_HEAP_CHECK
#include "gperftools/heap-checker.h"
#endif
#include "gperftools/malloc_extension.h"
#include "gperftools/malloc_extension_c.h"
#include "maybe_threads.h"
#include "base/googleinit.h"

using STL_NAMESPACE::string;
using STL_NAMESPACE::vector;

#ifndef TCMALLOC_SGX
static void DumpAddressMap(string* result) {
  *result += "\nMAPPED_LIBRARIES:\n";
  // We keep doubling until we get a fit
  const size_t old_resultlen = result->size();
  for (int amap_size = 10240; amap_size < 10000000; amap_size *= 2) {
    result->resize(old_resultlen + amap_size);
    bool wrote_all = false;
    const int bytes_written =
        tcmalloc_ocall::FillProcSelfMaps(&((*result)[old_resultlen]), amap_size,
                                   &wrote_all);
    if (wrote_all) {   // we fit!
      (*result)[old_resultlen + bytes_written] = '\0';
      result->resize(old_resultlen + bytes_written);
      return;
    }
  }
  result->reserve(old_resultlen);   // just don't print anything
}
#endif

// Note: this routine is meant to be called before threads are spawned.
void MallocExtension_ocall::Initialize() {
  static bool initialize_called = false;

  if (initialize_called) return;
  initialize_called = true;

#ifdef __GLIBC__
  // GNU libc++ versions 3.3 and 3.4 obey the environment variables
  // GLIBCPP_FORCE_NEW and GLIBCXX_FORCE_NEW respectively.  Setting
  // one of these variables forces the STL default allocator to call
  // new() or delete() for each allocation or deletion.  Otherwise
  // the STL allocator tries to avoid the high cost of doing
  // allocations by pooling memory internally.  However, tcmalloc
  // does allocations really fast, especially for the types of small
  // items one sees in STL, so it's better off just using us.
  // TODO: control whether we do this via an environment variable?
  setenv("GLIBCPP_FORCE_NEW", "1", false /* no overwrite*/);
  setenv("GLIBCXX_FORCE_NEW", "1", false /* no overwrite*/);

  // Now we need to make the setenv 'stick', which it may not do since
  // the env is flakey before main() is called.  But luckily stl only
  // looks at this env var the first time it tries to do an alloc, and
  // caches what it finds.  So we just cause an stl alloc here.
  string dummy("I need to be allocated");
  dummy += "!";         // so the definition of dummy isn't optimized out
#endif  /* __GLIBC__ */
}

// SysAllocator_ocall implementation
SysAllocator_ocall::~SysAllocator_ocall() {}

// Default implementation -- does nothing
MallocExtension_ocall::~MallocExtension_ocall() { }
bool MallocExtension_ocall::VerifyAllMemory() { return true; }
bool MallocExtension_ocall::VerifyNewMemory(const void* p) { return true; }
bool MallocExtension_ocall::VerifyArrayNewMemory(const void* p) { return true; }
bool MallocExtension_ocall::VerifyMallocMemory(const void* p) { return true; }

bool MallocExtension_ocall::GetNumericProperty(const char* property, size_t* value) {
  return false;
}

bool MallocExtension_ocall::SetNumericProperty(const char* property, size_t value) {
  return false;
}

void MallocExtension_ocall::GetStats(char* buffer, int length) {
  assert(length > 0);
  buffer[0] = '\0';
}

bool MallocExtension_ocall::MallocMemoryStats(int* blocks, size_t* total,
                                       int histogram[kMallocHistogramSize]) {
  *blocks = 0;
  *total = 0;
  memset(histogram, 0, sizeof(*histogram) * kMallocHistogramSize);
  return true;
}

void** MallocExtension_ocall::ReadStackTraces(int* sample_period) {
  return NULL;
}

void** MallocExtension_ocall::ReadHeapGrowthStackTraces() {
  return NULL;
}

void MallocExtension_ocall::MarkThreadIdle() {
  // Default implementation does nothing
}

void MallocExtension_ocall::MarkThreadBusy() {
  // Default implementation does nothing
}

SysAllocator_ocall* MallocExtension_ocall::GetSystemAllocator() {
  return NULL;
}

void MallocExtension_ocall::SetSystemAllocator(SysAllocator_ocall *a) {
  // Default implementation does nothing
}

void MallocExtension_ocall::ReleaseToSystem(size_t num_bytes) {
  // Default implementation does nothing
}

void MallocExtension_ocall::ReleaseFreeMemory() {
  ReleaseToSystem(static_cast<size_t>(-1));   // SIZE_T_MAX
}

void MallocExtension_ocall::SetMemoryReleaseRate(double rate) {
  // Default implementation does nothing
}

double MallocExtension_ocall::GetMemoryReleaseRate() {
  return -1.0;
}

size_t MallocExtension_ocall::GetEstimatedAllocatedSize(size_t size) {
  return size;
}

size_t MallocExtension_ocall::GetAllocatedSize(const void* p) {
  assert(GetOwnership(p) != kNotOwned);
  return 0;
}

MallocExtension_ocall::Ownership MallocExtension_ocall::GetOwnership(const void* p) {
  return kUnknownOwnership;
}

void MallocExtension_ocall::GetFreeListSizes(
    vector<MallocExtension_ocall::FreeListInfo>* v) {
  v->clear();
}

size_t MallocExtension_ocall::GetThreadCacheSize() {
  return 0;
}

void MallocExtension_ocall::MarkThreadTemporarilyIdle() {
  // Default implementation does nothing
}

// The current malloc extension object.

static MallocExtension_ocall* current_instance;

static void InitModule() {
  if (current_instance != NULL) {
    return;
  }
  current_instance = new MallocExtension_ocall;
#ifndef NO_HEAP_CHECK
  HeapLeakChecker::IgnoreObject(current_instance);
#endif
}

REGISTER_MODULE_INITIALIZER(malloc_extension_init, InitModule())

MallocExtension_ocall* MallocExtension_ocall::instance() {
  InitModule();
  return current_instance;
}

void MallocExtension_ocall::Register(MallocExtension_ocall* implementation) {
  InitModule();
  // When running under valgrind, our custom malloc is replaced with
  // valgrind's one and malloc extensions will not work.  (Note:
  // callers should be responsible for checking that they are the
  // malloc that is really being run, before calling Register.  This
  // is just here as an extra sanity check.)
  if (!RunningOnValgrind_ocall()) {
    current_instance = implementation;
  }
}

// -----------------------------------------------------------------------
// Heap sampling support
// -----------------------------------------------------------------------

namespace {

// Accessors
uintptr_t Count(void** entry) {
  return reinterpret_cast<uintptr_t>(entry[0]);
}
uintptr_t Size(void** entry) {
  return reinterpret_cast<uintptr_t>(entry[1]);
}
uintptr_t Depth(void** entry) {
  return reinterpret_cast<uintptr_t>(entry[2]);
}
void* PC(void** entry, int i) {
  return entry[3+i];
}

void PrintCountAndSize(MallocExtension_ocallWriter* writer,
                       uintptr_t count, uintptr_t size) {
  char buf[100];
  snprintf(buf, sizeof(buf),
           "%6" PRIu64 ": %8" PRIu64 " [%6" PRIu64 ": %8" PRIu64 "] @",
           static_cast<uint64>(count),
           static_cast<uint64>(size),
           static_cast<uint64>(count),
           static_cast<uint64>(size));
  writer->append(buf, strlen(buf));
}

void PrintHeader(MallocExtension_ocallWriter* writer,
                 const char* label, void** entries) {
  // Compute the total count and total size
  uintptr_t total_count = 0;
  uintptr_t total_size = 0;
  for (void** entry = entries; Count(entry) != 0; entry += 3 + Depth(entry)) {
    total_count += Count(entry);
    total_size += Size(entry);
  }

  const char* const kTitle = "heap profile: ";
  writer->append(kTitle, strlen(kTitle));
  PrintCountAndSize(writer, total_count, total_size);
  writer->append(" ", 1);
  writer->append(label, strlen(label));
  writer->append("\n", 1);
}

void PrintStackEntry(MallocExtension_ocallWriter* writer, void** entry) {
  PrintCountAndSize(writer, Count(entry), Size(entry));

  for (int i = 0; i < Depth(entry); i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), " %p", PC(entry, i));
    writer->append(buf, strlen(buf));
  }
  writer->append("\n", 1);
}

}

void MallocExtension_ocall::GetHeapSample(MallocExtension_ocallWriter* writer) {
   /*Disable in SGX*/
#ifndef TCMALLOC_SGX
  int sample_period = 0;
  void** entries = ReadStackTraces(&sample_period);
  if (entries == NULL) {
    const char* const kErrorMsg =
        "This malloc implementation does not support sampling.\n"
        "As of 2005/01/26, only tcmalloc supports sampling, and\n"
        "you are probably running a binary that does not use\n"
        "tcmalloc.\n";
    writer->append(kErrorMsg, strlen(kErrorMsg));
    return;
  }

  char label[32];
  sprintf(label, "heap_v2/%d", sample_period);
  PrintHeader(writer, label, entries);
  for (void** entry = entries; Count(entry) != 0; entry += 3 + Depth(entry)) {
    PrintStackEntry(writer, entry);
  }
  delete[] entries;

  DumpAddressMap(writer);
#endif
}

void MallocExtension_ocall::GetHeapGrowthStacks(MallocExtension_ocallWriter* writer) {
   /*disabled in SGX*/
#ifndef TCMALLOC_SGX    
  void** entries = ReadHeapGrowthStackTraces();
  if (entries == NULL) {
    const char* const kErrorMsg =
        "This malloc implementation does not support "
        "ReadHeapGrowthStackTraces().\n"
        "As of 2005/09/27, only tcmalloc supports this, and you\n"
        "are probably running a binary that does not use tcmalloc.\n";
    writer->append(kErrorMsg, strlen(kErrorMsg));
    return;
  }

  // Do not canonicalize the stack entries, so that we get a
  // time-ordered list of stack traces, which may be useful if the
  // client wants to focus on the latest stack traces.
  PrintHeader(writer, "growth", entries);
  for (void** entry = entries; Count(entry) != 0; entry += 3 + Depth(entry)) {
    PrintStackEntry(writer, entry);
  }
  delete[] entries;

  DumpAddressMap(writer);
#endif
}

void MallocExtension_ocall::Ranges(void* arg, RangeFunction func) {
  // No callbacks by default
}

// These are C shims that work on the current instance.

#define C_SHIM(fn, retval, paramlist, arglist)          \
  extern "C" PERFTOOLS_DLL_DECL retval MallocExtension_ocall_##fn paramlist {    \
    return MallocExtension_ocall::instance()->fn arglist;     \
  }

C_SHIM(VerifyAllMemory, int, (void), ());
C_SHIM(VerifyNewMemory, int, (const void* p), (p));
C_SHIM(VerifyArrayNewMemory, int, (const void* p), (p));
C_SHIM(VerifyMallocMemory, int, (const void* p), (p));
C_SHIM(MallocMemoryStats, int,
       (int* blocks, size_t* total, int histogram[kMallocHistogramSize]),
       (blocks, total, histogram));

C_SHIM(GetStats, void,
       (char* buffer, int buffer_length), (buffer, buffer_length));
C_SHIM(GetNumericProperty, int,
       (const char* property, size_t* value), (property, value));
C_SHIM(SetNumericProperty, int,
       (const char* property, size_t value), (property, value));

C_SHIM(MarkThreadIdle, void, (void), ());
C_SHIM(MarkThreadBusy, void, (void), ());
C_SHIM(ReleaseFreeMemory, void, (void), ());
C_SHIM(ReleaseToSystem, void, (size_t num_bytes), (num_bytes));
C_SHIM(GetEstimatedAllocatedSize, size_t, (size_t size), (size));
C_SHIM(GetAllocatedSize, size_t, (const void* p), (p));
C_SHIM(GetThreadCacheSize, size_t, (void), ());
C_SHIM(MarkThreadTemporarilyIdle, void, (void), ());

// Can't use the shim here because of the need to translate the enums.
extern "C"
MallocExtension_ocall_Ownership MallocExtension_ocall_GetOwnership(const void* p) {
  return static_cast<MallocExtension_ocall_Ownership>(
      MallocExtension_ocall::instance()->GetOwnership(p));
}
