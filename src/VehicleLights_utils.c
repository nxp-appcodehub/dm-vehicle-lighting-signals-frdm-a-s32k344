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

/*==================================================================================================
 *                                        INCLUDE FILES
 ==================================================================================================*/
#include "VehicleLights_utils.h"
#include "WS2812_utils.h"

/*==================================================================================================
 *                                      DEFINES AND MACROS
 ==================================================================================================*/
#define BLINK_CYCLES                    25U

/* Turn signal LED indices - Front and Rear */
#define LED_TURN_LEFT_FRONT             0U
#define LED_TURN_RIGHT_FRONT            3U
#define LED_TURN_LEFT_REAR              12U
#define LED_TURN_RIGHT_REAR             15U

/* High beam LED indices - changed to 8, 9, 10, 11 */
#define LED_HIGH_BEAM_LEFT_1            8U
#define LED_HIGH_BEAM_LEFT_2            9U
#define LED_HIGH_BEAM_RIGHT_1           10U
#define LED_HIGH_BEAM_RIGHT_2           11U

/*==================================================================================================
 *                                      LOCAL VARIABLES
 ==================================================================================================*/
/* LED State Array [LED_INDEX][R, G, B] */
static uint8 aui8DeliveryArray[NUM_LEDS][3] = { 0 };

/* Blink State Variables */
static uint32 ui32BlinkCounter = 0U;
static boolean bBlinkState = FALSE;
static boolean bLeftBlinkBeforeHazard = FALSE;
static boolean bRightBlinkBeforeHazard = FALSE;
static boolean bHazardActive = FALSE;

/*==================================================================================================
 *                                       LOCAL FUNCTIONS
 ==================================================================================================*/

/*==================================================================================================
 *                                       GLOBAL FUNCTIONS
 ==================================================================================================*/
/**
 * @brief Check if LED is currently set to specific color
 *
 * @param ui8LedIndex  LED index (0-15)
 * @param ui8R         Red value to check
 * @param ui8G         Green value to check
 * @param ui8B         Blue value to check
 * @return TRUE if LED matches this color, FALSE otherwise
 */
boolean IsLedActive(uint8 ui8LedIndex, uint8 ui8R, uint8 ui8G, uint8 ui8B) {
	boolean bResult = FALSE;

	if (ui8LedIndex < NUM_LEDS) {
		if ((aui8DeliveryArray[ui8LedIndex][0] == ui8R)
				&& (aui8DeliveryArray[ui8LedIndex][1] == ui8G)
				&& (aui8DeliveryArray[ui8LedIndex][2] == ui8B)) {
			bResult = TRUE;
		}
	}

	return bResult;
}

/**
 * @brief Set LED in state array
 *
 * @param ui8LedIndex  LED index (0-15)
 * @param ui8R         Red value (0-255)
 * @param ui8G         Green value (0-255)
 * @param ui8B         Blue value (0-255)
 */
void SetLed(uint8 ui8LedIndex, uint8 ui8R, uint8 ui8G, uint8 ui8B) {
	if (ui8LedIndex < NUM_LEDS) {
		aui8DeliveryArray[ui8LedIndex][0] = ui8R;
		aui8DeliveryArray[ui8LedIndex][1] = ui8G;
		aui8DeliveryArray[ui8LedIndex][2] = ui8B;
	}
}

/**
 * @brief Clear specific LED in state array
 *
 * @param ui8LedIndex  LED index (0-15)
 */
void ClearLed(uint8 ui8LedIndex) {
	if (ui8LedIndex < NUM_LEDS) {
		aui8DeliveryArray[ui8LedIndex][0] = 0U;
		aui8DeliveryArray[ui8LedIndex][1] = 0U;
		aui8DeliveryArray[ui8LedIndex][2] = 0U;
	}
}

/**
 * @brief Handle blinking turn signals
 */
void HandleBlinkSignals(void) {
	ui32BlinkCounter++;

	if (ui32BlinkCounter >= BLINK_CYCLES) {
		ui32BlinkCounter = 0U;
		bBlinkState = !bBlinkState;

		/* Handle left turn signal (LED 0 and 12) */
		if (bLeftBlinkActive == TRUE) {
			if (bBlinkState == TRUE) {
				SetLed(LED_TURN_LEFT_FRONT, COLOR_AMBER_R, COLOR_AMBER_G, COLOR_AMBER_B);
				SetLed(LED_TURN_LEFT_REAR, COLOR_AMBER_R, COLOR_AMBER_G, COLOR_AMBER_B);
			} else {
				ClearLed(LED_TURN_LEFT_FRONT);
				ClearLed(LED_TURN_LEFT_REAR);
			}
		}

		/* Handle right turn signal (LED 3 and 15) */
		if (bRightBlinkActive == TRUE) {
			if (bBlinkState == TRUE) {
				SetLed(LED_TURN_RIGHT_FRONT, COLOR_AMBER_R, COLOR_AMBER_G, COLOR_AMBER_B);
				SetLed(LED_TURN_RIGHT_REAR, COLOR_AMBER_R, COLOR_AMBER_G, COLOR_AMBER_B);
			} else {
				ClearLed(LED_TURN_RIGHT_FRONT);
				ClearLed(LED_TURN_RIGHT_REAR);
			}
		}
	}
}

/**
 * @brief Toggle LED pattern based on button press
 *
 * @param ui8ButtonId  Button ID (1-6)
 */
void ToggleLedPattern(uint8 ui8ButtonId) {
	switch (ui8ButtonId) {
	case 1U: /* Low beam toggle - LED 13 and 14 only */
		if (IsLedActive(13U, COLOR_WARM_WHITE_R, COLOR_WARM_WHITE_G, COLOR_WARM_WHITE_B) == TRUE) {
			/* Turning OFF low beam - also turn OFF high beam */
			ClearLed(13U);
			ClearLed(14U);

			/* Turn off high beam too (LEDs 8, 9, 10, 11) */
			ClearLed(LED_HIGH_BEAM_LEFT_1);
			ClearLed(LED_HIGH_BEAM_LEFT_2);
			ClearLed(LED_HIGH_BEAM_RIGHT_1);
			ClearLed(LED_HIGH_BEAM_RIGHT_2);

			bLowBeam = FALSE;
			bHighBeam = FALSE;
		} else {
			/* Turning ON low beam - LED 13 and 14 only */
			SetLed(13U, COLOR_WARM_WHITE_R, COLOR_WARM_WHITE_G, COLOR_WARM_WHITE_B);
			SetLed(14U, COLOR_WARM_WHITE_R, COLOR_WARM_WHITE_G, COLOR_WARM_WHITE_B);
			bLowBeam = TRUE;
		}
		break;

	case 2U: /* High beam toggle - LEDs 8, 9, 10, 11 */
		if (bHighBeam == TRUE) {
			/* Turning OFF high beam */
			bHighBeam = FALSE;
			ClearLed(LED_HIGH_BEAM_LEFT_1);
			ClearLed(LED_HIGH_BEAM_LEFT_2);
			ClearLed(LED_HIGH_BEAM_RIGHT_1);
			ClearLed(LED_HIGH_BEAM_RIGHT_2);
		} else {
			/* Turning ON high beam - only allowed if low beam is ON */
			if (bLowBeam == TRUE) {
				bHighBeam = TRUE;
				SetLed(LED_HIGH_BEAM_LEFT_1, COLOR_COOL_WHITE_R, COLOR_COOL_WHITE_G, COLOR_COOL_WHITE_B);
				SetLed(LED_HIGH_BEAM_LEFT_2, COLOR_COOL_WHITE_R, COLOR_COOL_WHITE_G, COLOR_COOL_WHITE_B);
				SetLed(LED_HIGH_BEAM_RIGHT_1, COLOR_COOL_WHITE_R, COLOR_COOL_WHITE_G, COLOR_COOL_WHITE_B);
				SetLed(LED_HIGH_BEAM_RIGHT_2, COLOR_COOL_WHITE_R, COLOR_COOL_WHITE_G, COLOR_COOL_WHITE_B);
			}
		}
		break;

	case 3U: /* Left turn signal toggle - LED 0 and 12 */
		/* Ignore if hazard lights are active - hazard has priority */
		if (bHazardActive == TRUE) {
			break;
		}

		if (bLeftBlinkActive == TRUE) {
			/* Turning OFF left turn signal */
			bLeftBlinkActive = FALSE;
			/* Clear both front and rear LEDs */
			ClearLed(LED_TURN_LEFT_FRONT);
			ClearLed(LED_TURN_LEFT_REAR);
		} else if (bRightBlinkActive == FALSE) {
			/* Turning ON left turn signal */
			bLeftBlinkActive = TRUE;
		}
		break;

	case 4U: /* Right turn signal toggle - LED 3 and 15 */
		/* Ignore if hazard lights are active - hazard has priority */
		if (bHazardActive == TRUE) {
			break;
		}

		if (bRightBlinkActive == TRUE) {
			/* Turning OFF right turn signal */
			bRightBlinkActive = FALSE;
			/* Clear both front and rear LEDs */
			ClearLed(LED_TURN_RIGHT_FRONT);
			ClearLed(LED_TURN_RIGHT_REAR);
		} else if (bLeftBlinkActive == FALSE) {
			/* Turning ON right turn signal */
			bRightBlinkActive = TRUE;
		}
		break;

	case 5U: /* Brake lights - LED 1 and 2 only */
		if (IsLedActive(1U, COLOR_RED_R, COLOR_RED_G, COLOR_RED_B) == TRUE) {
			ClearLed(1U);
			ClearLed(2U);
			ClearLed(5U);
			ClearLed(6U);
		} else {
			SetLed(1U, COLOR_RED_R, COLOR_RED_G, COLOR_RED_B);
			SetLed(2U, COLOR_RED_R, COLOR_RED_G, COLOR_RED_B);
			SetLed(5U, COLOR_RED_R, COLOR_RED_G, COLOR_RED_B);
			SetLed(6U, COLOR_RED_R, COLOR_RED_G, COLOR_RED_B);
		}
		break;

	case 6U: /* Hazard lights toggle - LEDs 0, 3, 12, and 15 */
		if (bHazardActive == TRUE) {
			/* Turning OFF hazard lights */
			bHazardActive = FALSE;

			/* Restore previous turn signal states */
			bLeftBlinkActive = bLeftBlinkBeforeHazard;
			bRightBlinkActive = bRightBlinkBeforeHazard;

			/* Clear all turn signal LEDs if turn signals were not active before */
			if (bLeftBlinkActive == FALSE) {
				ClearLed(LED_TURN_LEFT_FRONT);
				ClearLed(LED_TURN_LEFT_REAR);
			}

			if (bRightBlinkActive == FALSE) {
				ClearLed(LED_TURN_RIGHT_FRONT);
				ClearLed(LED_TURN_RIGHT_REAR);
			}
		} else {
			/* Turning ON hazard lights */
			bHazardActive = TRUE;

			/* Save current turn signal states before activating hazard */
			bLeftBlinkBeforeHazard = bLeftBlinkActive;
			bRightBlinkBeforeHazard = bRightBlinkActive;

			/* Activate both turn signals */
			bLeftBlinkActive = TRUE;

			bRightBlinkActive = TRUE;
		}
		break;
	}
}

/**
 * @brief Apply current LED state from array to physical LED strip
 */
void ApplyLedState(void) {
	uint8 ui8Index;

	for (ui8Index = 0U; ui8Index < NUM_LEDS; ui8Index++) {
		WS2812_SetPixel(&ledStrip, ui8Index, aui8DeliveryArray[ui8Index][0],
				aui8DeliveryArray[ui8Index][1], aui8DeliveryArray[ui8Index][2]);
	}

	WS2812_Show(&ledStrip);
}
