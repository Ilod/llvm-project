//===-- PlatformDarwin.h ----------------------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef liblldb_PlatformDarwin_h_
#define liblldb_PlatformDarwin_h_

// C Includes
// C++ Includes
// Other libraries and framework includes
// Project includes
#include "lldb/Target/Platform.h"

class PlatformDarwin : public lldb_private::Platform
{
public:
    PlatformDarwin (bool is_host);

    virtual
    ~PlatformDarwin();
    
    //------------------------------------------------------------
    // lldb_private::Platform functions
    //------------------------------------------------------------
    virtual lldb_private::Error
    ResolveExecutable (const lldb_private::FileSpec &exe_file,
                       const lldb_private::ArchSpec &arch,
                       lldb::ModuleSP &module_sp);

    virtual size_t
    GetSoftwareBreakpointTrapOpcode (lldb_private::Target &target, 
                                     lldb_private::BreakpointSite *bp_site);

    virtual bool
    GetRemoteOSVersion ();

    virtual bool
    GetRemoteOSBuildString (std::string &s);
    
    virtual bool
    GetRemoteOSKernelDescription (std::string &s);

    // Remote Platform subclasses need to override this function
    virtual lldb_private::ArchSpec
    GetRemoteSystemArchitecture ();

    virtual bool
    IsConnected () const;

    virtual lldb_private::Error
    ConnectRemote (lldb_private::Args& args);

    virtual lldb_private::Error
    DisconnectRemote ();

    virtual const char *
    GetHostname ();

    virtual const char *
    GetUserName (uint32_t uid);
    
    virtual const char *
    GetGroupName (uint32_t gid);

    virtual bool
    GetProcessInfo (lldb::pid_t pid, 
                    lldb_private::ProcessInfo &proc_info);
    
    virtual uint32_t
    FindProcesses (const lldb_private::ProcessInfoMatch &match_info,
                   lldb_private::ProcessInfoList &process_infos);
    
protected:
    lldb::PlatformSP m_remote_platform_sp; // Allow multiple ways to connect to a remote darwin OS

private:
    DISALLOW_COPY_AND_ASSIGN (PlatformDarwin);

};

#endif  // liblldb_PlatformDarwin_h_
