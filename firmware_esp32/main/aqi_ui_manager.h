/*
 * aqi_ui_manager.h
 *
 *  Created on: 29 mar 2025
 *      Author: nrr
 */

#ifndef MAIN_AQI_UI_MANAGER_H_
#define MAIN_AQI_UI_MANAGER_H_

#include <stdbool.h>
#include <stdint.h>

#include "lvgl.h"

/**
 * @brief Primero inicializa la interfaz grafica con LVGL en el LCD.
 *		  Inicia la task que maneja el refresco de la UI
 *
 *
 * @param disp Puntero al objeto de visualización LVGL.
 */
extern void aqi_ui_init(lv_disp_t *disp);

/**
 * @brief Establece el estado de la red en la interfaz gráfica.
 *
 * @param is_connected Estado de la conexión de red (true para conectado, false para desconectado).
 */
extern void aqi_ui_set_network_state(bool is_connected);

/**
 * @brief Establece la localización y la fecha/hora en la interfaz gráfica.
 *
 * @param city Cadena de caracteres con el nombre de la ciudad.
 * @param day Día del mes.
 * @param month Mes del año.
 * @param year Año.
 * @param hour Hora del día.
 * @param minutes Minutos.
 */
extern void aqi_ui_set_locale(const char* city, uint8_t day, uint8_t month,
		uint8_t year, uint8_t hour, uint8_t minutes);


#endif /* MAIN_AQI_UI_MANAGER_H_ */
