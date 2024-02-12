; G8RTOS_SchedulerASM.s
; Created: 2022-07-26
; Updated: 2022-07-26
; Contains assembly functions for scheduler.

	; Functions Defined
	.def G8RTOS_Start, PendSV_Handler

	; Dependencies
	.ref CurrentlyRunningThread, G8RTOS_Scheduler

	.thumb		; Set to thumb mode
	.align 2	; Align by 2 bytes (thumb mode uses allignment by 2 or 4)
	.text		; Text section

; Need to have the address defined in file
; (label needs to be close enough to asm code to be reached with PC relative addressing)
RunningPtr: .field CurrentlyRunningThread, 32

; G8RTOS_Start
;	Sets the first thread to be the currently running thread
;	Starts the currently running thread by setting Link Register to tcb's Program Counter
G8RTOS_Start:

	.asmfunc


	LDR R4, RunningPtr	;Loads the address of RunningPtr into R4
	LDR R5, [R4]		;Loads the currently running pointer into R5
	LDR R6, [R5]		;Loads the first thread's stack pointer into R6
	ADD R6, R6, #60
	STR R6, [R5]
	MOV SP, R6
	LDR LR, [R6, #-4]	;Loads LR with the first thread's PC
	CPSIE I ;Enable interrupts

	BX LR ;Branches to the first thread

	.endasmfunc

; PendSV_Handler
; - Performs a context switch in G8RTOS
; 	- Saves remaining registers into thread stack
;	- Saves current stack pointer to tcb
;	- Calls G8RTOS_Scheduler to get new tcb
;	- Set stack pointer to new stack pointer from new tcb
;	- Pops registers from thread stack
PendSV_Handler:

	.asmfunc

	CPSID I ;Disable interrupts

	PUSH {R4-R11} ;Store register file
	LDR R1, RunningPtr ;Load pointer to Current Thread
	LDR R2, [R1, #0] ;Load Address of SP
	STR SP, [R2, #0] ;Store SP into TCB


	MOV R7, LR ;Save LR before we go to G8RTOS_Scheduler
	BL G8RTOS_Scheduler ;Call G8RTOS_Scheduler
	MOV LR, R7 ;Restore LR from before we went to G8RTOS_Scheduler

	LDR R1, RunningPtr ;Load pointer to Current Thread
	LDR R2, [R1, #0] ;Load Address of SP
	LDR SP, [R2, #0] ;Load new SP from next TCB
	POP {R4-R11} ;Pop register file

	CPSIE I ;Enable interrupts
	BX LR

	.endasmfunc

	.align
	.end
