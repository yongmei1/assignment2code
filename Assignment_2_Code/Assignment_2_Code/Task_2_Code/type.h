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
typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;
#define printf kprintf

#define BLUE   0
#define GREEN  1
#define RED    2
#define WHITE  3
#define CYAN   4
#define YELLOW 5

#define N_SCAN 64

u32 *VIC_BASE = (u32 *)0x10140000;
#define VIC_STATUS    0x00/4
#define VIC_INTENABLE 0x10/4
#define VIC_VADDR     0x30/4

u32 *SIC_BASE = (u32 *)0x10003000;
#define SIC_STATUS    0x00/4
#define SIC_INTENABLE 0x08/4
#define SIC_ENSET     0x08/4
#define SIC_PICENSET  0x20/4

char *UART0_BASE = (char *)0x101f1000;
char *UART1_BASE = (char *)0x101f2000;
#define UDR   0x00
#define UFR   0x18
#define UIMSC 0x38

char *KBD_BASE = 0x10006000;
#define KBD_CR 0x00
#define KBD_DR 0x08
