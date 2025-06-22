//Include standard lib headers
#include <stdbool.h>
#include <stdint.h>
#include <string.h>


//Include FreeRTOS headers
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

//Include ESP submodules headers.
#include "esp_event.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include "driver/gpio.h"
#include "esp_check.h"
#include "esp_err.h"

//Include own project  headers
#include "gpio_leds.h"
#include "mqtt.h"

//FROZEN JSON parsing/fotmatting library header
#include "frozen.h"

// My project headers
#include "global_system_signaler.h"
#include "sensors_type.h"
#include "aqi_config_manager.h"
#include "alarm_type.h"

//****************************************************************************
//      VARIABLES GLOBALES STATIC
//****************************************************************************

static const char *TAG = "MQTT_CLIENT";
static esp_mqtt_client_handle_t client=NULL;
static TaskHandle_t senderTaskHandler=NULL;

//****************************************************************************
// Funciones.
//****************************************************************************

static void mqtt_sender_task(void *pvParameters);


// callback that will handle MQTT events. Will be called by  the MQTT internal task.
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    esp_err_t err = ESP_FAIL;

    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
        {
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
            msg_id = esp_mqtt_client_subscribe(client, MQTT_TOPIC_SUBSCRIBE_BASE, 0);
            ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);
            msg_id = esp_mqtt_client_subscribe(client, TOPIC_MEASURES, 0);
            ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);
            msg_id = esp_mqtt_client_subscribe(client, TOPIC_CONFIG, 0);
            ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

            //xTaskCreate(mqtt_sender_task, "mqtt_sender", 4096, NULL, 5, &senderTaskHandler); //Crea la tarea MQTT sennder
            xTaskCreatePinnedToCore(mqtt_sender_task, "mqtt_sender", 4096, NULL, 5, NULL, 0);

            break;
        }
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            //Deber�amos destruir la tarea que env�a....
            break;
        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_DATA:
        {
        	//Para poder imprimir el nombre del topic lo tengo que copiar en una cadena correctamente terminada....
        	char topic_name[event->topic_len+1]; //Esto va a la pila y es potencialmente peligroso si el nombre del topic es grande....
        	strncpy(topic_name,event->topic,event->topic_len);
        	topic_name[event->topic_len]=0; //a�ade caracter de terminacion al final.

        	ESP_LOGI(TAG, "MQTT_EVENT_DATA: Topic %s",topic_name);

        	// Handle data subscribed on topic config
        	if (strncmp(TOPIC_CONFIG, topic_name, strlen(TOPIC_CONFIG)) == 0)
        	{
        		err = aqi_process_received_config_data(event->data, event->data_len);
        		if (err == ESP_ERR_INVALID_ARG)
        		{
        			ESP_LOGE(TAG, "Incoming CONFIG JSON data invalid");
        		}
        		else if (err == ESP_FAIL)
        		{
        			ESP_LOGE(TAG, "Flash config data error");
        		}
        	}

        }
            break;

        case MQTT_EVENT_ERROR:
            ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
            break;
        case MQTT_EVENT_BEFORE_CONNECT:
            break;
        default:
            ESP_LOGI(TAG, "Other event id:%d", event->event_id);
            break;
    }
}


static void mqtt_sender_task(void *pvParameters)
{
	char buffer[JSON_OUT_BUFFER_SIZE]; //"buffer" para guardar el mensaje. Me debo asegurar que quepa...
	esp_err_t json_status = ESP_FAIL;
	GSS_Message recv_msg;

	while (1)
	{
		struct json_out out1 = JSON_OUT_BUF(buffer, JSON_OUT_BUFFER_SIZE);

		// wait for the reception of a signal that requires to publish a message
		if (gss_wait_for_signal(GSS_ID_MQTT_SENDER, &recv_msg, portMAX_DELAY) == ESP_OK)
		{
			switch (recv_msg.signal)
			{
			case GSS_SENSORS_DATA_READY:
				if ((json_status = sensors_type_to_JSON(&out1, JSON_OUT_BUFFER_SIZE, (Sensors_data_ptr)recv_msg.data)) == ESP_OK)
				{
					int msg_id = esp_mqtt_client_publish(client, TOPIC_MEASURES, buffer, 0, 0, 0);
					ESP_LOGI(TAG, "sent successful on TOPIC_MEASURES, msg_id=%d: %s", msg_id, buffer);

					ESP_LOGI(TAG, "Liberando memoria en %p", recv_msg.data);
					gss_release_message(&recv_msg);
				}
				else
				{
					ESP_LOGE(TAG, "JSON generation for sensors data failed with code: %s", esp_err_to_name(json_status));
				}

				break;
			case GSS_ALARM_READY:
				if ((json_status = alarm_type_to_JSON(&out1, JSON_OUT_BUFFER_SIZE, (Alarm_data_ptr)recv_msg.data)) == ESP_OK)
				{
					int msg_id = esp_mqtt_client_publish(client, TOPIC_ALARMS, buffer, 0, 0, 0);
					ESP_LOGI(TAG, "sent successful on TOPIC_ALARMS, msg_id=%d: %s", msg_id, buffer);

					ESP_LOGI(TAG, "Liberando memoria en %p", recv_msg.data);
					gss_release_message(&recv_msg);
				}
				else
				{
					ESP_LOGE(TAG, "JSON generation for alarms data failed with code: %s", esp_err_to_name(json_status));
				}

				break;
			}
		}
	}
}

esp_err_t mqtt_app_start(const char* url)
{
	esp_err_t error;

	if (client==NULL){

		esp_mqtt_client_config_t mqtt_cfg = {
				.broker.address.uri = MQTT_BROKER_URL,
		};
		if(url[0] != '\0'){
			mqtt_cfg.broker.address.uri= url;
		}

		ESP_LOGI(TAG, "Inicializando cliente MQTT con URI: %s", mqtt_cfg.broker.address.uri);

		client = esp_mqtt_client_init(&mqtt_cfg);
		esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, client);
		error=esp_mqtt_client_start(client);
		return error;
	}
	else {

		ESP_LOGE(TAG,"MQTT client already running");
		return ESP_FAIL;
	}
}


