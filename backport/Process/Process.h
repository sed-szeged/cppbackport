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

#ifndef exec_stream_h
#define exec_stream_h

#include <string>
#include <exception>
#include <istream>
#include <ostream>
#include <vector>

class Process {
public:
    Process();
    Process( std::string const & program, std::string const & arguments );
    template< class iterator > Process( std::string const & program, iterator args_begin, iterator args_end );
    
    ~Process();

    enum stream_kind_t { s_in=1, s_out=2, s_err=4, s_all=s_in|s_out|s_err, s_child=8  };

    void set_buffer_limit( int stream_kind, std::size_t size );
    
    typedef unsigned long timeout_t;
    void set_wait_timeout( int stream_kind, timeout_t milliseconds );
    
    void set_binary_mode( int stream_kind );
    void set_text_mode( int stream_kind );

    bool start( std::string const & program, std::string const & arguments );
    template< class iterator > bool start( std::string const & program, iterator args_begin, iterator args_end );
    bool start( std::string const & program, char const * arg1, char const * arg2 ); // to compensate for damage from the previous one
    bool start( std::string const & program, char * arg1, char * arg2 );
    
    bool close_in();
    bool close();
    bool kill();
    void wait();
    void wait(timeout_t timeout);
    bool isDone();
    int exit_code();
    bool isUnstarted();

    std::ostream & in();
    std::istream & out();
    std::istream & err();

    typedef unsigned long error_code_t;

private:
    Process( Process const & );
    Process & operator=( Process const & );

    struct Impl;
    friend struct impl_t;
    Impl * m_impl;
    bool unstarted;

    void exceptions( bool enable );

// helpers for template member functions
    void new_impl();
   
    class NextArg {
    public:
        virtual ~NextArg()
        {
        }
        
        virtual std::string const * next()=0;
    };
    
    template< class iterator > class NextArgImpl : public NextArg {
    public:
        NextArgImpl(iterator args_begin, iterator args_end)
        : m_args_i( args_begin ), m_args_end( args_end )
        {
        }

        virtual std::string const * next()
        {
            if( m_args_i==m_args_end ) {
                return 0;
            }else {
                m_arg=*m_args_i;
                ++m_args_i;
                return &m_arg;
            }
        }

    private:
        iterator m_args_i;
        iterator m_args_end;
        std::string m_arg;
   };
   
   bool start(std::string const & program, NextArg & next_arg);
};

template< class iterator > inline Process::Process( std::string const & program, iterator args_begin, iterator args_end )
{
    new_impl();
    exceptions( true );
    start( program, args_begin, args_end );
    unstarted = false;
}

template< class iterator > inline bool Process::start( std::string const & program, iterator args_begin, iterator args_end )
{
    Process::NextArgImpl< iterator > next_arg( args_begin, args_end );
    return start( program, next_arg );
}

inline bool Process::start( std::string const & program, char const * arg1, char const * arg2 )
{
    std::vector< std::string > args;
    args.push_back( std::string( arg1 ) );
    args.push_back( std::string( arg2 ) );
    return start( program, args.begin(), args.end() );
}

inline bool Process::start( std::string const & program, char * arg1, char * arg2 )
{
    std::vector< std::string > args;
    args.push_back( std::string( arg1 ) );
    args.push_back( std::string( arg2 ) );
    return start( program, args.begin(), args.end() );
}

#endif
