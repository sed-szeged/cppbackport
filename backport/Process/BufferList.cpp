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

#include <algorithm>
#include "BufferList.h"
#include "PlatformDependentIncludes.h"
#include "ProcessHelpers.h"

BufferList::BufferList()
{
    m_total_size=0;
    m_read_offset=0;
}

BufferList::~BufferList()
{
    clear();
}

void BufferList::get( char * dst, std::size_t & size )
{
    std::size_t written_size=0;
    while( size>0 && m_total_size>0 ) {
        std::size_t portion_size=std::min( size, m_buffers.front().size-m_read_offset );
        std::char_traits< char >::copy( dst, m_buffers.front().data+m_read_offset, portion_size );
        dst+=portion_size;
        size-=portion_size;
        m_total_size-=portion_size;
        m_read_offset+=portion_size;
        written_size+=portion_size;
        if( m_read_offset==m_buffers.front().size ) {
            delete[] m_buffers.front().data;
            m_buffers.pop_front();
            m_read_offset=0;
        }
    }
    size=written_size;
}

void BufferList::get_translate_crlf( char * dst, std::size_t & size )
{
    std::size_t written_size=0;
    while( written_size!=size && m_total_size>0 ) {
        while( written_size!=size && m_read_offset!=m_buffers.front().size ) {
            char c=m_buffers.front().data[m_read_offset];
            if( c!='\r' ) {  // MISFEATURE: single \r in the buffer will cause end of file
                *dst++=c;
                ++written_size;
            }
            --m_total_size;
            ++m_read_offset;
        }
        if( m_read_offset==m_buffers.front().size ) {
            delete[] m_buffers.front().data;
            m_buffers.pop_front();
            m_read_offset=0;
        }
    }
    size=written_size;
}

void BufferList::put( char * const src, std::size_t size )
{
    Buffer buffer;
    buffer.data=new char[size];
    buffer.size=size;
    std::char_traits< char >::copy( buffer.data, src, size );
    m_buffers.push_back( buffer );
    m_total_size+=buffer.size;
}

void BufferList::put_translate_crlf( char * const src, std::size_t size )
{
    char const * p=src;
    std::size_t lf_count=0;
    while( p!=src+size ) {
        if( *p=='\n' ) {
            ++lf_count;
        }
        ++p;
    }
    Buffer buffer;
    buffer.data=new char[size+lf_count];
    buffer.size=size+lf_count;
    p=src;
    char * dst=buffer.data;
    while( p!=src+size ) {
        if( *p=='\n' ) {
            *dst++='\r';
        }
        *dst++=*p;
        ++p;
    }
    m_buffers.push_back( buffer );
    m_total_size+=buffer.size;
}

BufferList::Buffer BufferList::detach()
{
    Buffer buffer=m_buffers.front();
    m_buffers.pop_front();
    m_total_size-=buffer.size;
    return buffer;
}

bool BufferList::empty()
{
    return m_total_size==0;
}

bool BufferList::full( std::size_t limit )
{
    return limit!=0 && m_total_size>=limit;
}

void BufferList::clear()
{
    for( buffers_t::iterator i=m_buffers.begin(); i!=m_buffers.end(); ++i ) {
        delete[] i->data;
    }
    m_buffers.clear();
    m_read_offset=0;
    m_total_size=0;
}
