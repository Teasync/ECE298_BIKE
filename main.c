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


#define WAIT_DUR 20000
#define WAIT_CT 20

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

volatile uint16_t front_value_flag;
volatile uint16_t back_value_flag;

volatile uint16_t front_value;
volatile uint16_t back_value;

volatile uint16_t trig_pulse;

Timer_A_initUpModeParam initUpParam0 = { 0 };



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

    while(1) {
        displayScrollText("abcdefghijklmnopqrstuvwxyz");
    }

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
    trig_pulse = 0;

    GPIO_enableInterrupt(ECHO1_PORT, ECHO1_PIN);
    GPIO_disableInterrupt(ECHO2_PORT, ECHO2_PIN);

    //Start both counters
    Timer_A_startCounter(TIMER_A1_BASE, TIMER_A_CONTINUOUS_MODE);
    Timer_A_startCounter(TIMER_A0_BASE, TIMER_A_UP_MODE);

    op_mode = USER_MODE;
    dir_mode = FRONT_MODE;

    //Enter LPM0, enable interrupts
//    __bis_SR_register(LPM0_bits + GIE);

    while(1) {
        if (front_value_flag) {
            showIntFirst3(front_value / 58);

            if (front_value < beep_d1)
            {
                beep(125, 100);
            }
            else if (front_value < beep_d2)
            {
                beep(250, 50);
            }

            front_value_flag = 0;
        }

        if (back_value_flag) {
            showIntLast3(back_value / 58);

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

            back_value_flag = 0;
        }

    }

    //For debugger
//    __no_operation();
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

        __delay_cycles(100);

        temp_val += 20;

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
        front_value = Timer_A_getCounterValue(TIMER_A0_BASE);
        front_value_flag = 1;

        listening_for_rising_edge = 1;
        GPIO_selectInterruptEdge(ECHO1_PORT, ECHO1_PIN,
                                 GPIO_LOW_TO_HIGH_TRANSITION);

        GPIO_disableInterrupt(ECHO1_PORT, ECHO1_PIN);
        dir_mode = BACK_MODE;
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

        __delay_cycles(100);

        next = 1;

        GPIO_clearInterrupt(NEXT_BTN_PORT, NEXT_BTN_PIN);
        GPIO_enableInterrupt(NEXT_BTN_PORT, NEXT_BTN_PIN);
        return;
    }

    //Start timer on rising edge, stop on falling edge, print counter value

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
        back_value = Timer_A_getCounterValue(TIMER_A0_BASE);
        back_value_flag = 1;

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

    if (wait_dur_ct <= WAIT_CT)
    {
        if (trig_pulse) {
            GPIO_setOutputLowOnPin(TRIG_PORT, TRIG_PIN);
            trig_pulse = 0;
        }
        // Set next interrupting value for CCR0
        incr_amt = WAIT_DUR;
        wait_dur_ct++;
    }
    else
    {
        incr_amt = TRIG_DUR;
        wait_dur_ct = 0;
        trig_pulse = 1;
        GPIO_setOutputHighOnPin(TRIG_PORT, TRIG_PIN);
    }

    uint16_t compVal = curr + incr_amt;

    //Add Offset to CCR0
    Timer_A_setCompareValue(TIMER_A1_BASE, TIMER_A_CAPTURECOMPARE_REGISTER_0,
                            compVal);
}
