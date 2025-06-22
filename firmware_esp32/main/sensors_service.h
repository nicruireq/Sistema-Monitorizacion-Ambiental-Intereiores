/*
 * sensors_service.h
 *
 *  Created on: 26 ene 2025
 *      Author: NRR
 */

#ifndef MAIN_SENSORS_SERVICE_H_
#define MAIN_SENSORS_SERVICE_H_

#include "i2c_master.h"
#include "sgp40driver.h"
#include "sht40driver.h"
#include "sensirion_gas_index_algorithm.h"

#include "global_system_signaler.h"

#include "esp_err.h"

//Include FreeRTOS headers
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// maximum number of measurements before aborting
// when sensors readings fails
#define SENSORS_SERVICE_MAX_RETRIES		3u

/**
 * @brief Initializes I2C controller, SGP40 and SHT40 drivers;
 * 		  and create the task of the sensors service
 *
 * @return Returns ESP_OK if all the configuration steps have been done
 * 		   successfully
 */
esp_err_t sensors_service_init(int sample_rate_ms, i2c_port_t i2c_master_port, gpio_num_t sda_pin,
        gpio_num_t scl_pin);

/**
 * @brief Stop the sensors service task, stop sensors drivers, i2c controller
 * 		  and clean all
 *
 * @return Returns ESP_OK if all the configuration steps have been done
 * 		   successfully
 */
esp_err_t sensors_service_stop();

#endif /* MAIN_SENSORS_SERVICE_H_ */
