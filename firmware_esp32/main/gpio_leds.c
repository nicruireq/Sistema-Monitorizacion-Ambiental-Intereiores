/*
 * gpio_leds.c
 *
 *  Created on: 6 dic. 2020
 *      Author: jcgar
 */

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "esp_system.h"
#include "esp_log.h"

#include "driver/uart.h"

#include "driver/adc.h"
#include "driver/ledc.h"
#include "driver/dac.h"
#include "gpio_leds.h"

static const char *TAG = "GPIO_LED";

static uint8_t g_RGBValues[3]={0,0,0};
static const int g_Channels[3]={LEDC_CHANNEL_0,LEDC_CHANNEL_1,LEDC_CHANNEL_2};
static const int g_Pins[3]={BLINK_GPIO_1,BLINK_GPIO_2,BLINK_GPIO_3};
static bool g_ledcstatus=0;

#define LEDC_HS_TIMER          LEDC_TIMER_0
#define LEDC_HS_MODE           LEDC_HIGH_SPEED_MODE
#define LEDC_HS_CH0_GPIO       (0)
#define LEDC_HS_CH0_CHANNEL    LEDC_CHANNEL_0

void GL_initGPIO(void)
{
	//Configura los pines del LED tricolor como salida.

	gpio_reset_pin(BLINK_GPIO_1);
	gpio_reset_pin(BLINK_GPIO_2);
	gpio_reset_pin(BLINK_GPIO_3);
	/* Set the GPIO as a push/pull output */
	gpio_set_direction(BLINK_GPIO_1, GPIO_MODE_OUTPUT);
	gpio_set_direction(BLINK_GPIO_2, GPIO_MODE_OUTPUT);
	gpio_set_direction(BLINK_GPIO_3, GPIO_MODE_OUTPUT);
}

void GL_initLEDC()
{
	int ch;

    ledc_timer_config_t ledc_timer = {
        .duty_resolution = LEDC_TIMER_8_BIT, // resolution of PWM duty
        .freq_hz = 5000,                      // frequency of PWM signal
        .speed_mode = LEDC_LOW_SPEED_MODE,           // timer mode
        .timer_num = LEDC_TIMER_1,            // timer index
        .clk_cfg = LEDC_AUTO_CLK,              // Auto select the source clock
    };
    // Set configuration of timer0 for high speed channels
    ledc_timer_config(&ledc_timer);


    ledc_channel_config_t ledc_channel[3] = {
            {
                .channel    = LEDC_CHANNEL_0,
                .duty       = 0,
                .gpio_num   = g_Pins[0],
                .speed_mode = LEDC_LOW_SPEED_MODE,
                .hpoint     = 0,
                .timer_sel  = LEDC_TIMER_1
            },
            {
                .channel    = LEDC_CHANNEL_1,
                .duty       = 0,
                .gpio_num   = g_Pins[1],
                .speed_mode = LEDC_LOW_SPEED_MODE,
                .hpoint     = 0,
                .timer_sel  = LEDC_TIMER_1
            },
            {
                .channel    = LEDC_CHANNEL_2,
                .duty       = 0,
                .gpio_num   = g_Pins[2],
                .speed_mode = LEDC_LOW_SPEED_MODE,
                .hpoint     = 0,
                .timer_sel  = LEDC_TIMER_1
            }
        };

    	ledc_fade_func_install(0);

        // Set LED Controller with previously prepared configuration
        for (ch = 0; ch < 3; ch++) {
            ledc_channel_config(&ledc_channel[ch]);
            ledc_set_duty_and_update(LEDC_LOW_SPEED_MODE,g_Channels[ch],g_RGBValues[ch],0);
        }



        g_ledcstatus=true;
}

extern void GL_setRGB(uint8_t *RGBLevels)
{
	int ch;
	if (g_ledcstatus)
	{
		for (ch = 0; ch < 3; ch++) {
				g_RGBValues[ch]=RGBLevels[ch];
		        ledc_set_duty_and_update(LEDC_LOW_SPEED_MODE,g_Channels[ch],RGBLevels[ch],0);
		}
	}
}

extern void GL_stopLEDC()
{
	int ch;
	if (g_ledcstatus)
		{
			for (ch = 0; ch < 3; ch++) {
			            ledc_stop(LEDC_LOW_SPEED_MODE,g_Channels[ch],0);
			}
			ledc_fade_func_uninstall();
			GL_initGPIO();
		}
	g_ledcstatus=false;
}

