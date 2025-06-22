/*
 * alarm_type.h
 *
 *  Created on: 6 may 2025
 *      Author: NRR
 */


#ifndef MAIN_ALARM_TYPE_H_
#define MAIN_ALARM_TYPE_H_

#include "esp_err.h"
#include "frozen.h"

typedef enum
{
    AC_TEMP_H,
    AC_TEMP_L,
    AC_HUM_H,
    AC_HUM_L,
    AC_VOC_LIMIT,
    AC_MAX_CLASSES
} Alarm_class;

typedef struct Alarm_data_t
{
    bool disable;
    Alarm_class alarm_class;
    const char* info;
} Alarm_data_t;

typedef Alarm_data_t* Alarm_data_ptr;

/**
 * For debug only
 */
const char* alarm_class_to_string(Alarm_class aclass);

/**
 * @brief Creates a new instance of an Alarm_data_t.
 * If alarm_data is NULL, creates a new structure in the heap
 * and initializes it with the data passed as arguments.
 *
 * @param[out] alarm_data pointer to an Alarm_data_ptr which
 *        pointed structure must be created and initialized
 * @param[in] disable
 * @param[in] alarm_class
 *
 * @return Returns ESP_OK if a new structure has been allocated.
 *         Returns ESP_FAIL if the new structure cannot be allocated in the heap
 *         and the creation fails.
 *
 * @warning If the contents of out_alarm_data is not null (It is currently pointing to
 *          a previously allocated structure) you may lose the pointer to some allocated memory
 */
bool alarm_type_create(Alarm_data_ptr* out_alarm_data, bool disable,
                       Alarm_class alarm_class);

/**
 * @brief Function to clone an Alarm_data_t structure
 *
 * @param src_alarm_data    Pointer to the source Alarm_data_t structure
 * @param out_alarm_data    Pointer to the pointer to the destination Alarm_data_t structure
 * @return Returns ESP_OK if the structure is successfully cloned.
 *         Returns ESP_ERR_INVALID_ARG if input data is null.
 *         Returns ESP_FAIL if the cloning fails.
 */
esp_err_t alarm_type_clone(Alarm_data_ptr src_alarm_data, Alarm_data_ptr* dst_alarm_data);

/**
 * @brief Function to generate JSON string with the data from alarm
 *        The JSON has the format:
 *
 * @param json_buffer      pointer to c-string to be populated with the generated JSON
 * @param buffer_size      length of json_buffer
 * @param alarm_data       pointer to an Alarm_data_t object with the data to generate
 *                         the JSON
 * @return Returns ESP_OK if data is well formatted and it is less than the buffer size.
 *         Returns ESP_ERR_INVALID_ARG if input data or buffer are null.
 *         Returns ESP_ERR_INVALID_SIZE if the JSON string generated is
 *         greater than the buffer_size.
 */
esp_err_t alarm_type_to_JSON(struct json_out * json_buffer, int buffer_size, Alarm_data_ptr alarm_data);

/**
 * @brief Release an 'Alarm_data_t'
 *
 * @param alarm_data
 */
void alarm_data_release(Alarm_data_t* alarm_data);

#endif /* MAIN_ALARM_TYPE_H_ */
