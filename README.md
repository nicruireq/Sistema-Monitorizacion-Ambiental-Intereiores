# Sistema-Monitorizacion-Ambiental-Intereiores

Este Trabajo Fin de Máster tiene como objetivo desarrollar un dispositivo de monitorización ambiental con conectividad inalámbrica, diseñado para integrarse en entornos Smart Home. El dispositivo permite medir variables ambientales como temperatura, humedad y compuestos orgánicos volátiles.

## Estructura del repositorio

En este repositorio se encuentra el material software necesario para regenerar el proyecto. A continuación detallamos los directorios y su contenido:

- firmware_esp32: se encuentran los ficheros de código (main/*.h/c), configuraciones (sdkconfig) que implementan el firmware del dispositivo de medición. Puede usarse para recrear el proyecto y compilar el firmware.
- iotserver_raspberry/IOTStack/volumes: se encuentran todos los ficheros de configuración y bases de datos necesarios para recrear las aplicaciones en el lado del servidor. Hay un subdirectorio por aplicación (grafana, home_assistant, influxdb, mosquitto, nodered).
- test: scripts en python para generar datos de prueba para el stack de aplicaciones servidor. Genera automaticamente datos que se publican en el bróker MQTT.

## Releases

En el apartado de releases de este repositorio hay dispponibles empaquetados zip del formware del dispositivo directamente importables en el Espressif-IDE v2.9.1.

## Regenerar apps servidor con IOTstack y ejecución en Raspberry Pi 3 B
