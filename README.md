# Operating-System-OS
* Course Number: CS 342302
* Year: 2021-Fall
> This course introduces the concepts over Operating-System (OS). There are totally 5 project checkpoints, and they illstrates the details about cooperative/preemptive threading. They consists of C and some assembly language implementation, and they're run on EdSim51 simulator to validate the correctness.

## ppc1
- Cooperative-multithreading implementation (e.g. **Thread-yield** mechanism)
## ppc2
- Preemptive-multithreading implementation
  - Single-buffer producer-consumer example
## ppc3
- Semaphore with busy wait for preemptive multithreading 
  - Classical bounded-buffer example
## ppc4
- Extend the test cases (two producers will compete with each other for writing) 
  - Discussion about fairness
## ppc5 (Final Project)
- Parking lot example 
  - Add a delay(n) function to preemptive multithreading and semaphore code.
  - Ensure the threads can terminate, so its space can be recycled by another thread.
  - Complete implementation about the parking lot example.
