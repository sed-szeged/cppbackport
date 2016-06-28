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

#ifndef PROCESS_STREAM_BUFFER_H
#define PROCESS_STREAM_BUFFER_H

#include <streambuf>

#include "Process.h"
#include "ProcessHelpers.h"

static const std::size_t STREAM_BUFFER_SIZE = 4096;

class ProcessStreamBuffer : public std::streambuf {
public:
    ProcessStreamBuffer( Process::stream_kind_t kind, ThreadBuffer & thread_buffer );
    virtual ~ProcessStreamBuffer();

    void clear();

protected:
    virtual int_type underflow();
    virtual int_type overflow( int_type c );
    virtual int sync();

private:
    bool send_buffer();
    bool send_char( char c );

    Process::stream_kind_t m_kind;
    ThreadBuffer & m_thread_buffer;
    char * m_stream_buffer;
};

#endif
