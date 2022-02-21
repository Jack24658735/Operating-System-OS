

#ifndef __PREEMPTIVE_H__
#define __PREEMPTIVE_H__

#define MAXTHREADS 4  /* not including the scheduler */
/* the scheduler does not take up a thread of its own */

#define CNAME(s) _ ## s

#define L(x) LABEL(x)
#define LABEL(x) x ## $

// counting semaphore s (which is initialized to n)
#define SemaphoreCreate(s, n) \
        s = n \

#define SemaphoreSignal(s) \
    { \
        __asm \
            INC CNAME(s) \
        __endasm; \
    }

// do busy wait on semaphore s
#define SemaphoreWaitBody(s, label) \
        { \
            __asm \
            label: \
                MOV A, CNAME(s) \
                JZ label \
                JB ACC.7, label \
                DEC CNAME(s) \
            __endasm; \
        }

// which generates a new integer literal each time it is used
#define SemaphoreWait(s) \
        SemaphoreWaitBody(s, L(__COUNTER__)) \

typedef char ThreadID;
typedef void (*FunctionPtr)(void);

ThreadID ThreadCreate(FunctionPtr);
void ThreadYield(void);
void ThreadExit(void);
// declare my timer ISR
void myTimer0Handler();

#endif // __PREEMPTIVE_H__