/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fsl_device_registers.h"
#include "fsl_common.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
/* Component ID definition, used by tools. */
#ifndef FSL_COMPONENT_ID
#define FSL_COMPONENT_ID "platform.drivers.mtb"
#endif

/*******************************************************************************
 * Variables
 ******************************************************************************/

#if (defined(__ICCARM__))
#pragma data_alignment = 128
uint32_t mtb[128] @ "MTB";
#elif (defined(__CC_ARM) || defined(__ARMCC_VERSION))
__attribute__((aligned(128))) uint32_t mtb[128] __attribute__((section("MTB"), zero_init));
#elif (defined(__GNUC__))
__attribute__((aligned(128))) uint32_t mtb[128] __attribute__((section("MTB"), zero_init));
#endif

/*******************************************************************************
 * Code
 ******************************************************************************/

void MTB_Init(void)
{
#if defined(MTB)
    /* Enable MTB */
    MTB->POSITION = 0;
    MTB->MASTER   = MTB_MASTER_TSTARTEN(0) | MTB_MASTER_HALTREQ(0) | MTB_MASTER_EN(1);
    MTB->BASE     = (uint32_t)mtb;
#endif
}

void MTB_Disable(void)
{
#if defined(MTB)
    /* Disable MTB */
    MTB->MASTER = (MTB->MASTER & ~MTB_MASTER_EN_MASK);
#endif
}
