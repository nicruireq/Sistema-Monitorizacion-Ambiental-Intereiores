/*
 * global_system_signaler.h
 *
 *  Created on: 25 ene 2025
 *      Author: NRR
 */

#ifndef MAIN_GLOBAL_SYSTEM_SIGNALER_H_
#define MAIN_GLOBAL_SYSTEM_SIGNALER_H_

/* FreeRTOS includes. */
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

/* Data types includes */
#include "sensors_type.h"
#include "alarm_type.h"

/* Define the lengths of the IPCs */
#define GSS_MQTT_SENDER_QUEUE_LENGTH	6
#define GSS_GUI_QUEUE_LENGTH			6

/* Number of channels */
#define GSS_MAX_CHANNELS	2

typedef enum
{
	GSS_ID_MQTT_SENDER,
	GSS_ID_GUI
} GSS_ID;

typedef enum
{
	GSS_SENSORS_DATA_READY,
	GSS_ALARM_READY
} GSS_SIGNAL;

typedef struct
{
	GSS_SIGNAL signal;
	void* data;
} GSS_Message;

/**
 * @brief Initialize IPCs used by GSS.
 *
 * @return ESP_OK if all the IPCs have been initialized properly
 * 			else returns ESP_FAIL.
 */
esp_err_t gss_initialize();

/**
 * @brief Blocks the caller task to wait for a signal from the GSS
 *
 * @param id Id of the data channel where the task is waiting for a signal.
 * @param recv_msg Message that wakes up the task.
 * @param xTicksToWait The maximun time in ticks to get blocked.
 *
 * @return Returns ESP_OK if  you have been unblocked in the channel
 * 		   because there is data to process.
 * 		   Returns ESP_ERR_TIMEOUT if you have not received any data after
 * 		   an amount of time xTicksToWait system ticks.
 * 		   In other words you must call this function to get blocked
 * 		   waiting for the signaling of an event through the GSS.
 */
esp_err_t  gss_wait_for_signal(GSS_ID id, GSS_Message* recv_msg,
								const TickType_t xTicksToWait);

/**
 * @brief Sends new sensors data message for the task assigned to the target tag.
 *
 * @param sensors_data	Pointer to the sensors data to be sent.
 * @param target	ID of the GSS data channel assigned to the destination task
 * 					that we want the data will be sent.
 * @return Returns ESP_OK if data message could be sent. If channel is full
 * 		   returns ESP_ERR_NO_MEM. If sensors_data is invalid pointer
 * 		   or data channels are NULL returns ESP_ERR_INVALID_ARG.
 * 		   Else returns ESP_FAIL
 *
 * @warning Ensure that target ID matches with the destination task.
 */
esp_err_t gss_send_sensors_data(Sensors_data_ptr sensors_data, GSS_ID target);

/**
 * @brief Sends new alarm data message for the task assigned to the target tag.
 *
 * @param alarm_data Pointer to the alarm data to be sent.
 * @param target     ID of the GSS data channel assigned to the destination task
 *                   that we want the data to be sent to.
 * @return Returns ESP_OK if data message could be sent. If channel is full
 *         returns ESP_ERR_NO_MEM. If alarm_data is invalid pointer
 *         or data channels are NULL returns ESP_ERR_INVALID_ARG.
 *         Else returns ESP_FAIL
 *
 * @warning Ensure that target ID matches with the destination task.
 */
esp_err_t gss_send_alarm_data(Alarm_data_ptr alarm_data, GSS_ID target);

/**
 * @brief Release the data of a 'GSS_Message'
 *
 * @param msg	Message which data want to be released.
 */
void gss_release_message(GSS_Message* msg);

#endif /* MAIN_GLOBAL_SYSTEM_SIGNALER_H_ */
