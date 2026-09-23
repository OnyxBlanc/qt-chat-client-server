# Qt Chat - Salon multi-utilisateurs (C++ / Qt 6)

Application de chat en local (TCP) developpee en C++17 avec Qt 6, avec deux composants :

- **QtChatApp** : application graphique (Widgets) qui permet d'heberger un salon ou de rejoindre un salon existant.
- **QtChatRelay** : serveur relais en console, sans interface graphique, a heberger sur une machine dediee (VPS, NAS, PC qui reste allume).

Tous les participants connectes au meme salon (via QtChatApp en mode hote, ou via QtChatRelay) recoivent les messages de tout le monde en temps reel, et peuvent tous envoyer des messages.

## Sommaire

- [Fonctionnalites](#fonctionnalites)
- [Architecture du reseau](#architecture-du-reseau)
- [Compiler le projet](#compiler-le-projet)
- [Utiliser QtChatApp](#utiliser-qtchatapp)
- [Utiliser QtChatRelay](#utiliser-qtchatrelay)
- [Protocole reseau](#protocole-reseau)
- [Recuperer les executables precompiles](#recuperer-les-executables-precompiles)
- [Feuille de route](#feuille-de-route)

## Fonctionnalites

- Parametres modifiables depuis l'application : **ID**, **Pseudo**, **port d'ecoute**, et **hebergement automatique** au demarrage (sauvegardes avec `QSettings`, donc conserves entre deux lancements).
- Zone d'**historique** affichant chaque message au format `ID - Pseudo : message`, y compris tes propres messages envoyes.
- Zone d'**ecriture** avec bouton d'envoi (ou touche Entree).
- Boutons pour **copier ton IP locale** et **ton port** en un clic, afin de les partager facilement a un autre utilisateur du meme reseau.
- Liste des **participants du salon** avec leur ID et pseudo, mise a jour en direct a chaque connexion/deconnexion.
- Bouton **＋ Rejoindre un autre salon** pour se connecter a un hote ou a un `QtChatRelay` distant (avec confirmation si tu heberges deja un salon, pour eviter de fermer ta propre session par erreur).
- Zone de jeux visible mais volontairement desactivee : fonctionnalite prevue pour plus tard (invitation de partie envoyee au client distant).

## Architecture du reseau

Deux facons d'utiliser l'application :

1. **Un des postes heberge lui-meme** : quelqu'un lance `QtChatApp`, active l'hebergement (automatique ou via Parametres), et donne son IP + son port aux autres. Les autres cliquent sur **＋ Rejoindre un autre salon** avec ces informations. L'hote relaie les messages a tout le monde.
2. **Un serveur dedie relaie les messages** : `QtChatRelay` tourne sans interface sur une machine qui reste allumee (VPS, NAS...). Tout le monde, y compris l'hote precedent, rejoint simplement ce relais avec **＋ Rejoindre un autre salon**. Personne n'a besoin de garder son PC ouvert pour que les autres restent connectes.

Dans les deux cas, chaque message envoye par un participant est retransmis a **tous les autres participants connectes**, et s'affiche aussi localement chez celui qui l'a envoye.

## Compiler le projet

Prerequis : Qt 6 (modules **Core**, **Network**, et **Widgets** pour QtChatApp uniquement), CMake 3.16+, compilateur C++17 (MSVC, GCC ou Clang).

```bash
cmake -S . -B build
cmake --build build --config Release
```

Cela produit deux executables : `QtChatApp` et `QtChatRelay`.

Sous Linux avec Ninja :

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/QtChatApp
./build/QtChatRelay --port 5000
```

## Utiliser QtChatApp

1. Lance l'application. Par defaut, elle **heberge automatiquement** un salon sur le port `5000`.
2. Clique sur l'icone **⚙ Parametres** pour modifier ton **ID**, ton **Pseudo**, le **port d'ecoute**, ou desactiver l'hebergement automatique.
3. Pour rejoindre un salon existant (hote ou `QtChatRelay`) : clique sur **＋ Rejoindre un autre salon**, entre l'IP et le port, puis valide. Si tu heberges deja, une confirmation t'est demandee car cela va fermer ton propre salon.
4. Utilise les boutons **Copier IP** et **Copier port** dans la zone **CONNEXION LOCALE** pour partager tes coordonnees reseau a un autre utilisateur du meme reseau local.
5. Ecris un message dans la zone du bas et clique sur **Envoyer ➜** (ou appuie sur Entree). Le message apparait chez toi et chez tous les autres participants connectes.
6. La liste **PARTICIPANTS DU SALON** a droite affiche qui est connecte, avec son ID et son pseudo.

## Utiliser QtChatRelay

`QtChatRelay` est un executable en ligne de commande, sans interface graphique. Il sert uniquement a relayer les messages entre plusieurs clients `QtChatApp`, sans qu'aucun d'eux n'ait besoin d'heberger lui-meme.

```bash
./QtChatRelay --port 5000
```

Options disponibles :

| Option | Description | Valeur par defaut |
|---|---|---|
| `-p`, `--port` | Port TCP sur lequel le relais ecoute | `5000` |
| `-h`, `--help` | Affiche l'aide | - |

Le relais affiche dans la console chaque connexion, deconnexion et message recu. Il tourne indefiniment jusqu'a interruption (Ctrl+C).

Pour l'utiliser : lance `QtChatRelay` sur une machine accessible par tout le monde (VPS, NAS, PC toujours allume), puis chaque participant rejoint cette IP et ce port depuis `QtChatApp` via **＋ Rejoindre un autre salon**.

## Protocole reseau

Les deux composants echangent des messages **JSON delimites par des retours a la ligne** (`\n`), au format :

```json
{"id": "PC-011", "pseudo": "Onyxo", "message": "bonjour"}
```

Chaque ligne recue est retransmise a tous les autres clients connectes (sauf a l'expediteur, qui affiche deja son propre message localement).

## Recuperer les executables precompiles

Le workflow GitHub Actions (`.github/workflows/build.yml`) compile automatiquement `QtChatApp` et `QtChatRelay` a chaque push sur `main` :

1. Va dans l'onglet **Actions** du depot.
2. Ouvre le run le plus recent lie au dernier commit.
3. Une fois les jobs **Windows executable** et **Linux executable** verts, telecharge dans la section **Artifacts** :
   - `QtChatApp-Windows-x64` : contient `QtChatApp.exe`, `QtChatRelay.exe` et les DLL Qt necessaires.
   - `QtChatApp-Linux-x64` : contient les binaires `QtChatApp` et `QtChatRelay`.

Sous Windows, dezippe l'archive entierement avant de lancer les executables (les DLL doivent rester a cote).

## Feuille de route

- [ ] Zone de jeux : envoi d'une invitation de partie au client distant
- [ ] Persistance de la liste des salons rejoints precedemment
- [ ] Chiffrement des echanges (au-dela du reseau local)
- [ ] Historique multi-onglets si connexion a plusieurs salons
