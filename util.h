#pragma once

#include "driverlib.h"
#include <msp430.h>
#include <stdio.h>
#include <hal_LCD.h>


#define WAIT_DUR 20000
#define WAIT_CT 5

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

#define BUZZ_PORT GPIO_PORT_P1
#define BUZZ_PIN GPIO_PIN4


#define TRIG_PORT GPIO_PORT_P5
#define TRIG_PIN GPIO_PIN0

#define ECHO1_PORT GPIO_PORT_P1
#define ECHO1_PIN GPIO_PIN5

#define ECHO2_PORT GPIO_PORT_P2
#define ECHO2_PIN GPIO_PIN7


#define FRONT_MODE 0
#define BACK_MODE 1

#define SETUP_MODE 0
#define USER_MODE 1


#define SET_BTN_PORT GPIO_PORT_P1
#define SET_BTN_PIN GPIO_PIN2

#define NEXT_BTN_PORT GPIO_PORT_P2
#define NEXT_BTN_PIN GPIO_PIN6


#define TRIG_DUR 10


void grn_on(void);
void ylw_on(void);
void org_on(void);
void red_on(void);
void all_off(void);
void beep(int pulse_per, int total_time);
void Timer_Init(void);
void Pin_Init(void);
void Init_UART(void);
