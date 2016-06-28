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

// Process::Impl
struct Process::Impl {
    Impl();

    HANDLE m_child_process;

    HANDLE m_in_pipe;
    HANDLE m_out_pipe;
    HANDLE m_err_pipe;

    ThreadBuffer m_in_thread;
    ThreadBuffer m_out_thread;
    ThreadBuffer m_err_thread;

    ProcessStreamBuffer m_in_buffer;
    ProcessStreamBuffer m_out_buffer;
    ProcessStreamBuffer m_err_buffer;

    ProcessOStream m_in;
    ProcessIStream m_out;
    ProcessIStream m_err;

    DWORD m_child_timeout;
    int m_exit_code;
};

Process::Impl::Impl()
: m_in_buffer( Process::s_in, m_in_thread ), m_out_buffer( Process::s_out, m_out_thread ), m_err_buffer( Process::s_err, m_err_thread ),
  m_in( m_in_buffer ), m_out( m_out_buffer ), m_err( m_err_buffer ) 
{
    m_out.tie( &m_in );
    m_err.tie( &m_in );
    m_child_process=0;
    m_in_pipe=0;
    m_out_pipe=0;
    m_err_pipe=0;
    m_child_timeout=500;
    m_exit_code=0;
}


void Process::set_buffer_limit( int stream_kind, std::size_t size )
{
    if( stream_kind&s_in ) {
        m_impl->m_in_thread.set_buffer_limit( size );
    }
    if( stream_kind&s_out ) {
        m_impl->m_out_thread.set_buffer_limit( size );
    }
    if( stream_kind&s_err ) {
        m_impl->m_err_thread.set_buffer_limit( size );
    }
}

void Process::set_wait_timeout( int stream_kind, Process::timeout_t milliseconds )
{
    if( stream_kind&s_in ) {
        m_impl->m_in_thread.set_wait_timeout( milliseconds );
    }
    if( stream_kind&s_out ) {
        m_impl->m_out_thread.set_wait_timeout( milliseconds );
    }
    if( stream_kind&s_err ) {
        m_impl->m_err_thread.set_wait_timeout( milliseconds );
    }
    if( stream_kind&s_child ) {
        m_impl->m_child_timeout=milliseconds;
        m_impl->m_in_thread.set_thread_termination_timeout( milliseconds );
        m_impl->m_out_thread.set_thread_termination_timeout( milliseconds );
        m_impl->m_err_thread.set_thread_termination_timeout( milliseconds );
    }
}

void Process::set_binary_mode( int stream_kind )
{
    if( stream_kind&s_in ) {
        m_impl->m_in_thread.set_binary_mode();
    }
    if( stream_kind&s_out ) {
        m_impl->m_out_thread.set_binary_mode();
    }
    if( stream_kind&s_err ) {
        m_impl->m_err_thread.set_binary_mode();
    }
}

void Process::set_text_mode( int stream_kind )
{
    if( stream_kind&s_in ) {
        m_impl->m_in_thread.set_text_mode();
    }
    if( stream_kind&s_out ) {
        m_impl->m_out_thread.set_text_mode();
    }
    if( stream_kind&s_err ) {
        m_impl->m_err_thread.set_text_mode();
    }
}

bool Process::start( std::string const & program, std::string const & arguments )
{
    if( !close() ) {
        return false;
    }

    Pipe in;
    Pipe out;
    Pipe err;
    SetSTDHandle set_in(STD_INPUT_HANDLE, in.r());
    SetSTDHandle set_out(STD_OUTPUT_HANDLE, out.w());
    SetSTDHandle set_err(STD_ERROR_HANDLE, err.w());
    HANDLE cp=GetCurrentProcess();
    if( !DuplicateHandle( cp, in.w(), cp, &m_impl->m_in_pipe, 0, FALSE, DUPLICATE_SAME_ACCESS ) ) {
        return false;
    }
    in.close_w();
    if( !DuplicateHandle( cp, out.r(), cp, &m_impl->m_out_pipe, 0, FALSE, DUPLICATE_SAME_ACCESS ) ) {
        return false;
    }
    out.close_r();
    if( !DuplicateHandle( cp, err.r(), cp, &m_impl->m_err_pipe, 0, FALSE, DUPLICATE_SAME_ACCESS ) ) {
        return false;
    }
    err.close_r();

    std::string command;
    command.reserve( program.size()+arguments.size()+3 );
    if( program.find_first_of( " \t" )!=std::string::npos ) {
        command+='"';
        command+=program;
        command+='"';
    }else
        command=program;
    if( arguments.size()!=0 ) {
        command+=' ';
        command+=arguments;
    }
    STARTUPINFO si;
    ZeroMemory( &si, sizeof( si ) );
    si.cb=sizeof( si );
    PROCESS_INFORMATION pi;
    ZeroMemory( &pi, sizeof( pi ) );
    if( !CreateProcess( 0, const_cast< char * >( command.c_str() ), 0, 0, TRUE, 0, 0, 0, &si, &pi ) ) {
        return false;
    }

    m_impl->m_child_process=pi.hProcess;
    unstarted = false;
    
    m_impl->m_in_buffer.clear();
    m_impl->m_out_buffer.clear();
    m_impl->m_err_buffer.clear();

    m_impl->m_in.clear();
    m_impl->m_out.clear();
    m_impl->m_err.clear();

    m_impl->m_out_thread.set_read_buffer_size( STREAM_BUFFER_SIZE );
    m_impl->m_out_thread.start_reader_thread( m_impl->m_out_pipe );

    m_impl->m_err_thread.set_read_buffer_size( STREAM_BUFFER_SIZE );
    m_impl->m_err_thread.start_reader_thread( m_impl->m_err_pipe );

    m_impl->m_in_thread.start_writer_thread( m_impl->m_in_pipe );

    return true;
}

bool Process::start( std::string const & program, Process::NextArg & next_arg )
{
    std::string arguments;
    while( std::string const * arg=next_arg.next() ) {
        if( arg->find_first_of( " \t\"" )!=std::string::npos ) {
            arguments+=" \"";
            std::string::size_type cur=0;
            while( cur<arg->size() ) {
                std::string::size_type next=arg->find( '"', cur );
                if( next==std::string::npos ) {
                    next=arg->size();
                    arguments.append( *arg, cur, next-cur );
                    cur=next;
                }else {
                    arguments.append( *arg, cur, next-cur );
                    arguments+="\\\"";
                    cur=next+1;
                }
            }
            arguments+="\"";
        }else {
            arguments+=" "+*arg;
        }
    }
    return start( program, arguments );
}

bool Process::close_in()
{
    if( m_impl->m_in_pipe!=0 ) {
        m_impl->m_in.flush();
        // stop writer thread before closing the handle it writes to,
        // the thread will attempt to write anything it can and close child's stdin
        // before thread_termination_timeout elapses
        if( m_impl->m_in_thread.stop_thread() ) {
            m_impl->m_in_pipe=0;
            return true;
        }else {
            return false;
        }
    }else {
        return true;
    }
}

bool Process::close()
{
    if( !close_in() ) {
        // need to close child's stdin no matter what, because otherwise "usual" child  will run forever
        // And before closing child's stdin the writer thread should be stopped no matter what,
        // because it may be blocked on Write to m_in_pipe, and in that case closing m_in_pipe may block. 
        if( !m_impl->m_in_thread.abort_thread() ) {
            return false;
        }
        // when thread is terminated abnormally, it may left child's stdin open
        // try to close it here 
        CloseHandle( m_impl->m_in_pipe );
        m_impl->m_in_pipe=0;
    }
    if( !m_impl->m_out_thread.stop_thread() ) {
        if( !m_impl->m_out_thread.abort_thread() ) {
            return false;
        }
    }
    if( !m_impl->m_err_thread.stop_thread() ) {
        if( !m_impl->m_err_thread.abort_thread() ) {
            return false;
        }
    }
    if( m_impl->m_out_pipe!=0 ) {
        if( !CloseHandle( m_impl->m_out_pipe ) ) {
            return false;
        }
        m_impl->m_out_pipe=0;
    }
    if( m_impl->m_err_pipe!=0 ) {
        if( !CloseHandle( m_impl->m_err_pipe ) ) {
            return false;
        }
        m_impl->m_err_pipe=0;
    }
    if( m_impl->m_child_process!=0 ) {
        WaitResult wait_result= ::wait( m_impl->m_child_process, m_impl->m_child_timeout );
        if( !wait_result.ok() & !wait_result.timed_out() ) {
            return false;
        }
        if( wait_result.ok() ) {
            DWORD exit_code;
            if( !GetExitCodeProcess( m_impl->m_child_process, &exit_code ) ) {
                return false;
            }
            m_impl->m_exit_code=exit_code;
            if( !CloseHandle( m_impl->m_child_process ) ) {
                return false;
            }
            m_impl->m_child_process=0;
        }
    }
    unstarted = true;
    return m_impl->m_child_process==0;
}

bool Process::kill() 
{
    if( m_impl->m_child_process!=0 ) {
        if( !TerminateProcess( m_impl->m_child_process, 0 ) ) {
            return false;
        }
        m_impl->m_exit_code=0;
        if( !CloseHandle( m_impl->m_child_process ) ) {
            return false;
        }
        m_impl->m_child_process=0;
    }

    return true;
}

int Process::exit_code()
{
    if( m_impl->m_child_process!=0 ) {
        if (isDone() == false)
            abort();
    }
    return m_impl->m_exit_code;
}

void Process::wait() {
    ::wait(this->m_impl->m_child_process, 0);
}

void Process::wait(timeout_t timeout) {
    ::wait(this->m_impl->m_child_process, timeout);
}

bool Process::isDone() {
    if (m_impl->m_child_process == 0)
        return true;

    WaitResult wait_result = ::wait(m_impl->m_child_process, 1);

    return wait_result.timed_out() == false;
}
