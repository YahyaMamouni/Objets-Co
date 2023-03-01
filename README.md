# Objets-Co
Travaux pratiques de la matière Objets connectés
------------------------------------------------

## Explication du fonctionnement

La carte enverra sur le réseau Helium un message crypté avec le protocole AES-CTR toutes les 30 secondes, en utilisant le code dans le fichier .ino ensuite les données sont envoyées vers un serveur MQTT que nous a ajouté.
Finalement, on s'abonne au serveur avec le script Python. C'est avec ce script qu'on va déchiffrer le message et l'afficher.

## Installation

Les biblbiothèques nécessaires.
```bash
pip install pycryptodome
pip install paho-mqtt
```
## Lancement

Pour lancer le script, exécutez la commande :
```python
 python3 mqttSubscriber.py
```

## Remarque

On a essayé d'ajouter le compteur à la partie de chiffrement, mais on a rencontré des problèmes.
