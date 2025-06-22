/*
 * aqi_alarm_triggers.h
 *
 *  Created on: 7 may 2025
 *      Author: NRR
 */

#ifndef MAIN_AQI_ALARM_TRIGGERS_H_
#define MAIN_AQI_ALARM_TRIGGERS_H_

#include <stdbool.h>
#include "sensors_type.h"           // Define Sensors_data_ptr
#include "aqi_device_config_type.h"    // Define AQI_device_config_data_t_ptr

/**
 * @brief Tipo de función para evaluar condiciones de activacion de alarmas.
 */
typedef bool evaluate_alarm_trigger_condition(const Sensors_data_ptr sensor_data,
                                              const AQI_device_config_data_t_ptr config);

/* Declaraciones de funciones evaluadoras */
bool aqi_is_temperature_below_min(const Sensors_data_ptr sensor_data, const AQI_device_config_data_t_ptr config);
bool aqi_is_temperature_above_max(const Sensors_data_ptr sensor_data, const AQI_device_config_data_t_ptr config);
bool aqi_is_humidity_below_min(const Sensors_data_ptr sensor_data, const AQI_device_config_data_t_ptr config);
bool aqi_is_humidity_above_max(const Sensors_data_ptr sensor_data, const AQI_device_config_data_t_ptr config);
bool aqi_is_voc_index_above_limit(const Sensors_data_ptr sensor_data, const AQI_device_config_data_t_ptr config);

#endif /* MAIN_AQI_ALARM_TRIGGERS_H_ */
