/*
 * blufi_manager.h
 *
 *  Created on: 16 may 2025
 *      Author: NRR
 */

#ifndef MAIN_BLUFI_MANAGER_H_
#define MAIN_BLUFI_MANAGER_H_

/**
 * @brief Inicia el blufi manager.
 *
 * @note NVS debe haber sido inicializado previamente
 */
extern void blufi_manager_start(TaskHandle_t caller_task);
extern void blufi_manager_start_wifi_only(TaskHandle_t caller_task);
extern bool blufi_manager_wifi_is_connected(void);
extern void blufi_manager_block_until_provisioning_ends();
extern esp_err_t blufi_manager_release_wifi_config();

#endif /* MAIN_BLUFI_MANAGER_H_ */
