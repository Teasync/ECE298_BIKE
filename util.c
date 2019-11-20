#include "util.h"

extern Timer_A_initUpModeParam initUpParam0;

void grn_on(void) {
    GPIO_setOutputHighOnPin(LED_GRN_PORT, LED_GRN_PIN);
    GPIO_setOutputLowOnPin(LED_RED_PORT, LED_RED_PIN);
    GPIO_setOutputLowOnPin(LED_ORG_PORT, LED_ORG_PIN);
    GPIO_setOutputLowOnPin(LED_YLW_PORT, LED_YLW_PIN);
}

void ylw_on(void) {
    GPIO_setOutputHighOnPin(LED_YLW_PORT, LED_YLW_PIN);
    GPIO_setOutputLowOnPin(LED_RED_PORT, LED_RED_PIN);
    GPIO_setOutputLowOnPin(LED_ORG_PORT, LED_ORG_PIN);
    GPIO_setOutputLowOnPin(LED_GRN_PORT, LED_GRN_PIN);
}

void org_on(void) {
    GPIO_setOutputHighOnPin(LED_ORG_PORT, LED_ORG_PIN);
    GPIO_setOutputLowOnPin(LED_RED_PORT, LED_RED_PIN);
    GPIO_setOutputLowOnPin(LED_GRN_PORT, LED_GRN_PIN);
    GPIO_setOutputLowOnPin(LED_YLW_PORT, LED_YLW_PIN);
}

void red_on(void) {
    GPIO_setOutputHighOnPin(LED_RED_PORT, LED_RED_PIN);
    GPIO_setOutputLowOnPin(LED_GRN_PORT, LED_GRN_PIN);
    GPIO_setOutputLowOnPin(LED_ORG_PORT, LED_ORG_PIN);
    GPIO_setOutputLowOnPin(LED_YLW_PORT, LED_YLW_PIN);
}

void all_off(void) {
    GPIO_setOutputLowOnPin(LED_RED_PORT, LED_RED_PIN);
    GPIO_setOutputLowOnPin(LED_GRN_PORT, LED_GRN_PIN);
    GPIO_setOutputLowOnPin(LED_ORG_PORT, LED_ORG_PIN);
    GPIO_setOutputLowOnPin(LED_YLW_PORT, LED_YLW_PIN);
}

void beep(int pulse_per, int total_time) {
    int i;
    for (i = 0; i < total_time; i++)
    {
        GPIO_toggleOutputOnPin(BUZZ_PORT, BUZZ_PIN);    //Set P1.2...

        int j;
        for (j = 0; j < pulse_per; j++)
        {
            __delay_cycles(1);
        }
    }
}

void Timer_Init(void) {
    //Start timer in continuous mode sourced by SMCLK
    //Initialize Timer 1 - For general use as a timer interrupt for polling
    Timer_A_initContinuousModeParam initContParam1 = { 0 };
    initContParam1.clockSource = TIMER_A_CLOCKSOURCE_SMCLK;
    initContParam1.clockSourceDivider = TIMER_A_CLOCKSOURCE_DIVIDER_1;
    initContParam1.timerInterruptEnable_TAIE = TIMER_A_TAIE_INTERRUPT_DISABLE;
    initContParam1.timerClear = TIMER_A_DO_CLEAR;
    initContParam1.startTimer = false;
    Timer_A_initContinuousMode(TIMER_A1_BASE, &initContParam1);

    //Initialize Timer 0 - For timing the length of echo pulse
    initUpParam0.clockSource = TIMER_A_CLOCKSOURCE_SMCLK;
    initUpParam0.clockSourceDivider = TIMER_A_CLOCKSOURCE_DIVIDER_1;
    initUpParam0.timerPeriod = 0xFFFF;
    initUpParam0.timerInterruptEnable_TAIE = TIMER_A_TAIE_INTERRUPT_DISABLE;
    initUpParam0.timerClear = TIMER_A_DO_CLEAR;
    initUpParam0.startTimer = false;
    Timer_A_initUpMode(TIMER_A0_BASE, &initUpParam0);

    //Initialize compare mode for Timer 1 - To setup interrupts for Timer 1 polling
    Timer_A_clearCaptureCompareInterrupt(TIMER_A1_BASE,
                                         TIMER_A_CAPTURECOMPARE_REGISTER_0);

    Timer_A_initCompareModeParam initCompParam = { 0 };
    initCompParam.compareRegister = TIMER_A_CAPTURECOMPARE_REGISTER_0;
    initCompParam.compareInterruptEnable =
            TIMER_A_CAPTURECOMPARE_INTERRUPT_ENABLE;
    initCompParam.compareOutputMode = TIMER_A_OUTPUTMODE_OUTBITVALUE;
    initCompParam.compareValue = WAIT_DUR;
    Timer_A_initCompareMode(TIMER_A1_BASE, &initCompParam);
}

void Pin_Init(void) {
    //Set output pins
    GPIO_setAsOutputPin(TRIG_PORT, TRIG_PIN);
    GPIO_setOutputLowOnPin(TRIG_PORT, TRIG_PIN);

    GPIO_setAsOutputPin(BUZZ_PORT, BUZZ_PIN);
    GPIO_setOutputLowOnPin(BUZZ_PORT, BUZZ_PIN);

    GPIO_setAsOutputPin(LED_GRN_PORT, LED_GRN_PIN);
    GPIO_setOutputLowOnPin(LED_GRN_PORT, LED_GRN_PIN);

    GPIO_setAsOutputPin(LED_RED_PORT, LED_RED_PIN);
    GPIO_setOutputLowOnPin(LED_RED_PORT, LED_RED_PIN);

    GPIO_setAsOutputPin(LED_YLW_PORT, LED_YLW_PIN);
    GPIO_setOutputLowOnPin(LED_YLW_PORT, LED_YLW_PIN);

    GPIO_setAsOutputPin(LED_ORG_PORT, LED_ORG_PIN);
    GPIO_setOutputLowOnPin(LED_ORG_PORT, LED_ORG_PIN);

    GPIO_setAsOutputPin(LED_PORT, LED_PIN);
    GPIO_setOutputLowOnPin(LED_PORT, LED_PIN);

    //Set input pins
    GPIO_setAsInputPinWithPullUpResistor(ECHO1_PORT, ECHO1_PIN);
    GPIO_disableInterrupt(ECHO1_PORT, ECHO1_PIN);
    GPIO_selectInterruptEdge(ECHO1_PORT, ECHO1_PIN,
                             GPIO_LOW_TO_HIGH_TRANSITION);
    GPIO_clearInterrupt(ECHO1_PORT, ECHO1_PIN);

    GPIO_setAsInputPinWithPullUpResistor(ECHO2_PORT, ECHO2_PIN);
    GPIO_disableInterrupt(ECHO2_PORT, ECHO2_PIN);
    GPIO_selectInterruptEdge(ECHO2_PORT, ECHO2_PIN,
                             GPIO_LOW_TO_HIGH_TRANSITION);
    GPIO_clearInterrupt(ECHO2_PORT, ECHO2_PIN);


    GPIO_setAsInputPinWithPullUpResistor(SET_BTN_PORT, SET_BTN_PIN);
    GPIO_setAsInputPinWithPullUpResistor(NEXT_BTN_PORT, NEXT_BTN_PIN);

    GPIO_enableInterrupt(SET_BTN_PORT, SET_BTN_PIN);
    GPIO_enableInterrupt(NEXT_BTN_PORT, NEXT_BTN_PIN);
    GPIO_selectInterruptEdge(SET_BTN_PORT, SET_BTN_PIN,
                             GPIO_HIGH_TO_LOW_TRANSITION);
    GPIO_selectInterruptEdge(NEXT_BTN_PORT, NEXT_BTN_PIN,
                             GPIO_HIGH_TO_LOW_TRANSITION);

    GPIO_clearInterrupt(SET_BTN_PORT, SET_BTN_PIN);
    GPIO_clearInterrupt(NEXT_BTN_PORT, NEXT_BTN_PIN);

}

/* UART Initialization */
void Init_UART(void)
{
    /* UART: It configures P1.0 and P1.1 to be connected internally to the
     * eSCSI module, which is a serial communications module, and places it
     * in UART mode. This let's you communicate with the PC via a software
     * COM port over the USB cable. You can use a console program, like PuTTY,
     * to type to your LaunchPad. The code in this sample just echos back
     * whatever character was received.
     */

    //Configure UART pins, which maps them to a COM port over the USB cable
    //Set P1.0 and P1.1 as Secondary Module Function Input.
    GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P1, GPIO_PIN1, GPIO_PRIMARY_MODULE_FUNCTION);
    GPIO_setAsPeripheralModuleFunctionOutputPin(GPIO_PORT_P1, GPIO_PIN0, GPIO_PRIMARY_MODULE_FUNCTION);

    /*
     * UART Configuration Parameter. These are the configuration parameters to
     * make the eUSCI A UART module to operate with a 9600 baud rate. These
     * values were calculated using the online calculator that TI provides at:
     * http://software-dl.ti.com/msp430/msp430_public_sw/mcu/msp430/MSP430BaudRateConverter/index.html
     */

    //SMCLK = 1MHz, Baudrate = 9600
    //UCBRx = 6, UCBRFx = 8, UCBRSx = 17, UCOS16 = 1
    EUSCI_A_UART_initParam param = {0};
        param.selectClockSource = EUSCI_A_UART_CLOCKSOURCE_SMCLK;
        param.clockPrescalar    = 6;
        param.firstModReg       = 8;
        param.secondModReg      = 17;
        param.parity            = EUSCI_A_UART_NO_PARITY;
        param.msborLsbFirst     = EUSCI_A_UART_LSB_FIRST;
        param.numberofStopBits  = EUSCI_A_UART_ONE_STOP_BIT;
        param.uartMode          = EUSCI_A_UART_MODE;
        param.overSampling      = 1;

    if(STATUS_FAIL == EUSCI_A_UART_init(EUSCI_A0_BASE, &param))
    {
        return;
    }

    EUSCI_A_UART_enable(EUSCI_A0_BASE);
    char *msg = "UART Enabled\n";
    int i;
    for (i = 0; i < 14; i++) {
        EUSCI_A_UART_transmitData(EUSCI_A0_BASE, msg[i]);
    }
}
