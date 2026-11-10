# Karateka — portage Gamebuino AKA

Sauvez la princesse Sania de l'Empereur Akuma : combattez au corps à corps sur votre chemin vers le château.

## Jeu d'origine

- **Jeu :** Karateka
- **Auteur :** Press Play On Tape (jeu Pokitto), d'après le classique *Karateka* de Jordan Mechner
- **Plateforme d'origine :** Pokitto
- **Source :** <https://github.com/Press-Play-On-Tape/Karateka_Pokitto>
- **Licence :** BSD 3-Clause (voir [LICENSE](LICENSE), qui reprend le texte de la licence d'origine)

Tout le mérite du jeu (conception, graphismes, musique, code) revient à son auteur. Ce dépôt n'est qu'un portage.

## Ce portage

Portage sur la console **Gamebuino AKA** (ESP32-S3, écran 320×240) réalisé par Jicehel avec l'outil AKA Port Studio : firmware ESP-IDF 5.5.1 (`idf.py set-target esp32s3 && idf.py build`). Les fichiers du dossier `sdcard_files/` se copient à la racine de la carte SD de la console.

## Licence

Ce portage est distribué sous la même licence que le jeu d'origine : **BSD 3-Clause**. Le texte complet, avec le copyright d'origine, est dans le fichier [LICENSE](LICENSE).
