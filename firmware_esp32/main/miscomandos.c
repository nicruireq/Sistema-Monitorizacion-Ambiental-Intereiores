/*
 * miscomandos.c
 *
 *  Created on: 9 dic. 2020
 *      Author: jcgar
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "esp_log.h"
#include "esp_console.h"
#include "esp_system.h"
#include "esp_sleep.h"
#include "esp_spi_flash.h"
#include "driver/rtc_io.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "argtable3/argtable3.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "miscomandos.h"
#include "sdkconfig.h"
#include "gpio_leds.h"
#include "wifi.h"
#include "mqtt.h"

#include "aqi_config_manager.h"


static int Cmd_led(int argc, char **argv)
{
	if (argc == 1)
	{
		//Si los par�metros no son suficientes, muestro la ayuda
		printf(" LED [on|off]\r\n");
	}
	else
	{
		/* chequeo el parametro */
		if (0==strncmp( argv[1], "on",2))
		{
			printf("Enciendo el LED\r\n");
			gpio_set_level(BLINK_GPIO_1, 1);
		}
		else if (0==strncmp( argv[1], "off",3))
		{
			printf("Apago el LED\r\n");
			gpio_set_level(BLINK_GPIO_1, 0);
		}
		else
		{
			//Si el par�metro no es correcto, muestro la ayuda
			printf(" LED [on|off]\r\n");
		}

	}
    return 0;

}

static void register_Cmd_led(void)
{
    const esp_console_cmd_t cmd = {
        .command = "led",
        .help = "Enciende y apaga el led rojo",
        .hint = NULL,
        .func = &Cmd_led,
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd) );
}

static int Cmd_blink(int argc, char **argv)
{
	int veces;
	int periodo;

	if (argc != 3)
	{
		//Si los par�metros no son suficientes, muestro la ayuda
		printf(" blink [veces] [periodo]\r\n");
	}
	else
	{
		veces=strtoul(argv[1],NULL,10);
		periodo=strtoul(argv[2],NULL,10);

		if ((veces<=0)||(periodo<=0))
		{
			//Si el par�metro no es correcto, muestro la ayuda
			printf(" blink [veces] [periodo]\r\n");
		}
		else
		{
			//G3.3 Hacer algo con los parametros, ahora mismo lo que hago es imprimir....
			printf(" %d %d \r\n",(int)veces,(int)periodo);
		}

	}

	return 0;
}

static void register_Cmd_blink(void)
{
    const esp_console_cmd_t cmd = {
        .command = "blink",
        .help = "Hace parpadear el LED x",
        .hint = NULL,
        .func = &Cmd_blink,
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd) );
}

//Comando modo de los LEDS
static int Cmd_mode(int argc, char **argv)
{
	if (argc == 1)
	{
		//Si los par�metros no son suficientes, muestro la ayuda
		printf(" mode [gpio|pwm]\r\n");
	}
	else
	{
		/* chequeo el parametro */
		if (0==strncmp( argv[1], "gpio",4))
		{
			printf("Modo GPIO\r\n");
			GL_stopLEDC();

		}
		else if (0==strncmp( argv[1], "pwm",3))
		{
			printf("Modo PWM\r\n");
			GL_initLEDC();
		}
		else
		{
			//Si el par�metro no es correcto, muestro la ayuda
			printf(" mode [gpio|pwm]\r\n");
		}

	}
    return 0;

}

static void register_Cmd_mode(void)
{
    const esp_console_cmd_t cmd = {
        .command = "mode",
        .help = "Cambia de modo GPIO a PWM",
        .hint = NULL,
        .func = &Cmd_mode,
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd) );
}

//Comando RGB
static int Cmd_rgb(int argc, char **argv)
{
	if (argc !=4)
	{
		//Si los par�metros no son suficientes, muestro la ayuda
		printf(" rgb <rojo> <verde> <azul>]\r\n");
	}
	else
	{
		uint8_t rgb[3];
		int i;
		for (i=0;i<3;i++)
		{
			rgb[i]=(uint8_t)strtoul(argv[i+1],NULL,10);
		}
		GL_setRGB(rgb);
	}
    return 0;

}

static void register_Cmd_rgb(void)
{
    const esp_console_cmd_t cmd = {
        .command = "rgb",
        .help = "Establece el nivel rgb",
        .hint = NULL,
        .func = &Cmd_rgb,
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd) );
}

//Comando Wifi
static int Cmd_wifi(int argc, char **argv)
{
	if (argc != 2)
	{
		//Si los par�metros no son suficientes, muestro la ayuda
		printf(" wifi [AP|STA]\r\n");
	}
	else
	{
		/* chequeo el parametro */
		if (0==strncmp( argv[1], "AP",2))
		{
			printf("WiFi modo Access Point\r\n");
		    //Conecta a la Wifi o se pone en modo AP
		    wifi_change_to_AP();
		}
		else if (0==strncmp( argv[1], "STA",3))
		{
			printf("WiFi modo Station\r\n");
			wifi_change_to_sta();
		}
		else
		{
			//Si el par�metro no es correcto, muestro la ayuda
			printf(" wifi [AP|STA]\r\n");
		}

	}
    return 0;

}

static void register_Cmd_wifi(void)
{
    const esp_console_cmd_t cmd = {
        .command = "wifi",
        .help = "Establece el modo de la WiFi",
        .hint = NULL,
        .func = &Cmd_wifi,
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd) );
}


//Comando MQTT
static int Cmd_mqtt(int argc, char **argv)
{
	if (argc != 2)
	{
		//Si los par�metros no son suficientes, muestro la ayuda
		printf(" mqtt [uri]\r\n");
	}
	else
	{
		printf("Trying to start MQTT client....");
		mqtt_app_start(argv[1]);
	}
    return 0;

}

static void register_Cmd_mqtt(void)
{
    const esp_console_cmd_t cmd = {
        .command = "mqtt",
        .help = "Arranca el cliente MQTT",
        .hint = NULL,
        .func = &Cmd_mqtt,
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd) );
}

static int Cmd_cfg(int argc, char **argv)
{
	AQI_device_config_data_t current;
	aqi_config_var_t failed = AQI_CV_NOT_VAR;

	ESP_ERROR_CHECK( aqi_config_manager_get_all(&current, &failed) );

	printf("=====FLASH CFG=====\n");
	printf("screen time=%u\n", current.save_screen_seconds);
	printf("room name=%s\n", current.room_name);
	printf("temp_H=%u\n", current.alarm_temp_h);
	printf("temp_L=%u\n", current.alarm_temp_l);
	printf("humidity_H=%u\n", current.alarm_humidity_h);
	printf("humidity_L=%u\n", current.alarm_humidity_l);
	printf("voc_level=%u\n", current.alarm_voc_index);
	printf("reset_wifi_prov=%u\n", current.reset_wifi_provisioning);
	printf("===================\n");

    return 0;

}

static void register_Cmd_read_config(void)
{
    const esp_console_cmd_t cmd = {
        .command = "cfg",
        .help = "Lee config de la flash",
        .hint = NULL,
        .func = &Cmd_cfg,
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd) );
}

void init_MisComandos(void)
{
	register_Cmd_led();
	register_Cmd_blink();
	register_Cmd_mode();
	register_Cmd_rgb();
	register_Cmd_wifi();
#if !CONFIG_EXAMPLE_CONNECT_WIFI_STATION
	register_Cmd_mqtt();
#endif

	register_Cmd_read_config();
}
