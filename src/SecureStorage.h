// // ------------------- PARAMETRAGES DU WIFI ------------------------
// const char* ssid = "snir";  // Nom du réseau Wi-Fi auquel l'ESP32 doit se connecter
// const char* password = "12345678";  // Mot de passe du réseau Wi-Fi

// // ------------------- PARAMETRAGES DU BROKER MQTT ------------------------
// const char* mqtt_server = "10.0.20.2";   // Adresse IP du serveur MQTT
// const int mqtt_port = 8883;                 // Port sécurisé MQTT (8883 est utilisé pour TLS/SSL)
// const char* mqtt_user = "userclient";       // Identifiant pour la connexion au broker MQTT
// const char* mqtt_pass = "ciel";             // Mot de passe pour la connexion au broker MQTT


// SecureStorage.h - Module pour stocker de manière sécurisée des données dans la NVS
#ifndef SECURE_STORAGE_H
#define SECURE_STORAGE_H

#include <Preferences.h>
#include <mbedtls/aes.h>
#include <mbedtls/gcm.h>
#include <esp_wifi.h>
#include <esp_efuse.h>

class SecureStorage {
private:
    // Espace de noms pour les préférences NVS
    Preferences preferences;
    // Taille de la clé AES-GCM (256 bits = 32 octets)
    static const size_t KEY_SIZE = 32;
    // Taille du tag d'authentification
    static const size_t TAG_SIZE = 16;
    // Taille du nonce
    static const size_t NONCE_SIZE = 12;
    // Clé dérivée de l'adresse MAC
    uint8_t derivedKey[KEY_SIZE];
    
    // Méthode pour dériver une clé de chiffrement à partir de l'adresse MAC
    bool deriveKeyFromMAC() {
        uint8_t mac[6];
        // Obtenir l'adresse MAC de l'ESP32
        if (esp_wifi_get_mac(WIFI_IF_STA, mac) != ESP_OK) {
            return false;
        }
        
        // Utiliser l'adresse MAC comme seed pour dériver une clé plus longue
        uint8_t expandedSeed[64];
        memcpy(expandedSeed, mac, 6);
        
        // Compléter avec des motifs pour renforcer l'entropie
        for (int i = 6; i < 64; i++) {
            expandedSeed[i] = mac[i % 6] ^ (i * 17);
        }
        
        // Hash simple pour dériver la clé finale
        for (size_t i = 0; i < KEY_SIZE; i++) {
            uint8_t hash = 0;
            for (size_t j = 0; j < 64; j++) {
                hash ^= expandedSeed[(i + j) % 64];
                hash = (hash << 1) | (hash >> 7); // Rotation à gauche
            }
            derivedKey[i] = hash;
        }
        
        return true;
    }

    // Méthode pour chiffrer des données avec AES-GCM
    bool encryptData(const char* plaintext, size_t plaintext_len, 
                     uint8_t* ciphertext, size_t* ciphertext_len) {
        
        mbedtls_gcm_context gcm;
        mbedtls_gcm_init(&gcm);
        
        // Générer un nonce aléatoire
        uint8_t nonce[NONCE_SIZE];
        for (size_t i = 0; i < NONCE_SIZE; i++) {
            nonce[i] = random(256);
        }
        
        // Configurer le contexte GCM avec la clé
        if (mbedtls_gcm_setkey(&gcm, MBEDTLS_CIPHER_ID_AES, derivedKey, KEY_SIZE * 8) != 0) {
            mbedtls_gcm_free(&gcm);
            return false;
        }
        
        // Tag d'authentification
        uint8_t tag[TAG_SIZE];
        
        // Chiffrer les données
        if (mbedtls_gcm_crypt_and_tag(&gcm, MBEDTLS_GCM_ENCRYPT, plaintext_len,
                                     nonce, NONCE_SIZE, NULL, 0,
                                     (const unsigned char*)plaintext, 
                                     ciphertext + NONCE_SIZE, TAG_SIZE, tag) != 0) {
            mbedtls_gcm_free(&gcm);
            return false;
        }
        
        // Copier le nonce et le tag dans la sortie
        memcpy(ciphertext, nonce, NONCE_SIZE);
        memcpy(ciphertext + NONCE_SIZE + plaintext_len, tag, TAG_SIZE);
        
        // Mettre à jour la taille du buffer chiffré
        *ciphertext_len = NONCE_SIZE + plaintext_len + TAG_SIZE;
        
        mbedtls_gcm_free(&gcm);
        return true;
    }

    // Méthode pour déchiffrer des données avec AES-GCM
    bool decryptData(const uint8_t* ciphertext, size_t ciphertext_len,
                     char* plaintext, size_t* plaintext_len) {
                     
        if (ciphertext_len < NONCE_SIZE + TAG_SIZE) {
            return false;
        }
        
        mbedtls_gcm_context gcm;
        mbedtls_gcm_init(&gcm);
        
        // Extraire le nonce et le tag
        const uint8_t* nonce = ciphertext;
        const uint8_t* encrypted_data = ciphertext + NONCE_SIZE;
        const uint8_t* tag = ciphertext + ciphertext_len - TAG_SIZE;
        
        size_t encrypted_data_len = ciphertext_len - NONCE_SIZE - TAG_SIZE;
        
        // Vérifier que le buffer de sortie est suffisamment grand
        if (*plaintext_len < encrypted_data_len) {
            mbedtls_gcm_free(&gcm);
            return false;
        }
        
        // Configurer le contexte GCM avec la clé
        if (mbedtls_gcm_setkey(&gcm, MBEDTLS_CIPHER_ID_AES, derivedKey, KEY_SIZE * 8) != 0) {
            mbedtls_gcm_free(&gcm);
            return false;
        }
        
        // Déchiffrer et vérifier les données
        if (mbedtls_gcm_auth_decrypt(&gcm, encrypted_data_len,
                                    nonce, NONCE_SIZE, NULL, 0,
                                    tag, TAG_SIZE, encrypted_data, 
                                    (unsigned char*)plaintext) != 0) {
            mbedtls_gcm_free(&gcm);
            return false;
        }
        
        // Mettre à jour la taille des données en clair
        *plaintext_len = encrypted_data_len;
        
        // Ajouter un terminateur nul pour les chaînes de caractères
        if (*plaintext_len < *plaintext_len + 1) {
            plaintext[*plaintext_len] = '\0';
        }
        
        mbedtls_gcm_free(&gcm);
        return true;
    }

public:
    SecureStorage() {
        // Initialiser le générateur de nombres aléatoires
        randomSeed(esp_random());
        // Dériver la clé de chiffrement
        deriveKeyFromMAC();
    }
    
    // Méthode pour stocker un secret dans la NVS
    bool storeSecret(const char* key, const char* value) {
        if (!preferences.begin("securestore", false)) {
            return false;
        }
        
        size_t value_len = strlen(value);
        size_t ciphertext_len = NONCE_SIZE + value_len + TAG_SIZE;
        uint8_t* ciphertext = new uint8_t[ciphertext_len];
        
        bool success = encryptData(value, value_len, ciphertext, &ciphertext_len);
        
        if (success) {
            // Stocker les données chiffrées dans la NVS
            success = preferences.putBytes(key, ciphertext, ciphertext_len) > 0;
        }
        
        // Effacer proprement la mémoire sensible
        memset(ciphertext, 0, ciphertext_len);
        delete[] ciphertext;
        
        preferences.end();
        return success;
    }
    
    // Méthode pour stocker un entier dans la NVS (pour le port MQTT)
    bool storeInt(const char* key, int value) {
        if (!preferences.begin("securestore", false)) {
            return false;
        }
        
        // Convertir l'entier en chaîne pour le chiffrer
        char valueStr[16];
        sprintf(valueStr, "%d", value);
        
        size_t value_len = strlen(valueStr);
        size_t ciphertext_len = NONCE_SIZE + value_len + TAG_SIZE;
        uint8_t* ciphertext = new uint8_t[ciphertext_len];
        
        bool success = encryptData(valueStr, value_len, ciphertext, &ciphertext_len);
        
        if (success) {
            // Stocker les données chiffrées dans la NVS
            success = preferences.putBytes(key, ciphertext, ciphertext_len) > 0;
        }
        
        // Effacer proprement la mémoire sensible
        memset(ciphertext, 0, ciphertext_len);
        delete[] ciphertext;
        
        preferences.end();
        return success;
    }
    
    // Méthode pour récupérer un secret de la NVS
    bool retrieveSecret(const char* key, char* value, size_t value_size) {
        if (!preferences.begin("securestore", true)) {
            return false;
        }
        
        // Lire les données chiffrées de la NVS
        size_t ciphertext_len = preferences.getBytesLength(key);
        if (ciphertext_len == 0) {
            preferences.end();
            return false;
        }
        
        uint8_t* ciphertext = new uint8_t[ciphertext_len];
        if (preferences.getBytes(key, ciphertext, ciphertext_len) != ciphertext_len) {
            delete[] ciphertext;
            preferences.end();
            return false;
        }
        
        // Déchiffrer les données
        size_t plaintext_len = value_size - 1; // Pour laisser de la place au terminateur nul
        bool success = decryptData(ciphertext, ciphertext_len, value, &plaintext_len);
        
        // Garantir que la chaîne est terminée par un nul
        if (success && plaintext_len < value_size) {
            value[plaintext_len] = '\0';
        }
        
        // Effacer proprement la mémoire sensible
        memset(ciphertext, 0, ciphertext_len);
        delete[] ciphertext;
        
        preferences.end();
        return success;
    }
    
    // Méthode pour récupérer un entier de la NVS
    bool retrieveInt(const char* key, int* value) {
        if (!preferences.begin("securestore", true)) {
            return false;
        }
        
        // Lire les données chiffrées de la NVS
        size_t ciphertext_len = preferences.getBytesLength(key);
        if (ciphertext_len == 0) {
            preferences.end();
            return false;
        }
        
        uint8_t* ciphertext = new uint8_t[ciphertext_len];
        if (preferences.getBytes(key, ciphertext, ciphertext_len) != ciphertext_len) {
            delete[] ciphertext;
            preferences.end();
            return false;
        }
        
        // Buffer pour stocker la valeur déchiffrée
        char valueStr[16];
        size_t plaintext_len = sizeof(valueStr) - 1;
        
        // Déchiffrer les données
        bool success = decryptData(ciphertext, ciphertext_len, valueStr, &plaintext_len);
        
        // Garantir que la chaîne est terminée par un nul
        if (success && plaintext_len < sizeof(valueStr)) {
            valueStr[plaintext_len] = '\0';
            // Convertir la chaîne en entier
            *value = atoi(valueStr);
        }
        
        // Effacer proprement la mémoire sensible
        memset(ciphertext, 0, ciphertext_len);
        delete[] ciphertext;
        memset(valueStr, 0, sizeof(valueStr));
        
        preferences.end();
        return success;
    }
    
    // Méthode pour vérifier si un secret existe
    bool secretExists(const char* key) {
        if (!preferences.begin("securestore", true)) {
            return false;
        }
        
        bool exists = preferences.isKey(key);
        preferences.end();
        return exists;
    }
    
    // Méthode pour supprimer un secret
    bool deleteSecret(const char* key) {
        if (!preferences.begin("securestore", false)) {
            return false;
        }
        
        bool success = preferences.remove(key);
        preferences.end();
        return success;
    }
    
    // Méthode pour supprimer tous les secrets
    bool clearAllSecrets() {
        if (!preferences.begin("securestore", false)) {
            return false;
        }
        
        bool success = preferences.clear();
        preferences.end();
        return success;
    }
};

#endif // SECURE_STORAGE_H

