#include <8051.h>

#include "preemptive.h"

__data __at(0x30) char SP_saved[MAXTHREADS];

// move thread id definition to .h file (so testparking.c could use)

__data __at(0x35) char thread_cnt;
__data __at(0x36) char bit_mask; 
__data __at(0x37) char i;
__data __at(0x38) char tmp_val; // for sp tmp value
__data __at(0x39) char new_thread_id; // for thread creation
__data __at(0x3a) char next_thread_id;
__data __at(0x3b) char loc;

__data __at(0x2d) char check_valid;
__data __at(0x2e) char is_valid;


// implement SAVESTATE
#define SAVESTATE \
        { __asm \
            PUSH ACC\
            PUSH B\
            PUSH DPL\
            PUSH DPH\
            PUSH PSW\
         __endasm; \
        SP_saved[cur_thread_id] = SP;\
        }


// implement RESTORESTATE
#define RESTORESTATE \
        { \
            SP = SP_saved[cur_thread_id];\
            __asm \
                POP PSW\
                POP DPH\
                POP DPL\
                POP B\
                POP ACC\
            __endasm; \
        }


 /* 
  * we declare main() as an extern so we can reference its symbol
  * when creating a thread for it.
  */

extern void main(void);

void Bootstrap(void) {
      
      // init some necessary parameters
      
      EA = 0; // turn off interrupt
      thread_cnt = 0;
      bit_mask = 0;
      check_valid = 0;
      SP_saved[0] = 0x3f;
      SP_saved[1] = 0x4f;
      SP_saved[2] = 0x5f;
      SP_saved[3] = 0x6f;

      TMOD = 0; // timer mode 0
      IE = 0x82; // enable timer 0 interrupt
      TR0 = 1; // kick out running timer 0
      PSW = 0;

      cur_thread_id = ThreadCreate(&main);

      RESTORESTATE;
      EA = 1; // turn on interrupt
}

/*
 * ThreadCreate() creates a thread data structure so it is ready
 * to be restored (context switched in).
 * The function pointer itself should take no argument and should
 * return no argument.
 */
ThreadID ThreadCreate(FunctionPtr fp) {

        EA = 0;
        
        // if no valid => enable interrupt, and return -1
        if (bit_mask == 15) {   
            EA = 1;
            return -1;
        }
        // find a thread ID
        // thread ID is one of 0001, 0011, 0111, 1111
        // part a, b, c
        for (i = 0; i < MAXTHREADS; ++i) {
            if ((bit_mask & (1 << i)) == 0) {
                bit_mask |= (1 << i);
                thread_cnt++;
                new_thread_id = i;
                break;
            } // means a thread ID found
        }
        tmp_val = SP;
        loc = PSW;
       
        SP = SP_saved[new_thread_id];
        // PSW: 0x00, 0x08, 0x10, 0x18
        PSW = new_thread_id << 3;

        // part d
        __asm
            PUSH DPL
            PUSH DPH
        __endasm;

        // part e
        __asm
            MOV A, #0
            PUSH ACC
            PUSH ACC
            PUSH ACC
            PUSH ACC
        __endasm;

        // part f
        __asm
            PUSH PSW
        __endasm;

        // part g
        SP_saved[new_thread_id] = SP;
        // part h
        SP = tmp_val;
        PSW = loc;
        // part i

        EA = 1; // turn on interrupt

        return new_thread_id;
}

void myTimer0Handler(void) {
    
    EA = 0;
    SAVESTATE;

    __asm
        MOV B, R0
        PUSH B
        MOV B, R1
        PUSH B
        MOV B, R2
        PUSH B
        MOV B, R3
        PUSH B
        MOV B, R4
        PUSH B
        MOV B, R5
        PUSH B
        MOV B, R6
        PUSH B
        MOV B, R7
        PUSH B
    __endasm;

    // count time!
    if (time_cnt == SMALL_CNT_LIMIT - 1) {
        time_cnt = 0;
        // if the time is over 99 => set back to 0
        // since I want to output last 2 digits (i.e. up to 99)
        if (curr_time >= LARGE_CNT_LIMIT) {
            curr_time = 0;
        }
        else {
            curr_time++;
        }
    }
    else {
        time_cnt++;
    }

    i = cur_thread_id; // save for temporary use
    // preemptive part
    do {
        // thread 0~3 in turn by round robin
        // if it reaches thread 3, then turn back to thread 0 
        if (i == 3) {
            i = 0;
            check_valid = 0x01;
        }
        else if (i == 0) {
            i = 1;
            check_valid = 0x02;
        }
        else if (i == 1) {
            i = 2;
            check_valid = 0x04;
        }
        else if (i == 2) {
            i = 3;
            check_valid = 0x08;
        }
        // from bit mask, find the corresponding thread
        if (bit_mask & check_valid)
            break;
    } while (1);
    
    cur_thread_id = i; // get next thread ID
    
    __asm
        POP B
        MOV R7, B
        POP B
        MOV R6, B
        POP B
        MOV R5, B
        POP B
        MOV R4, B
        POP B
        MOV R3, B
        POP B
        MOV R2, B
        POP B
        MOV R1, B
        POP B
        MOV R0, B
    __endasm;  

    RESTORESTATE;
    EA = 1;
    __asm
        RETI
    __endasm;
}

//// NO real usage in this checkpoint
/*
 * this is called by a running thread to yield control to another
 * thread.  ThreadYield() saves the context of the current
 * running thread, picks another thread (and set the current thread
 * ID to it), if any, and then restores its state.
 */

void ThreadYield(void) {
       EA = 0;
       SAVESTATE;
       do {
            // thread 0~3 in turn
            // if it reaches thread 3, then turn back to thread 0 
            if (cur_thread_id < 3)
                cur_thread_id++;
            else
                cur_thread_id = 0;
            
            // from bit mask, find the corresponding thread
            if (bit_mask & (1 << cur_thread_id))
                break;

        } while (1);
        RESTORESTATE;
        EA = 1;
}


/*
 * ThreadExit() is called by the thread's own code to terminate
 * itself.  It will never return; instead, it switches context
 * to another thread.
 */
void ThreadExit(void) {
        /*
         * clear the bit for the current thread from the
         * bit mask, decrement thread count (if any),
         * and set current thread to another valid ID.
         * Q: What happens if there are no more valid threads?
         */
        EA = 0;
        is_valid = 0;
        loc = 0x3f + (cur_thread_id << 4);
        thread_cnt--;
        
        bit_mask &= ~(1 << cur_thread_id);
        SP_saved[cur_thread_id] = loc;
        for (i = 0; i < MAXTHREADS; ++i) {

            if (cur_thread_id == 3) {
                next_thread_id = 0;
            }
            else if (cur_thread_id == 0) {
                next_thread_id = 1;
            }
            else if (cur_thread_id == 1) {
                next_thread_id = 2;
            }
            else if (cur_thread_id == 2) {
                next_thread_id = 3;
            }

            if (bit_mask & (1 << next_thread_id)) {
                is_valid = 1;
                break;
            }
        }
        cur_thread_id = next_thread_id;
        if (!is_valid) // no valid thread
            cur_thread_id = -1;
       
        RESTORESTATE;
        EA = 1;
}

unsigned char now(void) {
    return curr_time;
}



