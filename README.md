# Qt Chat Client / Serveur

Base de chat TCP écrite en C++17 avec Qt 6 Widgets et Qt Network.

## Fonctions déjà présentes

- Paramètres : ID, pseudo, couleurs hexadécimales et mode clair/foncé
- Couleurs par défaut : `#424C8F` (foncé) et `#A1ADED` (clair)
- Historique au format `ID - Pseudo : message`
- Zone d'écriture et bouton d'envoi
- Copie de l'IP locale et du port
- Hébergement TCP local ou connexion à un serveur distant
- Liste de serveurs, ajout, retrait et mode automatique
- Zone de jeux visible mais volontairement désactivée : fonctionnalité à développer plus tard

## Compiler localement

Prérequis : Qt 6 (Widgets + Network), CMake 3.16+, compilateur C++17.

```bash
cmake -S . -B build
cmake --build build --config Release
```

Sous Linux avec Ninja :

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/QtChatApp
```

## GitHub Actions

Le workflow `.github/workflows/build.yml` compile automatiquement :

- un package Windows x64 avec `QtChatApp.exe` et ses DLL Qt
- un binaire Linux x64

Télécharge-les depuis l'onglet **Actions**, dans la section **Artifacts** du run réussi.
