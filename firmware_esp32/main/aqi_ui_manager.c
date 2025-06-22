/*
 * aqi_ui_manager.c
 *
 *  Created on: 29 mar 2025
 *      Author: nrr
 */

#include "bsp/esp-bsp.h"
#include "esp_log.h"

#include "aqi_ui_manager.h"
#include "global_system_signaler.h"
#include "sensors_type.h"
#include "alarm_type.h"
#include "aqi_config_manager.h"

#define USE_BLUFI

#ifndef USE_BLUFI
#include "wifi.h"
#else
#include "blufi_manager.h"
#endif

#include <assert.h>

static const char *TAG = "AQI_UI";

// Miembros manejadores de la UI
typedef struct
{
    lv_obj_t* label_room;
    lv_obj_t* network_indicator;
//    lv_obj_t* label_locale;
} top_bar_data_handler;

typedef struct
{
    lv_obj_t* arc_handler;
    lv_obj_t* voc_label;
    lv_obj_t* voc_min;
    lv_obj_t* voc_max;
} voc_widget_data_handler;

static lv_obj_t* label_temperature = NULL;
static lv_obj_t* label_humidity = NULL;
static lv_obj_t* contenedor_alarmas = NULL;
static top_bar_data_handler ui_top_bar_handler;
static voc_widget_data_handler ui_voc_widget_handler;

static bool check_is_active_equal_class_alarm(lv_obj_t* container, Alarm_class wanted)
{
	Alarm_data_ptr current_alarm = NULL;
	lv_obj_t * label_alarm = NULL;

	for(uint32_t i = 0; i < lv_obj_get_child_cnt(container); i++)
	{
		label_alarm = lv_obj_get_child(container, i);

		if ((label_alarm != NULL) &&
				lv_obj_check_type(label_alarm, &lv_label_class) &&
				lv_obj_has_flag(label_alarm, LV_OBJ_FLAG_USER_1))
		{
			current_alarm = (Alarm_data_ptr)lv_obj_get_user_data(label_alarm);

			if ((current_alarm != NULL) && (current_alarm->alarm_class == wanted))
			{
				ESP_LOGI(TAG, "check alarm exists in UI: class_current=%s, wanted=%s",
									alarm_class_to_string(current_alarm->alarm_class),
									alarm_class_to_string(wanted));
				return true;
			}
		}
	}

	return false;
}

static void remove_alarm_row_by_class(lv_obj_t* container, Alarm_class wanted)
{
	Alarm_data_ptr current_alarm = NULL;
	lv_obj_t * label_alarm = NULL;

	for(uint32_t i = 0; i < lv_obj_get_child_cnt(container); i++)
	{
		label_alarm = lv_obj_get_child(container, i);

		if ((label_alarm != NULL) &&
				lv_obj_check_type(label_alarm, &lv_label_class) &&
				lv_obj_has_flag(label_alarm, LV_OBJ_FLAG_USER_1))
		{
			current_alarm = (Alarm_data_ptr)lv_obj_get_user_data(label_alarm);

			if ((current_alarm != NULL) && (current_alarm->alarm_class == wanted))
			{
				ESP_LOGI(TAG, "remove alarm in UI: class_current=%s, wanted=%s",
							alarm_class_to_string(current_alarm->alarm_class),
							alarm_class_to_string(wanted));
				// Esto lanzara el evento LV_EVENT_DELETE, el cb asociado libera el user_data
				// lo diferimos en el siguiente ciclo de la task interna de LVGL por si
				// hubiera acciones pendientes sobre este objeto (ej: animaciones)
				lv_obj_del_async(label_alarm);
			}
		}
	}
}


/***********************
 * 	LVGL CALLBACKS
 ***********************/

// Callbacks para eliminacion diferida de labels de alarmas
static void alarm_label_event_cb(lv_event_t * e)
{
    if (lv_event_get_code(e) == LV_EVENT_DELETE)
    {
        lv_obj_t * obj = lv_event_get_target(e);
        Alarm_data_ptr associated_alarm_data = (Alarm_data_ptr)lv_obj_get_user_data(obj);
        if (associated_alarm_data)
        {
            ESP_LOGI(TAG, "Liberando memoria de Alarm_data_t en LV_EVENT_DELETE: %p",
            		associated_alarm_data);
            alarm_data_release(associated_alarm_data);
        }
    }
}

static void update_periodic_ui_data_cb(lv_timer_t * timer)
{
	// update room name
    char* updated_room_name = (char*)calloc(AQI_MAX_ROOM_NAME_SZ, sizeof(char));

    if (updated_room_name != NULL)
    {
    	if (aqi_config_manager_get(AQI_CV_ROOM_NAME,
    			updated_room_name, AQI_MAX_ROOM_NAME_SZ) == ESP_OK)
    	{
    		const char* current_text = lv_label_get_text(ui_top_bar_handler.label_room);

    		if (strcmp(current_text, updated_room_name) != 0)
    		{
    			lv_label_set_text(ui_top_bar_handler.label_room, updated_room_name);
    		}
    	}
    }

    free(updated_room_name);

    // update network status icon
#ifdef USE_BLUFI
    if (blufi_manager_wifi_is_connected())
#else
    if (wifi_is_connected())
#endif
    {
    	lv_obj_add_flag(ui_top_bar_handler.network_indicator, LV_OBJ_FLAG_HIDDEN);
    }
    else
    {
    	lv_obj_clear_flag(ui_top_bar_handler.network_indicator, LV_OBJ_FLAG_HIDDEN);
    }
}



// Forward declaration
static void insert_alarm_row(lv_obj_t* container, const Alarm_data_ptr alarm_data);

// Task Function
void aqi_UI_Task (void *pvparameters)
{
	//TickType_t xLastWakeTime = xTaskGetTickCount();

	assert(label_temperature != NULL);
	assert(label_humidity != NULL);
	assert(contenedor_alarmas != NULL);

	while(1)
	{
		GSS_Message recv_msg;
		Sensors_data_ptr input_sensors_data = NULL;
		Alarm_data_ptr incoming_alarm = NULL;

		ESP_LOGI(TAG, "aqi_UI ME DESPIERTO");

		// EL CODIGO QUE SE BLOQUEA EN LA COLA CORRESPONDIENTE
		// DEL GRUPO DE COLAS USANDO LAS FUNCIONES DEL GLOBAL_SYSTEM_SIGNALER
		if (gss_wait_for_signal(GSS_ID_GUI, &recv_msg, portMAX_DELAY) == ESP_OK)
		{
			ESP_LOGI(TAG, "aqi_UI ME DESBLOQUEO");
			input_sensors_data = (Sensors_data_ptr)recv_msg.data;

			switch (recv_msg.signal)
			{
			case GSS_SENSORS_DATA_READY:
			{
				ESP_LOGI(TAG, "aqi_UI UPDATE UI");

				lv_label_set_text_fmt(label_temperature, "%u",
						input_sensors_data->temperature_celsius);
				lv_label_set_text_fmt(label_humidity, "%u",
						input_sensors_data->relative_humidity);
				// set valores widget de VOC
				ESP_LOGI(TAG, "Valor VOC para UI = %u", input_sensors_data->voc_index);
				lv_arc_set_value(ui_voc_widget_handler.arc_handler,
						input_sensors_data->voc_index);
				lv_label_set_text_fmt(ui_voc_widget_handler.voc_label, "%u",
						input_sensors_data->voc_index);

				// Para este tipo de mensaje si se puede liberar memoria tras procesar
				// el objeto aqui
				ESP_LOGI(TAG, "Liberando memoria de msg de sensor data en %p", recv_msg.data);
				gss_release_message(&recv_msg);
			}
			break;
			case GSS_ALARM_READY:
			{
				ESP_LOGI(TAG, "aqi_UI llega mensaje ALARMA");

				incoming_alarm = (Alarm_data_ptr)recv_msg.data;
				// Si es una activacion de alarma
				if (!incoming_alarm->disable)
				{
					// Solo si no hay ya una alarma de su misma clase activada
					// se crea y se añade a la UI
					if (!check_is_active_equal_class_alarm(contenedor_alarmas,
							incoming_alarm->alarm_class))
					{
						ESP_LOGI(TAG, "aqi_UI parece que la ALARMA no estaba ya creada en la UI y se va a crear");

						insert_alarm_row(contenedor_alarmas, incoming_alarm);
					}
				}
				else
				{
					ESP_LOGI(TAG, "aqi_UI eliminar ALARMA que se desactiva");
					// Si es una desactivacion de alarma
					// Solo liberar memoria reservada donde se aloja un Alarm_data_t
					// si se se desactiva la alarma (lo hace la siguiente funcion),
					// en este caso hay que liberar tambien
					// el del incoming_alarm que es un mensaje de desactivacion y
					// siempre hay que descartarlos
					remove_alarm_row_by_class(contenedor_alarmas, incoming_alarm->alarm_class);

					ESP_LOGI(TAG, "Liberando memoria de msg de desactivacion de alarma en %p",
							recv_msg.data);
					gss_release_message(&recv_msg);
				}
			}
				break;
			}
		}
		else
		{
			ESP_LOGE(TAG, "No se puede leer cola datos sensores para UI");
		}
	}
}

// Private utility functions
/**
 * @brief
 * @param parent
 * @return puntero al widget creado que sirve para actualizar un indicador en la UI
 */
static lv_obj_t* create_sensor_info_box(lv_obj_t* flex_row_parent, const char* title, const char* unit_text)
{
	lv_obj_t* box_sensor = lv_obj_create(flex_row_parent);
	lv_obj_set_height(box_sensor, lv_pct(100));           /*Fix size*/
	lv_obj_set_flex_grow(box_sensor, 1);           /*1 portion from the free space*/
	lv_obj_set_flex_flow(box_sensor, LV_FLEX_FLOW_COLUMN); /* Layout en columna */
	lv_obj_clear_flag(box_sensor, LV_OBJ_FLAG_SCROLLABLE);    // sin barra de scroll
	lv_obj_set_style_pad_all(box_sensor, 4, LV_PART_MAIN);

	// Crear el label "Temperatura"
	lv_obj_t* label_sensor_title = lv_label_create(box_sensor);
	lv_obj_set_height(label_sensor_title, lv_pct(15));
	lv_label_set_text(label_sensor_title, title);
	lv_obj_align(label_sensor_title, LV_ALIGN_TOP_LEFT, 0, 0);
	lv_obj_set_style_text_font(label_sensor_title, &lv_font_montserrat_12, LV_PART_MAIN);

	// Crear la línea separadora
	lv_obj_t* separator = lv_obj_create(box_sensor);
	lv_obj_set_size(separator, lv_pct(100), 2); // Línea de 2 píxeles de altura
	lv_obj_set_style_bg_color(separator, lv_color_black(), LV_PART_MAIN);

	// crear estilo para la fuente
	static lv_style_t temp_label_style;
	lv_style_init(&temp_label_style);
	lv_style_set_text_font(&temp_label_style, &lv_font_montserrat_28);
	lv_style_set_border_width(&temp_label_style, 0);

	// Crear la fila para los labels de temperatura
	lv_obj_t* measure_row = lv_obj_create(box_sensor);
	lv_obj_set_flex_flow(measure_row, LV_FLEX_FLOW_ROW); /* Layout en fila */
	lv_obj_set_size(measure_row, lv_pct(100), lv_pct(65));
	lv_obj_clear_flag(measure_row, LV_OBJ_FLAG_SCROLLABLE);    // sin barra de scroll
	//lv_obj_set_height(temp_row, lv_pct(70)); /* Altura según contenido */
	lv_obj_add_style(measure_row, &temp_label_style, 0);

	// Centrar elementos en el track
	lv_obj_set_flex_align(measure_row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

	// Crear el label para el valor de temperatura
	lv_obj_t* label_value = lv_label_create(measure_row);
	lv_label_set_text_fmt(label_value, "%d", 30); // Ejemplo de valor de temperatura
	//lv_obj_set_flex_grow(label_value, 1); // Ocupa todo el espacio restante
	//lv_obj_set_height(label_value, lv_pct(100));
	lv_obj_set_style_pad_right(label_value, 0, 0); // quitar right padding     para juntar labels

	// Crear el label para "ºC"
	lv_obj_t* label_unit = lv_label_create(measure_row);
	lv_label_set_text(label_unit, unit_text);
	//lv_obj_set_flex_grow(label_unit, 1);
	//lv_obj_set_height(label_unit, lv_pct(100));
	lv_obj_set_style_pad_left(label_unit, 0, 0); // quitar left padding para juntar labels


	return label_value;
}


/**
 * @brief
 *
 * @param flex_row_parent parent container
 * @param title            header
 * @param out_handler      object to handle the data shown by the widget
 */
static void create_aiq_info_widget(lv_obj_t* flex_row_parent, const char* title, voc_widget_data_handler* out_handler)
{
	lv_obj_t* box_sensor = lv_obj_create(flex_row_parent);
	lv_obj_set_height(box_sensor, lv_pct(100));           /*Fix size*/
	lv_obj_set_flex_grow(box_sensor, 1);           /*1 portion from the free space*/
	lv_obj_set_flex_flow(box_sensor, LV_FLEX_FLOW_COLUMN); /* Layout en columna */
	lv_obj_clear_flag(box_sensor, LV_OBJ_FLAG_SCROLLABLE);    // sin barra de scroll
	lv_obj_set_style_pad_all(box_sensor, 4, LV_PART_MAIN);

	// Crear el label con title
	lv_obj_t* label_sensor_title = lv_label_create(box_sensor);
	lv_obj_set_height(label_sensor_title, lv_pct(15));
	lv_label_set_text(label_sensor_title, title);
	lv_obj_align(label_sensor_title, LV_ALIGN_TOP_LEFT, 0, 0);
	lv_obj_set_style_text_font(label_sensor_title, &lv_font_montserrat_12, LV_PART_MAIN);

	// Crear la línea separadora
	lv_obj_t* separator = lv_obj_create(box_sensor);
	lv_obj_set_size(separator, lv_pct(100), 2); // Línea de 2 píxeles de altura
	lv_obj_set_style_bg_color(separator, lv_color_black(), LV_PART_MAIN);

	// crear estilo para la fuente
	//static lv_style_t temp_label_style;
	//lv_style_init(&temp_label_style);
	//lv_style_set_text_font(&temp_label_style, &lv_font_montserrat_38);
	//lv_style_set_border_width(&temp_label_style, 0);

	// Crear la fila para el widget que representa el nivel de aqi
	lv_obj_t* measure_row = lv_obj_create(box_sensor);
	lv_obj_set_flex_flow(measure_row, LV_FLEX_FLOW_ROW); /* Layout en fila */
	lv_obj_set_size(measure_row, lv_pct(100), lv_pct(80));
	//lv_obj_set_height(temp_row, lv_pct(70)); /* Altura según contenido */
	//lv_obj_add_style(measure_row, &temp_label_style, 0);
	lv_obj_set_style_border_width(measure_row, 0, LV_PART_MAIN);
	lv_obj_set_style_pad_top(measure_row, 0, LV_PART_MAIN); // para que el arc se posicione desde arriba de su contenedor
	lv_obj_clear_flag(measure_row, LV_OBJ_FLAG_SCROLLABLE);    // sin barra de scroll
	// quitarle padding por defecto para que coja el del contenedor padre y haya
	// luego mas espacio para el arco
	lv_obj_set_style_pad_left(measure_row, 0, LV_PART_MAIN);

	// Centrar elementos en el track
	//lv_obj_set_flex_align(measure_row, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

	// crear arco
	lv_obj_t* arc = lv_arc_create(measure_row);
	lv_obj_set_size(arc, 87, 100);
	lv_arc_set_rotation(arc, 180);
	lv_arc_set_bg_angles(arc, 10, 170);
	// alinear en base a contenedor para ajustar mas a la izquierda y tener
	// mas espacio para expandir el arco
	lv_obj_align(arc, LV_ALIGN_LEFT_MID, 0, 0);

	lv_arc_set_range(arc, 0, 500);
	// quitar knob y hacer no clicable
	lv_obj_remove_style(arc, NULL, LV_PART_KNOB);
	lv_obj_clear_flag(arc, LV_OBJ_FLAG_CLICKABLE);
	// Crear el estilo para el arco indicador (no funcionan degradados en arcos en la v8)
	lv_color_t color_ini = lv_color_make(80, 224, 21);
	lv_obj_set_style_arc_color(arc, color_ini, LV_PART_INDICATOR);  // si podemos ir cambiando el color según los tramos

	// Ajustar el grosor del arco
	lv_obj_set_style_arc_width(arc, 5, LV_PART_MAIN);  // Ajusta el grosor del arco principal
	lv_obj_set_style_arc_width(arc, 5, LV_PART_INDICATOR);  // Ajusta el grosor del arco indicador

	// setear valor de ejemplo
	uint16_t valor_aqi = 150;
	lv_arc_set_value(arc, valor_aqi);


	// Crear el label para que indica el valor numerico aqi
	lv_obj_t* label_value = lv_label_create(arc);
	lv_label_set_text_fmt(label_value, "%d", valor_aqi); // Ejemplo de valor de temperatura
	lv_obj_set_style_text_font(label_value, &lv_font_montserrat_20, LV_PART_MAIN);
	lv_obj_align(label_value, LV_ALIGN_TOP_MID, 0, 17);

	// Crear el label valor minimo
	lv_obj_t* label_min = lv_label_create(arc);
	lv_label_set_text(label_min, "0");
	lv_obj_set_style_text_font(label_min, &lv_font_montserrat_12, LV_PART_MAIN);
	lv_obj_align(label_min, LV_ALIGN_TOP_LEFT, 0, 40);

	// Crear el label valor maximo
	lv_obj_t* label_max = lv_label_create(arc);
	lv_label_set_text(label_max, "500");
	lv_obj_set_style_text_font(label_max, &lv_font_montserrat_12, LV_PART_MAIN);
	lv_obj_align(label_max, LV_ALIGN_TOP_RIGHT, 0, 40);

	// out info
	if (out_handler != NULL)
	{
	    out_handler->arc_handler = arc;
	    out_handler->voc_label = label_value;
	    out_handler->voc_min = label_min;
	    out_handler->voc_max = label_max;
	}
}


/**
 * @brief
 *
 * @param parent
 * @return puntero manejador a la linea de tachado del icono wifi. Para
 *          poder modificarlo segun el estado de la conexion
 */
static lv_obj_t* create_wifi_indicator(lv_obj_t* parent)
{
	// Crear el objeto de texto para el icono de WiFi
	lv_obj_t* wifi_label = lv_label_create(parent);
	lv_label_set_text(wifi_label, LV_SYMBOL_WIFI);
	//lv_obj_set_style_local_text_font(wifi_label, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &lv_font_fontawesome_16);
	lv_obj_align(wifi_label, LV_ALIGN_CENTER, 0, 0);

	// Crear la línea diagonal
	// Calcular coordenadas de la linea diagonal de tachado
	// tomando como referencia los pixeles de la fuente actual del icono
	const lv_font_t* font = lv_obj_get_style_text_font(wifi_label, LV_PART_MAIN);
	lv_coord_t lado = font->line_height;
	static lv_point_t line_points[2]; // = { {0, 0}, {50, 50} };
	line_points[0].x = 0;
	line_points[0].y = 0;
	line_points[1].x = lado;
	line_points[1].y = lado;

	lv_obj_t* line = lv_line_create(parent);
	lv_line_set_points(line, line_points, 2);
	//lv_obj_set_style_local_line_color(line, LV_LINE_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_RED);
	// estilos de la linea
	static lv_style_t style_line;
	lv_style_init(&style_line);
	lv_style_set_line_width(&style_line, lado / 8);
	lv_style_set_line_color(&style_line, lv_color_black());
	lv_obj_add_style(line, &style_line, 0);
	lv_obj_center(line);

	return line;
}


static void insert_alarm_row(lv_obj_t* container, const Alarm_data_ptr alarm_data)
{
	static lv_anim_t animation_template;
	static lv_style_t label_style;

	lv_anim_init(&animation_template);
	// Wait 1 second to start the first scroll
	lv_anim_set_delay(&animation_template, 1000);
    // Repeat the scroll 3 seconds after the label scrolls back
	// to the initial position
	lv_anim_set_repeat_delay(&animation_template, 3000);

	// Initialize the label style with the animation template
	lv_style_init(&label_style);
	lv_style_set_anim(&label_style, &animation_template);
	lv_style_set_border_width(&label_style, 1);
	lv_style_set_pad_all(&label_style, 1);
	lv_style_set_border_color(&label_style, lv_color_hex(0xDBDBD8));
	lv_style_set_text_font(&label_style, &lv_font_montserrat_10);
	lv_style_set_radius(&label_style, 4);

	ESP_LOGI(TAG, "insert_alarm_row: antes de crear el label de alarm class=%s:",
						alarm_class_to_string(alarm_data->alarm_class));

	lv_obj_t* label_alarm = lv_label_create(container);

	ESP_LOGI(TAG, "insert_alarm_row: el label de alarm debe haberse creado, class=%s:",
							alarm_class_to_string(alarm_data->alarm_class));

	// Circular scroll
	lv_label_set_long_mode(label_alarm, LV_LABEL_LONG_SCROLL_CIRCULAR);
	lv_obj_set_width(label_alarm, lv_pct(100));
	lv_label_set_text(label_alarm, alarm_data->info);
	// Asignar datos de alarma procedentes de gss
	lv_obj_set_user_data(label_alarm, (void*)alarm_data);
	// Asignamos callback para liberacion de memoria cuando se elimine realmente
	// el objeto label
	lv_obj_add_event_cb(label_alarm, alarm_label_event_cb, LV_EVENT_DELETE, NULL);
	// Este flag es importante para diferenciar si un objeto hijo de container
	// es realmente una alarma. (SAFETY)
	lv_obj_add_flag(label_alarm, LV_OBJ_FLAG_USER_1); // Marca como label_alarm válido

	//lv_obj_align(label_alarm, LV_ALIGN_CENTER, 0, 40);
	lv_obj_add_style(label_alarm, &label_style, LV_STATE_DEFAULT);
}

/**
 * @brief   inserta header y linea separadora en la fila del layout que
 *          se utilizara para insertar labels con alarmas
 *
 * @param flex_row_parent fila de layaut flex que se usara como contenedor
 * @return puntero aflex_row_parent
 */
static lv_obj_t* row_alarms_insert_header(lv_obj_t * flex_row_parent)
{
	// ajustar padding contenedor padre para reducirlo
	lv_obj_set_style_pad_all(flex_row_parent, 4, LV_PART_MAIN);

	// Crear el label header
	lv_obj_t* label_header = lv_label_create(flex_row_parent);
	lv_obj_set_height(label_header, lv_pct(10));
	lv_label_set_text(label_header, "Alarmas");
	lv_obj_align(label_header, LV_ALIGN_TOP_LEFT, 0, 0);
	lv_obj_set_style_text_font(label_header, &lv_font_montserrat_12, LV_PART_MAIN);

	// Crear la línea separadora
	lv_obj_t* separator = lv_obj_create(flex_row_parent);
	lv_obj_set_size(separator, lv_pct(100), 2); // Línea de 2 píxeles de altura
	lv_obj_set_style_bg_color(separator, lv_color_black(), LV_PART_MAIN);

	return flex_row_parent;
}


/**
 * @brief
 *
 * @param parent_screen objeto que apunta a la pantalla base
 * @param out_handler   objeto de salida con los punteros manejadores de los widgets
 *                      con datos modificables
 * @return puntero a manejador de la top bar
 */
static lv_obj_t* create_top_bar(lv_obj_t* parent_screen, top_bar_data_handler* out_handler)
{
	// TOP BAR
	lv_obj_t* top_bar = lv_obj_create(parent_screen);
	lv_obj_set_size(top_bar, lv_pct(100), lv_pct(8));
	static lv_style_t top_bar_style;
	lv_style_init(&top_bar_style);
	lv_color_t top_bar_color = lv_color_hex(0xCCCCCC); // gris claro //lv_color_make(0, 71, 62);
	lv_style_set_bg_color(&top_bar_style, top_bar_color);
	lv_style_set_bg_grad_color(&top_bar_style, lv_color_hex(0xFFFFFF)); // Color blanco
	lv_style_set_bg_grad_dir(&top_bar_style, LV_GRAD_DIR_VER); // Dirección del degradado
	lv_style_set_border_width(&top_bar_style, 0);
	lv_style_set_radius(&top_bar_style, 0);
	lv_style_set_pad_top(&top_bar_style, 0);
	lv_style_set_pad_bottom(&top_bar_style, 0);
	lv_style_set_pad_left(&top_bar_style, 0);
	lv_style_set_pad_right(&top_bar_style, 0);
	// Añadir sombra
	lv_style_set_shadow_color(&top_bar_style, lv_color_hex(0x000000)); // Color de la sombra (negro)
	lv_style_set_shadow_width(&top_bar_style, 10); // Ancho de la sombra
	lv_style_set_shadow_ofs_y(&top_bar_style, 3); // Desplazamiento de la sombra en el eje Y
	lv_style_set_shadow_opa(&top_bar_style, 20);
	lv_obj_add_style(top_bar, &top_bar_style, LV_PART_MAIN);  /* Apply the style */
	lv_obj_clear_flag(top_bar, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_add_flag(top_bar, LV_OBJ_FLAG_CLICKABLE);
	lv_obj_clear_flag(top_bar, LV_OBJ_FLAG_HIDDEN);

	// crear fila
	lv_obj_t* top_bar_row = lv_obj_create(top_bar);
	lv_obj_set_flex_flow(top_bar_row, LV_FLEX_FLOW_ROW); /* Layout en fila */
	lv_obj_set_size(top_bar_row, lv_pct(100), lv_pct(100));
	lv_obj_clear_flag(top_bar_row, LV_OBJ_FLAG_SCROLLABLE);    // sin barra de scroll
	lv_obj_add_style(top_bar_row, &top_bar_style, LV_PART_MAIN);


	// Crear el label para la room
	lv_obj_t* label_room = lv_label_create(top_bar);
	lv_label_set_text(label_room, "Mi room");
	lv_obj_align(label_room, LV_ALIGN_LEFT_MID, 0, 0);
	lv_obj_set_flex_grow(label_room, 1);
	lv_obj_set_style_pad_left(label_room, 4, LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(label_room, &lv_font_montserrat_12, LV_PART_MAIN);

	// Crear indicador wifi
	lv_obj_t* icon_container = lv_obj_create(top_bar);
	lv_obj_set_flex_grow(icon_container, 1);
	lv_obj_set_height(icon_container, lv_pct(100));
	lv_obj_align(icon_container, LV_ALIGN_CENTER, 0, 0);
	lv_obj_clear_flag(icon_container, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_style_border_width(icon_container, 0, LV_PART_MAIN);
	lv_obj_set_style_bg_opa(icon_container, 0, LV_PART_MAIN);
	lv_obj_t* wifi_icon_line = create_wifi_indicator(icon_container);

	// Crear label fecha y hora - localidad
//	lv_obj_t* label_locale = lv_label_create(top_bar);
//	uint8_t dia = 2, mes = 3, hora = 10, min = 25;
//	uint16_t anio = 2025;
//	const char* localidad = "Cadiz";
//	lv_label_set_text_fmt(label_locale, "%s - %02hhu/%02hhu/%04hu %02hhu:%02hhu",
//	    localidad, dia, mes, anio, hora, min);
//	lv_obj_align(label_locale, LV_ALIGN_RIGHT_MID, 0, 0);
//	lv_obj_set_flex_grow(label_locale, 1);
//	lv_obj_set_style_pad_right(label_locale, 4, LV_STATE_DEFAULT);
//	lv_obj_set_style_text_font(label_locale, &lv_font_montserrat_12, LV_PART_MAIN);

	// out
	if (out_handler != NULL)
	{
	    out_handler->label_room = label_room;
	    out_handler->network_indicator = wifi_icon_line;
//	    out_handler->label_locale = label_locale;
	}

	// necesario devolver al manejador de la top_bar para que otros
	// widgets se puedan posicionar realito a este si lo necesitaran
	return top_bar;
}

// Public API

void aqi_ui_init(lv_disp_t *disp)
{
	lv_obj_t *scr = lv_disp_get_scr_act(disp);

	lv_disp_set_rotation(disp, LV_DISP_ROT_90);

	// TOP BAR
	lv_obj_t* top_bar = create_top_bar(scr, &ui_top_bar_handler);
	// END TOP BAR ==============================================

	// 1st row
	lv_obj_t * cont_row = lv_obj_create(lv_scr_act());
	lv_obj_set_size(cont_row, lv_pct(100), lv_pct(46));
	lv_obj_align_to(cont_row, top_bar, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);
	lv_obj_set_flex_flow(cont_row, LV_FLEX_FLOW_ROW);
	lv_obj_set_style_pad_all(cont_row, 4, LV_PART_MAIN);

	label_temperature = create_sensor_info_box(cont_row, "Temperatura", "°C");

	label_humidity = create_sensor_info_box(cont_row, "Humedad", "%");

	create_aiq_info_widget(cont_row, "Calidad del aire", &ui_voc_widget_handler);


	// 2nd row
	lv_obj_t* cont_row2 = lv_obj_create(lv_scr_act());
	lv_obj_set_size(cont_row2, lv_pct(100), lv_pct(46));
	lv_obj_align_to(cont_row2, cont_row, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);
	lv_obj_set_flex_flow(cont_row2, LV_FLEX_FLOW_COLUMN);
	lv_obj_clear_flag(cont_row2, LV_OBJ_FLAG_SCROLLABLE);

	contenedor_alarmas = row_alarms_insert_header(cont_row2);

	// NOTA: a partir de aqui se pueden crear "alarms rows" con 'insert_alarm_row'
	// asignando como padre a 'contenedor_alarmas'

	// Lanzar task
	if (xTaskCreatePinnedToCore(aqi_UI_Task, "aqi_UI", 4096, NULL, 4, NULL, 1) != pdPASS)
	{
		ESP_LOGE(TAG, "No se ha creado la task de la UI");
	}

	// Crear temporizadores lvgl para tareas periodicas de la UI
	lv_timer_create(update_periodic_ui_data_cb, 5000, NULL);
}

void aqi_ui_set_network_state(bool is_connected)
{
    // TBD
}

void aqi_ui_set_locale(const char* city, uint8_t day, uint8_t month,
		uint8_t year, uint8_t hour, uint8_t minutes)
{
    // TBD
}
