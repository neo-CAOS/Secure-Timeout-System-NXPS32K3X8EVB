/*
 * FreeRTOS V202212.00
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * https://www.FreeRTOS.org
 * https://github.com/FreeRTOS
 *
 */

/* Peripheral includes */
#include "uart.h"
#include "IntTimer.h"
#include <stdio.h>

/* FreeRTOS interrupt handlers. */
extern void vPortSVCHandler( void );
extern void xPortPendSVHandler( void );
extern void xPortSysTickHandler( void );

/* Exception handlers. */
static void HardFault_Handler( void ) __attribute__( ( naked ) );
static void Default_Handler( void ) __attribute__( ( naked ) );
void Reset_Handler( void ) __attribute__( ( naked ) );

extern void vPortSetupMPU( void );

extern int main( void );

/* External declarations for script linker symbols */
extern uint32_t _estack;    /* End of stack address */
extern uint32_t _sidata;    /* FLASH address of .data */
extern uint32_t _sdata;     /* RAM address of .data */
extern uint32_t _edata;     /* End of .data in RAM */
extern uint32_t _sbss;      /* Start of .bss in RAM */
extern uint32_t _ebss;      /* End of .bss in RAM */

void Reset_Handler(void) 
{
    main();
}

/* Variables used to store the value of registers at the time a hardfault
 * occurs.  These are volatile to try and prevent the compiler/linker optimizing
 * them away as the variables never actually get used. */
volatile uint32_t r0;
volatile uint32_t r1;
volatile uint32_t r2;
volatile uint32_t r3;
volatile uint32_t r12;
volatile uint32_t lr;   /* Link register. */
volatile uint32_t pc;   /* Program counter. */
volatile uint32_t psr;  /* Program status register. */

/* Called from the hardfault handler to provide information on the processor
 * state at the time of the fault.
 */
__attribute__( ( used ) ) void prvGetRegistersFromStack( uint32_t *pulFaultStackAddress ) 
{
    r0  = pulFaultStackAddress[ 0 ];
    r1  = pulFaultStackAddress[ 1 ];
    r2  = pulFaultStackAddress[ 2 ];
    r3  = pulFaultStackAddress[ 3 ];

    r12 = pulFaultStackAddress[ 4 ];
    lr  = pulFaultStackAddress[ 5 ];
    pc  = pulFaultStackAddress[ 6 ];
    psr = pulFaultStackAddress[ 7 ];

    ////////////////////////////////////////////////////////////////////////////

    char buffer[100];

    UART_printf("\nHard Fault Handler:\n");
    sprintf(buffer,"R0   = 0x%08lX\n", r0);
    UART_printf(buffer);
    sprintf(buffer,"R1   = 0x%08lX\n", r1);
    UART_printf(buffer);
    sprintf(buffer,"R2   = 0x%08lX\n", r2);
    UART_printf(buffer);
    sprintf(buffer,"R3   = 0x%08lX\n", r3);
    UART_printf(buffer);
    sprintf(buffer,"R12  = 0x%08lX\n", r12);
    UART_printf(buffer);
    sprintf(buffer,"LR   = 0x%08lX\n", lr);
    UART_printf(buffer);
    sprintf(buffer,"PC   = 0x%08lX\n", pc);
    UART_printf(buffer);
    sprintf(buffer,"PSR  = 0x%08lX\n", psr);
    UART_printf(buffer);
    
    UART_printf("*** MPU FAULT DETECTED ***\n"); // Add this line

    for (;;);
}

void Default_Handler( void ) 
{
    __asm volatile
    (
        ".align 8                                \n"
        " ldr r3, =0xe000ed04                    \n" /* Load the address of the interrupt control register into r3. */
        " ldr r2, [r3, #0]                       \n" /* Load the value of the interrupt control register into r2. */
        " uxtb r2, r2                            \n" /* The interrupt number is in the least significant byte - clear all other bits. */
        "Infinite_Loop:                          \n" /* Sit in an infinite loop - the number of the executing interrupt is held in r2. */
        " b  Infinite_Loop                       \n"
        " .ltorg                                 \n"
    );
}

// void HardFault_Handler(void) {
//     uint32_t cfsr = (uint32_t)0xE000ED28;
//     if (cfsr & (1 << 7)) {  // Bit MMARVALID
//         uint32_t fault_address =(uint32_t *)0xE000ED34;
//         // UART_printf("MPU Fault at address: 0x%08lX\n", fault_address);
//     }
//     while(1);
// }

void HardFault_Handler( void ) 
{
    __asm volatile
    (
        ".align 8                                                   \n"
        " tst lr, #4                                                \n"
        " ite eq                                                    \n"
        " mrseq r0, msp                                             \n"
        " mrsne r0, psp                                             \n"
        " ldr r1, [r0, #24]                                         \n"
        " ldr r2, =prvGetRegistersFromStack                         \n"
        " bx r2                                                     \n"
        " .ltorg                                                    \n"
    );
}

/* Vector table. */
const uint32_t* isr_vector[] __attribute__((section(".isr_vector"), used)) = {

    /* For every interrupt we have got the name of the interrupt. When you power one the microprocessor, you need to know that the table is in a given location in the memory. */
    /* Every element in this table, corresponds to a given interrupt, this is based on the data sheet of the board, in particular all the zeros means that i do not have any routine associated to that given interrupt. You will jump to a null pointer in that case. If you look at the routines, there are a lot of routines that handle the low level stuffs, hardfaults etc. */
    
    ( uint32_t * ) &_estack,
    ( uint32_t * ) &Reset_Handler,       // Reset                -15
    ( uint32_t * ) &Default_Handler,     // NMI_Handler          -14
    ( uint32_t * ) &HardFault_Handler,   // HardFault_Handler    -13
    ( uint32_t * ) &Default_Handler,     // MemManage_Handler    -12
    ( uint32_t * ) &Default_Handler,     // BusFault_Handler     -11
    ( uint32_t * ) &Default_Handler,     // UsageFault_Handler   -10
    0, // reserved   -9
    0, // reserved   -8
    0, // reserved   -7
    0, // reserved   -6
    ( uint32_t * ) &vPortSVCHandler,     // SVC_Handler          -5
    ( uint32_t * ) &Default_Handler,     // DebugMon_Handler     -4
    0, // reserved   -3
    ( uint32_t * ) &xPortPendSVHandler,  // PendSV handler       -2
    ( uint32_t * ) &xPortSysTickHandler, // SysTick_Handler      -1
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    /* We set 3 handlers for the 3 available timers, if that timer generates an interrupt, the program will handle the interrupt with the handler that we have defined. */
    ( uint32_t * ) TIMER0_IRQHandler,    // Timer 0               8  
    ( uint32_t * ) TIMER1_IRQHandler,    // Timer 1               9
    ( uint32_t * ) TIMER2_IRQHandler,    // Timer 2               10
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0
};
