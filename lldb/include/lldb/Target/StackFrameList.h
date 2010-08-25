//===-- StackFrameList.h ----------------------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef liblldb_StackFrameList_h_
#define liblldb_StackFrameList_h_

// C Includes
// C++ Includes
#include <vector>

// Other libraries and framework includes
// Project includes
#include "lldb/Host/Mutex.h"
#include "lldb/Target/StackFrame.h"

namespace lldb_private {

class StackFrameList
{
public:
    friend class Thread;
    //------------------------------------------------------------------
    // Constructors and Destructors
    //------------------------------------------------------------------
    StackFrameList (Thread &thread, bool show_inline_frames);

    virtual
    ~StackFrameList();

    uint32_t
    GetNumFrames();

    lldb::StackFrameSP
    GetFrameAtIndex (uint32_t idx);

    // Mark a stack frame as the current frame
    uint32_t
    SetCurrentFrame (lldb_private::StackFrame *frame);

    uint32_t
    GetCurrentFrameIndex () const;

    // Mark a stack frame as the current frame using the frame index
    void
    SetCurrentFrameByIndex (uint32_t idx);

    void
    Clear ();

    void
    InvalidateFrames (uint32_t start_idx);
protected:

    bool
    SetActualFrameAtIndex (uint32_t idx, lldb::StackFrameSP &frame_sp);

    bool
    SetInlineFrameAtIndex (uint32_t idx, lldb::StackFrameSP &frame_sp);


    lldb::StackFrameSP
    GetActualFrameAtIndex (uint32_t idx) const;

    lldb::StackFrameSP
    GetInlineFrameAtIndex (uint32_t idx) const;

    typedef struct InlinedFrameInfo
    {
        uint32_t concrete_frame_index;
        uint32_t inline_height;
        Block *block;
    } InlinedFrameInfo;
    typedef std::vector<InlinedFrameInfo> InlinedFrameInfoCollection;

    //------------------------------------------------------------------
    // Classes that inherit from StackFrameList can see and modify these
    //------------------------------------------------------------------
    typedef std::vector<lldb::StackFrameSP> collection;
    typedef collection::iterator iterator;
    typedef collection::const_iterator const_iterator;

    Thread &m_thread;
    mutable Mutex m_mutex;
    collection m_actual_frames;
    collection m_inline_frames;
    InlinedFrameInfoCollection m_inlined_frame_info;
    uint32_t m_current_frame_idx;
    bool m_show_inlined_frames;

private:
    //------------------------------------------------------------------
    // For StackFrameList only
    //------------------------------------------------------------------
    DISALLOW_COPY_AND_ASSIGN (StackFrameList);
};

} // namespace lldb_private

#endif  // liblldb_StackFrameList_h_
