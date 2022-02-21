/* 
 * file: testcoop.c 
 */
#include <8051.h>
#include "cooperative.h"

#define BUFFER_SIZE 10
/* 
 * @@@ [2pt] 
 * declare your global variables here, for the shared buffer 
 * between the producer and consumer.  
 * Hint: you may want to manually designate the location for the 
 * variable.  you can use
 *        __data __at (0x30) type var; 
 * to declare a variable var of the type
 */ 
__data __at (0x20) char in_p; // in pointer
__data __at (0x21) char out_p; // out pointer
__data __at (0x22) char idx; 
__data __at (0x23) char j; 

__data __at (0x24) char next_item; 
__data __at (0x25) char buffer[BUFFER_SIZE];



/* [8 pts] for this function
 * the producer in this test program generates one characters at a
 * time from 'A' to 'Z' and starts from 'A' again. The shared buffer
 * must be empty in order for the Producer to write.
 */
void Producer(void) {
        /*
         * @@@ [2 pt]
         * initialize producer data structure, and then enter
         * an infinite loop (does not return)
         */
        next_item = 'A';
        in_p = 0;
        idx = 0;
        
        while (1) {
                /* @@@ [6 pt]
                 * wait for the buffer to be available, 
                 * and then write the new data into the buffer */
                idx = (in_p + 1) % BUFFER_SIZE;
                while (idx == out_p) {
                    // yield
                    ThreadYield();
                }
                buffer[in_p] = next_item;
                in_p = (in_p + 1) % BUFFER_SIZE;
                
                if (next_item != 'Z')
                    next_item = next_item + 1;
                else
                    next_item = 'A';
        
        }       
}

/* [10 pts for this function]
 * the consumer in this test program gets the next item from
 * the queue and consume it and writes it to the serial port.
 * The Consumer also does not return.
 */
void Consumer(void) {
        /* @@@ [2 pt] initialize Tx for polling */
        

        TI = 1; 
        out_p = 0;
        
        while (1) {
                /* @@@ [2 pt] wait for new data from producer
                 * @@@ [6 pt] write data to serial port Tx, 
                 * poll for Tx to finish writing (TI),
                 * then clear the flag
                 */
                while (in_p == out_p) {
                    ThreadYield();
                }
                while (TI == 0) {} // wait UART ready
                
                // TI = 1 means UART is OK
                SBUF = buffer[out_p];
                out_p = (out_p + 1) % BUFFER_SIZE;
                TI = 0; // clear the flag
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
        TMOD = 0x20; // send
        TH1 = -6; // 4800 baudrate
        SCON = 0x50; // 8-bit 1 stop REN
        TR1 = 1; // start timer 1
        in_p = 0;
        out_p = 0;
        for (j = 0; j < BUFFER_SIZE; ++j) {
            buffer[j] = 0;
        }
        // next_item = 'A';
        ThreadCreate(&Producer);
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
