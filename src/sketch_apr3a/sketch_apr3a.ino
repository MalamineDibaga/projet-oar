// Premier programme - Stockage des identifiants Wi-Fi
#include <Arduino.h>
#include "SecureStorage.h"

// Définition des identifiants Wi-Fi à stocker
const char* wifi_ssid = "tp link oar";
const char* wifi_pass = "cielnewton";

// Instance de la classe SecureStorage
SecureStorage storage;

void setup() {
  // Initialisation de la communication série
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("=== Programme de stockage des identifiants Wi-Fi ===");
  Serial.println("Effacement des anciens secrets...");
  
  // Effacer tous les secrets précédents (optionnel)
  if (storage.clearAllSecrets()) {
    Serial.println("Anciens secrets effacés avec succès!");
  } else {
    Serial.println("Erreur lors de l'effacement des secrets ou aucun secret à effacer.");
  }
  
  Serial.println("Stockage des identifiants Wi-Fi dans la mémoire NVS sécurisée...");
  
  // Stockage du SSID Wi-Fi
  if (storage.storeSecret("wifi_ssid", wifi_ssid)) {
    Serial.println("SSID Wi-Fi stocké avec succès!");
  } else {
    Serial.println("Erreur lors du stockage du SSID Wi-Fi!");
  }
  
  // Stockage du mot de passe Wi-Fi
  if (storage.storeSecret("wifi_pass", wifi_pass)) {
    Serial.println("Mot de passe Wi-Fi stocké avec succès!");
  } else {
    Serial.println("Erreur lors du stockage du mot de passe Wi-Fi!");
  }
  
  Serial.println("Vérification des secrets stockés...");
  
  // Vérification que les secrets ont bien été stockés
  if (storage.secretExists("wifi_ssid") && storage.secretExists("wifi_pass")) {
    Serial.println("Les identifiants Wi-Fi ont été correctement stockés.");
    
    // Affichage pour vérification
    char ssid[64] = {0};
    char pass[64] = {0};
    
    if (storage.retrieveSecret("wifi_ssid", ssid, sizeof(ssid))) {
      Serial.print("SSID Wi-Fi récupéré: ");
      Serial.println(ssid);
    }
    
    if (storage.retrieveSecret("wifi_pass", pass, sizeof(pass))) {
      Serial.print("Mot de passe Wi-Fi récupéré: ");
      Serial.println(pass);
    }
    
    Serial.println("\nLe stockage des identifiants Wi-Fi est terminé!");
    Serial.println("Vous pouvez maintenant téléverser le programme principal.");
  } else {
    Serial.println("Erreur: Les identifiants Wi-Fi n'ont pas été correctement stockés!");
  }
}

void loop() {
  // Rien à faire ici
  delay(1000);
}

