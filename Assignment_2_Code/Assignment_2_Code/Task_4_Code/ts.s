/********************************************************************
Copyright 2010-2017 K.C. Wang, <kwang@eecs.wsu.edu>
This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
********************************************************************/

.text
.code 32

.global reset
.global vectors_start, vectors_end
.global int_off, int_on
.global lock, unlock
.global getsr
.set vectorAddr, 0x10140030	
reset:	
vectors_start:

  LDR PC, reset_handler_addr
  LDR PC, undef_handler_addr
  LDR PC, swi_handler_addr
  LDR PC, prefetch_abort_handler_addr
  LDR PC, data_abort_handler_addr
  B .
  LDR PC, irq_handler_addr
  LDR PC, fiq_handler_addr

reset_handler_addr:          .word reset_handler
undef_handler_addr:          .word undef_handler
swi_handler_addr:            .word swi_handler
prefetch_abort_handler_addr: .word prefetch_abort_handler
data_abort_handler_addr:     .word data_abort_handler
irq_handler_addr:            .word irq_handler
fiq_handler_addr:            .word fiq_handler

vectors_end:

reset_handler:
  LDR sp, =svc_stack
  BL copy_vectors
	
  /* go in IRQ mode, set IRQ stack */
  MSR cpsr, #0x92
  LDR sp, =irq_stack
	
  /* go in SYS mode, set SYS stack */
  MSR cpsr, #0x9F
  LDR sp, =sys_stack

  /* go back in SVC mode with IRQ on */
  msr cpsr, #0x13
  BL main
  B .

.align 4
irq_handler:
//  sub	lr, lr, #4

  stmfd	sp!, {r12, r14} // save in IRQ mode stack

  mrs   r12, spsr       // copy spsr into r12
  stmfd sp!, {r12}      // save spsr in IRQ stack

  msr cpsr, #0x93   // to SVC mode with IRQ disabled
  stmfd sp!, {r0-r3, r12, lr}  // save context in SVC stack

// read vectoraddress register: MUST!!! else no interrupts
    ldr  r1, =vectorAddr
    ldr  r0, [r1]  // read vectorAddr register to ACK interrupt

  bl enterint

  msr cpsr, #0x13     // still in SVC mode but enable IRQ

  bl  irq_chandler     // handle IRQ in SVC mode 

  ldmfd sp!, {r0-r3, r12, lr}  // restore context from SVC stack

  msr cpsr, #0x92     // to IRQ mode with IRQ disabled	
	
  ldmfd sp!, {r12}    // get spsr
  msr   spsr, r12     // restore spsr

  bl exitint

// write to vector address register as EOI
//  ldr  r1, =vectorAddr
//  str  r0, [r1]
	
  ldmfd sp!, {r12, r14}
  subs pc, r14, #4
	
// int_on()/int_off(): turn on/off IRQ interrupts
int_on: // may pass parameter in r0
unlock:	
  MRS r4, cpsr
  BIC r4, r4, #0x80   // clear bit means UNMASK IRQ interrupt
  MSR cpsr, r4
  mov pc, lr	

int_off: // may pass parameter in r0
lock:	
  MRS r4, cpsr
  ORR r4, r4, #0x80   // set bit means MASK off IRQ interrupt 
  MSR cpsr, r4
  mov pc, lr	

getsr:
	mrs r0, cpsr
	mov pc,lr
.end
