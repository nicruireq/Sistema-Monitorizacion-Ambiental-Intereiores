/*
 * global_system_signaler.c
 *
 *  Created on: 25 ene 2025
 *      Author: NRR
 */

#include "global_system_signaler.h"

#include "esp_log.h"
#include "esp_err.h"

static const char * TAG = "GSS";

static QueueHandle_t channels[GSS_MAX_CHANNELS];
static uint8_t channels_size[] = {
		GSS_MQTT_SENDER_QUEUE_LENGTH,
		GSS_GUI_QUEUE_LENGTH
};

esp_err_t gss_initialize()
{
	/* queue creation and checking for each data channel */
	for (int ch = 0; ch < GSS_MAX_CHANNELS; ++ch)
	{
		if (channels[ch] == NULL)
		{
			// create each queue only one time
			channels[ch] = xQueueCreate(channels_size[ch],
							sizeof(GSS_Message));
			if (channels[ch] == NULL)
			{
				ESP_LOGE(TAG, "Not enough heap available for data channel creation");
				return ESP_FAIL;
			}
		}
	}

	return ESP_OK;
}

esp_err_t  gss_wait_for_signal(GSS_ID id, GSS_Message* recv_msg, const TickType_t xTicksToWait)
{
	esp_err_t ret = ESP_ERR_TIMEOUT;
	BaseType_t waiting_ret = errQUEUE_EMPTY;

	switch (id)
	{
	case GSS_ID_MQTT_SENDER:
		waiting_ret = xQueueReceive(channels[GSS_ID_MQTT_SENDER], (void *) recv_msg, xTicksToWait);
		break;
	case GSS_ID_GUI:
		waiting_ret = xQueueReceive(channels[GSS_ID_GUI], (void *) recv_msg, xTicksToWait);
		break;
	}

	ret = waiting_ret == pdPASS ? ESP_OK : ret;

	return ret;
}

esp_err_t gss_send_sensors_data(Sensors_data_ptr sensors_data, GSS_ID target)
{
	esp_err_t ret = ESP_FAIL;
	BaseType_t send_ret = errQUEUE_FULL;

	GSS_Message buffer;
	buffer.signal = GSS_SENSORS_DATA_READY;
	buffer.data = (void*) sensors_data;

	if (sensors_data == NULL)
	{
		ret = ESP_ERR_INVALID_ARG;
	}
	else
	{
		switch (target)
		{
		case GSS_ID_MQTT_SENDER:
			if (channels[GSS_ID_MQTT_SENDER] != NULL)
			{
				send_ret = xQueueSend(channels[GSS_ID_MQTT_SENDER], &buffer, 0);
			}
			else
			{
				ret = ESP_ERR_INVALID_ARG;
			}
			break;
		case GSS_ID_GUI:
			if (channels[GSS_ID_GUI] != NULL)
			{
				send_ret = xQueueSend(channels[GSS_ID_GUI], &buffer, 0);
			}
			else
			{
				ret = ESP_ERR_INVALID_ARG;
			}
			break;
		default:
			ret = ESP_ERR_INVALID_ARG;
		}

		if (ret != ESP_ERR_INVALID_ARG)
		{
			ret = (send_ret == pdPASS) ? ESP_OK : ESP_ERR_NO_MEM;
		}
	}

	return ret;
}

esp_err_t gss_send_alarm_data(Alarm_data_ptr alarm_data, GSS_ID target)
{
	esp_err_t ret = ESP_FAIL;
	BaseType_t send_ret = errQUEUE_FULL;

	GSS_Message buffer;
	buffer.signal = GSS_ALARM_READY;  // Usamos el valor correcto del enum GSS_SIGNAL
	buffer.data = (void*) alarm_data;

	if (alarm_data == NULL)
	{
		ret = ESP_ERR_INVALID_ARG;
	}
	else
	{
		switch (target)
		{
		case GSS_ID_MQTT_SENDER:
			if (channels[GSS_ID_MQTT_SENDER] != NULL)
			{
				send_ret = xQueueSend(channels[GSS_ID_MQTT_SENDER], &buffer, 0);
			}
			else
			{
				ret = ESP_ERR_INVALID_ARG;
			}
			break;
		case GSS_ID_GUI:
			if (channels[GSS_ID_GUI] != NULL)
			{
				send_ret = xQueueSend(channels[GSS_ID_GUI], &buffer, 0);
			}
			else
			{
				ret = ESP_ERR_INVALID_ARG;
			}
			break;
		default:
			ret = ESP_ERR_INVALID_ARG;
		}

		if (ret != ESP_ERR_INVALID_ARG)
		{
			ret = (send_ret == pdPASS) ? ESP_OK : ESP_ERR_NO_MEM;
		}
	}

	return ret;
}


void gss_release_message(GSS_Message* msg)
{
	assert(msg != NULL);

	if (msg->data != NULL)
	{
		free(msg->data);
		msg->data = NULL;
	}
}

