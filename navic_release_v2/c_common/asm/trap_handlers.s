! a generic isr model for the case when an interrupt handler
! does a lot of user-level work.

!
! When you get here, you are in the trap window T
!
.section .text.traphandlers
.global coprocessor_isr_ibuffer
.align 64
coprocessor_isr_ibuffer:
.skip 64
.global coprocessor_isr
coprocessor_isr:
   !
   ! Step 1
   !    First disable the interrupt controller.
   !
   sethi  %hi(0xffff3000), %l6
   sta %g0, [%l6] 0x20  ! disable irc

   !
   ! Step 2
   !    Save the PSR in l0
   !
   mov %psr, %l0
    	

   !
   ! enable traps, but set proc-ilvl=15
   ! so that non NMI interrupt will be blocked.
   !    This prevents a non NMI interrupt from interrupting
   !    the interrupt handler.
   or %l0,0x20, %l6       ! ET=1
   or %l6, 0xf00, %l6     ! [11:8] = 0xf
   wr %l6, %psr           ! write to PSR

   ! Step 3
   !  ensure that you have a stack
   !  to work with.  Also makes this
   !  window a valid window to do
   !  serious work in.
   restore
   save %sp, -96, %sp

   ! Now you are back in window T, but
   ! with a stack.  This stack is necessary
   ! for WOF/WUF traps that can be triggered by
   ! the high-level ISR.

   !
   ! Step 4
   !   The globals must be saved
   !   as well. The interrupt handler
   !   may modify them..
   ! Save them onto the interrupt buffer.
   !
   set coprocessor_isr_ibuffer, %l5
   std %g0, [%l5 + 0]
   std %g2, [%l5 + 8]
   std %g4, [%l5 + 16]
   std %g6, [%l5 + 24]

   ! Step 5
   !    call the interrupt handler.. free to
   !    do whatever it wants..
   call coprocessor_interrupt_handler
   nop

   ! Step 6
   !   back from complex code..
   !   recover globals.
   !
   set coprocessor_isr_ibuffer, %l5
   ld [%l5 + 4],  %g1
   ldd [%l5 + 8], %g2
   ldd [%l5 + 16],%g4
   ldd [%l5 + 24],%g6


   ! Step 7
   !
   ! Important: ensure that you
   ! have a valid window to rett
   ! into. rett into an invalid window
   ! will put the processor in error mode.
   !
   ! This restore may itself cause an 
   ! exception, which will be handled.
   restore
   save 

   ! Step 8
   !   back to old psr.. disable traps.
   !   That is, et = 0, proc-ilvl as before.
   wr %l0, %psr


   ! Step 9
   !  re-enable the interrupt controller.
   sethi  %hi(0xffff3000), %l6
   mov 0x1, %l7 
   sta %l7, [%l6] 0x20  ! enable irc

   ! Step 10
   !   go home.. guaranteed that the
   !   return window is valid (see Step 7 above)
   jmpl %r17, %r0
   rett %r18

   nop

/* reference 					*/
/* http://ieng9.ucsd.edu/~cs30x/sparcstack.html */

!
! A save instruction caused a window-overflow-trap.
! Window-id      1   2   3
! Symbol         N   T   W
! From Window W we wanted to move to window T.  T
! was invalid... Trap!  Trap-handler puts us in  window T.
! 
! We will save window N, N becomes the new trap window
! and T becomes available to reexecute the save.
!  
window_overflow_trap_handler:


	! when we enter the trap handler, we are in window T
	! Note that T may not have a stack...

        /* rotate WIM one bit right, we have 8 windows */
        mov %wim,%l3
        sll %l3,7,%l4
        srl %l3,1,%l3
        or  %l3,%l4,%l3
        and %l3,0xff,%l3

        /* disable WIM traps */
        mov %g0,%wim
        nop; nop; nop

	/*
        /* move to window N (this will not trap, since wim=0), .. */
	/* Note: o6 in this window will provide a stack pointer   */
	/*  which we assume to be valid                           */
	/*   (note: in linux kernel, validity of sp is also checked) */
        save

        /* dump registers to stack (note: use sp in N) */
        std %l0, [%sp +  0]
        std %l2, [%sp +  8]
        std %l4, [%sp + 16]
        std %l6, [%sp + 24]
        std %i0, [%sp + 32]
        std %i2, [%sp + 40]
        std %i4, [%sp + 48]
        std %i6, [%sp + 56]

        /* back to where we should be     */
	/* go back to the window T from which the save had trapped */
        restore

        /* set new value of window invalid mask       */
	/* new value of WIM.  (W+1) is marked invalid */
        mov %l3,%wim
        nop; nop; nop

        /* trap handler done, reexecute the save which trapped.  This time
	/* window W is available.. no trap.   */
        jmp %l1

	/* Note rett does a restore, puts us back in W */
        rett %l2

!
! window underflow trap handler
!   From window W-1 we did a restore and we find that window
!   W is unavailable.. trap handler puts us in window W-2.
!
!
window_underflow_trap_handler:

        /* rotate WIM on bit LEFT, we have 8 windows */ 
        mov %wim,%l3
        srl %l3,7,%l4
        sll %l3,1,%l3
        or  %l3,%l4,%l3
        and %l3,0xff,%l3

        /* disable WIM traps */
        mov %g0,%wim
        nop; nop; nop

        /* get to correct window, ie window W */
        restore 	
        restore	

        /* recovers locals and ins of W   */
        ldd [%sp +  0], %l0
        ldd [%sp +  8], %l2
        ldd [%sp + 16], %l4
        ldd [%sp + 24], %l6
        ldd [%sp + 32], %i0
        ldd [%sp + 40], %i2
        ldd [%sp + 48], %i4
        ldd [%sp + 56], %i6

        /* go to window W-2 ...*/
        save
        save

        /* set new value of wim (W+1 is marked invalid) */
        mov %l3,%wim
        nop; nop; nop

        /* go home */
        jmp %l1
	/* back in W-1, retry with W valid.*/
        rett %l2

.section .text.traptablebase
	.align 4096
.global trap_table_base;
trap_table_base:
hardware_trap_table_base:
HW_trap_0x00: ta 0; nop; nop; nop;
HW_trap_0x01: ta 0; nop; nop; nop;
HW_trap_0x02: ta 0; nop; nop; nop;
HW_trap_0x03: ta 0; nop; nop; nop;
HW_trap_0x04: ta 0; nop; nop; nop;
HW_trap_0x05: ! window overflow trap
	ba window_overflow_trap_handler; nop; nop; nop;
HW_trap_0x06: ! window underflow trap
	ba window_underflow_trap_handler; nop; nop; nop;
HW_trap_0x07: ta 0; nop; nop; nop;
HW_trap_0x08: ta 0; nop; nop; nop;
HW_trap_0x09: ta 0; nop; nop; nop;
HW_trap_0x0a: ta 0; nop; nop; nop;
HW_trap_0x0b: ta 0; nop; nop; nop;
HW_trap_0x0c: ta 0; nop; nop; nop;
HW_trap_0x0d: ta 0; nop; nop; nop;
HW_trap_0x0e: ta 0; nop; nop; nop;
HW_trap_0x0f: ta 0; nop; nop; nop;
HW_trap_0x10: ta 0; nop; nop; nop;
HW_trap_0x11: ta 0; nop; nop; nop;
HW_trap_0x12: ta 0; nop; nop; nop;
HW_trap_0x13: ta 0; nop; nop; nop;
HW_trap_0x14: ta 0; nop; nop; nop;
HW_trap_0x15: ta 0; nop; nop; nop;
HW_trap_0x16: ta 0; nop; nop; nop;
HW_trap_0x17: ta 0; nop; nop; nop;
HW_trap_0x18: ta 0; nop; nop; nop;
HW_trap_0x19: ta 0; nop; nop; nop;
! timer trap handler.
HW_trap_0x1a: ta 0; nop; nop; nop;
HW_trap_0x1b: ta 0; nop; nop; nop;
! serial device trap handler.
HW_trap_0x1c: ta 0; nop; nop; nop;
! coprocessor trap handler
HW_trap_0x1d: ba coprocessor_isr; nop; nop; nop;
HW_trap_0x1e:ta 0; nop; nop; nop;
! NMI
HW_trap_0x1f: ta 0; nop; nop; nop;
HW_trap_0x20: ta 0; nop; nop; nop;
HW_trap_0x21: ta 0; nop; nop; nop;
HW_trap_0x22: ta 0; nop; nop; nop;
HW_trap_0x23: ta 0; nop; nop; nop;
HW_trap_0x24: ta 0; nop; nop; nop;
HW_trap_0x25: ta 0; nop; nop; nop;
HW_trap_0x26: ta 0; nop; nop; nop;
HW_trap_0x27: ta 0; nop; nop; nop;
HW_trap_0x28: ta 0; nop; nop; nop;
HW_trap_0x29: ta 0; nop; nop; nop;
HW_trap_0x2a: ta 0; nop; nop; nop;
HW_trap_0x2b: ta 0; nop; nop; nop;
HW_trap_0x2c: ta 0; nop; nop; nop;
HW_trap_0x2d: ta 0; nop; nop; nop;
HW_trap_0x2e: ta 0; nop; nop; nop;
HW_trap_0x2f: ta 0; nop; nop; nop;
HW_trap_0x30: ta 0; nop; nop; nop;
HW_trap_0x31: ta 0; nop; nop; nop;
HW_trap_0x32: ta 0; nop; nop; nop;
HW_trap_0x33: ta 0; nop; nop; nop;
HW_trap_0x34: ta 0; nop; nop; nop;
HW_trap_0x35: ta 0; nop; nop; nop;
HW_trap_0x36: ta 0; nop; nop; nop;
HW_trap_0x37: ta 0; nop; nop; nop;
HW_trap_0x38: ta 0; nop; nop; nop;
HW_trap_0x39: ta 0; nop; nop; nop;
HW_trap_0x3a: ta 0; nop; nop; nop;
HW_trap_0x3b: ta 0; nop; nop; nop;
HW_trap_0x3c: ta 0; nop; nop; nop;
HW_trap_0x3d: ta 0; nop; nop; nop;
HW_trap_0x3e: ta 0; nop; nop; nop;
HW_trap_0x3f: ta 0; nop; nop; nop;
HW_trap_0x40: ta 0; nop; nop; nop;
HW_trap_0x41: ta 0; nop; nop; nop;
HW_trap_0x42: ta 0; nop; nop; nop;
HW_trap_0x43: ta 0; nop; nop; nop;
HW_trap_0x44: ta 0; nop; nop; nop;
HW_trap_0x45: ta 0; nop; nop; nop;
HW_trap_0x46: ta 0; nop; nop; nop;
HW_trap_0x47: ta 0; nop; nop; nop;
HW_trap_0x48: ta 0; nop; nop; nop;
HW_trap_0x49: ta 0; nop; nop; nop;
HW_trap_0x4a: ta 0; nop; nop; nop;
HW_trap_0x4b: ta 0; nop; nop; nop;
HW_trap_0x4c: ta 0; nop; nop; nop;
HW_trap_0x4d: ta 0; nop; nop; nop;
HW_trap_0x4e: ta 0; nop; nop; nop;
HW_trap_0x4f: ta 0; nop; nop; nop;
HW_trap_0x50: ta 0; nop; nop; nop;
HW_trap_0x51: ta 0; nop; nop; nop;
HW_trap_0x52: ta 0; nop; nop; nop;
HW_trap_0x53: ta 0; nop; nop; nop;
HW_trap_0x54: ta 0; nop; nop; nop;
HW_trap_0x55: ta 0; nop; nop; nop;
HW_trap_0x56: ta 0; nop; nop; nop;
HW_trap_0x57: ta 0; nop; nop; nop;
HW_trap_0x58: ta 0; nop; nop; nop;
HW_trap_0x59: ta 0; nop; nop; nop;
HW_trap_0x5a: ta 0; nop; nop; nop;
HW_trap_0x5b: ta 0; nop; nop; nop;
HW_trap_0x5c: ta 0; nop; nop; nop;
HW_trap_0x5d: ta 0; nop; nop; nop;
HW_trap_0x5e: ta 0; nop; nop; nop;
HW_trap_0x5f: ta 0; nop; nop; nop;
HW_trap_0x60: ta 0; nop; nop; nop;
HW_trap_0x61: ta 0; nop; nop; nop;
HW_trap_0x62: ta 0; nop; nop; nop;
HW_trap_0x63: ta 0; nop; nop; nop;
HW_trap_0x64: ta 0; nop; nop; nop;
HW_trap_0x65: ta 0; nop; nop; nop;
HW_trap_0x66: ta 0; nop; nop; nop;
HW_trap_0x67: ta 0; nop; nop; nop;
HW_trap_0x68: ta 0; nop; nop; nop;
HW_trap_0x69: ta 0; nop; nop; nop;
HW_trap_0x6a: ta 0; nop; nop; nop;
HW_trap_0x6b: ta 0; nop; nop; nop;
HW_trap_0x6c: ta 0; nop; nop; nop;
HW_trap_0x6d: ta 0; nop; nop; nop;
HW_trap_0x6e: ta 0; nop; nop; nop;
HW_trap_0x6f: ta 0; nop; nop; nop;
HW_trap_0x70: ta 0; nop; nop; nop;
HW_trap_0x71: ta 0; nop; nop; nop;
HW_trap_0x72: ta 0; nop; nop; nop;
HW_trap_0x73: ta 0; nop; nop; nop;
HW_trap_0x74: ta 0; nop; nop; nop;
HW_trap_0x75: ta 0; nop; nop; nop;
HW_trap_0x76: ta 0; nop; nop; nop;
HW_trap_0x77: ta 0; nop; nop; nop;
HW_trap_0x78: ta 0; nop; nop; nop;
HW_trap_0x79: ta 0; nop; nop; nop;
HW_trap_0x7a: ta 0; nop; nop; nop;
HW_trap_0x7b: ta 0; nop; nop; nop;
HW_trap_0x7c: ta 0; nop; nop; nop;
HW_trap_0x7d: ta 0; nop; nop; nop;
HW_trap_0x7e: ta 0; nop; nop; nop;
HW_trap_0x7f: ta 0; nop; nop; nop;

software_trap_table_base:

SW_trap_0x80: ta 0; nop; nop; nop;
SW_trap_0x81: ta 0; nop; nop; nop;
SW_trap_0x82: ta 0; nop; nop; nop;
SW_trap_0x83: ta 0; nop; nop; nop;
SW_trap_0x84: ta 0; nop; nop; nop;
SW_trap_0x85: ta 0; nop; nop; nop;
SW_trap_0x86: ta 0; nop; nop; nop;
SW_trap_0x87: ta 0; nop; nop; nop;
SW_trap_0x88: ta 0; nop; nop; nop;
SW_trap_0x89: ta 0; nop; nop; nop;
SW_trap_0x8a: ta 0; nop; nop; nop;
SW_trap_0x8b: ta 0; nop; nop; nop;
SW_trap_0x8c: ta 0; nop; nop; nop;
SW_trap_0x8d: ta 0; nop; nop; nop;
SW_trap_0x8e: ta 0; nop; nop; nop;
SW_trap_0x8f: ta 0; nop; nop; nop;
SW_trap_0x90: ta 0; nop; nop; nop;
SW_trap_0x91: ta 0; nop; nop; nop;
SW_trap_0x92: ta 0; nop; nop; nop;
SW_trap_0x93: ta 0; nop; nop; nop;
SW_trap_0x94: ta 0; nop; nop; nop;
SW_trap_0x95: ta 0; nop; nop; nop;
SW_trap_0x96: ta 0; nop; nop; nop;
SW_trap_0x97: ta 0; nop; nop; nop;
SW_trap_0x98: ta 0; nop; nop; nop;
SW_trap_0x99: ta 0; nop; nop; nop;
SW_trap_0x9a: ta 0; nop; nop; nop;
SW_trap_0x9b: ta 0; nop; nop; nop;
SW_trap_0x9c: ta 0; nop; nop; nop;
SW_trap_0x9d: ta 0; nop; nop; nop;
SW_trap_0x9e: ta 0; nop; nop; nop;
SW_trap_0x9f: ta 0; nop; nop; nop;
SW_trap_0xa0: ta 0; nop; nop; nop;
SW_trap_0xa1: ta 0; nop; nop; nop;
SW_trap_0xa2: ta 0; nop; nop; nop;
SW_trap_0xa3: ta 0; nop; nop; nop;
SW_trap_0xa4: ta 0; nop; nop; nop;
SW_trap_0xa5: ta 0; nop; nop; nop;
SW_trap_0xa6: ta 0; nop; nop; nop;
SW_trap_0xa7: ta 0; nop; nop; nop;
SW_trap_0xa8: ta 0; nop; nop; nop;
SW_trap_0xa9: ta 0; nop; nop; nop;
SW_trap_0xaa: ta 0; nop; nop; nop;
SW_trap_0xab: ta 0; nop; nop; nop;
SW_trap_0xac: ta 0; nop; nop; nop;
SW_trap_0xad: ta 0; nop; nop; nop;
SW_trap_0xae: ta 0; nop; nop; nop;
SW_trap_0xaf: ta 0; nop; nop; nop;
SW_trap_0xb0: ta 0; nop; nop; nop;
SW_trap_0xb1: ta 0; nop; nop; nop;
SW_trap_0xb2: ta 0; nop; nop; nop;
SW_trap_0xb3: ta 0; nop; nop; nop;
SW_trap_0xb4: ta 0; nop; nop; nop;
SW_trap_0xb5: ta 0; nop; nop; nop;
SW_trap_0xb6: ta 0; nop; nop; nop;
SW_trap_0xb7: ta 0; nop; nop; nop;
SW_trap_0xb8: ta 0; nop; nop; nop;
SW_trap_0xb9: ta 0; nop; nop; nop;
SW_trap_0xba: ta 0; nop; nop; nop;
SW_trap_0xbb: ta 0; nop; nop; nop;
SW_trap_0xbc: ta 0; nop; nop; nop;
SW_trap_0xbd: ta 0; nop; nop; nop;
SW_trap_0xbe: ta 0; nop; nop; nop;
SW_trap_0xbf: ta 0; nop; nop; nop;
SW_trap_0xc0: ta 0; nop; nop; nop;
SW_trap_0xc1: ta 0; nop; nop; nop;
SW_trap_0xc2: ta 0; nop; nop; nop;
SW_trap_0xc3: ta 0; nop; nop; nop;
SW_trap_0xc4: ta 0; nop; nop; nop;
SW_trap_0xc5: ta 0; nop; nop; nop;
SW_trap_0xc6: ta 0; nop; nop; nop;
SW_trap_0xc7: ta 0; nop; nop; nop;
SW_trap_0xc8: ta 0; nop; nop; nop;
SW_trap_0xc9: ta 0; nop; nop; nop;
SW_trap_0xca: ta 0; nop; nop; nop;
SW_trap_0xcb: ta 0; nop; nop; nop;
SW_trap_0xcc: ta 0; nop; nop; nop;
SW_trap_0xcd: ta 0; nop; nop; nop;
SW_trap_0xce: ta 0; nop; nop; nop;
SW_trap_0xcf: ta 0; nop; nop; nop;
SW_trap_0xd0: ta 0; nop; nop; nop;
SW_trap_0xd1: ta 0; nop; nop; nop;
SW_trap_0xd2: ta 0; nop; nop; nop;
SW_trap_0xd3: ta 0; nop; nop; nop;
SW_trap_0xd4: ta 0; nop; nop; nop;
SW_trap_0xd5: ta 0; nop; nop; nop;
SW_trap_0xd6: ta 0; nop; nop; nop;
SW_trap_0xd7: ta 0; nop; nop; nop;
SW_trap_0xd8: ta 0; nop; nop; nop;
SW_trap_0xd9: ta 0; nop; nop; nop;
SW_trap_0xda: ta 0; nop; nop; nop;
SW_trap_0xdb: ta 0; nop; nop; nop;
SW_trap_0xdc: ta 0; nop; nop; nop;
SW_trap_0xdd: ta 0; nop; nop; nop;
SW_trap_0xde: ta 0; nop; nop; nop;
SW_trap_0xdf: ta 0; nop; nop; nop;
SW_trap_0xe0: ta 0; nop; nop; nop;
SW_trap_0xe1: ta 0; nop; nop; nop;
SW_trap_0xe2: ta 0; nop; nop; nop;
SW_trap_0xe3: ta 0; nop; nop; nop;
SW_trap_0xe4: ta 0; nop; nop; nop;
SW_trap_0xe5: ta 0; nop; nop; nop;
SW_trap_0xe6: ta 0; nop; nop; nop;
SW_trap_0xe7: ta 0; nop; nop; nop;
SW_trap_0xe8: ta 0; nop; nop; nop;
SW_trap_0xe9: ta 0; nop; nop; nop;
SW_trap_0xea: ta 0; nop; nop; nop;
SW_trap_0xeb: ta 0; nop; nop; nop;
SW_trap_0xec: ta 0; nop; nop; nop;
SW_trap_0xed: ta 0; nop; nop; nop;
SW_trap_0xee: ta 0; nop; nop; nop;
SW_trap_0xef: ta 0; nop; nop; nop;
SW_trap_0xf0: ta 0; nop; nop; nop;
SW_trap_0xf1: ta 0; nop; nop; nop;
SW_trap_0xf2: ta 0; nop; nop; nop;
SW_trap_0xf3: ta 0; nop; nop; nop;
SW_trap_0xf4: ta 0; nop; nop; nop;
SW_trap_0xf5: ta 0; nop; nop; nop;
SW_trap_0xf6: ta 0; nop; nop; nop;
SW_trap_0xf7: ta 0; nop; nop; nop;
SW_trap_0xf8: ta 0; nop; nop; nop;
SW_trap_0xf9: ta 0; nop; nop; nop;
SW_trap_0xfa: ta 0; nop; nop; nop;
SW_trap_0xfb: ta 0; nop; nop; nop;
SW_trap_0xfc: ta 0; nop; nop; nop;
SW_trap_0xfd: ta 0; nop; nop; nop;
SW_trap_0xfe: ta 0; nop; nop; nop;

nop
nop

