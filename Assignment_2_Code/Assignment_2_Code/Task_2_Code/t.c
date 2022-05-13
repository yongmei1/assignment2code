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

// IRQ interrupts handler entry point
TIMER *tp[4];

void IRQ_chandler()
{
    int vicstatus, sicstatus;
    // read VIC SIV status registers to find out which interrupt
    vicstatus = *(VIC_BASE + VIC_STATUS);
    sicstatus = *(SIC_BASE + SIC_STATUS);  

    // VIC status BITs: timer0,1=4, uart0=13, uart1=14, SIC=31: KBD at 3

    if (vicstatus & 0x0010){   // timer0,1=bit4
      if (*(tp[0]->base + TVALUE)==0) // timer 0
         timer_handler(0);
      if (*(tp[1]->base + TVALUE)==0)
         timer_handler(1);
    }
    if (vicstatus & 0x0020){   // timer2,3=bit5
       if(*(tp[2]->base + TVALUE)==0)
         timer_handler(2);
       if (*(tp[3]->base + TVALUE)==0)
         timer_handler(3);
    }
}

int main()
{
   int i; 

   color = YELLOW;
   row = col = 0; 
   fbuf_init();

   /* enable timer0,1, uart0,1 SIC interrupts */
   *(VIC_BASE + VIC_INTENABLE) |= (1<<4);  // timer0,1 at bit4 
   *(VIC_BASE + VIC_INTENABLE) |= (1<<5);  // timer2,3 at bit5 
 
   printf("Program C3.1 start: test timer driver by interrupts\n");
   timer_init();
 
   for (i=0; i<4; i++){
      tp[i] = &timer[i];
      timer_start(i);
   }

   kprintf("Enter while(1) loop\n", 1234);
}
