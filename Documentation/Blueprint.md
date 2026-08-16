# rbfx Blueprint

## Vue d’ensemble

Cette branche ajoute à rbfx un premier socle de **visual scripting de type Blueprint** pour les projets 2D et 3D. Le système est séparé en deux parties : un modèle de graphe sérialisable et un runtime indépendant de l’interface de l’éditeur. Cette séparation permet d’exécuter un Blueprint dans le jeu, de le tester sans interface graphique et d’ajouter de nouveaux nœuds C++ sans modifier le canvas.

Le moteur rbfx conserve ses capacités natives de rendu 2D/3D, scènes, composants, physique, audio, ressources et éditeur. Le module Blueprint ajoute une ressource `.blueprint` native, un registre de nœuds, une validation des connexions, un exécuteur déterministe et un onglet de graphe dans l’éditeur.

## Architecture

| Élément | Rôle |
|---|---|
| `BlueprintDefs.h/.cpp` | Identifiants, types de pins, types de données, nœuds, liens, variables, commentaires, fonctions et diagnostics. |
| `BlueprintGraph.h/.cpp` | Conteneur du graphe, CRUD, validation, sérialisation JSON, recherche, placement automatique, commentaires et fonctions/sous-graphes. |
| `BlueprintRuntime.h/.cpp` | Registre extensible, exécution déterministe, variables, fonctions paramétrées, événements d’entrée, Delay/Tick, nœuds 2D/3D et débogage avancé. |
| `BlueprintReflection.h/.cpp` | Génération automatique de nœuds Get/Set à partir des métadonnées `ObjectReflection` rbfx. |
| `BlueprintTab.h/.cpp` | Canvas ImGui intégré : zoom, déplacement, sélection multiple, câbles interactifs, palette contextuelle, copier-coller, duplication, suppression, snapshots undo/redo, commentaires, minimap, auto-layout, Watch Window et debug. |
| `BlueprintResource.h/.cpp` | Ressource rbfx chargeable et sauvegardable via `ResourceCache`, avec enregistrement ObjectFactory. |
| `TestBlueprint.cpp` | Tests Catch2 du graphe, JSON, migration de schéma, édition logique, fonctions paramétrées, runtime latent, ressource, réflexion, registre et debugger. |

## Nœuds intégrés

Le registre fournit les nœuds de flux `Event.OnStart`, `Event.OnKeyPressed`, `Event.OnMouseClick`, `Flow.Branch`, `Flow.Print` et `Flow.Delay`, les nœuds mathématiques `Math.AddFloat`, `Math.MultiplyFloat`, `Math.LessFloat`, `Math.SubtractFloat`, `Math.DivideFloat`, `Math.ClampFloat`, `Math.LerpFloat`, `Math.SinFloat`, `Math.CosFloat`, `Math.AddVector3` et `Math.ScaleVector3`, les variables `Variable.Get` et `Variable.Set`, ainsi que `Function.Entry`, `Function.Return` et `Function.Call`. Les primitives de scène couvrent les positions, translations, rotations et échelles 2D/3D : `Scene.GetPosition2D/3D`, `SetPosition2D/3D`, `Translate2D/3D`, `GetRotation2D/3D`, `SetRotation2D/3D`, `GetScale2D/3D` et `SetScale2D/3D`. Le registre est ouvert : un module ou un jeu peut enregistrer ses propres nœuds avec un nom stable, une catégorie, une description, un mode d’exécution, des pins et une fonction C++.

Les pins d’exécution sont distincts des pins de données. Les pins de données prennent en charge les types Blueprint de base et les valeurs par défaut `Variant`. La validation détecte notamment les nœuds invalides, les pins en double, les liens manquants, les types incompatibles et les entrées multiples non autorisées.

## Compilation Linux

La configuration suivante construit l’éditeur avec le panneau Blueprint :

```bash
cmake -S . -B build-editor -G Ninja \
  -DURHO3D_EDITOR=ON \
  -DURHO3D_PLAYER=OFF \
  -DURHO3D_TESTING=OFF \
  -DURHO3D_CSHARP=OFF
cmake --build build-editor --target Editor -j2
```

La compilation testée produit `build-editor/bin/Debug/Editor`. Les dépendances de développement Linux nécessaires à cette validation incluaient un compilateur C++ et `libdbus-1-dev`.

## Tests

Pour compiler et exécuter la suite de tests :

```bash
cmake -S . -B build -G Ninja \
  -DURHO3D_TESTING=ON \
  -DURHO3D_EDITOR=OFF \
  -DURHO3D_PLAYER=OFF \
  -DURHO3D_CSHARP=OFF
cmake --build build --target Tests -j2
ctest --test-dir build --output-on-failure
```

La validation réalisée sur cette branche a donné **223 tests réussis sur 223** dans la suite complète. Les tests Blueprint et rbscript couvrent le graphe, les commentaires, les fonctions paramétrées, les sous-graphes sérialisés, le runtime latent, les événements d’entrée, la migration de schéma, les ressources natives, la réflexion, le débogage, le lexer, le parseur, le système de types, le compilateur, le VM, les bindings et l’interopérabilité. La compilation de l’éditeur produit également `build-editor/bin/Debug/Editor` sous Linux.

## Compatibilité des plateformes

Le code Blueprint est écrit en C++17 et utilise les APIs portables de rbfx, EASTL, JSONValue, Variant, ObjectReflection et Dear ImGui. Il ne contient pas de dépendance spécifique à Linux, Windows ou macOS dans le module Blueprint lui-même. La compatibilité de compilation est donc conçue pour les trois plateformes, sous réserve que les dépendances et les backends graphiques rbfx soient correctement configurés.

La validation automatisée complète réalisée dans l’environnement disponible a été effectuée sous **Linux x86_64** : compilation de `Editor` et **199 tests réussis sur 199**. Windows et macOS n’ont pas été compilés dans cet environnement ; ils sont considérés comme des cibles portables à valider avec Visual Studio/MSVC pour Windows et Xcode/Clang pour macOS avant de les déclarer officiellement vérifiés.

Les zones à contrôler lors d’une validation Windows/macOS sont la détection CMake des dépendances, les backends OpenGL/Vulkan/Metal ou DirectX sélectionnés par rbfx, les chemins de fichiers, les warnings de compilateur traités comme erreurs et l’intégration du Resource Browser.

## Utilisation dans l’éditeur

L’onglet **Blueprint** est enregistré automatiquement par `EditorApplication`. Il permet de créer un graphe, de sélectionner plusieurs nœuds, de les déplacer en groupe, de créer des liens par glisser-déposer entre pins compatibles, d’ouvrir une palette contextuelle au clic droit, de rechercher les nœuds par type/titre/catégorie, de dupliquer, copier, coller et supprimer, d’afficher des commentaires et une minimap, d’appliquer un placement automatique, de valider le graphe et de sauvegarder une ressource native dans `Blueprints/Main.blueprint`. Les raccourcis d’édition sont reliés aux snapshots undo/redo de l’éditeur. La barre de débogage expose **Start**, **Step**, **Continue** et **Stop**, permet d’activer les breakpoints et affiche la Watch Window ainsi que la pile d’appels.

Les fonctions Blueprint sont stockées comme des sous-graphes JSON dans `BlueprintFunction::body`. Un nœud `Function.Call` les charge, valide leur `Function.Entry`, exécute leur flux et partage l’état des variables avec le graphe appelant. La pile d’appels est limitée et les appels récursifs sont diagnostiqués pour éviter les boucles non bornées.

## Ajouter un nœud C++

Un nœud runtime peut être enregistré ainsi :

```cpp
runtime.GetRegistry().Register({
    "Gameplay.SetVelocity",
    "Gameplay",
    "Set an object's velocity",
    BlueprintExecutionMode::Immediate,
    [](BlueprintExecutionContext& context)
    {
        const Variant velocity = context.GetInput("velocity");
        // Appeler ici le composant ou le service du jeu.
        context.ContinueWith("then");
    }
});
```

Le type doit être stable car il est stocké dans le fichier `.blueprint`. Pour un jeu réel, les nœuds doivent vérifier explicitement les types de pins, gérer les références invalides et produire des diagnostics utiles plutôt que d’échouer silencieusement.

## Feuille de route

Le cœur de l’expérience Blueprint est maintenant disponible et compilable. Les extensions restantes concernent surtout la profondeur fonctionnelle : nœuds spécialisés pour sprites, corps physiques, caméras, lumières, matériaux, sons et composants rbfx ; tableaux et dictionnaires ; timelines ; tâches asynchrones ; interfaces ; macros ; reroutage manuel des câbles ; et création/édition visuelle des signatures de fonctions directement dans l’éditeur.

La documentation complémentaire du langage textuel, du bytecode, du VM, des bindings et de l’éditeur rbscript se trouve dans [`Documentation/RbScript.md`](RbScript.md). La prochaine étape de production sera d’ajouter une factory dédiée au Resource Browser pour créer et renommer les fichiers `.blueprint` depuis l’interface, ainsi que la liaison automatique du Blueprint à l’objet ou au Node sélectionné dans la scène. Le format est déjà versionné en **schéma 2** et accepte les fichiers historiques de format 1 ; les migrations futures pourront être ajoutées dans `BlueprintGraph::FromJSON`.

> Cette version constitue une **extension C++ native compilable, testée et intégrée à rbfx**, avec un runtime indépendant de l’interface et une base solide pour poursuivre les fonctionnalités de production sans fragiliser le cœur 2D/3D.
