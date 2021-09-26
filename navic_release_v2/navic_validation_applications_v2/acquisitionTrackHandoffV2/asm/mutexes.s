!
! A utility package consisting of mutex routines.
! Two versions are provided.  One with SWAP and
! one with ldstub.
!
.global __ajit_swap_asm__
__ajit_swap_asm__:
	swap [%o0], %o1
	mov %o1, %o0
	retl
	nop;

!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
!!  SWAP version				          !!
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
!
! has one argument assumed to be in %i0,
! which specifies the address of the mutex
! variable to be used. 
!
.align 4
SWAP_MUTEX_FLAG:
.byte 0x0
.byte 0x0
.byte 0x0
.byte 0x0
.global minimal_acquire_mutex_using_swap
minimal_acquire_mutex_using_swap :
	save %sp, -96, %sp
	set SWAP_MUTEX_FLAG, %o0
	mov 0x1, %l5 
	!  LOCK_FLAG is swapped with 0x1
	swap [%o0], %l5
	tst %l5
	! If read-value of LOCK_FLAG is 0,
	!   then lock has been acquired.
	be swap_lock_acquired
	nop
spin_on_swap_lock:
	! lock not acquired.. spin until
	! LOCK_FLAG becomes 0..	
	ld [%o0], %l5
	tst %l5
	bne spin_on_swap_lock
	nop
	ba,a minimal_acquire_mutex_using_swap
	nop
swap_lock_acquired:
	retl
	restore
!
! single argument: %i0 contains the address
! of the mutex variable to be cleared.
!
.global minimal_release_mutex_using_swap
minimal_release_mutex_using_swap:
	save %sp, -96, %sp
	set SWAP_MUTEX_FLAG, %o0
	st %g0, [%o0]
	retl
	restore

!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
!!  LDSTUB version				          !!
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
.global minimal_acquire_mutex_using_ldstub
LDSTUB_MUTEX_FLAG:
.byte 0x0
.byte 0x0
.byte 0x0
.byte 0x0
!
! has one argument assumed to be in %i0,
! which specifies the address of the mutex
! variable to be used. 
!
minimal_acquire_mutex_using_ldstub :
	save %sp, -96, %sp
	set LDSTUB_MUTEX_FLAG, %o0
	mov 0x1, %o1
	ldstub [%o0], %o1
	tst %o1
	! If read-value of LOCK_FLAG is 0,
	!   then lock has been acquired.
	be ldstub_mutex_acquired
	nop
spin_on_ldstub_mutex:
	! lock not acquired.. spin until
	! LOCK_FLAG becomes 0..	
	ldub [%o0], %o1
	tst %o1
	bne spin_on_ldstub_mutex
	nop
	! Someone has released the lock
	! Try to acquire it!
	ba,a minimal_acquire_mutex_using_ldstub
ldstub_mutex_acquired:
	retl
	restore
!
! has one argument assumed to be in %i0,
! which specifies the address of the mutex
! variable to be used. 
!
.global minimal_release_mutex_using_ldstub
minimal_release_mutex_using_ldstub:
	save %sp, -96, %sp
	set LDSTUB_MUTEX_FLAG, %o0
	stub %g0, [%o0]
	retl
	restore
