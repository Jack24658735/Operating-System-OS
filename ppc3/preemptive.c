#include <8051.h>

#include "preemptive.h"

__data __at(0x30) char SP_saved[MAXTHREADS];
__data __at(0x34) char cur_thread_id;
__data __at(0x35) char thread_cnt;
__data __at(0x36) char bit_mask;
__data __at(0x37) char i;
__data __at(0x38) char tmp_val;
__data __at(0x39) char new_thread_id;
__data __at(0x3a) char next_thread_id;
__data __at(0x3b) char loc;




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
        
        if (thread_cnt >= MAXTHREADS) {
            
            EA = 1;
            
            return -1;
        }
        // find a thread ID
        // thread ID is one of 0001, 0011, 0111, 1111
        // part a, b, c
        for (i = 0; i < 4; ++i) {
            if ((bit_mask & (1 << i)) == 0) {
                bit_mask |= (1 << i);
                thread_cnt++;
                new_thread_id = i;
                tmp_val = SP;
                // SP = (new_thread_id == 0) ? 0x3F: 0x3F + (1 << (new_thread_id + 3));
                SP = SP_saved[i];
                // i == 0: 0x3F
                // i == 1: 0x3F + 1<<4 = 0x4F
                // i == 2: 0x3F + 1<<5 = 0x5F
                // i == 3: 0x3F + 1<<6 = 0x6F
                break;
            } // means a thread ID found
        }
        loc = PSW;
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
        PSW = new_thread_id << 3;
        __asm
            PUSH PSW
        __endasm;

        // part g
        SP_saved[new_thread_id] = SP;
        // part h
        SP = tmp_val;
        PSW = loc;
        // part i

        EA = 1; // turn on

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


    // preemptive part
    do {
            
        // thread 0~3 in turn
        // if it reaches thread 3, then turn back to thread 0 
        if (cur_thread_id < 3)
            cur_thread_id++;
        else
            cur_thread_id = 0;
        
        if (cur_thread_id == 0)
            next_thread_id = 1;
        else if (cur_thread_id == 1)
            next_thread_id = 2;
        else if (cur_thread_id == 2)
            next_thread_id = 4;
        else if (cur_thread_id == 3)
            next_thread_id = 8;
        
        // from bit mask, find the corresponding thread
        if (bit_mask & next_thread_id)
            break;

    } while (1);
   

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
        bit_mask &= (~(1 << cur_thread_id)); // clear bit mask
        thread_cnt--;
        for (i = 0; i < 4; ++i) {
            if (bit_mask & (1 << i)) {
                cur_thread_id = i;
                break;
            }
        }
        // no valid thread..
        if (i > 3)
            cur_thread_id = -1;
        RESTORESTATE;
        EA = 1;
}


