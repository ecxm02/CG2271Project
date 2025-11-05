// ****************************************************************************// Simple hard fault handler that captures stack context and halts execution.

// semihost_hardfault.c

//                - Provides hard fault handler to allow semihosting code not#include <stdint.h>

//                  to hang application when debugger not connected.

//volatile uint32_t g_hardFaultStackPointer = 0u;

// ****************************************************************************volatile uint32_t g_hardFaultProgramCounter = 0u;

// Copyright 2017-2020 NXPvolatile uint32_t g_hardFaultExceptionReturn = 0u;

// All rights reserved.

//void HardFault_HandlerC(uint32_t *stack, uint32_t excReturn);

// SPDX-License-Identifier: BSD-3-Clause

// ****************************************************************************__attribute__((naked)) void HardFault_Handler(void)

{

#if defined (__cplusplus)    __asm volatile(

extern "C" {        "tst lr, #4\n"

#endif        "ite eq\n"

        "mrseq r0, msp\n"

// Allow handler to be removed by setting a define (via command line)        "mrsne r0, psp\n"

#if !defined (__SEMIHOST_HARDFAULT_DISABLE)        "mov r1, lr\n"

        "b HardFault_HandlerC\n"

__attribute__((naked))    );

void HardFault_Handler(void){}

    __asm(  ".syntax unified\n"

        // Check which stack is in usevoid HardFault_HandlerC(uint32_t *stack, uint32_t excReturn)

            "MOVS   R0, #4  \n"{

            "MOV    R1, LR  \n"    g_hardFaultStackPointer = (uint32_t)stack;

            "TST    R0, R1  \n"    g_hardFaultProgramCounter = stack[6];

            "BEQ    _MSP    \n"    g_hardFaultExceptionReturn = excReturn;

            "MRS    R0, PSP \n"

            "B      _process\n"    __asm volatile("bkpt #0");

            "_MSP:  \n"

            "MRS    R0, MSP \n"    for (;;) {

        // Load the instruction that triggered hard fault        __asm volatile("nop");

        "_process:     \n"    }

            "LDR    R1,[R0,#24] \n"}

        // Semihosting instruction is "BKPT 0xAB" (0xBEAB)

            "LDR    R2,=0xBEAB \n"
            "LDRH   R3,[r1] \n"
            "CMP    R2,R3 \n"
            "BEQ    _semihost_return \n"
        // Wasn't semihosting instruction so enter infinite loop
            "B . \n"
        // Was semihosting instruction, so adjust location to
        // return to by 2 bytes.
        "_semihost_return: \n"
            "ADDS    R1,#2 \n"
            "STR    R1,[R0,#24] \n"
        // And return
            "BX LR \n"
        ".syntax divided\n") ;
}

#endif

#if defined (__cplusplus)
}
#endif
