/*
Copyright (C)  2004 Artem Khodush

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.

3. The name of the author may not be used to endorse or promote products
derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "ProcessHelpers.h"

// Pipe
Pipe::Pipe()
: m_direction(closed), m_r(INVALID_HANDLE_VALUE), m_w(INVALID_HANDLE_VALUE)
{
    open();
}

Pipe::~Pipe()
{
    close();
}

void Pipe::close_r()
{
    if (m_direction == both || m_direction == read) {
        if (!CloseHandle(m_r)) {
            abort();
        }
        m_direction = m_direction == both ? write : closed;
    }
}

void Pipe::close_w()
{
    if (m_direction == both || m_direction == write) {
        if (!CloseHandle(m_w)) {
            abort();
        }
        m_direction = m_direction == both ? read : closed;
    }
}

void Pipe::close()
{
    close_r();
    close_w();
}

void Pipe::open()
{
    close();
    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = true;
    sa.lpSecurityDescriptor = 0;
    if (!CreatePipe(&m_r, &m_w, &sa, 0))
        abort();
    m_direction = both;
}

HANDLE Pipe::r() const
{
    return m_r;
}

HANDLE Pipe::w() const
{
    return m_w;
}

// SetSTDHandle
SetSTDHandle::SetSTDHandle(DWORD kind, HANDLE handle)
: m_kind(kind), m_save_handle(GetStdHandle(kind))
{
    if (m_save_handle == INVALID_HANDLE_VALUE)
        abort();
    if (!SetStdHandle(kind, handle))
        abort();
}

SetSTDHandle::~SetSTDHandle()
{
    SetStdHandle(m_kind, m_save_handle);
}

//WaitResult
WaitResult::WaitResult()
{
    m_signaled_object = INVALID_HANDLE_VALUE;
    m_timed_out = false;
    m_error_code = ERROR_SUCCESS;
    m_error_message = "";
}

WaitResult::WaitResult(DWORD wait_result, int objects_count, HANDLE const * objects)
{
    m_signaled_object = INVALID_HANDLE_VALUE;
    m_timed_out = false;
    m_error_code = ERROR_SUCCESS;
    m_error_message = "";
    if (wait_result >= WAIT_OBJECT_0 && wait_result < WAIT_OBJECT_0 + objects_count) {
        m_signaled_object = objects[wait_result - WAIT_OBJECT_0];
    }
    else if (wait_result >= WAIT_ABANDONED_0 && wait_result < WAIT_ABANDONED_0 + objects_count) {
        m_error_message = "WaitResult: one of the wait objects was abandoned";
    }
    else if (wait_result == WAIT_TIMEOUT) {
        m_timed_out = true;
        m_error_message = "WaitResult: timeout elapsed";
    }
    else if (wait_result == WAIT_FAILED) {
        m_error_code = GetLastError();
    }
    else {
        m_error_message = "WaitResult: weird error: unrecognised WaitForMultipleObjects return value";
        m_error_code = wait_result;
    }
}

bool WaitResult::ok()
{
    return m_error_code == ERROR_SUCCESS && m_error_message[0] == 0;
}

bool WaitResult::is_signaled(Event & event)
{
    return m_signaled_object == event.m_handle;
}

bool WaitResult::timed_out()
{
    return m_timed_out;
}

DWORD WaitResult::error_code()
{
    return m_error_code;
}

char const * WaitResult::error_message()
{
    return m_error_message;
}

// Event
Event::Event()
{
    m_handle = CreateEvent(0, TRUE, FALSE, 0);
    if (m_handle == 0) {
        abort();
    }
}

Event::~Event()
{
    CloseHandle(m_handle);
}

bool Event::set()
{
    return SetEvent(m_handle) != 0;
}

bool Event::reset()
{
    return ResetEvent(m_handle) != 0;
}

// wait functions
WaitResult wait(HANDLE h, DWORD timeout)
{
    return WaitResult(WaitForSingleObject(h, timeout), 1, &h);
}

WaitResult wait(Event & e, DWORD timeout)
{
    return WaitResult(WaitForSingleObject(e.m_handle, timeout), 1, &e.m_handle);
}

WaitResult wait(Event & e1, Event & e2, DWORD timeout)
{
    HANDLE h[2];
    h[0] = e1.m_handle;
    h[1] = e2.m_handle;
    return WaitResult(WaitForMultipleObjects(2, h, FALSE, timeout), 2, h);
}

// Mutex
Mutex::Mutex()
{
    m_handle = CreateMutex(0, FALSE, 0);
    if (m_handle == 0) {
        abort();
    }
}

Mutex::~Mutex()
{
    CloseHandle(m_handle);
}

// GrabMutex
GrabMutex::GrabMutex(Mutex & mutex, DWORD timeout)
{
    m_mutex = mutex.m_handle;
    m_wait_result = wait(m_mutex, timeout);
}

GrabMutex::~GrabMutex()
{
    if (m_wait_result.ok()) {
        ReleaseMutex(m_mutex);
    }
}

bool GrabMutex::ok()
{
    return m_wait_result.ok();
}

DWORD GrabMutex::error_code()
{
    return m_wait_result.error_code();
}

char const * GrabMutex::error_message()
{
    return m_wait_result.error_message();
}

// ThreadBuffer
ThreadBuffer::ThreadBuffer()
{
    m_direction = dir_none;

    m_message_prefix = "";
    m_error_code = ERROR_SUCCESS;
    m_error_message = "";

    m_wait_timeout = 2000;
    m_buffer_limit = 0;
    m_read_buffer_size = 4096;

    m_thread = 0;

    m_thread_termination_timeout = 500;
    m_translate_crlf = true;
}

ThreadBuffer::~ThreadBuffer()
{
    bool stopped = false;
    try {
        stopped = stop_thread();
    }
    catch (...){
    }
    if (!stopped) {
        // one more time, with attitude
        try {
            stopped = abort_thread();
        }
        catch (...){
        }
        if (!stopped) {
            DWORD code = GetLastError();
            // otherwize, the thread will be left running loose stomping on freed memory.
            std::terminate();
        }
    }
}

void ThreadBuffer::set_wait_timeout(DWORD milliseconds)
{
    if (m_direction != dir_none) {
        abort();
    }
    m_wait_timeout = milliseconds;
}

// next three set values that are accessed in the same thread only, so they may be called anytime
void ThreadBuffer::set_thread_termination_timeout(DWORD milliseconds) {
    m_thread_termination_timeout = milliseconds;
}

void ThreadBuffer::set_binary_mode()
{
    m_translate_crlf = false;
}

void ThreadBuffer::set_text_mode()
{
    m_translate_crlf = true;
}

void ThreadBuffer::set_buffer_limit(std::size_t limit)
{
    if (m_direction != dir_none) {
        abort();
    }
    m_buffer_limit = limit;
}

void ThreadBuffer::set_read_buffer_size(std::size_t size)
{
    if (m_direction != dir_none) {
        abort();
    }
    m_read_buffer_size = size;
}

void ThreadBuffer::start_reader_thread(HANDLE pipe)
{
    start_thread(pipe, dir_read);
}

void ThreadBuffer::start_writer_thread(HANDLE pipe)
{
    start_thread(pipe, dir_write);
}

void ThreadBuffer::start_thread(HANDLE pipe, direction_t direction)
{
    if (m_direction != dir_none) {
        abort();
    }
    m_buffer_list.clear();
    m_pipe = pipe;
    if (!m_stop_thread.reset()) {
        abort();
    }
    if (!m_got_data.reset()) {
        abort();
    }
    if (!m_want_data.set()) {
        abort();
    }
    DWORD thread_id;
    m_thread = CreateThread(0, 0, direction == dir_read ? reader_thread : writer_thread, this, 0, &thread_id);
    if (m_thread == 0) {
        abort();
    }
    m_direction = direction == dir_read ? dir_read : dir_write;
}

bool ThreadBuffer::check_thread_stopped()
{
    WaitResult wait_result = wait(m_thread, m_thread_termination_timeout);
    if (!wait_result.ok() && !wait_result.timed_out()) {
        check_error("ThreadBuffer::check_thread_stopped: wait for thread to finish failed", wait_result.error_code(), wait_result.error_message());
    }
    if (wait_result.ok()) {
        CloseHandle(m_thread);
        m_direction = dir_none;
        return true;
    }
    else {
        return false;
    }
}

bool ThreadBuffer::stop_thread()
{
    if (m_direction != dir_none) {
        if (!m_stop_thread.set()) {
            abort();
        }
        bool res = check_thread_stopped();
        if (res) {
            check_error(m_message_prefix, m_error_code, m_error_message);
        }
        return res;
    }
    return true;
}

bool ThreadBuffer::abort_thread()
{
    if (m_direction != dir_none) {
        if (!TerminateThread(m_thread, 0)) {
            abort();
        }
        return check_thread_stopped();
    }
    return true;
}

void ThreadBuffer::get(Process::stream_kind_t, char * dst, std::size_t & size, bool & no_more)
{
    if (m_direction != dir_read) {
        abort();
    }
    // check thread status
    DWORD thread_exit_code;
    if (!GetExitCodeThread(m_thread, &thread_exit_code)) {
        abort();
    }

    restart:if (thread_exit_code != STILL_ACTIVE) {
        if (!m_buffer_list.empty()) {
            // we have data - deliver it first
            // when thread terminated, there is no need to synchronize
            if (m_translate_crlf) {
                m_buffer_list.get_translate_crlf(dst, size);
            }
            else {
                m_buffer_list.get(dst, size);
            }
            no_more = false;
        }
        else {
            // thread terminated and we have no more data to return - report errors, if any
            check_error(m_message_prefix, m_error_code, m_error_message);
            // if terminated without error  - signal eof 
            no_more = true;
            size = 0;
        }
    }
    else {
        no_more = false;
        // thread still running - synchronize
        // wait for both m_got_data, m_mutex
        WaitResult wait_result = wait(m_got_data, m_wait_timeout);
        if (!wait_result.ok()) {

            if (!GetExitCodeThread(m_thread, &thread_exit_code)) {
                abort();
            }

            if (thread_exit_code != STILL_ACTIVE)
                goto restart;

            check_error("ThreadBuffer::get: wait for got_data failed", wait_result.error_code(), wait_result.error_message());
        }
        GrabMutex grab_mutex(m_mutex, m_wait_timeout);
        if (!grab_mutex.ok()) {

            if (!GetExitCodeThread(m_thread, &thread_exit_code)) {
                abort();
            }

            if (thread_exit_code != STILL_ACTIVE)
                goto restart;

            check_error("ThreadBuffer::get: wait for mutex failed", grab_mutex.error_code(), grab_mutex.error_message());
        }

        if (m_translate_crlf) {
            m_buffer_list.get_translate_crlf(dst, size);
        }
        else {
            m_buffer_list.get(dst, size);
        }

        // if buffer is not too long tell the thread we want more data
        if (!m_buffer_list.full(m_buffer_limit)) {
            if (!m_want_data.set()) {
                abort();
            }
        }
        // if no data left - make the next get() wait until it arrives
        if (m_buffer_list.empty()) {
            if (!m_got_data.reset()) {
                abort();
            }
        }
    }
}

DWORD WINAPI ThreadBuffer::reader_thread(LPVOID param)
{
    ThreadBuffer * p = static_cast<ThreadBuffer *>(param);
    // accessing p anywhere here is safe because ThreadBuffer destructor 
    // ensures the thread is terminated before p get destroyed
    char * read_buffer = 0;
    try {
        read_buffer = new char[p->m_read_buffer_size];

        while (true) {
            // see if get() wants more data, or if someone wants to stop the thread
            WaitResult wait_result = wait(p->m_stop_thread, p->m_want_data, p->m_wait_timeout);
            if (!wait_result.ok() && !wait_result.timed_out()) {
                p->note_thread_error("ThreadBuffer::reader_thread: wait for want_data, destruction failed", wait_result.error_code(), wait_result.error_message());
                break;
            }

            if (wait_result.is_signaled(p->m_stop_thread)) {
                // they want us to stop
                break;
            }

            if (wait_result.is_signaled(p->m_want_data)) {
                // they want more data - read the file
                DWORD read_size = 0;
                DWORD read_status = ERROR_SUCCESS;
                if (!ReadFile(p->m_pipe, read_buffer, p->m_read_buffer_size, &read_size, 0)) {
                    read_status = GetLastError();
                    if (read_status != ERROR_BROKEN_PIPE) {
                        p->note_thread_error("ThreadBuffer::reader_thread: ReadFile failed", read_status, "");
                        break;
                    }
                }

                // read something - append to p->m_buffers
                if (read_size != 0) {
                    GrabMutex grab_mutex(p->m_mutex, p->m_wait_timeout);
                    if (!grab_mutex.ok()) {
                        p->note_thread_error("ThreadBuffer::reader_thread: wait for mutex failed", grab_mutex.error_code(), grab_mutex.error_message());
                        break;
                    }

                    p->m_buffer_list.put(read_buffer, read_size);

                    // if buffer is too long - do not read any more until it shrinks
                    if (p->m_buffer_list.full(p->m_buffer_limit)) {
                        if (!p->m_want_data.reset()) {
                            p->note_thread_error("ThreadBuffer::reader_thread: unable to reset m_want_data event", GetLastError(), "");
                            break;
                        }
                    }
                    // tell get() we got some data
                    if (!p->m_got_data.set()) {
                        p->note_thread_error("ThreadBuffer::reader_thread:  unable to set m_got_data event", GetLastError(), "");
                        break;
                    }
                }
                // pipe broken - quit thread, which will be seen by get() as eof.
                if (read_status == ERROR_BROKEN_PIPE) {
                    break;
                }
            }
        }
    }
    catch (...) {
        // might only be std::bad_alloc
        p->note_thread_error("", ERROR_SUCCESS, "ThreadBuffer::reader_thread: unknown exception caught");
    }

    delete[] read_buffer;

    // ensure that get() is not left waiting on got_data
    p->m_got_data.set();
    return 0;
}

void ThreadBuffer::put(char * const src, std::size_t & size, bool & no_more)
{
    if (m_direction != dir_write) {
        abort();
    }
    // check thread status
    DWORD thread_exit_code;
    if (!GetExitCodeThread(m_thread, &thread_exit_code)) {
        abort();
    }

    if (thread_exit_code != STILL_ACTIVE) {
        // thread terminated - check for errors
        check_error(m_message_prefix, m_error_code, m_error_message);
        // if terminated without error  - signal eof, since no one will ever write our data
        size = 0;
        no_more = true;
    }
    else {
        // wait for both m_want_data and m_mutex
        WaitResult wait_result = wait(m_want_data, m_wait_timeout);
        if (!wait_result.ok()) {
            check_error("ThreadBuffer::put: wait for want_data failed", wait_result.error_code(), wait_result.error_message());
        }
        GrabMutex grab_mutex(m_mutex, m_wait_timeout);
        if (!grab_mutex.ok()) {
            check_error("ThreadBuffer::put: wait for mutex failed", grab_mutex.error_code(), grab_mutex.error_message());
        }

        // got them - put data
        no_more = false;
        if (m_translate_crlf) {
            m_buffer_list.put_translate_crlf(src, size);
        }
        else {
            m_buffer_list.put(src, size);
        }

        // if the buffer is too long - make the next put() wait until it shrinks
        if (m_buffer_list.full(m_buffer_limit)) {
            if (!m_want_data.reset()) {
                abort();
            }
        }
        // tell the thread we got data
        if (!m_buffer_list.empty()) {
            if (!m_got_data.set()) {
                abort();
            }
        }
    }
}

DWORD WINAPI ThreadBuffer::writer_thread(LPVOID param)
{
    ThreadBuffer * p = static_cast<ThreadBuffer *>(param);
    // accessing p anywhere here is safe because ThreadBuffer destructor 
    // ensures the thread is terminated before p get destroyed
    try {
        BufferList::Buffer buffer;
        buffer.data = 0;
        buffer.size = 0;
        std::size_t buffer_offset = 0;

        while (true) {
            // wait for got_data or destruction, ignore timeout errors 
            // for destruction the timeout is normally expected,
            // for got data the timeout is not normally expected but tolerable (no one wants to write)
            WaitResult wait_result = wait(p->m_got_data, p->m_stop_thread, p->m_wait_timeout);

            if (!wait_result.ok() && !wait_result.timed_out()) {
                p->note_thread_error("ThreadBuffer::writer_thread: wait for got_data, destruction failed", wait_result.error_code(), wait_result.error_message());
                break;
            }

            // if no data in local buffer to write - get from p->m_buffers
            if (buffer.data == 0 && wait_result.is_signaled(p->m_got_data)) {
                GrabMutex grab_mutex(p->m_mutex, p->m_wait_timeout);
                if (!grab_mutex.ok()) {
                    p->note_thread_error("ThreadBuffer::writer_thread: wait for mutex failed", grab_mutex.error_code(), grab_mutex.error_message());
                    break;
                }
                if (!p->m_buffer_list.empty()) {
                    // we've got buffer - detach it
                    buffer = p->m_buffer_list.detach();
                    buffer_offset = 0;
                }
                // if no data left in p->m_buffers - wait until it arrives
                if (p->m_buffer_list.empty()) {
                    if (!p->m_got_data.reset()) {
                        p->note_thread_error("ThreadBuffer::writer_thread: unable to reset m_got_data event", GetLastError(), "");
                        break;
                    }
                }
                // if buffer is not too long - tell put() it can proceed
                if (!p->m_buffer_list.full(p->m_buffer_limit)) {
                    if (!p->m_want_data.set()) {
                        p->note_thread_error("ThreadBuffer::writer_thread: unable to set m_want_data event", GetLastError(), "");
                        break;
                    }
                }
            }

            // see if they want us to stop, but only when all is written
            if (buffer.data == 0 && wait_result.is_signaled(p->m_stop_thread)) {
                break;
            }

            if (buffer.data != 0) {
                // we have buffer - write it
                DWORD written_size;
                if (!WriteFile(p->m_pipe, buffer.data + buffer_offset, buffer.size - buffer_offset, &written_size, 0)) {
                    p->note_thread_error("ThreadBuffer::writer_thread: WriteFile failed", GetLastError(), "");
                    break;
                }
                buffer_offset += written_size;
                if (buffer_offset == buffer.size) {
                    delete[] buffer.data;
                    buffer.data = 0;
                }
            }

        }

        // we won't be writing any more - close child's stdin
        CloseHandle(p->m_pipe);

        // buffer may be left astray - clean up
        delete[] buffer.data;

    }
    catch (...) {
        // unreachable code. really.
        p->note_thread_error("", ERROR_SUCCESS, "ThreadBuffer::writer_thread: unknown exception caught");
    }
    // ensure that put() is not left waiting on m_want_data
    p->m_want_data.set();
    return 0;
}

void ThreadBuffer::check_error(std::string const & message_prefix, DWORD error_code, std::string const & error_message)
{
    if (!error_message.empty()) {
        abort();
    }
    else if (error_code != ERROR_SUCCESS) {
        abort();
    }
}

void ThreadBuffer::note_thread_error(char const * message_prefix, DWORD error_code, char const * error_message)
{
    m_message_prefix = message_prefix;
    m_error_code = error_code;
    m_error_message = error_message;
}

