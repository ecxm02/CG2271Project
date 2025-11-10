/*
 * Copyright 2016-2025 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>

#include "board.h"
#include "peripherals.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "fsl_debug_console.h"

/* FreeRTOS */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "queue.h"

/* ==== Pins ==== */
#define REDLED      4   // PTA4
#define GREENLED    4   // PTD4
#define BLUELED     5   // PTA5
#define LDRPIN      2   // PTC2 (polled)
#define SOILPIN     1   // PTA1 (interrupt)
#define PUMPPIN     13   // PTA2 (relay drive)

#define LDR_POLL_MS          20
#define DEBOUNCE_MS          30
#define PUMP_MIN_HOLD_MS     500

#define BAUD_RATE 9600
#define UART_TX_PTE22 	22
#define UART_RX_PTE23 	23
#define UART2_INT_PRIO	128

#define MAX_MSG_LEN		256
char send_buffer[MAX_MSG_LEN];

#define QLEN	5
QueueHandle_t queue;
typedef struct tm {
	char message[MAX_MSG_LEN];
} TMessage;

/* ==== Globals ==== */
volatile bool relayState = false;   // false=OFF, true=ON
volatile bool lightState = false;
volatile int  lastSoilState = 0;    // last stable level at SOILPIN

/* Binary semaphore signaled by soil ISR */
static SemaphoreHandle_t gSoilSem = NULL;

/* ==== LED helpers ==== */
static void initLEDs(void) {
    SIM->SCGC5 |= (SIM_SCGC5_PORTA_MASK | SIM_SCGC5_PORTD_MASK);

    /* PTA4, PTA5 GPIO */
    PORTA->PCR[REDLED]  = (PORTA->PCR[REDLED]  & ~PORT_PCR_MUX_MASK)  | PORT_PCR_MUX(1);
    PORTA->PCR[BLUELED] = (PORTA->PCR[BLUELED] & ~PORT_PCR_MUX_MASK)  | PORT_PCR_MUX(1);
    /* PTD4 GPIO */
    PORTD->PCR[GREENLED]= (PORTD->PCR[GREENLED]& ~PORT_PCR_MUX_MASK)  | PORT_PCR_MUX(1);

    /* Outputs */
    GPIOA->PDDR |= (1 << REDLED) | (1 << BLUELED);
    GPIOD->PDDR |= (1 << GREENLED);
}

static inline void ledOn(void) {
    GPIOA->PSOR = (1 << REDLED) | (1 << BLUELED);  // Set LED pins high
    GPIOD->PSOR = (1 << GREENLED);
    lightState = true;  // Update light state to ON
}

// Function to turn the LED off
static inline void ledOff(void) {
    GPIOA->PCOR = (1 << REDLED) | (1 << BLUELED);  // Set LED pins low
    GPIOD->PCOR = (1 << GREENLED);
    lightState = false;  // Update light state to OFF
}

/* ==== Pump / Relay helpers ==== */
static void initPump(void) {
    SIM->SCGC5 |= SIM_SCGC5_PORTA_MASK;
    PORTA->PCR[PUMPPIN] = (PORTA->PCR[PUMPPIN] & ~PORT_PCR_MUX_MASK) | PORT_PCR_MUX(1);
    GPIOA->PDDR |= (1 << PUMPPIN);
}

static inline void pumpOff(void) {
    GPIOA->PCOR = (1 << PUMPPIN);
    relayState = false;
}

static inline void pumpOn(void) {
    GPIOA->PSOR = (1 << PUMPPIN);
    relayState = true;
}

/* ==== LDR (polled) ==== */
static void initLDR(void) {
    SIM->SCGC5 |= SIM_SCGC5_PORTC_MASK;

    /* PTC2 GPIO + pull-up */
    PORTC->PCR[LDRPIN] = (PORTC->PCR[LDRPIN] & ~PORT_PCR_MUX_MASK) | PORT_PCR_MUX(1);
    PORTC->PCR[LDRPIN] |= PORT_PCR_PE_MASK | PORT_PCR_PS_MASK;  // enable pull-up
    GPIOC->PDDR &= ~(1 << LDRPIN);                             // input
}

/* ==== Soil moisture interrupt on PTA1 ==== */
static void initSOILInterrupt(void) {
    NVIC_DisableIRQ(PORTA_IRQn);

    SIM->SCGC5 |= SIM_SCGC5_PORTA_MASK;

    /* PTA1 GPIO + pull-up + both-edge interrupt */
    PORTA->PCR[SOILPIN] = (PORTA->PCR[SOILPIN] & ~PORT_PCR_MUX_MASK) | PORT_PCR_MUX(1);
    PORTA->PCR[SOILPIN] |= PORT_PCR_PE_MASK | PORT_PCR_PS_MASK;     // pull-up
    GPIOA->PDDR &= ~(1 << SOILPIN);                                // input

    /* Interrupt on both edges (0b1011) */
    PORTA->PCR[SOILPIN] = (PORTA->PCR[SOILPIN] & ~PORT_PCR_IRQC_MASK) | PORT_PCR_IRQC(0b1011);

    NVIC_SetPriority(PORTA_IRQn, 192);
    NVIC_ClearPendingIRQ(PORTA_IRQn);
    NVIC_EnableIRQ(PORTA_IRQn);
}

void initUART2(uint32_t baud_rate)
{
	NVIC_DisableIRQ(UART2_FLEXIO_IRQn);

	//enable clock to UART2 and PORTE
	SIM->SCGC4 |= SIM_SCGC4_UART2_MASK;
	SIM->SCGC5 |= SIM_SCGC5_PORTE_MASK;

	//Ensure Tx and Rx are disabled before configuration
	UART2->C2 &= ~((UART_C2_TE_MASK) | (UART_C2_RE_MASK));

	//connect UART pins for PTE22, PTE23
	PORTE->PCR[UART_TX_PTE22] &= ~PORT_PCR_MUX_MASK;
	PORTE->PCR[UART_TX_PTE22] |= PORT_PCR_MUX(4);

	PORTE->PCR[UART_RX_PTE23] &= ~PORT_PCR_MUX_MASK;
	PORTE->PCR[UART_RX_PTE23] |= PORT_PCR_MUX(4);

	// Set the baud rate
	uint32_t bus_clk = CLOCK_GetBusClkFreq();

	// This version of sbr does integer rounding.
	uint32_t sbr = (bus_clk + (baud_rate * 8)) / (baud_rate * 16);

	// Set SBR. Bits 8 to 12 in BDH, 0-7 in BDL.
	// MUST SET BDH FIRST!
	UART2->BDH &= ~UART_BDH_SBR_MASK;
	UART2->BDH |= ((sbr >> 8) & UART_BDH_SBR_MASK);
	UART2->BDL = (uint8_t) (sbr &0xFF);

	// Disable loop mode
	UART2->C1 &= ~UART_C1_LOOPS_MASK;
	UART2->C1 &= ~UART_C1_RSRC_MASK;

	// Disable parity
	UART2->C1 &= ~UART_C1_PE_MASK;

	// 8-bit mode
	UART2->C1 &= ~UART_C1_M_MASK;

	//Enable RX interrupt
	UART2->C2 |= UART_C2_RIE_MASK;

	// Enable the receiver
	UART2->C2 |= UART_C2_RE_MASK;

	NVIC_SetPriority(UART2_FLEXIO_IRQn, UART2_INT_PRIO);
	NVIC_ClearPendingIRQ(UART2_FLEXIO_IRQn);
	NVIC_EnableIRQ(UART2_FLEXIO_IRQn);

}

void UART2_FLEXIO_IRQHandler(void)
{
	// Send and receive pointers
	static int recv_ptr=0, send_ptr=0;
	char rx_data;
	char recv_buffer[MAX_MSG_LEN];

//VIC_ClearPendingIRQ(UART2_FLEXIO_IRQn);
	if(UART2->S1 & UART_S1_TDRE_MASK) // Send data
	{
		if(send_buffer[send_ptr] == '\0') {
			send_ptr = 0;

			// Disable the transmit interrupt
			UART2->C2 &= ~UART_C2_TIE_MASK;

			// Disable the transmitter
			UART2->C2 &= ~UART_C2_TE_MASK;
		}
		else {
			UART2->D = send_buffer[send_ptr++];
		}
	}

	if(UART2->S1 & UART_S1_RDRF_MASK)
	{
		TMessage msg;
		rx_data = UART2->D;
		recv_buffer[recv_ptr++] = rx_data;
		if(rx_data == '\n') {
			// Copy over the string
			BaseType_t hpw;
			recv_buffer[recv_ptr]='\0';
			strncpy(msg.message, recv_buffer, MAX_MSG_LEN);
			xQueueSendFromISR(queue, (void *)&msg, &hpw);
			portYIELD_FROM_ISR(hpw);
			recv_ptr = 0;
		}
	}

}

void sendMessage(char *message) {
	strncpy(send_buffer, message, MAX_MSG_LEN);

	// Enable the TIE interrupt
	UART2->C2 |= UART_C2_TIE_MASK;

	// Enable the transmitter
	UART2->C2 |= UART_C2_TE_MASK;
}

/* ==== FreeRTOS Tasks ==== */
/* Task: Poll LDR and drive LEDs */
static void TaskLDRPoll(void *arg) {
    (void)arg;
    int prev = -1;

    for (;;) {
        int ldrState = (GPIOC->PDIR & (1 << LDRPIN)) ? 1 : 0;

        /* Simple: LEDs ON when pin is HIGH, OFF when LOW (invert if needed) */
        if (ldrState != prev) {
            if (ldrState) {
                ledOn();
            } else {
                ledOff();
            }
            prev = ldrState;
        }

        vTaskDelay(pdMS_TO_TICKS(LDR_POLL_MS));
    }
}

/* Task: Wait for soil IRQ, debounce, set relay by level (no toggle) */
static void TaskPumpControl(void *arg) {
    (void)arg;

    /* Define polarity here:
       With pull-up, many soil modules pull LOW when WET. Adjust if yours differs. */
    const int LEVEL_WET = 0;                 /* change to 1 if HIGH==wet */
    const int LEVEL_DRY = 1 - LEVEL_WET;

    for (;;) {
        /* Wait until ISR signals a change */
        xSemaphoreTake(gSoilSem, portMAX_DELAY);

        /* Debounce with two spaced reads */
        vTaskDelay(pdMS_TO_TICKS(DEBOUNCE_MS));
        int s1 = (GPIOA->PDIR & (1 << SOILPIN)) ? 1 : 0;
        vTaskDelay(pdMS_TO_TICKS(DEBOUNCE_MS));
        int s2 = (GPIOA->PDIR & (1 << SOILPIN)) ? 1 : 0;

        if (s1 == s2 && s2 != lastSoilState) {
            lastSoilState = s2;

            if (lastSoilState == LEVEL_DRY) {
                pumpOn();
                PRINTF("Pump ON (dry)\r\n");
            } else { /* WET */
                pumpOff();
                PRINTF("Pump OFF (wet)\r\n");
            }

            /* Hold state briefly to avoid chatter from supply dips/coil kick */
            vTaskDelay(pdMS_TO_TICKS(PUMP_MIN_HOLD_MS));
        }
        /* else: bounce/no real change => ignore */
    }
}

/* ==== ISR (minimal): give semaphore, clear flag, request yield ==== */
void PORTA_IRQHandler(void) {
    NVIC_ClearPendingIRQ(PORTA_IRQn);

    if (PORTA->ISFR & (1 << SOILPIN)) {
        BaseType_t hpw = pdFALSE;
        if (gSoilSem) {
            xSemaphoreGiveFromISR(gSoilSem, &hpw);
        }
        PORTA->ISFR |= (1 << SOILPIN);   /* clear edge flag */
        portYIELD_FROM_ISR(hpw);
    }
}


/* ==== Sending Status Data to ESP32 ==== */
void sendStatusData(void) {
    char statusMessage[MAX_MSG_LEN];

    // Check all 4 permutations of Relay and Light states
    if (relayState && lightState) {
        // Relay ON, Light ON
        snprintf(statusMessage, MAX_MSG_LEN, "Relay ON, Light ON");
        sendMessage(statusMessage);  // Send status message
    }
    else if (relayState && !lightState) {
        // Relay ON, Light OFF
        snprintf(statusMessage, MAX_MSG_LEN, "Relay ON, Light OFF");
        sendMessage(statusMessage);  // Send status message
    }
    else if (!relayState && lightState) {
        // Relay OFF, Light ON
        snprintf(statusMessage, MAX_MSG_LEN, "Relay OFF, Light ON");
        sendMessage(statusMessage);  // Send status message
    }
    else {
        // Relay OFF, Light OFF
        snprintf(statusMessage, MAX_MSG_LEN, "Relay OFF, Light OFF");
        sendMessage(statusMessage);  // Send status message
    }
}


/* ==== FreeRTOS Tasks ==== */
static void recvTask(void *p) {
    while(1) {
        TMessage msg;
        if (xQueueReceive(queue, (TMessage *)&msg, portMAX_DELAY) == pdTRUE) {
            PRINTF("Received message: %s\r\n", msg.message);

            // Try to convert the received message to an integer (water level)
            int waterLevel = atoi(msg.message);  // Convert string to integer

            // Check if the message represents a water level
            if (waterLevel != 0) {  // If it's a numeric value (non-zero)
                // If the water level is less than 6000, print "Water level too low"
                if (waterLevel < 6000) {
                    PRINTF("Water level too low\r\n");
                } else {
                    PRINTF("Water level is sufficient: %d\r\n", waterLevel);
                }
            } else {
                // Process relay and light commands
                if (strcmp(msg.message, "Turn Relay ON") == 0) {
                    pumpOn();  // Turn the relay on
                    sendStatusData();  // Send updated status after relay is turned on
                } else if (strcmp(msg.message, "Turn Relay OFF") == 0) {
                    pumpOff();  // Turn the relay off
                    sendStatusData();  // Send updated status after relay is turned off
                }

                // Check if the command is to turn the light on or off
                if (strcmp(msg.message, "Turn Light ON") == 0) {
                    lightState = true;  // Set lightState to true (light on)
                    ledOn();  // Turn the light on
                    sendStatusData();  // Send updated status of light
                } else if (strcmp(msg.message, "Turn Light OFF") == 0) {
                    lightState = false;  // Set lightState to false (light off)
                    ledOff();  // Turn the light off
                    sendStatusData();  // Send updated status of light
                }
            }
        }
    }
}

/* Task to send periodic status updates */
static void sendTask(void *p) {
    while (1) {
        sendStatusData();  // Periodically send the current status of relay and light
        vTaskDelay(pdMS_TO_TICKS(5000));  // Send status every 5 seconds
    }
}
/* ==== Main ==== */
int main(void) {
    BOARD_InitBootPins();
    BOARD_InitBootClocks();
    BOARD_InitBootPeripherals();
#ifndef BOARD_INIT_DEBUG_CONSOLE_PERIPHERAL
    BOARD_InitDebugConsole();
#endif

    initLEDs();
    ledOff();

    initPump();
    pumpOff();

    initLDR();
    initSOILInterrupt();

    initUART2(9600);
    PRINTF("UART2 DEMO\r\n");

    queue = xQueueCreate(QLEN, sizeof(TMessage));


    /* Create the binary semaphore and sample baseline soil level */
    gSoilSem = xSemaphoreCreateBinary();
    lastSoilState = (GPIOA->PDIR & (1 << SOILPIN)) ? 1 : 0;

    /* Create tasks */
    xTaskCreate(TaskLDRPoll, "LDR", 256, NULL, tskIDLE_PRIORITY + 1, NULL);
    xTaskCreate(TaskPumpControl, "PUMP", 256, NULL, tskIDLE_PRIORITY + 2, NULL);
    xTaskCreate(recvTask, "recvTask", configMINIMAL_STACK_SIZE+100, NULL, 2, NULL);
    xTaskCreate(sendTask, "sendTask", configMINIMAL_STACK_SIZE+100, NULL, 1, NULL);

    vTaskStartScheduler();

    /* Should never get here */
    volatile static int i = 0 ;
    /* Enter an infinite loop, just incrementing a counter. */
    while(1) {
        i++ ;
        /* 'Dummy' NOP to allow source level single stepping of
            tight while() loop */
        __asm volatile ("nop");
    }
    return 0 ;
}
