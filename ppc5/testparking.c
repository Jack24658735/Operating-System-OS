/* 
 * file: testparking.c 
 */
#include <8051.h>
#include "preemptive.h"


__data __at(0x26) char mutex;
__data __at(0x27) char lock_lot;
__data __at(0x28) char lock_car;

__data __at(0x29) char car_val;
__data __at(0x2a) char car_val_tmp;

// init 2 positions
__data __at(0x2b) char spot1;
__data __at(0x2c) char spot2;

__data __at(0x3c) char cars[MAXTHREADS]; 
// 3c: thread 0, 3d: thread 1, ... and so on

// function header for output human-readable texts
void OutputUART(char, char, char); 

void ProducerLot(void) {

    // check if there exists a parking lot
    SemaphoreWait(lock_lot);
    SemaphoreWait(mutex);
    EA = 0; // for critical section

    if (spot1 == 'O') {
        spot1 = cars[cur_thread_id];
        OutputUART(spot1, '1', 'I');
    }
    else if (spot2 == 'O' && spot1 != 'O') {
        spot2 = cars[cur_thread_id];
        OutputUART(spot2, '2', 'I');
    }

    EA = 1; // for critical section
    SemaphoreSignal(mutex);
    
    // delay 4 time cnts after my trial and error
    delay(4);

    SemaphoreWait(mutex);
    EA = 0; // for critical section

    if (spot1 == cars[cur_thread_id] && spot2 != cars[cur_thread_id]) {
        spot1 = 'O';
        OutputUART(cars[cur_thread_id], '1', 'O');
    }
    else if (spot2 == cars[cur_thread_id] && spot1 != cars[cur_thread_id]) {
        spot2 = 'O';
        OutputUART(cars[cur_thread_id], '2', 'O');
    }

    EA = 1; // for critical section
    SemaphoreSignal(mutex);
    SemaphoreSignal(lock_lot);
    SemaphoreSignal(lock_car);
    // thread termination
    ThreadExit();
}


void ProducerCar(void) {

    // initial car value
    car_val = '1';

    // keep creating cars
    for (;;) {
        SemaphoreWait(lock_car);
        car_val_tmp = ThreadCreate(&ProducerLot);
        cars[car_val_tmp] = car_val;
        car_val = (car_val == '5') ? '1' : car_val + 1;
        delay(1);
    }      
}

void main(void) {
    TMOD |= 0x20; // send
    TH1 = -6; // 4800 baudrate
    SCON = 0x50; // 8-bit 1 stop REN
    TR1 = 1; // start timer 1
    
    // initial the value of semaphores
    SemaphoreCreate(mutex, 1);
	SemaphoreCreate(lock_lot, 2);
	SemaphoreCreate(lock_car, 3);

    spot1 = 'O';
    spot2 = 'O';
    ProducerCar(); // this func. will create thread for cars
}

void OutputUART(char car, char spot, char in_out) {
    
    SBUF = '0' + (curr_time / 10);
    while(TI == 0) {}
    TI = 0;
    SBUF = '0' + (curr_time % 10);
    while(TI == 0) {}
    TI = 0;

    SBUF = '%';
    while(TI == 0) {}
    TI = 0;

    SBUF = car;
    while(TI == 0) {}
    TI = 0;

    if (in_out == 'I') {
        SBUF = 'i';
        while(TI == 0) {}
        TI = 0;
    }
    else if (in_out == 'O') {
        SBUF = 'o';
        while(TI == 0) {}
        TI = 0;
    }
    SBUF = spot;
    while(TI == 0) {}
    TI = 0;
    SBUF = '\n';
    while(TI == 0) {}
    TI = 0;

}

void _sdcc_gsinit_startup(void){
	__asm 
		ljmp _Bootstrap
	__endasm;
}

void _mcs51_genRAMCLEAR(void) {}
void _mcs51_genXINIT(void) {}
void _mcs51_genXRAMCLEAR(void) {}

void timer0_ISR(void) __interrupt(1){
	__asm 
		ljmp _myTimer0Handler
	__endasm;
}