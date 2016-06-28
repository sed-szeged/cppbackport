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

#include "Process.h"

#include <list>
#include <vector>
#include <algorithm>
#include <exception>

#include "PlatformDependentIncludes.h"

#include "BufferList.h"            // helper classes
#include "ProcessHelpers.h"        // platform-dependent helpers
#include "ProcessStreamBuffer.h"   // stream buffer class
#include "ProcessIOStream.h"       // stream classes
#include "ProcessHelpersImpl.inc"  // platform-dependent implementation


//platform-independent Process member functions
Process::Process()
{
    m_impl = new Impl;
    exceptions(true);
    unstarted = true;
}

Process::Process(std::string const & program, std::string const & arguments)
{
    m_impl = new Impl;
    exceptions(true);
    start(program, arguments);
    unstarted = false;
}

void Process::new_impl()
{
    m_impl = new Impl;
}

bool Process::isUnstarted() {
    return unstarted;
}

Process::~Process()
{
    close();

    delete m_impl;
}

std::ostream & Process::in()
{
    return m_impl->m_in;
}

std::istream & Process::out()
{
    return m_impl->m_out;
}

std::istream & Process::err()
{
    return m_impl->m_err;
}

void Process::exceptions(bool enable)
{
    if (enable) {
        // getline sets failbit on eof, so we should enable badbit and badbit _only_ to propagate our exceptions through iostream code.
        m_impl->m_in.exceptions(std::ios_base::badbit);
        m_impl->m_out.exceptions(std::ios_base::badbit);
        m_impl->m_err.exceptions(std::ios_base::badbit);
    }
    else {
        m_impl->m_in.exceptions(std::ios_base::goodbit);
        m_impl->m_out.exceptions(std::ios_base::goodbit);
        m_impl->m_err.exceptions(std::ios_base::goodbit);
    }
}
