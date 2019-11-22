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
#include "util.h"


volatile uint16_t ct = 0xFFFF;
uint16_t wait_inner = 0;
uint16_t wait_outer = 0;
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

volatile uint16_t front_value_flag;
volatile uint16_t back_value_flag;

volatile uint16_t front_value;
volatile uint16_t back_value;

volatile uint16_t trig_pulse;

Timer_A_initUpModeParam initUpParam0 = { 0 };

volatile uint16_t last_front_value = 0;
volatile uint16_t last_back_value = 0;


void main(void)
{
    //Stop Watchdog Timer
    WDT_A_hold(WDT_A_BASE);

    dir_mode = FRONT_MODE;
    op_mode = SETUP_MODE;

    led_d1 = PULSE_CONST * 10 * 2;
    led_d2 = PULSE_CONST * 10 * 4;
    led_d3 = PULSE_CONST * 10 * 8;
    led_d4 = PULSE_CONST * 10 * 16;

    beep_d1 = PULSE_CONST * 10 * 2;
    beep_d2 = PULSE_CONST * 10 * 4;

    Pin_Init();

    Timer_Init();

    Init_LCD();

    PMM_unlockLPM5();

    next = 0;
    temp_val = 0;

    __enable_interrupt();


    // SETUP mode: Use set button to incr distance by 20, use next button to move to next setting

    displayScrollText("SETUP");

    temp_val = led_d1;
    while (!next) {
        showStrDistCM("Fr1", temp_val / PULSE_CONST);
    }
    next = 0;
    led_d1 = temp_val;
    temp_val = 0;


    temp_val = led_d2;
    while (!next) {
        showStrDistCM("Fr2", temp_val / PULSE_CONST);
    }
    next = 0;
    led_d2 = temp_val;
    temp_val = 0;


    temp_val = led_d3;
    while (!next) {
        showStrDistCM("Fr3", temp_val / PULSE_CONST);
    }
    next = 0;
    led_d3 = temp_val;
    temp_val = 0;


    temp_val = led_d4;
    while (!next) {
        showStrDistCM("Fr4", temp_val / PULSE_CONST);
    }
    next = 0;
    led_d4 = temp_val;
    temp_val = 0;


    temp_val = beep_d1;
    while (!next) {
        showStrDistCM("Bk1", temp_val / PULSE_CONST);
    }
    next = 0;
    beep_d1 = temp_val;
    temp_val = 0;


    temp_val = beep_d2;
    while (!next) {
        showStrDistCM("Bk2", temp_val / PULSE_CONST);
    }
    next = 0;
    beep_d2 = temp_val;
    temp_val = 0;

    /*
     * Disable the GPIO power-on default high-impedance mode to activate
     * previously configured port settings
     */
    trig_pulse = 0;

//    GPIO_enableInterrupt(ECHO1_PORT, ECHO1_PIN);
//    GPIO_disableInterrupt(ECHO2_PORT, ECHO2_PIN);
    GPIO_enableInterrupt(ECHO2_PORT, ECHO2_PIN);

    GPIO_disableInterrupt(SET_BTN_PORT, SET_BTN_PIN);
    GPIO_disableInterrupt(NEXT_BTN_PORT, NEXT_BTN_PIN);

    //Start both counters
//    Timer_A_startCounter(TIMER_A1_BASE, TIMER_A_CONTINUOUS_MODE);
//    Timer_A_startCounter(TIMER_A0_BASE, TIMER_A_UP_MODE);

    op_mode = USER_MODE;
    dir_mode = FRONT_MODE;


    //Enter LPM0, enable interrupts
//    __bis_SR_register(LPM0_bits + GIE);
    showIntB(0);
    showIntF(0);

    while(1) {
        if (wait_inner >= WAIT_DUR) {
            ++wait_outer;
            wait_inner = 0;
        } else {
            ++wait_inner;
        }

        if (wait_outer >= WAIT_CT) {
            __disable_interrupt();

            GPIO_setOutputHighOnPin(TRIG_PORT, TRIG_PIN);

            __delay_cycles(10);

            GPIO_setOutputLowOnPin(TRIG_PORT, TRIG_PIN);

            __enable_interrupt();
            wait_outer = 0;
        }

        // Send a trigger pulse signal
    }

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

        __delay_cycles(150000);

        temp_val += PULSE_CONST * 10 * 1;

        GPIO_clearInterrupt(SET_BTN_PORT, SET_BTN_PIN);
        GPIO_enableInterrupt(SET_BTN_PORT, SET_BTN_PIN);
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

        listening_for_rising_edge = 1;
        GPIO_selectInterruptEdge(ECHO1_PORT, ECHO1_PIN,
                                 GPIO_LOW_TO_HIGH_TRANSITION);

        GPIO_disableInterrupt(ECHO1_PORT, ECHO1_PIN);

        front_value = Timer_A_getCounterValue(TIMER_A0_BASE);
        showIntF(front_value / PULSE_CONST);

        if (front_value < beep_d1)
        {
            beep(60, 40);
        }
        else if (front_value < beep_d2)
        {
            beep(120, 20);
        }

        GPIO_enableInterrupt(ECHO2_PORT, ECHO2_PIN);
    }

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

        __delay_cycles(150000);

        next = 1;

        GPIO_clearInterrupt(NEXT_BTN_PORT, NEXT_BTN_PIN);
        GPIO_enableInterrupt(NEXT_BTN_PORT, NEXT_BTN_PIN);
        return;
    }

    //Start timer on rising edge, stop on falling edge, print counter value

    if (listening_for_rising_edge == 1)
    {
        Timer_A_initUpMode(TIMER_A1_BASE, &initUpParam0);
        Timer_A_startCounter(TIMER_A1_BASE, TIMER_A_UP_MODE);
        listening_for_rising_edge = 0;
        GPIO_selectInterruptEdge(ECHO2_PORT, ECHO2_PIN,
                                 GPIO_HIGH_TO_LOW_TRANSITION);
    }
    else
    {
        Timer_A_stop(TIMER_A1_BASE);

        listening_for_rising_edge = 1;
        GPIO_selectInterruptEdge(ECHO2_PORT, ECHO2_PIN,
                                 GPIO_LOW_TO_HIGH_TRANSITION);

//        GPIO_disableInterrupt(ECHO2_PORT, ECHO2_PIN);

        back_value = Timer_A_getCounterValue(TIMER_A1_BASE);
        showIntB(back_value / PULSE_CONST);

        if (back_value < led_d1)
        {
            red_on();
        }
        else if (back_value < led_d2)
        {
            org_on();
        }
        else if (back_value < led_d3)
        {
            ylw_on();
        }
        else if (back_value < led_d4)
        {
            grn_on();
        }
        else
        {
            all_off();
        }
//        GPIO_enableInterrupt(ECHO1_PORT, ECHO1_PIN);
    }

//        beep(50);

    GPIO_clearInterrupt(ECHO2_PORT, ECHO2_PIN);
}


//TIMER1_A0 interrupt vector service routine.
//#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
//#pragma vector=TIMER1_A0_VECTOR
//__interrupt
//#elif defined(__GNUC__)
//__attribute__((interrupt(TIMER1_A0_VECTOR)))
//#endif
//
//void TIMER1_A0_ISR(void)
//{
//
//    // Main polling interrupt
//    uint16_t curr = Timer_A_getCaptureCompareCount(
//            TIMER_A1_BASE, TIMER_A_CAPTURECOMPARE_REGISTER_0);
//
//    trig_pulse = 1;
//
//    uint16_t compVal = curr + WAIT_DUR;
//
//    //Add Offset to CCR0
//    Timer_A_setCompareValue(TIMER_A1_BASE, TIMER_A_CAPTURECOMPARE_REGISTER_0,
//                            compVal);
//
//    Timer_A_clearCaptureCompareInterrupt(TIMER_A1_BASE, TIMER_A_CAPTURECOMPARE_REGISTER_0);
//}
