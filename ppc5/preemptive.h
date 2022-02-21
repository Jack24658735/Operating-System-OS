#ifndef __PREEMPTIVE_H__
#define __PREEMPTIVE_H__

#define MAXTHREADS 4  /* not including the scheduler */
/* the scheduler does not take up a thread of its own */
#define SMALL_CNT_LIMIT 12
#define LARGE_CNT_LIMIT 99

__data __at (0x20) unsigned char time_array[MAXTHREADS];
__data __at (0x24) unsigned char time_cnt;
__data __at (0x25) unsigned char curr_time;
// move current thread ID declaration here for testparking.c usage
__data __at(0x34) char cur_thread_id;


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

// for delay and now
#define delay(n) \
        time_array[cur_thread_id] = curr_time + n; \
        while (curr_time != time_array[cur_thread_id]) { \
        }
#define now(void) \
    Now() \
    
unsigned char Now(void);

#endif // __PREEMPTIVE_H__