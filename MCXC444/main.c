/*
 * Copyright 2016-2025 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file    timer_demo.c
 * @brief   Application entry point.
 */
#include <stdio.h>
#include "board.h"
#include "peripherals.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "fsl_debug_console.h"
#include <math.h>
/* TODO: insert other include files here. */

/* TODO: insert other definitions and declarations here. */

// LED pin numbers
#define REDLED		4 //PTA4
#define GREENLED	4 //PTD4
#define BLUELED		5 //PTA5
#define LDRPIN      2 //PTC2
#define SOILPIN     1 //PTA1
#define PUMPPIN		2 //PTA2

volatile bool relayState = false;  // Track current relay state (false = off, true = on)
volatile int lastSoilState = 0;    // Track last soil moisture state


void initLEDs() {
	SIM->SCGC5 |= (SIM_SCGC5_PORTA_MASK | SIM_SCGC5_PORTD_MASK);
	PORTA->PCR[REDLED] &= ~PORT_PCR_MUX_MASK;
	PORTA->PCR[REDLED] |= PORT_PCR_MUX(1);

	PORTA->PCR[BLUELED] &= ~PORT_PCR_MUX_MASK;
	PORTA->PCR[BLUELED] |= PORT_PCR_MUX(1);

	PORTD->PCR[GREENLED] &= PORT_PCR_MUX_MASK;
	PORTD->PCR[GREENLED] |= PORT_PCR_MUX(1);

	// Set PDDR
	GPIOA->PDDR |= ((1 << REDLED) | (1 << BLUELED));
	GPIOD->PDDR |= (1 << GREENLED);
}

// Switch off all LEDs
void ledOff() {
	GPIOA->PCOR |= ((1 << REDLED) | (1 << BLUELED));
	GPIOD->PCOR |= (1 << GREENLED);
}

// Switch on all LEDs
void ledOn() {
	GPIOA->PSOR |= ((1 << REDLED) | (1 << BLUELED));
	GPIOD->PSOR |= (1 << GREENLED);
}

// Initialize interrupt for LDR (Light Dependent Resistor)
void initLDR() {
    SIM->SCGC5 |= SIM_SCGC5_PORTC_MASK;

    // Configure as GPIO
    PORTC->PCR[LDRPIN] &= ~PORT_PCR_MUX_MASK;
    PORTC->PCR[LDRPIN] |= PORT_PCR_MUX(1);

    // Enable pull-up resistor
    PORTC->PCR[LDRPIN] |= PORT_PCR_PE_MASK;
    PORTC->PCR[LDRPIN] |= PORT_PCR_PS_MASK;

    // Set as input
    GPIOC->PDDR &= ~(1 << LDRPIN);
}



void initPump(){
	SIM->SCGC5 |= (SIM_SCGC5_PORTA_MASK);
	PORTA->PCR[PUMPPIN] &= ~PORT_PCR_MUX_MASK;
	PORTA->PCR[PUMPPIN] |= PORT_PCR_MUX(1);
	GPIOA->PDDR |= (1 << PUMPPIN);
}

void pumpOff(){
	GPIOA->PCOR |= (1 << PUMPPIN);
}

void pumpOn(){
	GPIOA->PSOR |= (1 << PUMPPIN);
}

void initSOILInterrupt() {
    // Disable PORTE_D Interrupts
    NVIC_DisableIRQ(PORTA_IRQn);

    // LDRPIN is at PTC3. Enable port C
    SIM->SCGC5 |= SIM_SCGC5_PORTA_MASK;

    // Configure as GPIO
    PORTA->PCR[SOILPIN] &= ~PORT_PCR_MUX_MASK;
    PORTA->PCR[SOILPIN] |= PORT_PCR_MUX(1);

    // Configure pullup
    PORTA->PCR[SOILPIN] &= ~PORT_PCR_PE_MASK;
    PORTA->PCR[SOILPIN] |= PORT_PCR_PE(1);
    PORTA->PCR[SOILPIN] &= ~PORT_PCR_PS_MASK;
    PORTA->PCR[SOILPIN] |= PORT_PCR_PS(1);

    // Configure as input
    GPIOA->PDDR &= ~(1 << SOILPIN);

    PORTA->PCR[SOILPIN] &= ~PORT_PCR_PE_MASK;
    PORTA->PCR[SOILPIN] |= PORT_PCR_PE(1);

    PORTA->PCR[SOILPIN] &= ~PORT_PCR_IRQC_MASK;
    PORTA->PCR[SOILPIN] |= PORT_PCR_IRQC(0b1011);

    // Set lowest priority
    NVIC_SetPriority(PORTA_IRQn, 192);

    // Clear pending interrupts
    NVIC_ClearPendingIRQ(PORTA_IRQn);

    // Enable interrupts
    NVIC_EnableIRQ(PORTA_IRQn);
}


// Polling for LDR (light level)
void pollLDR() {
    // Read the LDR sensor state (GPIO input read)
    int ldrState = GPIOC->PDIR & (1 << LDRPIN);  // Read pin value for LDR

    // If the LDR state is not 0 (not dark), turn LEDs on
    if (ldrState != 0) {
        ledOn();  // Turn the LEDs on
    } else {
        ledOff();  // Turn the LEDs off when it's dark
    }
}

void PORTA_IRQHandler() {
	NVIC_ClearPendingIRQ(PORTA_IRQn);

	if (PORTA->ISFR & (1 << SOILPIN)) {
		// Read the current soil moisture level
		int currentSoilState = GPIOA->PDIR & (1 << SOILPIN);

		// Check if the soil state has changed (from dry to wet or vice versa)
		if (currentSoilState != lastSoilState) {
			// Toggle the relay if the state changes
			relayState = !relayState;

			// Toggle pump based on relay state
			if (relayState) {
				pumpOn();  // Turn on the water pump
				PRINTF("Water pump ON\r\n");
			} else {
				pumpOff(); // Turn off the water pump
				PRINTF("Water pump OFF\r\n");
			}

			// Update the last soil state
			lastSoilState = currentSoilState;
		}
	}

	PORTA->ISFR |= (1 << SOILPIN);
}


int main(void) {

    /* Init board hardware. */
    BOARD_InitBootPins();
    BOARD_InitBootClocks();
    BOARD_InitBootPeripherals();
#ifndef BOARD_INIT_DEBUG_CONSOLE_PERIPHERAL
    /* Init FSL debug console. */
    BOARD_InitDebugConsole();
#endif

    // Initialize LEDs
    initLEDs();

    // Switch off LEDs
    ledOff();

    initPump();

    pumpOff();

    initLDR();

    // Initialize SW2 and SW3 interrupt

    initSOILInterrupt();

    /* Force the counter to be placed into memory. */
    /* Enter an infinite loop, just incrementing a counter. */
    while(1) {
        // Poll the LDR (light sensor) to control the LEDs
        pollLDR();

        // Optionally, add a small delay here for polling rate control
        // (not needed in this case, unless you want to slow down polling)
    }
    return 0 ;
}
