	.syntax unified
        .type PendSV_Handler, %function
        .global PendSV_Handler
        .extern curr_task
        .extern curr_thread
        .extern next_task
        .extern next_thread

PendSV_Handler:
        // Context switching code
        // Simple version - assume No floating point support
        // -------------------------
        // Save current context
        MRS R0, PSP        // Get current process stack pointer value
        //STMDB R0!,{R4-R11} // Save R4 to R11 in task stack (8 regs)
	MOV R1, R4
	MOV R2, R5
	MOV R3, R6
	MOV R4, R7
	MOV R5, SP
	MOV SP, R0
	PUSH {R1-R4}

	MOV R1, R8
	MOV R2, R9
	MOV R3, R10
	MOV R4, R11
	PUSH {R1-R4}
	MOV R0, SP
	MOV SP, R5

        LDR R1,=curr_task

        // curr_thread->sp = R0;
        LDR R2,=curr_thread
        LDR R3,[R2]
        // R3 + offsetof(struct thread, sp) == 0
        STR R0,[R3]

        // -------------------------
        // Load next context
        LDR R4,=next_task
        LDR R4,[R4]          // Get next task ID
        STR R4,[R1]      // curr_task = next_task

        // curr_thread = next_thread;
        LDR R4,=next_thread
        LDR R4,[R4]
        STR R4,[R2]

	// pop user context
        LDR R0,[R4]
	
	LDMIA R0!, {R1-R3}
	MOV R4, R1
	MOV R5, R2
	MOV R6, R3

	LDMIA R0!, {R1-R3}
	MOV R7, R1
	MOV R8, R2
	MOV R9, R3

	LDMIA R0!, {R1-R2}
	MOV R10, R1
	MOV R11, R2

        MSR PSP, R0      // Set PSP to next task
        BX LR            // Return
