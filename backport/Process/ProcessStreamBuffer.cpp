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

#include "ProcessStreamBuffer.h"

ProcessStreamBuffer::ProcessStreamBuffer( Process::stream_kind_t kind, ThreadBuffer & thread_buffer )
: m_kind( kind ), m_thread_buffer( thread_buffer )
{
    m_stream_buffer=new char[STREAM_BUFFER_SIZE];
    clear();
}

ProcessStreamBuffer::~ProcessStreamBuffer()
{
    delete[] m_stream_buffer;
}

void ProcessStreamBuffer::clear()
{
    if( m_kind==Process::s_in ) {
        setp( m_stream_buffer, m_stream_buffer+STREAM_BUFFER_SIZE );
    }else {
        setg( m_stream_buffer, m_stream_buffer+STREAM_BUFFER_SIZE, m_stream_buffer+STREAM_BUFFER_SIZE );
    }
}

ProcessStreamBuffer::int_type ProcessStreamBuffer::underflow()
{
    if( gptr()==egptr() ) {
        std::size_t read_size=STREAM_BUFFER_SIZE;
        bool no_more;
        m_thread_buffer.get( m_kind, m_stream_buffer, read_size, no_more );
        if( no_more || read_size==0 ) { // there is no way for underflow to return something other than eof when 0 bytes are read
            return traits_type::eof();
        }else {
            setg( m_stream_buffer, m_stream_buffer, m_stream_buffer+read_size );
        }
    }
    return traits_type::to_int_type( *eback() );
}

bool ProcessStreamBuffer::send_buffer()
{
    if( pbase()!=pptr() ) {
        std::size_t write_size=pptr()-pbase();
        std::size_t n=write_size;
        bool no_more;
        m_thread_buffer.put( pbase(), n, no_more );
        if( no_more || n!=write_size ) {
            return false;
        }else {
            setp( m_stream_buffer, m_stream_buffer+STREAM_BUFFER_SIZE );
        }
    }
    return true;
}

bool ProcessStreamBuffer::send_char( char c ) 
{
    std::size_t write_size=1;
    bool no_more;
    m_thread_buffer.put( &c, write_size, no_more );
    return write_size==1 && !no_more;
}

ProcessStreamBuffer::int_type ProcessStreamBuffer::overflow( ProcessStreamBuffer::int_type c )
{
    if( !send_buffer() ) {
        return traits_type::eof();
    }
    if( c!=traits_type::eof() ) {
        if( pbase()==epptr() ) {
            if( !send_char( c ) ) {
                return traits_type::eof();
            }
        }else {
            sputc( c );
        }
    }
    return traits_type::not_eof( c );
}

int ProcessStreamBuffer::sync()
{
    if( !send_buffer() ) {
        return -1;
    }
    return 0;
}
