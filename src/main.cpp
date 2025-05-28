// Programme principal - Récupération et utilisation des identifiants Wi-Fi et MQTT
#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <WiFiClientSecure.h>
#include "SecureStorage.h"

// ------------------- PARAMETRAGES DU CAPTEUR DHT ------------------------
#define DHTPIN 4               // Définit la broche GPIO 4 de l'ESP32 pour le capteur DHT22
#define DHTTYPE DHT22          // Spécifie que le capteur utilisé est le DHT22 (température et humidité)
DHT dht(DHTPIN, DHTTYPE);      // Crée une instance du capteur DHT22 sur la broche définie

// ------------------- CERTIFICAT DE L'AUTORITÉ DE CERTIFICATION ------------------------
const char* ca_cert = R"(
-----BEGIN CERTIFICATE-----
MIIDVzCCAj+gAwIBAgIUMmWs9hnIQUR2FDQmV4br5mZNr6swDQYJKoZIhvcNAQEL
BQAwOzELMAkGA1UEBhMCRlIxDDAKBgNVBAoMA0lPVDEPMA0GA1UECwwGY2xpZW50
MQ0wCwYDVQQDDAR0ZXN0MB4XDTI1MDMwNzA3MDkwMFoXDTMwMDMxNDA3MDkwMFow
OzELMAkGA1UEBhMCRlIxDDAKBgNVBAoMA0lPVDEPMA0GA1UECwwGY2xpZW50MQ0w
CwYDVQQDDAR0ZXN0MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAqeTc
n+IZSgKQ5bTnUjqOdajd+y6JV0xy3Y1Em3j6pZirHJ6TSjorAbhgCbYSTWdExUwS
2DpAnobx3M8+4++1lFNTVKTu/Lw5Om4dFX/KjRT9BMBJX/gDY60AvsU/mD7YKfHh
r9rJHUQTFmPvHsz6TAmz6cWJemHckmGDVI8cfgR2U/iwim/JxIc8pNvQg8mPQfnW
LPfUWth+m09aUSIC1G/71G+adav+CaClk4kt4T73W8Qt8LeVeDNxwBwQ5E8yxk7G
LqyPjLI0HBj918IQt9BEGCSbD/St43CgazXU9OXrts65ftoFnCwHMx67sYqaelu5
tNlb0ZzxAQmByb/0WwIDAQABo1MwUTAdBgNVHQ4EFgQUCWpI+UCyQoGK9mNcn1Qs
cms9zkUwHwYDVR0jBBgwFoAUCWpI+UCyQoGK9mNcn1Qscms9zkUwDwYDVR0TAQH/
BAUwAwEB/zANBgkqhkiG9w0BAQsFAAOCAQEAENE95YbtPDaaTRJlgb/3ByIBRnTD
sETwDfat3FqELfNJLmGP2nvQV1NyEaQexVC/WJuT7G4QeTuaVsY2iEAC7SF6BRPX
9c9k8OBSdQ6xxS2nA1CmGsjX7diOf9ZCjQBIXfuGiGNTA1PR03HHIZBzheD5C3dL
duFbrKQUkK7gIailBhhRcbvDJnkJOJsf0kgKgV5a3gkkL/GgQVVNQT0RIQiMLcBw
3RxHYWDp589/Jlf7bmQgLLa1gKeDiFUWTJB0U7BgB6Pvco8uoV4Ldt2q2EMrH2Ix
9ZcuWxYzKUtw000wC51diAkIzXLK08ayUiuXPq4O3pq8rj0Ahzb5NMrs4g==
-----END CERTIFICATE-----
)";  // Certificat du CA utilisé pour sécuriser la connexion MQTT via TLS/SSL

// ------------------- OBJETS POUR LA CONNEXION WIFI ET MQTT ------------------------
WiFiClientSecure espClient;     // Objet pour gérer la connexion sécurisée (SSL/TLS) Wi-Fi
PubSubClient client(espClient); // Objet pour gérer la connexion MQTT via l'objet WiFiClientSecure
SecureStorage storage;          // Instance de la classe SecureStorage pour récupérer les identifiants

// Variables pour stocker les identifiants récupérés
char wifi_ssid[64] = {0};
char wifi_pass[64] = {0};
char mqtt_server[64] = {0};
int mqtt_port = 0;
char mqtt_user[64] = {0};
char mqtt_pass[64] = {0};

// ------------------- FONCTION DE RECONNEXION MQTT ------------------------
void reconnect() {
  // Boucler jusqu'à ce que nous soyons reconnectés
  int tentatives = 0;
  while (!client.connected() && tentatives < 5) {
    Serial.print("Tentative de connexion MQTT...");
    tentatives++;
    
    // Tentative de connexion avec les identifiants récupérés
    if (client.connect("ESP32Client", mqtt_user, mqtt_pass)) {
      Serial.println("Connecté au broker MQTT!");
      
      // Souscription aux topics si nécessaire
      // client.subscribe("commandes/led");
      
      // Publier un message pour signaler la connexion
      client.publish("device/status", "ESP32 connecté");
    } else {
      Serial.print("Échec, code d'erreur: ");
      Serial.print(client.state());
      Serial.println(" Nouvelle tentative dans 2 secondes...");
      delay(2000);
    }
  }
  
  if (!client.connected() && tentatives >= 5) {
    Serial.println("Impossible de se connecter au broker MQTT après 5 tentatives.");
    Serial.println("Vérifiez les identifiants MQTT ou la connectivité du serveur.");
  }
}

// Fonction pour récupérer et afficher toutes les informations stockées
void displayAllStoredInformation() {
  Serial.println("\n=== Informations récupérées depuis la NVS ===");
  
  // Affichage des informations Wi-Fi
  Serial.print("SSID Wi-Fi: ");
  Serial.println(wifi_ssid);
  Serial.print("Mot de passe Wi-Fi: ");
  Serial.println(wifi_pass);
  
  // Affichage des informations MQTT
  Serial.print("Serveur MQTT: ");
  Serial.println(mqtt_server);
  Serial.print("Port MQTT: ");
  Serial.println(mqtt_port);
  Serial.print("Utilisateur MQTT: ");
  Serial.println(mqtt_user);
  Serial.print("Mot de passe MQTT: ");
  Serial.println(mqtt_pass);
}

// ------------------- FONCTION D'INITIALISATION (SETUP) ------------------------
void setup() {
  // Initialisation de la communication série pour le debug
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("=== Programme principal avec récupération des identifiants Wi-Fi et MQTT ===");
  WiFi.persistent(false);
  
  // Initialisation du capteur DHT22
  dht.begin();
  
  // Récupération des identifiants Wi-Fi depuis le stockage sécurisé
  Serial.println("Récupération des identifiants Wi-Fi depuis la mémoire NVS...");
  
  bool wifi_creds_ok = storage.retrieveSecret("wifi_ssid", wifi_ssid, sizeof(wifi_ssid)) &&
                      storage.retrieveSecret("wifi_pass", wifi_pass, sizeof(wifi_pass));
  
  // Récupération des informations MQTT depuis le stockage sécurisé
  Serial.println("Récupération des informations MQTT depuis la mémoire NVS...");
  
  bool mqtt_creds_ok = storage.retrieveSecret("mqtt_server", mqtt_server, sizeof(mqtt_server)) &&
                      storage.retrieveInt("mqtt_port", &mqtt_port) &&
                      storage.retrieveSecret("mqtt_user", mqtt_user, sizeof(mqtt_user)) &&
                      storage.retrieveSecret("mqtt_pass", mqtt_pass, sizeof(mqtt_pass));
  
  if (wifi_creds_ok && mqtt_creds_ok) {
    // Affichage de toutes les informations récupérées
    displayAllStoredInformation();
    
    // Connexion au réseau Wi-Fi avec les identifiants récupérés
    Serial.println("\nConnexion au Wi-Fi...");
    WiFi.begin(wifi_ssid, wifi_pass);
    
    // Attente que la connexion Wi-Fi soit établie (avec timeout)
    int tentatives = 0;
    while (WiFi.status() != WL_CONNECTED && tentatives < 30) {
      delay(1000);
      Serial.print(".");
      tentatives++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\nConnecté au Wi-Fi!");
      Serial.print("Adresse IP: ");
      Serial.println(WiFi.localIP());
      
      // Charger le certificat de l'autorité de certification pour établ
      espClient.setCACert(ca_cert);
      
      // Configuration du serveur MQTT
      client.setServer(mqtt_server, mqtt_port);
    } else {
      Serial.println("\nImpossible de se connecter au Wi-Fi. Vérifiez les identifiants.");
    }
  } else {
    Serial.println("Erreur lors de la récupération des identifiants Wi-Fi!");
    Serial.println("Veuillez d'abord exécuter le programme de stockage des identifiants.");
  }
}

// ------------------- BOUCLE PRINCIPALE (LOOP) ------------------------
void loop() {
  // Vérifier si on est connecté au Wi-Fi
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Wi-Fi déconnecté. Tentative de reconnexion...");
    WiFi.begin(wifi_ssid, wifi_pass);
    delay(5000);
    return;
  }
  
  // Si le client MQTT n'est pas connecté, on tente de se reconnecter
  if (!client.connected()) {
    reconnect();
  }
  
  client.loop(); // Gère l'écoute des messages MQTT entrants et la gestion de la communication
  
  // Lecture des valeurs de température et d'humidité du capteur DHT
  float humidity = dht.readHumidity();           // Lecture de l'humidité
  float temperature = dht.readTemperature();     // Lecture de la température en °C
  
  // Vérification si les données lues sont valides (non NaN)
  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("Erreur de lecture du capteur DHT!");
    return; // Sortie de la fonction si une erreur est détectée
  }
  
  // Envoi de la température au broker MQTT sur le topic "sensors/temperature"
  if (client.publish("sensors/temperature", String(temperature).c_str())) {
    Serial.print("Température envoyée : ");
    Serial.println(temperature);
  } else {
    Serial.println("Erreur lors de l'envoi de la température.");
  }
  
  // Envoi de l'humidité au broker MQTT sur le topic "sensors/humidity"
  if (client.publish("sensors/humidity", String(humidity).c_str())) {
    Serial.print("Humidité envoyée : ");
    Serial.println(humidity);
  } else {
    Serial.println("Erreur lors de l'envoi de l'humidité.");
  }
  
  // Attente de 10 secondes avant la prochaine lecture
  delay(10000);
}

// // Premier programme - Stockage des identifiants Wi-Fi et MQTT
// #include <Arduino.h>
// #include "SecureStorage.h"

// // Définition des identifiants Wi-Fi à stocker
// const char* wifi_ssid = "tp link oar";
// const char* wifi_pass = "cielnewton";

// // Définition des informations MQTT à stocker
// const char* mqtt_server = "10.0.20.2";
// const int mqtt_port = 8883;
// const char* mqtt_user = "userclient";
// const char* mqtt_pass = "ciel";

// // Instance de la classe SecureStorage
// SecureStorage storage;

// // Fonction pour afficher les informations stockées
// void displayStoredInformation() {
//   char ssid[64] = {0};// // Premier programme - Stockage des identifiants Wi-Fi et MQTT
// #include <Arduino.h>
// #include "SecureStorage.h"

// // Définition des identifiants Wi-Fi à stocker
// const char* wifi_ssid = "tp link oar";
// const char* wifi_pass = "cielnewton";

// // Définition des informations MQTT à stocker
// const char* mqtt_server = "10.0.20.2";
// const int mqtt_port = 8883;
// const char* mqtt_user = "userclient";
// const char* mqtt_pass = "ciel";

//   char pass[64] = {0};
//   char server[64] = {0};
//   int port = 0;
//   char user[64] = {0};
//   char mqtt_password[64] = {0};
  
//   Serial.println("\n=== Informations stockées ===");
  
//   // Récupération et affichage du SSID Wi-Fi
//   if (storage.retrieveSecret("wifi_ssid", ssid, sizeof(ssid))) {
//     Serial.print("SSID Wi-Fi: ");
//     Serial.println(ssid);
//   } else {
//     Serial.println("Erreur lors de la récupération du SSID Wi-Fi!");
//   }
  
//   // Récupération et affichage du mot de passe Wi-Fi
//   if (storage.retrieveSecret("wifi_pass", pass, sizeof(pass))) {
//     Serial.print("Mot de passe Wi-Fi: ");
//     Serial.println(pass);
//   } else {
//     Serial.println("Erreur lors de la récupération du mot de passe Wi-Fi!");
//   }
  
//   // Récupération et affichage du serveur MQTT
//   if (storage.retrieveSecret("mqtt_server", server, sizeof(server))) {
//     Serial.print("Serveur MQTT: ");
//     Serial.println(server);
//   } else {
//     Serial.println("Erreur lors de la récupération du serveur MQTT!");
//   }
  
//   // Récupération et affichage du port MQTT
//   if (storage.retrieveInt("mqtt_port", &port)) {
//     Serial.print("Port MQTT: ");
//     Serial.println(port);
//   } else {
//     Serial.println("Erreur lors de la récupération du port MQTT!");
//   }
  
//   // Récupération et affichage de l'utilisateur MQTT
//   if (storage.retrieveSecret("mqtt_user", user, sizeof(user))) {
//     Serial.print("Utilisateur MQTT: ");
//     Serial.println(user);
//   } else {
//     Serial.println("Erreur lors de la récupération de l'utilisateur MQTT!");
//   }
  
//   // Récupération et affichage du mot de passe MQTT
//   if (storage.retrieveSecret("mqtt_pass", mqtt_password, sizeof(mqtt_password))) {
//     Serial.print("Mot de passe MQTT: ");
//     Serial.println(mqtt_password);
//   } else {
//     Serial.println("Erreur lors de la récupération du mot de passe MQTT!");
//   }
// }

// void setup() {
//   // Initialisation de la communication série
//   Serial.begin(115200);
//   delay(1000);
  
//   Serial.println("=== Programme de stockage des identifiants Wi-Fi et MQTT ===");
//   Serial.println("Effacement des anciens secrets...");
  
//   // Effacer tous les secrets précédents (optionnel)
//   if (storage.clearAllSecrets()) {
//     Serial.println("Anciens secrets effacés avec succès!");
//   } else {
//     Serial.println("Erreur lors de l'effacement des secrets ou aucun secret à effacer.");
//   }
  
//   Serial.println("\nStockage des identifiants Wi-Fi dans la mémoire NVS sécurisée...");
  
//   // Stockage du SSID Wi-Fi
//   if (storage.storeSecret("wifi_ssid", wifi_ssid)) {
//     Serial.println("SSID Wi-Fi stocké avec succès!");
//   } else {
//     Serial.println("Erreur lors du stockage du SSID Wi-Fi!");
//   }
  
//   // Stockage du mot de passe Wi-Fi
//   if (storage.storeSecret("wifi_pass", wifi_pass)) {
//     Serial.println("Mot de passe Wi-Fi stocké avec succès!");
//   } else {
//     Serial.println("Erreur lors du stockage du mot de passe Wi-Fi!");
//   }
  
//   Serial.println("\nStockage des informations MQTT dans la mémoire NVS sécurisée...");
  
//   // Stockage du serveur MQTT
//   if (storage.storeSecret("mqtt_server", mqtt_server)) {
//     Serial.println("Serveur MQTT stocké avec succès!");
//   } else {
//     Serial.println("Erreur lors du stockage du serveur MQTT!");
//   }
  
//   // Stockage du port MQTT
//   if (storage.storeInt("mqtt_port", mqtt_port)) {
//     Serial.println("Port MQTT stocké avec succès!");
//   } else {
//     Serial.println("Erreur lors du stockage du port MQTT!");
//   }
  
//   // Stockage de l'utilisateur MQTT
//   if (storage.storeSecret("mqtt_user", mqtt_user)) {
//     Serial.println("Utilisateur MQTT stocké avec succès!");
//   } else {
//     Serial.println("Erreur lors du stockage de l'utilisateur MQTT!");
//   }
  
//   // Stockage du mot de passe MQTT
//   if (storage.storeSecret("mqtt_pass", mqtt_pass)) {
//     Serial.println("Mot de passe MQTT stocké avec succès!");
//   } else {
//     Serial.println("Erreur lors du stockage du mot de passe MQTT!");
//   }
  
//   Serial.println("\nVérification des secrets stockés...");
  
//   // Vérification que tous les secrets ont bien été stockés
//   if (storage.secretExists("wifi_ssid") && 
//       storage.secretExists("wifi_pass") &&
//       storage.secretExists("mqtt_server") &&
//       storage.secretExists("mqtt_port") &&
//       storage.secretExists("mqtt_user") &&
//       storage.secretExists("mqtt_pass")) {
    
//     Serial.println("Tous les identifiants ont été correctement stockés.");
    
//     // Affichage des informations stockées pour vérification
//     displayStoredInformation();
    
//     Serial.println("\nLe stockage des identifiants Wi-Fi et MQTT est terminé!");
//     Serial.println("Vous pouvez maintenant téléverser le programme principal.");
//   } else {
//     Serial.println("Erreur: Certains identifiants n'ont pas été correctement stockés!");
//     // Afficher quand même ce qui a été stocké pour diagnostiquer d'éventuels problèmes
//     displayStoredInformation();
//   }
// }

// void loop() {
//   // Rien à faire ici
//   delay(1000);
// }


// #include <Arduino.h>
// #include "nvs_flash.h"
// #include "nvs.h"

// void setup() {
//     Serial.begin(115200);
//     Serial.println("\nDémarrage de l'ESP32...");

//     // Initialisation de la mémoire flash NVS
//     esp_err_t ret = nvs_flash_init();
//     if (ret == ESP_OK) {
//         Serial.println("NVS initialisé avec succès.");
//     } else {
//         Serial.printf("Erreur lors de l'initialisation NVS: %s\n", esp_err_to_name(ret));
//     }

//     // Effacement complet de la partition NVS
//     Serial.println("Effacement de la partition NVS...");
//     ret = nvs_flash_erase();
//     if (ret == ESP_OK) {
//         Serial.println("Partition NVS effacée avec succès !");
//     } else {
//         Serial.printf("Erreur lors de l'effacement de la partition NVS: %s\n", esp_err_to_name(ret));
//     }
// }

// void loop() {
//     Serial.println("ESP32 en fonctionnement...");
//     delay(5000);  // Affiche un message toutes les 5 secondes
// }

// // ------------------- BIBLIOTHEQUES ------------------------
// #include <WiFi.h>              // Bibliothèque pour gérer la connexion Wi-Fi
// #include <PubSubClient.h>      // Bibliothèque pour gérer le protocole MQTT
// #include <DHT.h>               // Bibliothèque pour interagir avec les capteurs DHT
// #include <WiFiClientSecure.h>  // Bibliothèque pour gérer les connexions sécurisées (SSL/TLS)
// #include "secrets.h"           // Fichier contenant les identifiants Wi-Fi et MQTT

// // ------------------- PARAMETRAGES DU CAPTEUR DHT ------------------------
// #define DHTPIN 4               // Définit la broche GPIO 4 de l'ESP32 pour le capteur DHT22
// #define DHTTYPE DHT22          // Spécifie que le capteur utilisé est le DHT22 (température et humidité)
// DHT dht(DHTPIN, DHTTYPE);     // Crée une instance du capteur DHT22 sur la broche définie

// // ------------------- CERTIFICAT DE L'AUTORITÉ DE CERTIFICATION ------------------------
// const char* ca_cert = R"(
// -----BEGIN CERTIFICATE-----
// MIIDVzCCAj+gAwIBAgIUMmWs9hnIQUR2FDQmV4br5mZNr6swDQYJKoZIhvcNAQEL
// BQAwOzELMAkGA1UEBhMCRlIxDDAKBgNVBAoMA0lPVDEPMA0GA1UECwwGY2xpZW50
// MQ0wCwYDVQQDDAR0ZXN0MB4XDTI1MDMwNzA3MDkwMFoXDTMwMDMxNDA3MDkwMFow
// OzELMAkGA1UEBhMCRlIxDDAKBgNVBAoMA0lPVDEPMA0GA1UECwwGY2xpZW50MQ0w
// CwYDVQQDDAR0ZXN0MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAqeTc
// n+IZSgKQ5bTnUjqOdajd+y6JV0xy3Y1Em3j6pZirHJ6TSjorAbhgCbYSTWdExUwS
// 2DpAnobx3M8+4++1lFNTVKTu/Lw5Om4dFX/KjRT9BMBJX/gDY60AvsU/mD7YKfHh
// r9rJHUQTFmPvHsz6TAmz6cWJemHckmGDVI8cfgR2U/iwim/JxIc8pNvQg8mPQfnW
// LPfUWth+m09aUSIC1G/71G+adav+CaClk4kt4T73W8Qt8LeVeDNxwBwQ5E8yxk7G
// LqyPjLI0HBj918IQt9BEGCSbD/St43CgazXU9OXrts65ftoFnCwHMx67sYqaelu5
// tNlb0ZzxAQmByb/0WwIDAQABo1MwUTAdBgNVHQ4EFgQUCWpI+UCyQoGK9mNcn1Qs
// cms9zkUwHwYDVR0jBBgwFoAUCWpI+UCyQoGK9mNcn1Qscms9zkUwDwYDVR0TAQH/
// BAUwAwEB/zANBgkqhkiG9w0BAQsFAAOCAQEAENE95YbtPDaaTRJlgb/3ByIBRnTD
// sETwDfat3FqELfNJLmGP2nvQV1NyEaQexVC/WJuT7G4QeTuaVsY2iEAC7SF6BRPX
// 9c9k8OBSdQ6xxS2nA1CmGsjX7diOf9ZCjQBIXfuGiGNTA1PR03HHIZBzheD5C3dL
// duFbrKQUkK7gIailBhhRcbvDJnkJOJsf0kgKgV5a3gkkL/GgQVVNQT0RIQiMLcBw
// 3RxHYWDp589/Jlf7bmQgLLa1gKeDiFUWTJB0U7BgB6Pvco8uoV4Ldt2q2EMrH2Ix
// 9ZcuWxYzKUtw000wC51diAkIzXLK08ayUiuXPq4O3pq8rj0Ahzb5NMrs4g==
// -----END CERTIFICATE-----
// )";  // Certificat du CA utilisé pour sécuriser la connexion MQTT via TLS/SSL

// // ------------------- OBJETS POUR LA CONNEXION WIFI ET MQTT ------------------------
// WiFiClientSecure espClient;   // Objet pour gérer la connexion sécurisée (SSL/TLS) Wi-Fi
// PubSubClient client(espClient); // Objet pour gérer la coexnnexion MQTT via l'objet WiFiClientSecure

// // ------------------- FONCTION DE RECONNEXION MQTT ------------------------
// void reconnect() {
//   // Tant que le client MQTT n'est pas connecté
//   while (!client.connected()) {
//     Serial.print("Tentative de connexion MQTT...");
//     // Tente de se connecter avec les identifiants et mot de passe MQTT
//     if (client.connect("ESP32Client", mqtt_user, mqtt_pass)) {
//       // Si la connexion réussit
//       Serial.println("Connecté au broker MQTT");
//     } else {
//       // Si la connexion échoue
//       Serial.print("Échec, code erreur : ");
//       Serial.println(client.state());
//       Serial.println("Tentative dans 5 secondes...");
//       delay(5000); // Attente de 5 secondes avant une nouvelle tentative
//     }
//   }
// }

// // ------------------- FONCTION D'INITIALISATION (SETUP) ------------------------
// void setup() {
//   // Initialisation de la communication série pour le debug
//   Serial.begin(115200);      
//   dht.begin();               // Initialisation du capteur DHT22

//   // Connexion au réseau Wi-Fi
//   WiFi.begin(ssid, password); // Connexion avec le SSID et le mot de passe définis
//   Serial.println("Connexion au Wi-Fi...");
//   // Attente que la connexion Wi-Fi soit établie
//   while (WiFi.status() != WL_CONNECTED) {
//     delay(1000); // Attente d'une seconde avant de réessayer
//     Serial.println("Tentative de connexion...");
//   }
//   Serial.println("Connecté au Wi-Fi");  // Confirmation de la connexion Wi-Fi

//   // Charger le certificat de l'autorité de certification pour établir une connexion sécurisée avec le broker MQTT
//   espClient.setCACert(ca_cert);  

//   // Connexion au serveur MQTT sécurisé
//   client.setServer(mqtt_server, mqtt_port);  // Configuration du serveur et du port du broker MQTT
// }
// // ------------------- BOUCLE PRINCIPALE (LOOP) ------------------------
// void loop() {
//   // Si le client MQTT n'est pas connecté, on tente de se reconnecter
//   if (!client.connected()) {
//     reconnect();
//   }

//   client.loop(); // Gère l'écoute des messages MQTT entrants et la gestion de la communication

//   // Lecture des valeurs de température et d'humidité du capteur DHT
//   float humidity = dht.readHumidity();           // Lecture de l'humidité
//   float temperature = dht.readTemperature();     // Lecture de la température en °C

//   // Vérification si les données lues sont valides (non NaN)
//   if (isnan(humidity) || isnan(temperature)) {
//     Serial.println("Erreur de lecture du capteur DHT!");  // Affiche un message d'erreur si les données sont invalides
//     return; // Sortie de la fonction si une erreur est détectée
//   }

//   // Envoi de la température au broker MQTT sur le topic "sensors/temperature"
//   if (client.publish("sensors/temperature", String(temperature).c_str())) {
//     // Si l'envoi est réussi
//     Serial.print("Température envoyée : ");
//     Serial.println(temperature);
//   } else {
//     // Si l'envoi échoueser
//     Serial.println("Erreur lors de l'envoi de la température.");
//   }

//   // Envoi de l'humidité au broker MQTT sur le topic "sensors/humidity"
//   if (client.publish("sensors/humidity", String(humidity).c_str())) {
//     // Si l'envoi est réussi
//     Serial.print("Humidité envoyée : ");
//     Serial.println(humidity);
//   } else {
//     // Si l'envoi échoue
//     Serial.println("Erreur lors de l'envoi de l'humidité.");
//   }

//   // Attente de 10 secondes avant la prochaine lecture
//   delay(10000);
// }

// #include <WiFi.h>

// const char* ssid = "tp link oar";
// const char* password = "cielnewton";

// void setup() {
//   Serial.begin(115200);
//   WiFi.begin(ssid, password);
  
//   Serial.print("Connexion au WiFi");
//   while (WiFi.status() != WL_CONNECTED) {
//     delay(500);
//     Serial.print(".");
//   }
  
//   Serial.println("");
//   Serial.println("Connecté au WiFi");
//   Serial.println(WiFi.localIP());
// }

// void loop() {
// }
