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
#include "type.h"

char *tab = "0123456789ABCDEF";
int color;

#include "string.c"
#include "uart.c"
#include "kbd.c"
#include "timer.c"
#include "vid.c"
#include "excepts.c"

void copy_vectors(void) {
    extern u32 vectors_start;
    extern u32 vectors_end;
    u32 *vectors_src = &vectors_start;
    u32 *vectors_dst = (u32 *)0;
    while(vectors_src < &vectors_end)
       *vectors_dst++ = *vectors_src++;
}
int kprintf(char *fmt, ...);
void timer_handler();

// use vectored interrupts of PL190
void timer0_handler()
{  
  timer_handler(0);
}
void uart0_handler()
{
  uart_handler(&uart[0]);
}
void uart1_handler()
{
  uart_handler(&uart[1]);
}

int vectorInt_init()
{
  printf("vectorInterrupt_init()\n");
  //printf("t=%x u0=%x u1=%x kbd=%x\n", 
  //	 timer0_handler, uart0_handler, uart1_handler, kbd_handler);
 
  // set up vectored interrupts for (REF: KCW's armHowtoVectorIntPlan file)
  // timer0 at IRQ4, UART0 at IRQ12, UART1 at IRQ13, KBD to IRQ31:

  // (1) write to vectoraddr0 (0x100) with ISR of timer0,
  //              vectoraddr1 (0x104) with ISR of UART0,
  //              vectoraddr2 (0x108) with ISR of UART1,
  //              vectoraddr3 (0x10C) with ISR of KBD
  // all are offsets from VIC base at 0x10140000; (SIC is NOT used at all)
  *((int *)(VIC_BASE+0x100/4)) = (int)timer0_handler; 
  *((int *)(VIC_BASE+0x104/4)) = (int)uart0_handler; 
  *((int *)(VIC_BASE+0x108/4)) = (int)uart1_handler; 
  *((int *)(VIC_BASE+0x10C/4)) = (int)kbd_handler; 
  //(2). write to intControlRegs = E=1|IRQ# =  1xxxxx
  *((int *)(VIC_BASE+0x200/4)) = 0x24;    //100100 at IRQ 4
  *((int *)(VIC_BASE+0x204/4)) = 0x2C;    //101100 at IRQ 12
  *((int *)(VIC_BASE+0x208/4)) = 0x2D;    //101101 at IRQ 13
  *((int *)(VIC_BASE+0x20C/4)) = 0x3F;    //111111 at IRQ 31

  //write 32-bit 0's to IntSelectReg to generate IRQ interrupts (1 for FIQs)
  // *((int *)(VIC_BASE_ADDR+0x0C)) = 0;
}

// Must rewrite irq_handler() to use vectors: HOW:
// IRQ => still comes to irq_handler() => no need to read status registers 
// to determine the interrupt source ==> should get the ISR address directly 
// by reading vectorAddrReg at 0x30 => get ISR address, then invoke it.
// upon return, must write to vectorAddrReg (any value) as EOI

void *oldISR;
volatile int tcount=0;

KBD  *kp;
/**********
int enterint() // clear interrupt source
{
  int scode; 
  void *isr; int sr = getsr();
  oldISR = isr = *((int *)(VIC_BASE_ADDR+0x30)); // read ISR address in vectorAddr     
  // compare ISR with installed ISR 
  if (isr == timer0_handler){
    tcount++;    
    timer_clearInterrupt(0); //kprintf("T1 ");
  }
  if (isr == uart0_handler)
    kprintf("UART0 ");
  if (isr == uart1_handler)
    kprintf("uart1 ");
  if (isr == kbd_handler){
    scode = *(kp->base+KDATA);
    kprintf("eKBD ");
  }
}
***********/
volatile int status;
int enterint() // clear interrupt source
{
  int scode; 

  status = *((int *)(VIC_BASE)); // read status register     
  //  kprintf("status=%x\n", status);
  if (status & 0x10){ // timer0
    tcount++;
  }
  if (status & 0x80000000){ // KBD
     scode = *(kp->base+KDATA);
     kprintf("enterKBD ");
  }
}

/******************
int exitint() // clear interrupt source
{
  //if (oldISR == timer0_handler)
  //   kprintf("T2 ");
  if (oldISR == uart0_handler)
    kprintf("UART0exit ");
  if (oldISR == uart1_handler)
    kprintf("uart1EXIT ");
  if (oldISR == kbd_handler){
     kprintf("xKBD=%d\n", tcount);
    tcount=0;
  }
}
*****************/
int exitint()
{
  //int status = *((int *)(VIC_BASE_ADDR)); // read status register
  if (status & (1<<31)){ // KBD
     kprintf("exitKBD: tcount=%d\n", tcount);
     tcount=0;
  }
}

void irq_chandler(int lr, int pc)
{
  void *(*f)();                         // f is a function pointer
  f =(void *)*((int *)(VIC_BASE + 0x30/4)); // read ISR address in vectorAddr
  (*f)();                                // call the ISR function
  *((int *)(VIC_BASE + 0x30/4)) = 1; // write to VIC vectorAddr reg as EOI
}

int main()
{
   int i; 
   u8 kbdstatus, scode, key;
   char line[128];
   UART *up;

   color = CYAN;
   row = col = 0; 
   fbuf_init();
   kbd_init();
   
   /* enable timer0,1, uart0,1 SIC interrupts */
   *(VIC_BASE + VIC_INTENABLE) |= (1<<4);  // timer0,1 at bit4 
   *(VIC_BASE + VIC_INTENABLE) |= (1<<12); // UART0 at bit12
   *(VIC_BASE + VIC_INTENABLE) |= (1<<13); // UART1 at bit13
   *(VIC_BASE + VIC_INTENABLE) |= (1<<31); // SIC to VIC's IRQ31
  
   /* enable KBD IRQ */
   *(SIC_BASE + SIC_INTENABLE) |= (1<<3); // KBD int=bit3 on SIC
 
   kprintf("Program C3.6 start: test Vectored Interrupts\n");
   vectorInt_init(); // must do thie AFTER driver_init()
   timer_init();
   timer_start(0);

   uart_init();
   up = &uart[0];

   kprintf("test uart0 I/O: enter text from UART 0\n");
   while(1){
     ugets(up, line);
     uprintf("  line=%s\n", line);
     if (strcmp(line, "end")==0)
	break;
   }

   kprintf("test UART1 I/O: enter text from UART 1\n");
   up = &uart[1];
   while(1){
     ugets(up, line);
     ufprintf(up, "  line=%s\n", line);
     if (strcmp(line, "end")==0)
	break;
   }

   //uprintf("test KBD inputs\n"); // print to uart0
   kprintf("test KBD inputs\n"); // print to LCD
   
   while(1){
      kgets(line);
      color = GREEN;
      kprintf("line=%s\n", line);
      if (strcmp(line, "end")==0)
         break;
   }
   kprintf("END OF run %d\n", 1234);
}
