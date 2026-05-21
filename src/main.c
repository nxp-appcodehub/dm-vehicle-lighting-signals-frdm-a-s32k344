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
#include "Mcal.h"
#include "Clock_Ip.h"
#include "IntCtrl_Ip.h"
#include "OsIf.h"
#include "Siul2_Port_Ip.h"
#include "Adc_Sar_Ip.h"
#include "Flexio_Mcl_Ip.h"
#include "WS2812_utils.h"
#include "VehicleLights_utils.h"

/*==================================================================================================
 *                                      DEFINES AND MACROS
 ==================================================================================================*/
/* FlexIO Base Pointer */
#define FXIOBASE                        Flexio_Ip_paxBase[0]

/* ADC Configuration */
#define ADC_VREF_MV                     3300U
#define ADC_RESOLUTION                  16384U
#define ADC_CHANNEL                     0U
#define ADC_CALIBRATION_CYCLES          5U

/* Signal Processing */
#define N_SAMPLES                       64U
#define VOLTAGE_THRESHOLD               200U

/* Debounce Configuration */
#define DEBOUNCE_READS                  3U
#define DEBOUNCE_DELAY_CYCLES           5U

/* LED Configuration */
#define LED_BRIGHTNESS                  25U
#define BUTTON_HOLD_TIME_MS             1000U
#define BUTTON_COOLDOWN_MS              300U

/* Button Voltage Ranges (in millivolts) */
#define BTN1_VOLTAGE_MIN                3000U
#define BTN1_VOLTAGE_MAX                3500U
#define BTN2_VOLTAGE_MIN                2500U
#define BTN2_VOLTAGE_MAX                2900U
#define BTN3_VOLTAGE_MIN                1900U
#define BTN3_VOLTAGE_MAX                2400U
#define BTN4_VOLTAGE_MIN                1400U
#define BTN4_VOLTAGE_MAX                1800U
#define BTN5_VOLTAGE_MIN                800U
#define BTN5_VOLTAGE_MAX                1200U
#define BTN6_VOLTAGE_MIN                300U
#define BTN6_VOLTAGE_MAX                700U

/*==================================================================================================
 *                          LOCAL TYPEDEFS (STRUCTURES, UNIONS, ENUMS)
 ==================================================================================================*/

/*==================================================================================================
 *                                       LOCAL MACROS
 ==================================================================================================*/

/*==================================================================================================
 *                                      LOCAL CONSTANTS
 ==================================================================================================*/

/*==================================================================================================
 *                                      LOCAL VARIABLES
 ==================================================================================================*/

/* Last detected button to prevent multiple triggers */
static uint8 ui8LastButton = 0U;

/*==================================================================================================
 *                                      GLOBAL CONSTANTS
 ==================================================================================================*/

/*==================================================================================================
 *                                      GLOBAL VARIABLES
 ==================================================================================================*/
/* WS2812 LED Strip Instance */
WS2812_Strip_t ledStrip;

/* ADC Notification Flags and Data */
volatile boolean notif_triggered = FALSE;
volatile uint16 ui16RawValue = 0U;

/* LED Control State Flags */
boolean bLowBeam = FALSE;
boolean bHighBeam = FALSE;
boolean bLeftBlinkActive = FALSE;
boolean bRightBlinkActive = FALSE;

/* System Exit Code */
volatile sint32 exit_code = 0;

/*==================================================================================================
 *                                   LOCAL FUNCTION PROTOTYPES
 ==================================================================================================*/
static uint16 AdcToMillivolts(uint16 ui16RawValue);
static uint16 ReadAdcVoltage(void);
static uint8 GetButtonFromVoltage(uint16 ui16VoltageMv);
static uint8 ReadButtonDebounced(void);

static void WS2812_InitSequence(WS2812_Strip_t *strip);
static void DelayMs(uint32 ui32TimeoutMs);

/*==================================================================================================
 *                                   EXTERNAL FUNCTION PROTOTYPES
 ==================================================================================================*/
extern void Adc_Sar_0_Isr(void);

/*==================================================================================================
 *                                       LOCAL FUNCTIONS
 ==================================================================================================*/

/**
 * @brief Convert ADC raw value to millivolts
 *
 * @param ui16RawValue  Raw ADC reading (14-bit)
 * @return Voltage in millivolts
 */
static uint16 AdcToMillivolts(uint16 ui16RawValue) {
	return (uint16) ((uint32) ui16RawValue * ADC_VREF_MV / ADC_RESOLUTION);
}

/**
 * @brief Read ADC voltage with notification wait
 *
 * @return Voltage in millivolts
 */
static uint16 ReadAdcVoltage(void) {
	Adc_Sar_Ip_StartConversion(ADC_VOLTAGE_DIVIDER_HW_INSTANCE,
			ADC_SAR_IP_CONV_CHAIN_NORMAL);

	while (notif_triggered != TRUE) {
		/* Wait for conversion complete */
	}
	notif_triggered = FALSE;

	return AdcToMillivolts(ui16RawValue);
}

/**
 * @brief Map voltage to button ID
 *
 * @param ui16VoltageMv  Measured voltage in millivolts
 * @return Button ID (0 = no button, 1-6 = button number)
 */
static uint8 GetButtonFromVoltage(uint16 ui16VoltageMv) {
	uint8 ui8ButtonId = 0U;

	if ((ui16VoltageMv >= BTN1_VOLTAGE_MIN)
			&& (ui16VoltageMv <= BTN1_VOLTAGE_MAX)) {
		ui8ButtonId = 1U;
	} else if ((ui16VoltageMv >= BTN2_VOLTAGE_MIN)
			&& (ui16VoltageMv <= BTN2_VOLTAGE_MAX)) {
		ui8ButtonId = 2U;
	} else if ((ui16VoltageMv >= BTN3_VOLTAGE_MIN)
			&& (ui16VoltageMv <= BTN3_VOLTAGE_MAX)) {
		ui8ButtonId = 3U;
	} else if ((ui16VoltageMv >= BTN4_VOLTAGE_MIN)
			&& (ui16VoltageMv <= BTN4_VOLTAGE_MAX)) {
		ui8ButtonId = 4U;
	} else if ((ui16VoltageMv >= BTN5_VOLTAGE_MIN)
			&& (ui16VoltageMv <= BTN5_VOLTAGE_MAX)) {
		ui8ButtonId = 5U;
	} else if ((ui16VoltageMv >= BTN6_VOLTAGE_MIN)
			&& (ui16VoltageMv <= BTN6_VOLTAGE_MAX)) {
		ui8ButtonId = 6U;
	} else {
		ui8ButtonId = 0U;
	}

	return ui8ButtonId;
}

/**
 * @brief Read button with debouncing
 *
 * @return Button ID (0 if no button or unstable reading)
 */
static uint8 ReadButtonDebounced(void) {
	uint16 ui16VoltageMv;
	uint32 ui32VoltageSum;
	uint16 ui16VoltageAvgMv;
	uint8 aui8ButtonReads[DEBOUNCE_READS];
	uint8 ui8Index;
	uint8 ui8SampleCnt;

	for (ui8Index = 0U; ui8Index < DEBOUNCE_READS; ui8Index++) {
		ui32VoltageSum = 0U;

		for (ui8SampleCnt = 0U; ui8SampleCnt < N_SAMPLES; ui8SampleCnt++) {
			ui16VoltageMv = ReadAdcVoltage();
			ui32VoltageSum += ui16VoltageMv;
			delay_cycles(1);
		}

		ui16VoltageAvgMv = (uint16) (ui32VoltageSum / N_SAMPLES);
		aui8ButtonReads[ui8Index] = GetButtonFromVoltage(ui16VoltageAvgMv);

		delay_cycles(DEBOUNCE_DELAY_CYCLES);
	}

	for (ui8Index = 1U; ui8Index < DEBOUNCE_READS; ui8Index++) {
		if (aui8ButtonReads[ui8Index] != aui8ButtonReads[0]) {
			return 0U;
		}
	}

	return aui8ButtonReads[0];
}

/**
 * @brief Performs initialization and test sequence for WS2812 LED strip
 *
 * @param strip  Pointer to the WS2812 strip structure
 */
static void WS2812_InitSequence(WS2812_Strip_t *strip) {
	uint16 ui16Index;

	/* Step 1: Sequential white LED activation */
	for (ui16Index = 0U; ui16Index < NUM_LEDS; ui16Index++) {
		WS2812_SetPixel(strip, ui16Index, 255U, 255U, 255U);
		WS2812_Show(strip);
		delay_cycles(5000000);
	}

	delay_cycles(5000000);
	WS2812_Clear(strip);
	WS2812_Show(strip);

	/* Step 2: Color cycle - RED */
	for (ui16Index = 0U; ui16Index < NUM_LEDS; ui16Index++) {
		WS2812_SetPixel(strip, ui16Index, 255U, 0U, 0U);
	}
	WS2812_Show(strip);
	delay_cycles(10000000);

	/* Color cycle - GREEN */
	for (ui16Index = 0U; ui16Index < NUM_LEDS; ui16Index++) {
		WS2812_SetPixel(strip, ui16Index, 0U, 255U, 0U);
	}
	WS2812_Show(strip);
	delay_cycles(10000000);

	/* Color cycle - BLUE */
	for (ui16Index = 0U; ui16Index < NUM_LEDS; ui16Index++) {
		WS2812_SetPixel(strip, ui16Index, 0U, 0U, 255U);
	}
	WS2812_Show(strip);
	delay_cycles(10000000);

	/* Step 3: Clear all LEDs */
	WS2812_Clear(strip);
	WS2812_Show(strip);
	delay_cycles(5000000);
}

/**
 * @brief Millisecond delay function using OsIf timer
 *
 * @param ui32TimeoutMs  Delay time in milliseconds
 */
static void DelayMs(uint32 ui32TimeoutMs) {
	uint32 ui32CurTime = OsIf_GetCounter(OSIF_COUNTER_SYSTEM);
	uint32 ui32ElapsedTicks = 0U;
	uint32 ui32TimeoutTicks = OsIf_MicrosToTicks(ui32TimeoutMs * 1000U,
			OSIF_COUNTER_SYSTEM);

	while (ui32ElapsedTicks < ui32TimeoutTicks) {
		ui32ElapsedTicks += OsIf_GetElapsed(&ui32CurTime, OSIF_COUNTER_SYSTEM);
	}
}

/*==================================================================================================
 *                                       GLOBAL FUNCTIONS
 ==================================================================================================*/
/**
 * @brief ADC End of Chain Notification Callback
 */
void AdcEndOfChainNotif(void) {
	ui16RawValue = Adc_Sar_Ip_GetConvData(ADC_VOLTAGE_DIVIDER_HW_INSTANCE,
	ADC_CHANNEL);
	notif_triggered = TRUE;
}

/*==================================================================================================
 *                                           MAIN FUNCTION
 ==================================================================================================*/
int main(void) {
	uint16 ui16VoltageMv;
	uint8 ui8CurrentButton = 0U;
	uint8 ui8Index;

	/*==============================================================================================
	 *                                 SYSTEM INITIALIZATION
	 ==============================================================================================*/
	/* Clock initialization */
	Clock_Ip_Init(&Clock_Ip_aClockConfig[0]);
	OsIf_Init(NULL_PTR);

	/* GPIO initialization */
	Siul2_Port_Ip_Init(
	NUM_OF_CONFIGURED_PINS_PortContainer_0_BOARD_InitPeripherals,
			g_pin_mux_InitConfigArr_PortContainer_0_BOARD_InitPeripherals);

	/* FlexIO initialization */
	Flexio_Mcl_Ip_Init(FXIOBASE);
	Flexio_Mcl_Ip_InitDevice(&Flexio_Ip_Sa_xFlexioInit);
	Flexio_Mcl_Ip_SetEnable(FXIOBASE, TRUE);

	/* ADC initialization */
	Adc_Sar_Ip_Init(ADC_VOLTAGE_DIVIDER_HW_INSTANCE, &ADC_VOLTAGE_DIVIDER_HW);
	Adc_Sar_Ip_EnableChannel(ADC_VOLTAGE_DIVIDER_HW_INSTANCE,
			ADC_SAR_IP_CONV_CHAIN_NORMAL,
			ADC_CHANNEL);

	/* ADC interrupt setup */
	IntCtrl_Ip_InstallHandler(ADC0_IRQn, Adc_Sar_0_Isr, NULL_PTR);
	IntCtrl_Ip_EnableIrq(ADC0_IRQn);

	/* ADC calibration */
	for (ui8Index = 0U; ui8Index < ADC_CALIBRATION_CYCLES; ui8Index++) {
		Adc_Sar_Ip_DoCalibration(ADC_VOLTAGE_DIVIDER_HW_INSTANCE);
	}

	/* Enable ADC notifications */
	Adc_Sar_Ip_EnableNotifications(ADC_VOLTAGE_DIVIDER_HW_INSTANCE,
	ADC_SAR_IP_NOTIF_FLAG_NORMAL_ENDCHAIN);

	/* WS2812 LED strip initialization */
	WS2812_Init(&ledStrip, 8, 0, 0);

	/* Adjust brightness (0-255): 25 = ~10%, 50 = ~20%, 128 = 50% */
	ledStrip.brightness = 50;  // Set to 20% brightness

	WS2812_InitSequence(&ledStrip);

	/*==============================================================================================
	 *                                 MAIN CONTROL LOOP
	 ==============================================================================================*/
	while (1) {
		/* Handle blinking turn signals */
		HandleBlinkSignals();

		/* Read ADC voltage */
		ui16VoltageMv = ReadAdcVoltage();

		/* Check if button is pressed */
		if (ui16VoltageMv > VOLTAGE_THRESHOLD) {
			/* Perform debounced button read */
			ui8CurrentButton = ReadButtonDebounced();

			/* Process new button press */
			if ((ui8CurrentButton != 0U)
					&& (ui8CurrentButton != ui8LastButton)) {
				/* Toggle LED pattern based on button */
				ToggleLedPattern(ui8CurrentButton);
				ui8LastButton = ui8CurrentButton;

				while (ReadAdcVoltage() > VOLTAGE_THRESHOLD) {
					DelayMs(50U);
				}
			}
		} else {
			/* No button pressed - reset state */
			ui8LastButton = 0U;
		}

		/* Update LED strip with current state */
		ApplyLedState();

		/* Main loop delay */
		DelayMs(10U);
	}

	return exit_code;
}
