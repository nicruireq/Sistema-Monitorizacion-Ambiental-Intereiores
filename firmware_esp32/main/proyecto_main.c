/* Console example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#define USE_BLUFI

#include <stdio.h>
#include <string.h>
#include "esp_system.h"
#include "esp_log.h"
#include "esp_console.h"
#include "esp_vfs_dev.h"
#include "driver/uart.h"
#include "linenoise/linenoise.h"
#include "argtable3/argtable3.h"
#include "cmd_system.h"
#include "miscomandos.h"
#include "esp_vfs_fat.h"
#include "nvs_flash.h"

#include <time.h>

#include "driver/ledc.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>


#ifdef USE_BLUFI
	#include "blufi_manager.h"
#else
	#include "wifi.h"
#endif

#include "bsp/esp-bsp.h"
#include "gpio_leds.h"

#include "mqtt.h"
#include "sensors_service.h"
#include "global_system_signaler.h"
#include "aqi_config_manager.h"
#include "aqi_ui_manager.h"


//TAG para los mensajes de consola
static const char *TAG = "AQI_CMD";


/* Console command history can be stored to and loaded from a file.
 * The easiest way to do this is to use FATFS filesystem on top of
 * wear_levelling library.
 */
#if CONFIG_STORE_HISTORY

#define MOUNT_PATH "/data"
#define HISTORY_PATH MOUNT_PATH "/history.txt"

static void initialize_filesystem(void)
{
    static wl_handle_t wl_handle;
    const esp_vfs_fat_mount_config_t mount_config = {
            .max_files = 4,
            .format_if_mount_failed = true
    };
    esp_err_t err = esp_vfs_fat_spiflash_mount_rw_wl(MOUNT_PATH, "storage", &mount_config, &wl_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to mount FATFS (%s)", esp_err_to_name(err));
        return;
    }
}
#endif // CONFIG_STORE_HISTORY

//Inicializa el interprete de comandos/consola serie
static void initialize_console(void)
{
    /* Drain stdout before reconfiguring it */
    fflush(stdout);
    fsync(fileno(stdout));

    /* Disable buffering on stdin */
    setvbuf(stdin, NULL, _IONBF, 0);

    /* Minicom, screen, idf_monitor send CR when ENTER key is pressed */
    esp_vfs_dev_uart_port_set_rx_line_endings(CONFIG_ESP_CONSOLE_UART_NUM,ESP_LINE_ENDINGS_CR);
    /* Move the caret to the beginning of the next line on '\n' */
    esp_vfs_dev_uart_port_set_tx_line_endings(CONFIG_ESP_CONSOLE_UART_NUM,ESP_LINE_ENDINGS_CRLF);

    /* Configure UART. Note that REF_TICK is used so that the baud rate remains
     * correct while APB frequency is changing in light sleep mode.
     */
    const uart_config_t uart_config = {
            .baud_rate = CONFIG_ESP_CONSOLE_UART_BAUDRATE,
            .data_bits = UART_DATA_8_BITS,
            .parity = UART_PARITY_DISABLE,
            .stop_bits = UART_STOP_BITS_1,
            .source_clk = UART_SCLK_DEFAULT,
    };
    /* Install UART driver for interrupt-driven reads and writes */
    ESP_ERROR_CHECK( uart_driver_install(CONFIG_ESP_CONSOLE_UART_NUM,
            256, 0, 0, NULL, 0) );
    ESP_ERROR_CHECK( uart_param_config(CONFIG_ESP_CONSOLE_UART_NUM, &uart_config) );

    /* Tell VFS to use UART driver */
    esp_vfs_dev_uart_use_driver(CONFIG_ESP_CONSOLE_UART_NUM);

    /* Initialize the console */
    esp_console_config_t console_config = {
            .max_cmdline_args = 8,
            .max_cmdline_length = 256,
#if CONFIG_LOG_COLORS
            .hint_color = atoi(LOG_COLOR_CYAN)
#endif
    };
    ESP_ERROR_CHECK( esp_console_init(&console_config) );

    /* Configure linenoise line completion library */
    /* Enable multiline editing. If not set, long commands will scroll within
     * single line.
     */
    linenoiseSetMultiLine(1);

    /* Tell linenoise where to get command completions and hints */
    linenoiseSetCompletionCallback(&esp_console_get_completion);
    linenoiseSetHintsCallback((linenoiseHintsCallback*) &esp_console_get_hint);

    /* Set command history size */
    linenoiseHistorySetMaxLen(100);

#if CONFIG_STORE_HISTORY
    /* Load command history from filesystem */
    linenoiseHistoryLoad(HISTORY_PATH);
#endif
}

void app_main(void)
{

	//Initialize NVS
	bool provisioning_flag = false;

	esp_err_t ret = aqi_config_manager_init();
	ESP_ERROR_CHECK(ret);


	//Inicializa el GPIO
	GL_initGPIO(); //Inicializa los pines de salida


#if CONFIG_STORE_HISTORY
    initialize_filesystem();
    ESP_LOGI(TAG, "Command history enabled");
#else
    ESP_LOGI(TAG, "Command history disabled");
#endif

   // Inicializacion Wifi
#ifndef USE_BLUFI
    //Inicializa la biblioteca WiFi (crea el DEFAULT event loop()
    wifi_initlib();

    //Conecta a la Wifi o se pone en modo AP
#if CONFIG_EXAMPLE_CONNECT_WIFI_STATION
    wifi_init_sta();
#else // CONFIG_EXAMPLE_CONNECT_WIFI_STATION
    wifi_init_softap();
#endif
#else
    if (aqi_config_manager_get(AQI_CV_WIFI_PROVISIONING_STATE,
    		&provisioning_flag, sizeof(provisioning_flag)) == ESP_OK)
    {
    	if (provisioning_flag)
    	{
    		ESP_LOGI(TAG, "Start wifi provisioning");

    		blufi_manager_start(xTaskGetCurrentTaskHandle());
    	}
    	else
    	{
    		ESP_LOGI(TAG, "Start wifi no provision required");

    		blufi_manager_start_wifi_only(xTaskGetCurrentTaskHandle());
    	}
    }

    // en este punto habria que bloquearse hasta que termine el provisioning
    // o haya conexion wifi con IP asignada por AP
    blufi_manager_block_until_provisioning_ends();
    // limpiar flag en NVS
    provisioning_flag = false;
    ret = aqi_config_manager_set(AQI_CV_WIFI_PROVISIONING_STATE,
    		&provisioning_flag, sizeof(provisioning_flag));
    switch (ret)
    {
    case ESP_OK:
    	ESP_LOGI(TAG, "Provisioning flag limpiado.");
    	break;
    case ESP_ERR_INVALID_ARG:
    	ESP_LOGE(TAG, "Argumento para provisioning flag invalido (tipo o size).");
    	break;
    case ESP_FAIL:
    	ESP_LOGE(TAG, "Error limpiando provisioning flag. Error de escritura en NVS.");
    	break;
    default:
    	ESP_LOGE(TAG, "Error desconocido limpiando provisioning flag.");
    }
#endif
    // Fin inicializacion wifi

    //inicializa la consola
    initialize_console();

    /* Register commands */
    esp_console_register_help_command();
    register_system();
    init_MisComandos();



    /******************************
     *  IMPORTANTE MIS MODULOS
     *
     *******************************/

    // Inicializar el Global System Signaler
    ret = gss_initialize();
    ESP_ERROR_CHECK(ret);

    // Inicializar servicio de sensores
    ret = sensors_service_init(1000, I2C_NUM_0, GPIO_NUM_26, GPIO_NUM_27);
    ESP_ERROR_CHECK(ret);

    // Inicializar UI
    lv_disp_t *disp = bsp_display_start();
    aqi_ui_init(disp);
    bsp_display_backlight_on();
    /*************************************************/

    // Arranca el cliente MQTT.
#if CONFIG_EXAMPLE_CONNECT_WIFI_STATION
    mqtt_app_start(MQTT_BROKER_URL);
    // error control missing SHOULD BE ADDED.
#endif

    /* Prompt to be printed before each line.
     * This can be customized, made dynamic, etc.
     */
    const char* prompt = LOG_COLOR_I "esp32> " LOG_RESET_COLOR;

    printf("\n"
           "This is an example of ESP-IDF console component.\n"
           "Type 'help' to get the list of commands.\n"
           "Use UP/DOWN arrows to navigate through command history.\n"
           "Press TAB when typing command name to auto-complete.\n");



    /* Figure out if the terminal supports escape sequences */
    int probe_status = linenoiseProbe();
    if (probe_status) { /* zero indicates success */
        printf("\n"
               "Your terminal application does not support escape sequences.\n"
               "Line editing and history features are disabled.\n"
               "On Windows, try using Putty instead.\n");
        linenoiseSetDumbMode(1);
#if CONFIG_LOG_COLORS
        /* Since the terminal doesn't support escape sequences,
         * don't use color codes in the prompt.
         */
        prompt = "esp32> ";
#endif //CONFIG_LOG_COLORS
    }

    /* Main loop (ejecuta el interprete de comandos)*/
    while(true) {
        /* Get a line using linenoise.
         * The line is returned when ENTER is pressed.
         */
        char* line = linenoise(prompt);
        if (line == NULL) { /* Ignore empty lines */
            continue;
        }
        /* Add the command to the history */
        linenoiseHistoryAdd(line);
#if CONFIG_STORE_HISTORY
        /* Save command history to filesystem */
        linenoiseHistorySave(HISTORY_PATH);
#endif

        /* Try to run the command */
        int ret;
        esp_err_t err = esp_console_run(line, &ret);
        if (err == ESP_ERR_NOT_FOUND) {
            printf("Unrecognized command\n");
        } else if (err == ESP_ERR_INVALID_ARG) {
            // command was empty
        } else if (err == ESP_OK && ret != ESP_OK) {
            printf("Command returned non-zero error code: 0x%x (%s)\n", ret, esp_err_to_name(ret));
        } else if (err != ESP_OK) {
            printf("Internal error: %s\n", esp_err_to_name(err));
        }
        /* linenoise allocates line buffer on the heap, so need to free it */
        linenoiseFree(line);
    }
}


