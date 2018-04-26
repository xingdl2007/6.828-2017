/* See COPYRIGHT for copyright information. */

#include <inc/mmu.h>
#include <inc/memlayout.h>
#include <inc/trap.h>

#include <kern/picirq.h>


###################################################################
# exceptions/interrupts
###################################################################

/* TRAPHANDLER defines a globally-visible function for handling a trap.
 * It pushes a trap number onto the stack, then jumps to _alltraps.
 * Use TRAPHANDLER for traps where the CPU automatically pushes an error code.
 *
 * You shouldn't call a TRAPHANDLER function from C, but you may
 * need to _declare_ one in C (for instance, to get a function pointer
 * during IDT setup).  You can declare the function with
 *   void NAME();
 * where NAME is the argument passed to TRAPHANDLER.
 */
#define TRAPHANDLER(name, num)						\
	.globl name;		/* define global symbol for 'name' */	\
	.type name, @function;	/* symbol type is function */		\
	.align 2;		/* align function definition */		\
	name:			/* function starts here */		\
	pushl $(num);							\
	jmp _alltraps

/* Use TRAPHANDLER_NOEC for traps where the CPU doesn't push an error code.
 * It pushes a 0 in place of the error code, so the trap frame has the same
 * format in either case.
 */
#define TRAPHANDLER_NOEC(name, num)					\
	.globl name;							\
	.type name, @function;						\
	.align 2;							\
	name:								\
	pushl $0;							\
	pushl $(num);							\
	jmp _alltraps

.text

/*
 * Lab 3: Your code here for generating entry points for the different traps.
 */
TRAPHANDLER_NOEC(v_divide, T_DIVIDE)
TRAPHANDLER_NOEC(v_debug, T_DEBUG)
TRAPHANDLER_NOEC(v_nmi, T_NMI)
TRAPHANDLER_NOEC(v_breakpoint, T_BRKPT)
TRAPHANDLER_NOEC(v_overflow, T_OFLOW)
TRAPHANDLER_NOEC(v_bound, T_BOUND)
TRAPHANDLER_NOEC(v_illop, T_ILLOP)
TRAPHANDLER_NOEC(v_coprocessor, T_DEVICE)
TRAPHANDLER_NOEC(v_float, T_FPERR)
TRAPHANDLER_NOEC(v_machine, T_MCHK)
TRAPHANDLER_NOEC(v_simd, T_SIMDERR)
TRAPHANDLER_NOEC(v_syscall, T_SYSCALL)

TRAPHANDLER(v_double, T_DBLFLT)
TRAPHANDLER(v_tss, T_TSS)
TRAPHANDLER(v_segment, T_SEGNP)
TRAPHANDLER(v_general, T_GPFLT)
TRAPHANDLER(v_page, T_PGFLT)
TRAPHANDLER(v_align, T_ALIGN)

/*
 * Lab 3: Your code here for _alltraps
 */

.global _alltraps
_alltraps:
	# Build trap frame
	pushl %ds
	pushl %es
	pushal

	# Set up data segments
	movw $(GD_KD), %ax
	movw %ax, %ds
	movw %ax, %es

	# Call trap(tf), where tf = %esp
	pushl %esp
	call trap
	addl $4, %esp

	# Return falls through to trapret
.global trapret
trapret:
	popal
	popl %es
	popl %ds
	addl $0x8, %esp  # trapno and errcode
	iret
