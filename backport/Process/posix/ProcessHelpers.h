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

#ifndef PROCESS_HELPERS_H
#define PROCESS_HELPERS_H

#include <cstddef>
#include <list>
#include <ostream>
#include "BufferList.h"
#include "Process.h"

template< class T > class Buffer {
public:
    typedef T data_t;

    Buffer()
    {
        m_buf=0;
        m_size=0;
    }
    
    ~Buffer()
    {
        delete [] m_buf;
    }

    data_t * new_data( std::size_t size )
    {
        m_buf=new T[size];
        m_size=size;
        return m_buf;
    }

    void append_data( data_t const * data, std::size_t size )
    {
        Buffer new_buf;
        new_buf.new_data( m_size+size );
        std::char_traits< data_t >::copy( new_buf.m_buf, m_buf, m_size );
        std::char_traits< data_t >::copy( new_buf.m_buf+m_size, data, size );
        std::swap( this->m_buf, new_buf.m_buf );
        std::swap( this->m_size, new_buf.m_size );
    }
        
    data_t * data()
    {
        return m_buf;
    }

    std::size_t size()
    {
        return m_size;
    }
    
private:
    Buffer( Buffer const & );
    Buffer & operator=( Buffer const & );

    data_t * m_buf;
    std::size_t m_size;
};


class Pipe {
public:
    Pipe();
    ~Pipe();
    int r() const;
    int w() const;
    void close_r();
    void close_w();
    void close();
    void open();
private:
    enum direction_t{ closed, read, write, both };
    direction_t m_direction;
    int m_fds[2];
};


class Mutex {
public:
    Mutex();
    ~Mutex();
    
private:
    pthread_mutex_t m_mutex;

    friend class Event;
    friend class GrabMutex;
};


class GrabMutex {
public:
    GrabMutex( Mutex & mutex, class MutexRegistrator * mutex_registrator );
    ~GrabMutex();
    
    int release();
    bool ok();
    int error_code();
    
private:
    pthread_mutex_t * m_mutex;
    int m_error_code;
    bool m_grabbed;
    class MutexRegistrator * m_mutex_registrator;

    friend class MutexRegistrator;
};

class MutexRegistrator {
public:
    ~MutexRegistrator();
    void add( GrabMutex * g );
    void remove( GrabMutex * g );
    void release_all();
private:
    typedef std::list< GrabMutex * > mutexes_t;
    mutexes_t m_mutexes;
};


class WaitResult {
public:
    WaitResult( unsigned signaled_state, int error_code, bool timed_out );

    bool ok();
    bool is_signaled( int state );
    int error_code();
    bool timed_out();

private:
    unsigned m_signaled_state;
    int m_error_code;
    bool m_timed_out;
};


class Event {
public:
    Event();
    ~Event();

    int set( unsigned bits, MutexRegistrator * mutex_registrator );
    int reset( unsigned bits, MutexRegistrator * mutex_registrator );
    
    WaitResult wait( unsigned any_bits, unsigned long timeout, MutexRegistrator * mutex_registrator );

private:
    Mutex m_mutex;
    pthread_cond_t m_cond;
    unsigned volatile m_state;    
};


class ThreadBuffer {
public:
    ThreadBuffer( Pipe & in_pipe, Pipe & out_pipe, Pipe & err_pipe, std::ostream & in );
    ~ThreadBuffer();

    void set_wait_timeout( int stream_kind, unsigned long milliseconds );
    void set_buffer_limit( int stream_kind, std::size_t limit );
    void set_read_buffer_size( int stream_kind, std::size_t size );

    void start();

    void get( Process::stream_kind_t kind, char * dst, std::size_t & size, bool & no_more );
    void put( char * src, std::size_t & size, bool & no_more );

    void close_in();
    bool stop_thread();
    bool abort_thread();

private:
    static void * thread_func( void * param );

    pthread_t m_thread;    
    Mutex m_mutex; // protecting m_in_buffer, m_out_buffer, m_err_buffer
    
    BufferList m_in_buffer;
    BufferList m_out_buffer;
    BufferList m_err_buffer;

    Event m_thread_control;  // s_in : in got_data;  s_out: out want data; s_err: err want data; s_child: stop thread
    Event m_thread_responce; // s_in : in want data; s_out: out got data;  s_err: err got data;  s_child: thread stopped

    char const * m_error_prefix;
    int m_error_code;

    bool m_thread_started; // set in start(), checked in set_xxx(), get() and put()
    bool m_in_closed; // set in close_in(), checked in put()
    
    Pipe & m_in_pipe;
    Pipe & m_out_pipe;
    Pipe & m_err_pipe;
    
    unsigned long m_in_wait_timeout;
    unsigned long m_out_wait_timeout;
    unsigned long m_err_wait_timeout;
    
    unsigned long m_thread_termination_timeout;
    
    std::size_t m_in_buffer_limit;
    std::size_t m_out_buffer_limit;
    std::size_t m_err_buffer_limit;

    std::size_t m_out_read_buffer_size;
    std::size_t m_err_read_buffer_size;

    // workaround for not-quite-conformant libstdc++ (see put())
    std::ostream & m_in;
    bool m_in_bad;
};

#endif
