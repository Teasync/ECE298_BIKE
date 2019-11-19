//!  Toggle LED1 using software and TA_0 ISR. Toggles every
//!  50000 SMCLK cycles. SMCLK provides clock source for TACLK.
//!  During the TA_0 ISR, LED1 is toggled and 50000 clock cycles are added to
//!  CCR0. TA_0 ISR is triggered every 50000 cycles. CPU is normally off and
//!  used only during TA_ISR.
//!  ACLK = n/a, MCLK = SMCLK = TACLK = default DCO ~1.048MHz

#include "driverlib.h"
#include <msp430.h>
#include <stdio.h>
#include <hal_LCD.h>

#define TRIG_DUR 10

#define WAIT_DUR 20000
#define WAIT_CT 5

#define TRIG_PORT GPIO_PORT_P5
#define TRIG_PIN GPIO_PIN0

#define LED_PORT GPIO_PORT_P4
#define LED_PIN GPIO_PIN0

#define LED_RED_PORT GPIO_PORT_P1
#define LED_RED_PIN GPIO_PIN6

#define LED_ORG_PORT GPIO_PORT_P1
#define LED_ORG_PIN GPIO_PIN7

#define LED_YLW_PORT GPIO_PORT_P1
#define LED_YLW_PIN GPIO_PIN3

#define LED_GRN_PORT GPIO_PORT_P5
#define LED_GRN_PIN GPIO_PIN3

#define ECHO1_PORT GPIO_PORT_P1
#define ECHO1_PIN GPIO_PIN5

#define ECHO2_PORT GPIO_PORT_P2
#define ECHO2_PIN GPIO_PIN7

#define BUZZ_PORT GPIO_PORT_P1
#define BUZZ_PIN GPIO_PIN4

#define SET_BTN_PORT GPIO_PORT_P1
#define SET_BTN_PIN GPIO_PIN2

#define NEXT_BTN_PORT GPIO_PORT_P2
#define NEXT_BTN_PIN GPIO_PIN6

#define FRONT_MODE 0
#define BACK_MODE 1

#define SETUP_MODE 0
#define USER_MODE 1

void beep(int pulse_per, int total_time)
{
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

volatile uint16_t ct = 0xFFFF;
volatile uint16_t poll = 1;
volatile uint32_t wait_dur_ct = 0;
volatile uint16_t listening_for_rising_edge = 1;

volatile uint16_t led_d1;
volatile uint16_t led_d2;
volatile uint16_t led_d3;
volatile uint16_t led_d4;

volatile uint16_t beep_d1;
volatile uint16_t beep_d2;

volatile uint16_t dir_mode;
volatile uint16_t op_mode;

volatile uint16_t next;
volatile uint16_t temp_val;

Timer_A_initUpModeParam initUpParam0 = { 0 };

void Pin_Init(void) {
    //Set output pins
    GPIO_setAsOutputPin(TRIG_PORT, TRIG_PIN);
    GPIO_setOutputLowOnPin(TRIG_PORT, TRIG_PIN);

    GPIO_setAsOutputPin(BUZZ_PORT, BUZZ_PIN);
    GPIO_setOutputLowOnPin(BUZZ_PORT, BUZZ_PIN);

    GPIO_setAsOutputPin(LED_GRN_PORT, LED_GRN_PIN);
    GPIO_setOutputHighOnPin(LED_GRN_PORT, LED_GRN_PIN);

    GPIO_setAsOutputPin(LED_RED_PORT, LED_RED_PIN);
    GPIO_setOutputHighOnPin(LED_RED_PORT, LED_RED_PIN);

    GPIO_setAsOutputPin(LED_YLW_PORT, LED_YLW_PIN);
    GPIO_setOutputHighOnPin(LED_YLW_PORT, LED_YLW_PIN);

    GPIO_setAsOutputPin(LED_ORG_PORT, LED_ORG_PIN);
    GPIO_setOutputHighOnPin(LED_ORG_PORT, LED_ORG_PIN);

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
    initCompParam.compareValue = TRIG_DUR;
    Timer_A_initCompareMode(TIMER_A1_BASE, &initCompParam);
}

void main(void)
{
    //Stop Watchdog Timer
    WDT_A_hold(WDT_A_BASE);

    // TODO: User config mode

    dir_mode = FRONT_MODE;
    op_mode = SETUP_MODE;

    led_d1 = 580;
    led_d2 = 580 * 2;
    led_d3 = 580 * 4;
    led_d4 = 580 * 8;

    beep_d1 = 580 * 2;
    beep_d2 = 580 * 4;


/*    led_d1 = 20;
    led_d2 = 20 * 2;
    led_d3 = 20 * 6;
    led_d4 = 20 * 12;

    beep_d1 = 20 * 2;
    beep_d2 = 20 * 4;*/

    Pin_Init();

    Timer_Init();

    Init_LCD();

    PMM_unlockLPM5();

    next = 0;
    temp_val = 0;

    __enable_interrupt();

    // SETUP mode: Use set button to incr distance by 20, use next button to move to next setting
/*
    displayScrollText("SETUP");

    displayScrollText("FRONT1");

    temp_val = led_d1;
    while (!next) {
        showInt(temp_val);
    }
    next = 0;
    led_d1 = temp_val * 58;
    temp_val = 0;


    displayScrollText("FRONT2");

    temp_val = led_d2;
    while (!next) {
        showInt(temp_val);
    }
    next = 0;
    led_d2 = temp_val * 58;
    temp_val = 0;


    displayScrollText("FRONT3");

    temp_val = led_d3;
    while (!next) {
        showInt(temp_val);
    }
    next = 0;
    led_d3 = temp_val * 58;
    temp_val = 0;


    displayScrollText("FRONT4");

    temp_val = led_d4;
    while (!next) {
        showInt(temp_val);
    }
    next = 0;
    led_d4 = temp_val * 58;
    temp_val = 0;


    displayScrollText("BACK1");

    temp_val = beep_d1;
    while (!next) {
        showInt(temp_val);
    }
    next = 0;
    beep_d1 = temp_val * 58;
    temp_val = 0;


    displayScrollText("BACK2");

    temp_val = beep_d2;
    while (!next) {
        showInt(temp_val);
    }
    next = 0;
    beep_d2 = temp_val * 58;
    temp_val = 0;
*/


    /*
     * Disable the GPIO power-on default high-impedance mode to activate
     * previously configured port settings
     */

    //Start both counters
    Timer_A_startCounter(TIMER_A1_BASE, TIMER_A_CONTINUOUS_MODE);
    Timer_A_startCounter(TIMER_A0_BASE, TIMER_A_UP_MODE);

    op_mode = USER_MODE;

    //Enter LPM0, enable interrupts
    __bis_SR_register(LPM0_bits + GIE);

    //For debugger
    __no_operation();
}

//PORT1 interrupt vector service routine
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=PORT1_VECTOR
__interrupt
#elif defined(__GNUC__)
__attribute__((interrupt(PORT1_VECTOR)))
#endif

void P1_ISR(void)
{
    //Start timer on rising edge, stop on falling edge, print counter value

    if (op_mode == SETUP_MODE) {
        GPIO_disableInterrupt(SET_BTN_PORT, SET_BTN_PIN);

        __delay_cycles(10);

        temp_val += 20;

        GPIO_clearInterrupt(SET_BTN_PORT, SET_BTN_PIN);

        GPIO_enableInterrupt(SET_BTN_PORT, SET_BTN_PIN);
        return;
    }

    if (!GPIO_getInterruptStatus(ECHO1_PORT, ECHO1_PIN) || dir_mode != FRONT_MODE) {
        GPIO_clearInterrupt(ECHO1_PORT, GPIO_PIN_ALL16);
        return;
    }

    if (listening_for_rising_edge == 1)
    {
        Timer_A_initUpMode(TIMER_A0_BASE, &initUpParam0);
        Timer_A_startCounter(TIMER_A0_BASE, TIMER_A_UP_MODE);
        listening_for_rising_edge = 0;
        GPIO_selectInterruptEdge(ECHO1_PORT, ECHO1_PIN,
                                 GPIO_HIGH_TO_LOW_TRANSITION);
    }
    else
    {
        Timer_A_stop(TIMER_A0_BASE);
        ct = Timer_A_getCounterValue(TIMER_A0_BASE);
//        showInt(ct / 58);

        if (ct < beep_d1)
        {
            beep(125, 100);
        }
        else if (ct < beep_d2)
        {
            beep(250, 50);
        }

        listening_for_rising_edge = 1;
        GPIO_selectInterruptEdge(ECHO1_PORT, ECHO1_PIN,
                                 GPIO_LOW_TO_HIGH_TRANSITION);

        GPIO_disableInterrupt(ECHO1_PORT, ECHO1_PIN);
        dir_mode = BACK_MODE;
    }

    //    beep(50);

    GPIO_clearInterrupt(ECHO1_PORT, ECHO1_PIN);
}


//PORT2 interrupt vector service routine
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=PORT2_VECTOR
__interrupt
#elif defined(__GNUC__)
__attribute__((interrupt(PORT2_VECTOR)))
#endif

void P2_ISR(void)
{
    if (op_mode == SETUP_MODE) {
        GPIO_disableInterrupt(NEXT_BTN_PORT, NEXT_BTN_PIN);

        __delay_cycles(10);

        next = 1;

        GPIO_clearInterrupt(NEXT_BTN_PORT, NEXT_BTN_PIN);

        GPIO_enableInterrupt(NEXT_BTN_PORT, NEXT_BTN_PIN);
        return;
    }

    //Start timer on rising edge, stop on falling edge, print counter value

    if (!GPIO_getInterruptStatus(ECHO2_PORT, ECHO2_PIN) || dir_mode != BACK_MODE) {
        GPIO_clearInterrupt(ECHO2_PORT, GPIO_PIN_ALL16);
        return;
    }

    if (listening_for_rising_edge == 1)
    {
        Timer_A_initUpMode(TIMER_A0_BASE, &initUpParam0);
        Timer_A_startCounter(TIMER_A0_BASE, TIMER_A_UP_MODE);
        listening_for_rising_edge = 0;
        GPIO_selectInterruptEdge(ECHO2_PORT, ECHO2_PIN,
                                 GPIO_HIGH_TO_LOW_TRANSITION);
    }
    else
    {
        Timer_A_stop(TIMER_A0_BASE);
        ct = Timer_A_getCounterValue(TIMER_A0_BASE);
        showInt(ct / 58);

        if (ct < led_d1)
        {
            red_on();
        }
        else if (ct < led_d2)
        {
            org_on();
        }
        else if (ct < led_d3)
        {
            ylw_on();
        }
        else if (ct < led_d4)
        {
            grn_on();
        }
        else
        {
            all_off();
        }

        listening_for_rising_edge = 1;
        GPIO_selectInterruptEdge(ECHO2_PORT, ECHO2_PIN,
                                 GPIO_LOW_TO_HIGH_TRANSITION);

        GPIO_disableInterrupt(ECHO2_PORT, ECHO2_PIN);
        dir_mode = FRONT_MODE;
    }

    //    beep(50);

    GPIO_clearInterrupt(ECHO2_PORT, ECHO1_PIN);
}


//TIMER1_A0 interrupt vector service routine.
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=TIMER1_A0_VECTOR
__interrupt
#elif defined(__GNUC__)
__attribute__((interrupt(TIMER1_A0_VECTOR)))
#endif

void TIMER1_A0_ISR(void)
{

    // Main polling interrupt

    uint16_t curr = Timer_A_getCaptureCompareCount(
            TIMER_A1_BASE, TIMER_A_CAPTURECOMPARE_REGISTER_0);
    uint16_t incr_amt;

    if (dir_mode == FRONT_MODE) {
        GPIO_enableInterrupt(ECHO1_PORT, ECHO1_PIN);
        GPIO_disableInterrupt(ECHO2_PORT, ECHO2_PIN);

        GPIO_clearInterrupt(ECHO1_PORT, ECHO1_PIN);
    } else {
        GPIO_enableInterrupt(ECHO2_PORT, ECHO2_PIN);
        GPIO_disableInterrupt(ECHO1_PORT, ECHO1_PIN);

        GPIO_clearInterrupt(ECHO2_PORT, ECHO2_PIN);
    }

    if (wait_dur_ct <= WAIT_CT)
    {
        all_off();
        // Set next interrupting value for CCR0
        incr_amt = WAIT_DUR;

        // Set TRIG to low
//        GPIO_setOutputLowOnPin(TRIG_PORT, TRIG_PIN);

        wait_dur_ct++;
    }
    else
    {
        grn_on();
        incr_amt = TRIG_DUR;

        GPIO_setOutputHighOnPin(TRIG_PORT, TRIG_PIN);
        __delay_cycles(10);
        GPIO_setOutputLowOnPin(TRIG_PORT, TRIG_PIN);

        wait_dur_ct = 0;
    }

    //    GPIO_toggleOutputOnPin(LED_PORT, LED_PIN);

    uint16_t compVal = curr + incr_amt;

    //Add Offset to CCR0
    Timer_A_setCompareValue(TIMER_A1_BASE, TIMER_A_CAPTURECOMPARE_REGISTER_0,
                            compVal);
}
