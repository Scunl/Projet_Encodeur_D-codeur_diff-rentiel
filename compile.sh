#!/bin/bash

# Arrêter le script si une commande échoue
set -e

echo "1. Construction de la bibliothèque..."
cd Codec
make

echo "2. Construction de l'application..."
cd ../App
make

echo "3. Terminé ! L'exécutable est prêt."
