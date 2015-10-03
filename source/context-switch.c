#include <stdint.h>
#include <stddef.h>

#include <cmsis.h> /* mbed CMSIS header */

LPC_GPIO_TypeDef * const LED0_GPIO = LPC_GPIO1;
LPC_GPIO_TypeDef * const LED1_GPIO = LPC_GPIO1;

enum {
	LED0_pin = 8,
	LED1_pin = 7,
	LED0 = (1 << LED0_pin),
	LED1 = (1 << LED1_pin),
};

void task0(void);
void task1(void);

volatile uint32_t systick_count = 0;
struct exc_context {
	uint32_t r0, r1, r2, r3, r12, r14;
	void *pc;
	uint32_t psr;
};

struct user_context {
	uint32_t r4, r5, r6, r7, r8, r9, r10, r11;
};

struct thread {
	uint8_t *sp;
};

static void thread_preserve_stack(struct thread *t, size_t n)
{
	t->sp -= n;
}

static void thread_create(struct thread *t, void(*func)(void), uint8_t *stack_pointer)
{
	struct exc_context *ep;
	t->sp = stack_pointer;
	thread_preserve_stack(t, sizeof(struct user_context) + sizeof(struct exc_context));
	ep = (struct exc_context*)(t->sp + sizeof(struct user_context));
	ep->pc = func;
	ep->psr = 0x01000000; /* Only Thumb Mode bit on */
}

long long task0_stack[16], task1_stack[16];

#define THREAD_STACK_SIZE (sizeof(task0_stack))

enum {
	N_THREADS = 2,
};

struct thread_table {
	struct thread thread;
	void (*func)(void);
	uint8_t *stack;
};

struct thread_table thread_table[N_THREADS] = {
	{ {0}, task0, (uint8_t*)task0_stack },
	{ {0}, task1, (uint8_t*)task1_stack },
};

struct thread *threads[N_THREADS];

struct thread *curr_thread,
	      *next_thread;
int curr_task, next_task;

int main(void)
{
	int i;
	SCB->CCR |= SCB_CCR_STKALIGN_Msk; /* enable double word stack alignment */

	LED0_GPIO->DIR |= LED0;	/* set the direction of the LED pin to output */
	LED1_GPIO->DIR |= LED1;	/* set the direction of the LED pin to output */
	for (i = 0; i < N_THREADS; i++) {
		thread_create(&thread_table[i].thread,
		              thread_table[i].func,
		              thread_table[i].stack + THREAD_STACK_SIZE);
		threads[i] = &thread_table[i].thread;
	}
	curr_task = 0;
	curr_thread = threads[curr_task];
	/* set PSP to top of task 0 stack */
	__set_PSP((uint32_t)(curr_thread->sp + sizeof(struct user_context) + sizeof(struct exc_context)));
	NVIC_SetPriority(PendSV_IRQn, 0xFF); /* set PendSV to lowest possible priority */

	SysTick_Config(SystemCoreClock/1000);
	__set_CONTROL(0x2); /* switch to use Process Stack */
	__ISB(); /* execute ISB after changing CONTROL (architectural recommendation) */

	thread_table[curr_task].func();
}

void task0(void)
{
	while (1) {
		if (systick_count & 0x80) {
			LED0_GPIO->DATA |= LED0;
		} else {
			LED0_GPIO->DATA &= ~LED0;
		}
	};
}

void task1(void)
{
	while (1) {
		if (systick_count & 0x100) {
			LED1_GPIO->DATA |= LED1;
		} else {
			LED1_GPIO->DATA &= ~LED1;
		}
	};
}

void SysTick_Handler()
{
	systick_count++;

	/* simple round robin scheduler */
	switch (curr_task) {
		case(0): next_task=1; break;
		case(1): next_task=0; break;
		default: next_task=0; break;
	}
	next_thread = threads[next_task];
	if (curr_task != next_task) { /* context switch needed */
		SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk; /* set PendSV to pending */
	}
}
