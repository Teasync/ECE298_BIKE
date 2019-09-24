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

#define LOW_LED_PORT GPIO_PORT_P1
#define LOW_LED_PIN GPIO_PIN3

#define HI_LED_PORT GPIO_PORT_P5
#define HI_LED_PIN GPIO_PIN3

#define ECHO_PORT GPIO_PORT_P1
#define ECHO_PIN GPIO_PIN5

#define BUZZ_PORT GPIO_PORT_P1
#define BUZZ_PIN GPIO_PIN4


void beep(int per, int len) {
    int i;
    for (i = 0; i < len; i++) {
        GPIO_toggleOutputOnPin(BUZZ_PORT, BUZZ_PIN);    //Set P1.2...

        int j;
        for (j = 0; j < per; j++) {
            __delay_cycles(1);
        }
    }
}

volatile uint16_t ct = 0xFFFF;
volatile uint16_t poll = 1;
volatile uint32_t wait_counter = 0;
volatile uint16_t listening_for_rising_edge = 1;
Timer_A_initUpModeParam initUpParam0 = {0};

void main(void) {
    //Stop Watchdog Timer
    WDT_A_hold(WDT_A_BASE);

    //Set output pins
    GPIO_setAsOutputPin(TRIG_PORT, TRIG_PIN);
    GPIO_setOutputHighOnPin(TRIG_PORT, TRIG_PIN);

    GPIO_setAsOutputPin(BUZZ_PORT, BUZZ_PIN);
    GPIO_setOutputHighOnPin(BUZZ_PORT, BUZZ_PIN);

    GPIO_setAsOutputPin(HI_LED_PORT, HI_LED_PIN);
    GPIO_setOutputHighOnPin(HI_LED_PORT, HI_LED_PIN);

    GPIO_setAsOutputPin(LOW_LED_PORT, HI_LED_PIN);
    GPIO_setOutputHighOnPin(LOW_LED_PORT, LOW_LED_PIN);

    GPIO_setAsOutputPin(BUZZ_PORT, BUZZ_PIN);
    GPIO_setOutputHighOnPin(BUZZ_PORT, BUZZ_PIN);

    //Set input pins
    GPIO_setAsInputPinWithPullUpResistor(ECHO_PORT, ECHO_PIN);
    GPIO_enableInterrupt(ECHO_PORT, ECHO_PIN);
    GPIO_selectInterruptEdge(ECHO_PORT, ECHO_PIN, GPIO_LOW_TO_HIGH_TRANSITION);
    GPIO_clearInterrupt(ECHO_PORT, ECHO_PIN);

    Init_LCD();

    /*
     * Disable the GPIO power-on default high-impedance mode to activate
     * previously configured port settings
     */
    PMM_unlockLPM5();

    //Start timer in continuous mode sourced by SMCLK
    //Initialize Timer 1
    Timer_A_initContinuousModeParam initContParam1 = {0};
    initContParam1.clockSource = TIMER_A_CLOCKSOURCE_SMCLK;
    initContParam1.clockSourceDivider = TIMER_A_CLOCKSOURCE_DIVIDER_1;
    initContParam1.timerInterruptEnable_TAIE = TIMER_A_TAIE_INTERRUPT_DISABLE;
    initContParam1.timerClear = TIMER_A_DO_CLEAR;
    initContParam1.startTimer = false;
    Timer_A_initContinuousMode(TIMER_A1_BASE, &initContParam1);

    //Initialize Timer 0
    initUpParam0.clockSource = TIMER_A_CLOCKSOURCE_SMCLK;
    initUpParam0.clockSourceDivider = TIMER_A_CLOCKSOURCE_DIVIDER_1;
    initUpParam0.timerPeriod = 0xFFFF;
    initUpParam0.timerInterruptEnable_TAIE = TIMER_A_TAIE_INTERRUPT_DISABLE;
    initUpParam0.timerClear = TIMER_A_DO_CLEAR;
    initUpParam0.startTimer = false;
    Timer_A_initUpMode(TIMER_A0_BASE, &initUpParam0);

    //Initialize compare mode for Timer 1
    Timer_A_clearCaptureCompareInterrupt(TIMER_A1_BASE, TIMER_A_CAPTURECOMPARE_REGISTER_0);

    Timer_A_initCompareModeParam initCompParam = {0};
    initCompParam.compareRegister = TIMER_A_CAPTURECOMPARE_REGISTER_0;
    initCompParam.compareInterruptEnable = TIMER_A_CAPTURECOMPARE_INTERRUPT_ENABLE;
    initCompParam.compareOutputMode = TIMER_A_OUTPUTMODE_OUTBITVALUE;
    initCompParam.compareValue = TRIG_DUR;
    Timer_A_initCompareMode(TIMER_A1_BASE, &initCompParam);

    //Start both counters
    Timer_A_startCounter(TIMER_A1_BASE, TIMER_A_CONTINUOUS_MODE);
    Timer_A_startCounter(TIMER_A0_BASE, TIMER_A_UP_MODE);

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

void P1_ISR(void) {
    //Start timer on rising edge, stop on falling edge, print counter value

    if (listening_for_rising_edge == 1) {
        Timer_A_initUpMode(TIMER_A0_BASE, &initUpParam0);
        Timer_A_startCounter(TIMER_A0_BASE, TIMER_A_UP_MODE);
        listening_for_rising_edge = 0;
        GPIO_selectInterruptEdge(ECHO_PORT, ECHO_PIN, GPIO_HIGH_TO_LOW_TRANSITION);
    } else {
        Timer_A_stop(TIMER_A0_BASE);
        ct = Timer_A_getCounterValue(TIMER_A0_BASE);
        showInt(ct/58);
        if (ct < 580) {
            beep(125, 100);
            GPIO_setOutputHighOnPin(LOW_LED_PORT, LOW_LED_PIN);
            GPIO_setOutputLowOnPin(HI_LED_PORT, HI_LED_PIN);
        } else if (ct < 2900) {
            GPIO_setOutputHighOnPin(HI_LED_PORT, HI_LED_PIN);
            GPIO_setOutputLowOnPin(LOW_LED_PORT, LOW_LED_PIN);
            beep(250, 50);
        } else {
            GPIO_setOutputLowOnPin(HI_LED_PORT, HI_LED_PIN);
            GPIO_setOutputLowOnPin(LOW_LED_PORT, LOW_LED_PIN);
        }
        listening_for_rising_edge = 1;
        GPIO_selectInterruptEdge(ECHO_PORT, ECHO_PIN, GPIO_LOW_TO_HIGH_TRANSITION);
    }

//    beep(50);

    GPIO_clearInterrupt(ECHO_PORT, ECHO_PIN);
}

//TIMER1_A0 interrupt vector service routine.
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=TIMER1_A0_VECTOR
__interrupt
#elif defined(__GNUC__)
__attribute__((interrupt(TIMER1_A0_VECTOR)))
#endif

void TIMER1_A0_ISR(void) {

    uint16_t curr = Timer_A_getCaptureCompareCount(TIMER_A1_BASE, TIMER_A_CAPTURECOMPARE_REGISTER_0);
    uint16_t incr_amt;

    if (wait_counter <= 5) {
        incr_amt = WAIT_DUR;
    } else {
        incr_amt = TRIG_DUR;
    }

    //Toggle TRIG
    if (wait_counter > 5) {
        GPIO_setOutputHighOnPin(TRIG_PORT, TRIG_PIN);
    } else {
        GPIO_setOutputLowOnPin(TRIG_PORT, TRIG_PIN);
    }

    if (wait_counter > 5) {
        wait_counter = 0;
    } else {
        wait_counter++;
    }

//    GPIO_toggleOutputOnPin(LED_PORT, LED_PIN);

    uint16_t compVal = curr + incr_amt;

    //Add Offset to CCR0
    Timer_A_setCompareValue(TIMER_A1_BASE, TIMER_A_CAPTURECOMPARE_REGISTER_0, compVal);
}
