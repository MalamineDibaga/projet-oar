import sys
import time
import ssl
import broadlink
import paho.mqtt.client as mqtt
import requests

# === CONFIGURATION ===
BROADLINK_IP = "192.168.5.79"  # Adresse IP du Broadlink
MQTT_BROKER = "10.0.20.2"  # Adresse IP du broker MQTT Mosquitto
MQTT_PORT = 8883  # Port sécurisé MQTTS
MQTT_TOPIC_TEMP = "sensors/temperature"  # Topic pour la température
MQTT_TOPIC_HUMID = "sensors/humidity"  # Topic pour l'humidité
CA_CERT_PATH = "/etc/mosquitto/certs/ca.crt"  # Certificat de l'Autorité de Certification
IR_COMMAND = bytes.fromhex("26004800000122941313121312141213121312381213121412371238123812371337121412371238121313371213131312131213121412131237131312371337133712381238123713000d05")  # Commande IR par défaut
SHELLY_IP = "192.168.5.251"  # Adresse IP de l'appareil Shelly pour obtenir la consommation

# === Seuils d'hystérésis ===
TEMP_UPPER_THRESHOLD = 22.0  # Seuil supérieur (active le système de refroidissement si > 22°C)
TEMP_LOWER_THRESHOLD = 20.0  # Seuil inférieur (désactive le système de refroidissement si < 18°C)
CONSUMPTION_THRESHOLD = 5.0  # Seuil de consommation minimale (W) pour considérer que la prise est allumée

# === Classe de contrôle du serveur ===
class ServerRoomControlApp:
    def __init__(self):
        self.device = None
        self.client = None
        self.temperature = None
        self.humidity = None
        self.consumption = None  # Valeur de consommation

        # Connexion au Broadlink et au MQTT
        self.connect_broadlink()
        self.connect_mqtt()

        # Timer pour rafraîchir les données de température, d'humidité et de consommation
        self.run()

    def connect_broadlink(self):
        """Connexion au Broadlink"""
        try:
            print(f"Connexion au Broadlink sur {BROADLINK_IP}...")
            self.device = broadlink.hello(BROADLINK_IP)
            self.device.auth()
            print("Connexion réussie au Broadlink.")
        except Exception as e:
            print(f"Erreur de connexion au Broadlink : {e}")

    def connect_mqtt(self):
        """Connexion au broker MQTT avec authentification"""
        self.client = mqtt.Client()

        # Définir les informations d'authentification
        self.client.username_pw_set(username="userbroker", password="ciel")
        self.client.tls_set(CA_CERT_PATH, certfile=None, keyfile=None, tls_version=ssl.PROTOCOL_TLSv1_2)
        self.client.tls_insecure_set(True)  # Désactivation de la vérification du certificat (utile si auto-signé)

        self.client.on_connect = self.on_connect
        self.client.on_message = self.on_message

        try:
            self.client.connect(MQTT_BROKER, MQTT_PORT, 60)
            print(f"Connexion au broker MQTT à {MQTT_BROKER} sur le port {MQTT_PORT}...")
            self.client.loop_start()
        except Exception as e:
            print(f"Erreur de connexion au broker MQTT : {e}")

    def on_connect(self, client, userdata, flags, rc):
        """Abonnement aux topics de température et d'humidité"""
        print(f"Connecté au broker MQTT avec code {rc}")
        client.subscribe(MQTT_TOPIC_TEMP)
        client.subscribe(MQTT_TOPIC_HUMID)

    def on_message(self, client, userdata, msg):
        """Gestion des messages MQTT"""
        if msg.topic == MQTT_TOPIC_TEMP:
            self.temperature = float(msg.payload.decode())
            print(f"Température : {self.temperature:.2f}°C")
            self.control_air_conditioner()  # Gère l'activation/désactivation du système de refroidissement

        elif msg.topic == MQTT_TOPIC_HUMID:
            self.humidity = float(msg.payload.decode())
            print(f"Humidité : {self.humidity:.2f}%")

    def control_air_conditioner(self):
        """Logique d'hystérésis pour contrôler le climatiseur, avec vérification de la consommation"""
        if self.consumption is None:
            print("Impossible de vérifier la consommation.")
            return

        # Si la température est supérieure au seuil et que la consommation est faible (prise éteinte)
        if self.temperature > TEMP_UPPER_THRESHOLD:
            if self.consumption < CONSUMPTION_THRESHOLD:
                # La consommation est faible, donc la prise est probablement éteinte. On allume le système de refroidissement.
                print("Température élevée et la prise est éteinte, envoi de la commande IR pour activer le système de refroidissement.")
                self.device.send_data(IR_COMMAND)
            else:
                print("Température élevée, mais le système est déjà allumé (consommation > seuil).")

        # Si la température est inférieure au seuil et que la consommation est élevée (prise allumée)
        elif self.temperature < TEMP_LOWER_THRESHOLD:
            if self.consumption >= CONSUMPTION_THRESHOLD:
                # La consommation est supérieure au seuil, donc la prise est allumée. On arrête le système de refroidissement.
                print("Température faible et la prise est allumée, envoi de la commande IR pour éteindre le système de refroidissement.")
                self.device.send_data(IR_COMMAND)
            else:
                print("Température faible, mais le système est déjà éteint (consommation faible).")

    def update_consumption(self):
        """Récupère la consommation en watts de l'appareil Shelly via une requête GET"""
        try:
            response = requests.get(f"http://{SHELLY_IP}/status")
            if response.status_code == 200:
                data = response.json()
                consumption = data.get('meters', [{}])[0].get('power', None)  # Consommation en watts
                if consumption is not None:
                    self.consumption = consumption
                    print(f"Consommation : {self.consumption} W")
                else:
                    print("Aucune donnée de consommation trouvée.")
            else:
                print(f"Erreur de récupération des données Shelly, code HTTP : {response.status_code}")
        except Exception as e:
            print(f"Erreur de connexion à l'API Shelly : {e}")

    def run(self):
        """Boucle principale pour afficher les données et mettre à jour la consommation"""
        try:
            while True:
                # Mettre à jour les valeurs de température, d'humidité et de consommation
                if self.temperature is not None:
                    print(f"Température actuelle : {self.temperature:.2f}°C")
                if self.humidity is not None:
                    print(f"Humidité actuelle : {self.humidity:.2f}%")

                self.update_consumption()  # Mettre à jour la consommation toutes les secondes

                # Attendre 1 seconde avant la prochaine mise à jour
                time.sleep(1)
        except KeyboardInterrupt:
            print("\nArrêt de l'application...")

# === Lancement de l'application ===
if __name__ == "__main__":
    app = ServerRoomControlApp()

