# ProjectScaffold

`ProjectScaffold` est l’outil de démarrage de projet fourni par rbfx-blueprint. Il produit une arborescence CMake minimale mais exploitable, destinée à évoluer vers une production complète sans imposer un wrapper de scripting autour du moteur C++.

## Génération

Après compilation des outils, exécutez :

```bash
ProjectScaffold <2d|3d|networked> <répertoire-de-sortie> <nom-du-projet>
```

Par exemple :

```bash
ProjectScaffold 2d ./MyGame MyGame
ProjectScaffold 3d ./MyWorld MyWorld
ProjectScaffold networked ./ArenaServer ArenaServer
```

Chaque projet contient `CMakeLists.txt`, `project.json`, `README.md` et `Source/Main.cpp`. Le fichier de projet conserve le template choisi, les capacités initiales et la compatibilité déclarée avec **C++17, Blueprint et rbscript**. Les fichiers générés ne copient pas de binaire ni de dépendance cachée : le projet consomme un SDK rbfx explicitement fourni via `CMAKE_PREFIX_PATH`.

## Templates

| Template | Cible initiale | Systèmes activés dans la description | Point d’entrée |
|---|---|---|---|
| `2d` | Jeux 2D | Urho2D, Physics2D et rendu 2D | Scène 2D, caméra et gameplay |
| `3d` | Jeux 3D | Physics, Navigation, Animation et monde partitionné | Scène 3D et systèmes de production |
| `networked` | Multiplayer autoritatif | Network, Replica, Rollback et DedicatedServer | Rôle client/serveur et réplication |

Le générateur laisse volontairement les choix de gameplay dans le projet consommateur. Les modules de production restent injectables dans BlueprintRuntime et partageables avec rbscript par la réflexion rbfx, ce qui évite de créer une seconde hiérarchie d’objets incompatible.

## Build du projet généré

Depuis le projet généré :

```bash
cmake -S . -B build -G Ninja -DCMAKE_PREFIX_PATH=/chemin/vers/rbfx-sdk
cmake --build build
```

Pour une livraison reproductible, utilisez ensuite le profil adapté de `packaging/profiles/` et exécutez la validation de production du dépôt moteur. Les répertoires `build*` doivent rester hors du contrôle de version.

## Extension par plugin

Un projet généré peut ajouter une classe dérivée de `PluginApplication` ou de `MainPluginApplication`, enregistrer ses types avec la réflexion rbfx, puis partager les services avec BlueprintRuntime. Le SDK conserve ainsi le cycle de vie de plugin existant de rbfx au lieu de définir une ABI parallèle.
