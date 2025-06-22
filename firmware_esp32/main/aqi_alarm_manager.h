/*
 * aqi_alarm_manager.h
 *
 *  Created on: 5 may 2025
 *      Author: Sigma
 */

#ifndef MAIN_AQI_ALARM_MANAGER_H_
#define MAIN_AQI_ALARM_MANAGER_H_

#include "sensors_type.h"
#include "alarm_type.h"
#include "aqi_device_config_type.h"

typedef bool evaluate_alarm_trigger_condition(const Sensors_data_ptr, const AQI_device_config_data_t_ptr);

/**
 * @brief Inicializa los estados de activación de alarmas
 * 		  como desactivadas
 */
void aqi_alarm_manager_init();


void aqi_alarm_manager_evaluate(Sensors_data_ptr incoming_sensor_data);


#endif /* MAIN_AQI_ALARM_MANAGER_H_ */
