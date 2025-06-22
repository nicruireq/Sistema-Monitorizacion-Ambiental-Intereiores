idf_component_register(SRCS "proyecto_main.c" "miscomandos.c" "wifi.c" "gpio_leds.c" "frozen.c" "mqtt.c" 
							"sensirion_crc.c" "sgp40driver.c" "sht40driver.c" "global_system_signaler.c"
							"sensors_type.c" "sensors_service.c" "sensirion_gas_index_algorithm.c" "i2c_master.c"
							"aqi_config_manager.c" "aqi_device_config_type.c"
							"aqi_ui_manager.c" "aqi_alarm_manager.c" "alarm_type.c"	"aqi_alarm_triggers.c"	
							"blufi_security.c" "blufi_init.c" "blufi_manager.c"		
     INCLUDE_DIRS "."                    	
)

# IMPORTANTE: CREA la imagen.... cuidado con el nombre que debe ser el de la partisao.                    
# Create a SPIFFS image from the contents of the 'spiffs_image' directory
# that fits the partition named 'storage'. FLASH_IN_PROJECT indicates that
# the generated image should be flashed when the entire project is flashed to
# the target with 'idf.py -p PORT flash'.
# partspiffs es el nombre que tiene en la tabla de particiones y debe coincidir con el c�digo que lo monta. 