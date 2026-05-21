/*==================================================================================================
 * Project : RTD AUTOSAR 4.9
 * Platform : CORTEXM
 * Peripheral : S32K3XX
 * Dependencies : none
 *
 * Autosar Version : 4.9.0
 * Autosar Revision : ASR_REL_4_9_REV_0000
 * Autosar Conf.Variant :
 * SW Version : 7.0.0
 * Build Version : S32K3_RTD_7_0_0_QLP03_D2512_ASR_REL_4_9_REV_0000_20251210
 *
 * Copyright 2020 - 2026 NXP
 *
 *   NXP Proprietary. This software is owned or controlled by NXP and may only be
 *   used strictly in accordance with the applicable license terms. By expressly
 *   accepting such terms or by downloading, installing, activating and/or otherwise
 *   using the software, you are agreeing that you have read, and that you agree to
 *   comply with and are bound by, such license terms. If you do not agree to be
 *   bound by the applicable license terms, then you may not retain, install,
 *   activate or otherwise use the software.
 ==================================================================================================*/

#ifndef VEHICLELIGHTS_UTILS_H
#define VEHICLELIGHTS_UTILS_H

/*==================================================================================================
 *                                        INCLUDE FILES
 ==================================================================================================*/
#include "Mcal.h"
#include "WS2812_utils.h"

/*==================================================================================================
 *                                      DEFINES AND MACROS
 ==================================================================================================*/
/* LED Color Definitions (R, G, B) */
#define COLOR_WARM_WHITE_R              255U
#define COLOR_WARM_WHITE_G              200U
#define COLOR_WARM_WHITE_B              120U

#define COLOR_COOL_WHITE_R              255U
#define COLOR_COOL_WHITE_G              255U
#define COLOR_COOL_WHITE_B              255U

#define COLOR_AMBER_R                   255U
#define COLOR_AMBER_G                   100U
#define COLOR_AMBER_B                   0U

#define COLOR_RED_R                     255U
#define COLOR_RED_G                     0U
#define COLOR_RED_B                     0U

/* LED Position Mappings */
/* Front Headlights */
#define LED_FRONT_HIGH_LEFT             0U
#define LED_FRONT_LOW_LEFT              1U
#define LED_FRONT_LOW_RIGHT             2U
#define LED_FRONT_HIGH_RIGHT            3U

/* Front Turn Signals */
#define LED_FRONT_TURN_LEFT             4U

/* Rear Brake/Tail Lights */
#define LED_REAR_BRAKE_LEFT             5U
#define LED_REAR_BRAKE_RIGHT            6U

/* Rear Turn Signals */
#define LED_REAR_TURN_RIGHT             7U
#define LED_REAR_TURN_LEFT              8U

/* Rear Additional Brake Lights */
#define LED_REAR_BRAKE_CENTER_LEFT      9U
#define LED_REAR_BRAKE_CENTER_RIGHT     10U

/* Rear Additional Turn Signals */
#define LED_REAR_TURN_RIGHT_2           11U

/* Rear Headlights */
#define LED_REAR_HIGH_LEFT              12U
#define LED_REAR_LOW_LEFT               13U
#define LED_REAR_LOW_RIGHT              14U
#define LED_REAR_HIGH_RIGHT             15U

/*==================================================================================================
 *                                       GLOBAL VARIABLES
 ==================================================================================================*/
extern WS2812_Strip_t ledStrip;
extern boolean bLowBeam;
extern boolean bHighBeam;
extern boolean bLeftBlinkActive;
extern boolean bRightBlinkActive;

/*==================================================================================================
 *                                   FUNCTION PROTOTYPES
 ==================================================================================================*/

boolean IsLedActive(uint8 ui8LedIndex, uint8 ui8R, uint8 ui8G, uint8 ui8B);

void SetLed(uint8 ui8LedIndex, uint8 ui8R, uint8 ui8G, uint8 ui8B);

void ClearLed(uint8 ui8LedIndex);

void HandleBlinkSignals(void);

void ToggleLedPattern(uint8 ui8ButtonId);

void ApplyLedState(void);

#endif /* VEHICLELIGHTS_UTILS_H */
