/* 
 * file: testpreemptive.c 
 */
#include <8051.h>
#include "preemptive.h"

#define BUFFER_SIZE 3
/* 
 * @@@ [2pt] 
 * declare your global variables here, for the shared buffer 
 * between the producer and consumer.  
 * Hint: you may want to manually designate the location for the 
 * variable.  you can use
 *        __data __at (0x30) type var; 
 * to declare a variable var of the type
 */ 

// declare 3-bounded buffer
__data __at (0x20) char in_p; // in pointer
__data __at (0x21) char out_p; // out pointer
__data __at (0x22) char j; 

// __data __at (0x22) char idx; 

__data __at (0x24) char next_item; 
__data __at (0x25) char bounded_buffer[BUFFER_SIZE];
__data __at (0x28) char next_item_2; 

// declare vars. for producer semaphore
__data __at (0x2a) char producer_1; 
__data __at (0x2b) char producer_2; 



// declare vars. for semaphore
__data __at (0x3D) char mutex; 
__data __at (0x3E) char empty; 
__data __at (0x3F) char full; 




/* [8 pts] for this function
 * the producer in this test program generates one characters at a
 * time from 'A' to 'Z' and starts from 'A' again. The shared buffer
 * must be empty in order for the Producer to write.
 */
void Producer1(void) {
        /*
         * @@@ [2 pt]
         * initialize producer data structure, and then enter
         * an infinite loop (does not return)
         */
        next_item = 'A';
        
        while (1) {
                /* @@@ [6 pt]
                 * wait for the buffer to be available, 
                 * and then write the new data into the buffer */
                SemaphoreWait(producer_1);
                SemaphoreWait(empty);
                SemaphoreWait(mutex);
                // idx = (in_p + 1) % BUFFER_SIZE;
                // while (idx == out_p) {
                //     // yield
                //     // ThreadYield();
                // }
                bounded_buffer[in_p] = next_item;
                in_p = (in_p + 1) % BUFFER_SIZE;
                
                if (next_item != 'Z')
                    next_item = next_item + 1;
                else
                    next_item = 'A';
                SemaphoreSignal(mutex);
                SemaphoreSignal(full);
                SemaphoreSignal(producer_2);
        
        }       
}

void Producer2(void) {
        /*
         * @@@ [2 pt]
         * initialize producer data structure, and then enter
         * an infinite loop (does not return)
         */
        next_item_2 = '0';
        
        while (1) {
                /* @@@ [6 pt]
                 * wait for the buffer to be available, 
                 * and then write the new data into the buffer */
                
                SemaphoreWait(producer_2);
                SemaphoreWait(empty);
                SemaphoreWait(mutex);
                // idx = (in_p + 1) % BUFFER_SIZE;
                // while (idx == out_p) {
                //     // yield
                //     // ThreadYield();
                // }
                bounded_buffer[in_p] = next_item_2;
                in_p = (in_p + 1) % BUFFER_SIZE;
                
                if (next_item_2 != '9')
                    next_item_2 = next_item_2 + 1;
                else
                    next_item_2 = '0';
                SemaphoreSignal(mutex);
                SemaphoreSignal(full);
                SemaphoreSignal(producer_1);
        
        }       
}


/* [10 pts for this function]
 * the consumer in this test program gets the next item from
 * the queue and consume it and writes it to the serial port.
 * The Consumer also does not return.
 */
void Consumer(void) {
        /* @@@ [2 pt] initialize Tx for polling */

        
        while (1) {
                /* @@@ [2 pt] wait for new data from producer
                 * @@@ [6 pt] write data to serial port Tx, 
                 * poll for Tx to finish writing (TI),
                 * then clear the flag
                 */
                // while (in_p == out_p) {
                //     // ThreadYield();
                // }
                SemaphoreWait(full);
                SemaphoreWait(mutex);
                SBUF = bounded_buffer[out_p];
                out_p = (out_p + 1) % BUFFER_SIZE;

                // TI = 1 means UART is OK
                while (TI == 0) {} // wait UART ready
                TI = 0; // clear the flag
                SemaphoreSignal(mutex);
                SemaphoreSignal(empty);
                
        }
}

/* [5 pts for this function]
 * main() is started by the thread bootstrapper as thread-0.
 * It can create more thread(s) as needed:
 * one thread can act as producer and another as consumer.
 */
void main(void) {
          /* 
           * @@@ [1 pt] initialize globals 
           * @@@ [4 pt] set up Producer and Consumer.
           * Because both are infinite loops, there is no loop
           * in this function and no return.
           */
        
        TMOD |= 0x20; // send
        TH1 = -6; // 4800 baudrate
        SCON = 0x50; // 8-bit 1 stop REN
        TR1 = 1; // start timer 1

        SemaphoreCreate(mutex, 1);
		SemaphoreCreate(full, 0);
		SemaphoreCreate(empty, 3);
		SemaphoreCreate(producer_1, 1);
		SemaphoreCreate(producer_2, 0);
        in_p = 0;
        out_p = 0;
        for (j = 0; j < BUFFER_SIZE; ++j) {
            bounded_buffer[j] = 0;
        }
        ThreadCreate(&Producer1);
        ThreadCreate(&Producer2);
        Consumer();
}

void _sdcc_gsinit_startup(void) {
        __asm
                ljmp  _Bootstrap
        __endasm;
}

void _mcs51_genRAMCLEAR(void) {}
void _mcs51_genXINIT(void) {}
void _mcs51_genXRAMCLEAR(void) {}
void timer0_ISR(void) __interrupt(1) {
        __asm
            ljmp _myTimer0Handler
        __endasm;
}