import paho.mqtt.client as mqtt
import json
import time
import random

# Configuración del broker MQTT
broker_address = "192.168.1.17"
broker_port = 1883
topic_measures = "/tfm/nrr/airquality/measures"
topic_alarms = "/tfm/nrr/airquality/alarms"

# Rango de valores normales
alarm_ranges = {
    0: lambda temp: temp > 27,  # Temperatura elevada
    1: lambda temp: temp < 19,  # Temperatura baja
    2: lambda hum: hum > 65,    # Humedad elevada
    3: lambda hum: hum < 50,    # Humedad baja
    4: lambda voc: voc > 150    # Calidad del aire mala
}

# Mensajes de alarma
alarm_messages = {
    0: "La temperatura es demasiado elevada. Recomendamos activar la climatizacion.",
    1: "La temperatura es demasiado baja. Recomendamos activar la climatizacion.",
    2: "La humedad es demasiado elevada. Recomendamos deshumidificar el ambiente.",
    3: "La humedad es demasiado baja. Recomendamos humidificar el ambiente.",
    4: "La calidad del aire es mala. Recomendamos ventilar o purificar el aire."
}

# Función para generar datos aleatorios
def generate_data():
    return {
        "humidity_rh": random.randint(0, 100),
        "temp_celsius": random.randint(-10, 55),
        "voc_raw": random.randint(100, 1000),
        "voc_index": random.randint(1, 500)
    }

# Función para generar alarmas si se violan los rangos
#   Devuelve lista con alarms en forma de diccionario
#   para poder generar los json de forma sencilla
def check_alarms_activacion(data):
    alarms = []
    activadas = []
    
    data_check = {
        0: data["temp_celsius"],
        1: data["temp_celsius"],
        2: data["humidity_rh"],
        3: data["humidity_rh"],
        4: data["voc_index"],
    }
    
    for tipo_alarm in range(5):
        if (alarm_ranges[tipo_alarm](data_check[tipo_alarm])):
            activadas.append(tipo_alarm)
            alarms.append({
                "disable": False,
                "alarm_class": tipo_alarm,
                "info": alarm_messages[tipo_alarm]
            })
    
    return set(activadas), alarms


# Crear una instancia del cliente MQTT
client = mqtt.Client()
client.connect(broker_address, broker_port, 60)

# Alarmas enviadas activas (solo sus clases para checkear)
last_sent_alarms = set()

# Publicar datos cada segundo y alarmas cada minuto
try:
    last_alarm_time = time.time()
    while True:
        data = generate_data()
        payload = json.dumps(data)
        client.publish(topic_measures, payload)
        print(f"Publicado en {topic_measures}: {payload}")

        current_time = time.time()
        if current_time - last_alarm_time >= 10:
        
        # si habia alguna alarma activada y esta vez no esta en el 
        # checkeo de las nuevas activadas ==> esa debe desactivarse
            generadas, alarms_to_activate = check_alarms_activacion(data)            

            # Si alarmas activas no esta vacio y alguna en activas
            # no esta en generadas --> enviar desactivacion
            para_desactivar = last_sent_alarms.difference(generadas)                            
            for tipo_desactivar in para_desactivar:
                alarm_payload = json.dumps({
                    "disable": True,
                    "alarm_class": tipo_desactivar,
                    "info": alarm_messages[tipo_desactivar]
                })
                client.publish(topic_alarms, alarm_payload)
                print(f"Publicado DESACTIVACION en {topic_alarms}: {alarm_payload}")
                last_sent_alarms.remove(tipo_desactivar)                                    
            
            for alarm_message in alarms_to_activate:
                # enviar las generadas que no se hayan enviado previamente (no esten en activadas)
                if (alarm_message["alarm_class"] not in last_sent_alarms):
                    alarm_payload = json.dumps(alarm_message)
                    last_sent_alarms.add(alarm_message["alarm_class"])                    
                    client.publish(topic_alarms, alarm_payload)
                    print(f"Publicado ACTIVACION en {topic_alarms}: {alarm_payload}")                              

            last_alarm_time = current_time

        time.sleep(1)

except KeyboardInterrupt:
    print("Interrumpido por el usuario")

client.disconnect()
