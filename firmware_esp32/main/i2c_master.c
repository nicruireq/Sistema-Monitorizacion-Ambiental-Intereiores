/**
 *
 */

#include "driver/i2c.h"
#include "esp_err.h"
#include "esp_log.h"


static bool i2c_port_0_started = false;
static bool i2c_port_1_started = false;

//Logging TAG for this module I2C_master
static const char *TAG = "I2C_master";

/**
 *
 */
esp_err_t i2c_master_init(i2c_port_t i2c_master_port, gpio_num_t sda_io_num,
                                    gpio_num_t scl_io_num,  gpio_pullup_t pull_up_en, bool fastMode)
{
	esp_err_t result = ESP_OK;

	ESP_LOGD(TAG, "Begin i2c_master_init");

	// Only continue if the desired port has not been initialized previously
	if ( ((i2c_master_port == I2C_NUM_0) && !i2c_port_0_started)
			|| ((i2c_master_port == I2C_NUM_1) && !i2c_port_1_started) )
	{
		ESP_LOGD(TAG, "Entra en el if para inicializar el puerto i2c");

		// Configure I2C basic parameters
		i2c_config_t conf;

		conf.mode = I2C_MODE_MASTER;
		conf.sda_io_num = sda_io_num;
		conf.sda_pullup_en = pull_up_en;
		conf.scl_io_num = scl_io_num;
		conf.scl_pullup_en = pull_up_en;
		if(fastMode)
			conf.master.clk_speed = 400000;
		else
			conf.master.clk_speed = 100000;
		conf.clk_flags = 0; // asegurar compatibilidad con ESP-IDF v5.x

		if ((result = i2c_param_config(i2c_master_port, &conf)) == ESP_OK)
		{
			esp_err_t i2c_installation_state =i2c_driver_install(i2c_master_port, conf.mode, 0, 0, 0);

			ESP_LOGD(TAG, "Driver install devuelve: %d", i2c_installation_state);

			if (i2c_installation_state == ESP_OK)
			{
				// mark what i2c port has been installed
				i2c_port_0_started = (i2c_master_port == I2C_NUM_0) ? true : false;
				i2c_port_1_started = (i2c_master_port == I2C_NUM_1) ? true : false;

				if (i2c_set_timeout(i2c_master_port,1000000) != ESP_OK)
				{
					ESP_LOGD(TAG, "i2c set timeout devuelve ESP_ERR_INVALID_ARG ");
				}

				result = i2c_installation_state;
			}
		}
		else
		{
			ESP_LOGD(TAG, "i2c_param_config returns ESP_ERR_INVALID_ARG ");
		}
	}
	else
	{
		result = ESP_FAIL;

		if (i2c_port_0_started)
		{
			ESP_LOGD(TAG, "Port I2C_NUM_0 has already been initialized before");
		}

		if (i2c_port_1_started)
		{
			ESP_LOGD(TAG, "Port I2C_NUM_1 has already been initialized before");
		}

		if ((i2c_master_port != I2C_NUM_0) && (i2c_master_port != I2C_NUM_1))
		{
			ESP_LOGD(TAG, "Unknown I2C port");
		}
	}

	ESP_LOGD(TAG, "End i2c_master_init");

	return (result);
}


/**
 * Uninstall i2c port
 */
esp_err_t i2c_master_close(i2c_port_t i2c_master_port)
{
	ESP_LOGD(TAG, "Begin i2c_master_close");

	esp_err_t result = ESP_FAIL;

	if ( ((i2c_master_port == I2C_NUM_0) && i2c_port_0_started)
				|| ((i2c_master_port == I2C_NUM_1) && i2c_port_1_started) )
	{
		result = i2c_driver_delete(i2c_master_port);
		// if uninstall has gone successfully mark the i2c port
		// as not started
		if (result == ESP_OK)
		{
			i2c_port_0_started = (i2c_master_port == I2C_NUM_0) ? false : i2c_port_0_started;
			i2c_port_1_started = (i2c_master_port == I2C_NUM_1) ? false : i2c_port_1_started;

			ESP_LOGD(TAG, "i2c_master_close, port %d uninstalled", i2c_master_port);
		}
	}
	else
	{
		ESP_LOGD(TAG, "i2c_master_close fail on port %d", i2c_master_port);
	}

	ESP_LOGD(TAG, "End i2c_master_close");

	return result;
}
