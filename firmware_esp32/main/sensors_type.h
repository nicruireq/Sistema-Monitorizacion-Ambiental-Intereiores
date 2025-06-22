/*
 * sensors_type.h
 *
 *  Created on: 25 ene 2025
 *      Author: Sigma
 */

#ifndef MAIN_SENSORS_TYPE_H_
#define MAIN_SENSORS_TYPE_H_

#include "esp_err.h"

#include "frozen.h"

typedef struct Sensors_data_t
{
	uint16_t voc_raw;
	uint16_t voc_index;
	uint16_t temperature_celsius;
	uint16_t relative_humidity;
} Sensors_data_t;

typedef Sensors_data_t* Sensors_data_ptr;

/**
 * @brief Creates a new instance of a Sensors_data_t.
 * If sensor_data is NULL, creates a new structure in the heap
 * and intializes it with the data passed as arguments.
 *
 * @param[out] sensor_data pointer to a Sensosr_data_ptr which
 * 		  pointed structure must be created and initialized
 * @param[in] voc_raw
 * @param[in] voc_index
 * @param[in] temperature_celsius
 * @param[in] relative_humidity
 *
 * @return Returns ESP_OK if a new structure has been allocated.
 * 		   Returns ESP_FAIL if the new structure can not be allocated in the heap
 * 		   and the creations fails.
 *
 * @warning If the contents of out_sensor_data is not null (It is currently pointing to
 * 			a previously allocated structure) you may lose the pointer to some allocated memory
 */
bool sensors_type_create(Sensors_data_ptr* out_sensor_data , uint16_t voc_raw,
						uint16_t voc_index, uint16_t temperature_celsius,
						uint16_t relative_humidity);

/**
 * @brief Function to generate JSON string with the data from sensors
 * 		  The JSON has the format:
 *
 * @param json_buffer		pointer to c-string to be populated with the generated JSON
 * @param buffer_size		length of json_buffer
 * @param sensors_data		pointer to a Sensors_data_t object with the data to generate
 * 							the JSON
 * @return Returns ESP_OK if data is well formatted and it is less than the buffer size.
 * 		   Returns ESP_ERR_INVALID_ARG if input data or buffer are null.
 * 		   Returns ESP_ERR_INVALID_SIZE if the JSON string generated is
 * 		   greater than the buffer_size.
 */
esp_err_t sensors_type_to_JSON(struct json_out * json_buffer, uint32_t buffer_size, Sensors_data_ptr sensors_data);

/**
 * @brief Function to parse incoming sensors_type data inside a C string
 * 			in json format to Sensors_type_t
 *
 * @param data			Incoming sensors_data datatype in json string format
 * @param data_len		length of data
 * @param sensors_data	pointer to a Sensors_data_t object
 * @return Returns ESP_OK if data is well formatted else returns ESP_FAIL
 */
//esp_err_t sensors_type_parse(char *data, int data_len, Sensors_data_ptr sensors_data);

#endif /* MAIN_SENSORS_TYPE_H_ */
