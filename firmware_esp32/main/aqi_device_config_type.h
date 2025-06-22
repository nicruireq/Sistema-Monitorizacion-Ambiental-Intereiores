/*
 * aqi_device_config.h
 *
 *  Created on: 18 feb 2025
 *      Author: NRR
 */

#ifndef MAIN_AQI_DEVICE_CONFIG_TYPE_H_
#define MAIN_AQI_DEVICE_CONFIG_TYPE_H_

#include "esp_err.h"

#include "frozen.h"

#define AQI_MAX_ROOM_NAME_SZ	16

typedef struct AQI_device_config_data_t
{
	uint8_t save_screen_seconds;
	char room_name[AQI_MAX_ROOM_NAME_SZ];
	uint16_t alarm_temp_h;
	uint16_t alarm_temp_l;
	uint16_t alarm_humidity_h;
	uint16_t alarm_humidity_l;
	uint16_t alarm_voc_index;
	bool reset_wifi_provisioning;	// true: queremos entrar en modo wifi provisioning
} AQI_device_config_data_t;

typedef AQI_device_config_data_t* AQI_device_config_data_t_ptr;

/**
 * @brief Function to initialize an AQI_device_config_data_t structure with provided configuration values.
 *			This function sets all fields of the AQI_device_config_data_t structure, including screen timeout,
 * 			room name, alarm thresholds for temperature and humidity, VOC index, and Wi-Fi provisioning flag.
 * 			It ensures the room name is safely copied and null-terminated.
 *
 * @param device_config_data         Pointer to the AQI_device_config_data_t structure to initialize.
 * @param save_screen_seconds        Timeout in seconds before the screen turns off.
 * @param room_name                  Null-terminated string representing the room name (can be NULL).
 * @param alarm_temp_h               High temperature alarm threshold.
 * @param alarm_temp_l               Low temperature alarm threshold.
 * @param alarm_humidity_h           High humidity alarm threshold.
 * @param alarm_humidity_l           Low humidity alarm threshold.
 * @param alarm_voc_index            VOC index alarm threshold.
 * @param reset_wifi_provisioning    Boolean flag to trigger Wi-Fi provisioning mode.
 * @return Returns true if initialization was successful, false if the input pointer is NULL.
 */
bool aqi_device_config_data_type_init(AQI_device_config_data_t_ptr device_config_data ,
										uint8_t save_screen_seconds,
										const char * room_name,
										uint16_t alarm_temp_h,
										uint16_t alarm_temp_l,
										uint16_t alarm_humidity_h,
										uint16_t alarm_humidity_l,
										uint16_t alarm_voc_index,
										bool reset_wifi_provisioning);

/**
 * @brief Function to clone the contents of one AQI_device_config_data_t structure into another.
 *			This function performs a deep copy of all fields from the source AQI_device_config_data_t
 * 			structure to the destination structure, including safe copying of the room_name string.
 *
 * @param src   Pointer to the source AQI_device_config_data_t structure.
 * @param dest  Pointer to the destination AQI_device_config_data_t structure.
 * @return Returns true if the cloning was successful, false if either pointer is NULL.
 */
bool aqi_device_config_data_clone(const AQI_device_config_data_t_ptr src, AQI_device_config_data_t_ptr dest);


/**
 * @brief Function to parse incoming AQI_device_config_data_t data inside a C string
 * 			in json format to a AQI_device_config_data_t
 *
 * @param data			Incoming sensors_data datatype in json string format
 * @param data_len		length of data
 * @param sensors_data	pointer to a AQI_device_config_data_t object
 * @return Returns ESP_OK if data is well formatted else returns ESP_FAIL
 */
esp_err_t aqi_device_config_data_type_parse(const char *data, int data_len,
								AQI_device_config_data_t_ptr device_config_data);


#endif /* MAIN_AQI_DEVICE_CONFIG_TYPE_H_ */
