/*  GRAPHITENG LICENSING

    Copyright 2010, SIL International
    All rights reserved.

    This library is free software; you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published
    by the Free Software Foundation; either version 2.1 of License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should also have received a copy of the GNU Lesser General Public
    License along with this library in the file named "LICENSE".
    If not, write to the Free Software Foundation, Inc., 59 Temple Place, 
    Suite 330, Boston, MA 02111-1307, USA or visit their web page on the 
    internet at http://www.fsf.org/licenses/lgpl.html.
*/
// This call threaded interpreter implmentation for machine.h
// Author: Tim Eves

// Build either this interpreter or the direct_machine implementation.
// The call threaded interpreter is portable across compilers and 
// architectures as well as being useful to debug (you can set breakpoints on 
// opcodes) but is slower that the direct threaded interpreter by a factor of 2

#include <cassert>
#include <graphiteng/SlotHandle.h>
#include "Machine.h"
#include "GrSegment.h"
#include "XmlTraceLog.h"
#include "Slot.h"

#define registers           const byte * & dp, vm::Machine::stack_t * & sp, vm::Machine::stack_t * const sb,\
                            regbank & reg

// These are required by opcodes.h and should not be changed
#define STARTOP(name)	    bool name(registers) REGPARM(4);\
			                bool name(registers) { \
                                STARTTRACE(name,is);
#define ENDOP		            ENDTRACE; \
                                return (sp - sb)/Machine::STACK_MAX==0; \
                            }

#define EXIT(status)        push(status); ENDTRACE return false

// This is required by opcode_table.h
#define do_(name)           instr(name)


using namespace vm;
    using namespace org::sil::graphite::v2;

struct regbank  {
    GrSegment       & seg;
    slotref         is;
    const instr * & ip;
    int           & count;
    int             isb;
    int             maxmap;
    slotref *       map;        // given slot index from start of rule give slot to *read* from
    int8            flags;
};

typedef bool        (* ip_t)(registers);

// Pull in the opcode definitions
// We pull these into a private namespace so these otherwise common names dont
// pollute the toplevel namespace.
namespace {
#define seg     reg.seg
#define is      reg.is
#define ip      reg.ip
#define count   reg.count
#define isb     reg.isb
#define maxmap  reg.maxmap
#define map     reg.map
#define flags   reg.flags

#include "opcodes.h"

#undef seg
#undef is
#undef ip
#undef count
#undef isb
#undef maxmap
#undef map
#undef flags
}

Machine::stack_t  Machine::run(const instr   * program,
                               const byte    * data,
                               GrSegment &     seg,
                               slotref &       islot_idx,
                               int &           count,
                               int             nPre,
                               status_t &      status,
                               int             nMap,
                               slotref *       map,
                               int &           flags)
{
    assert(program != 0);

    // Declare virtual machine registers
    const instr   * ip = program-1;
    const byte    * dp = data;
    stack_t       * sp      = _stack + Machine::STACK_GUARD,
            * const sb = sp;
    regbank         reg = {seg, islot_idx, ip, count, nPre, nMap, map, 0};

    // Run the program        
    while ((reinterpret_cast<ip_t>(*++ip))(dp, sp, sb, reg)) {}
    const stack_t ret = sp == _stack+STACK_GUARD+1 ? *sp-- : 0;

    check_final_stack(sp, status);
    islot_idx = reg.is;
    flags = reg.flags;
    return ret;
}

// Pull in the opcode table
namespace {
#include "opcode_table.h"
}

const opcode_t * Machine::getOpcodeTable() throw()
{
    return opcode_table;
}


