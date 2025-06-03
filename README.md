# Système de régulation thermique – Partie de Malamine Dibaga

## Objectif

La régulation thermique de la baie serveur a pour but de :
- Surveiller la température ambiante de la salle serveur.
- Contrôler automatiquement le climatiseur pour maintenir des conditions optimales.
- Mesurer la consommation électrique du climatiseur.

---

## Composants matériels

- **ESP32** avec capteur de température et d’humidité.
- **Passerelle infrarouge-WiFi** pour la commande du climatiseur.
- **Climatiseur** équipé d’un récepteur infrarouge.
- **Prise WiFi Shelly** pour mesurer la consommation électrique.
- **Serveur Dell** pour l’hébergement des applications conteneurisées.

---

## Fonctionnalités principales

✅ Lecture périodique de la température ambiante (via ESP32).  
✅ Commande du climatiseur en envoyant les signaux infrarouges adaptés.  
✅ Surveillance des seuils de température et envoi d’alertes en cas de dépassement.  
✅ Mesure de la consommation électrique du climatiseur.  
✅ Intégration avec un broker MQTT pour l’échange des données.  

---

## Technologies utilisées

- **PlatformIO (ESP32)** : Développement du firmware en C++ (pas Arduino IDE), pour la mesure et la transmission des données du capteur.
- **Python** : Application de régulation thermique pour le contrôle du climatiseur.
- **MQTT** : Protocole de communication entre les composants (ESP32, serveur).
- **Docker** : Conteneurisation des services (broker MQTT et application de régulation) pour un déploiement efficace sur le serveur Dell.

---

## Sécurité et confidentialité

- Les **secrets (identifiants, mots de passe)** stockés dans le firmware de l’ESP32 sont **chiffrés** afin de les protéger contre le reverse engineering.
- L’accès au système est restreint par authentification.
- Les échanges MQTT sont chiffrés à l’aide de certificats auto-signés.
- Les conteneurs Docker disposent d’un accès à Internet (via un routeur configuré pour gérer le routage, même avec des adresses IP internes distinctes).

---

## Architecture et déploiement

- L’ESP32 lit la température et publie les données sur le broker MQTT.
- Une application Python abonnée au broker MQTT reçoit ces données et contrôle le climatiseur.
- Les conteneurs (broker MQTT et application Python) sont hébergés sur le serveur Dell.
- Le climatiseur est commandé via la passerelle IR/WiFi.

---

## Diagrammes

Des **diagrammes SysML de séquence** détaillent :
- Les échanges entre l’ESP32 et le broker MQTT.
- Les interactions entre le broker MQTT et l’application Python.
- La communication de l’application Python avec la passerelle IR/WiFi pour le climatiseur.

---

## Déploiement

Le système est déployé sur un serveur Dell via Docker :
- Conteneur **broker MQTT**.
- Conteneur **application Python** de régulation thermique.

---

## Matériels utilisés

| Composant                  | Rôle                                       |
|----------------------------|--------------------------------------------|
| ESP32 + capteur temp/hum   | Mesure de la température / humidité        |
| Passerelle IR/WiFi         | Transmission des commandes au climatiseur  |
| Climatiseur                | Contrôle thermique de la salle serveur     |
| Prise WiFi Shelly          | Mesure de la consommation du climatiseur   |
| Serveur Dell (Docker)      | Hébergement des conteneurs MQTT/Python     |

---

## Conclusion

Ce système de régulation thermique offre une surveillance proactive de la température de la salle serveur et un contrôle automatique du climatiseur, tout en assurant la sécurité des données et l’intégrité des échanges. Grâce à PlatformIO et à l’architecture conteneurisée, il garantit une compatibilité optimale, une protection des secrets contre les attaques de reverse engineering et une accessibilité réseau maîtrisée (routage via un routeur dédié, même avec des IP différentes).

Pour toute question ou contribution, merci de contacter **Malamine Dibaga**.
