//===-- ThreadLauncher.cpp ---------------------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

// lldb Includes
#include "lldb/Core/Log.h"
#include "lldb/Host/HostNativeThread.h"
#include "lldb/Host/HostThread.h"
#include "lldb/Host/ThisThread.h"
#include "lldb/Host/ThreadLauncher.h"

#if defined(_WIN32)
#include "lldb/Host/windows/windows.h"
#endif

using namespace lldb;
using namespace lldb_private;

HostThread
ThreadLauncher::LaunchThread(llvm::StringRef name, lldb::thread_func_t thread_function, lldb::thread_arg_t thread_arg, Error *error_ptr)
{
    Error error;
    if (error_ptr)
        error_ptr->Clear();

    // Host::ThreadCreateTrampoline will delete this pointer for us.
    HostThreadCreateInfo *info_ptr = new HostThreadCreateInfo(name.data(), thread_function, thread_arg);
    lldb::thread_t thread;
#ifdef _WIN32
    thread = (lldb::thread_t)::_beginthreadex(0, 0, HostNativeThread::ThreadCreateTrampoline, info_ptr, 0, NULL);
    if (thread == (lldb::thread_t)(-1L))
        error.SetError(::GetLastError(), eErrorTypeWin32);
#else
    int err = ::pthread_create(&thread, NULL, HostNativeThread::ThreadCreateTrampoline, info_ptr);
    error.SetError(err, eErrorTypePOSIX);
#endif
    if (error_ptr)
        *error_ptr = error;
    if (!error.Success())
        thread = LLDB_INVALID_HOST_THREAD;

    return HostThread(thread);
}
