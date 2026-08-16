# rbfx Blueprint

## Vue d’ensemble

Cette branche ajoute à rbfx un premier socle de **visual scripting de type Blueprint** pour les projets 2D et 3D. Le système est séparé en deux parties : un modèle de graphe sérialisable et un runtime indépendant de l’interface de l’éditeur. Cette séparation permet d’exécuter un Blueprint dans le jeu, de le tester sans interface graphique et d’ajouter de nouveaux nœuds C++ sans modifier le canvas.

Le moteur rbfx conserve ses capacités natives de rendu 2D/3D, scènes, composants, physique, audio, ressources et éditeur. Le module Blueprint ajoute une ressource JSON lisible, un registre de nœuds, une validation des connexions, un exécuteur déterministe et un onglet de graphe dans l’éditeur.

## Architecture

| Élément | Rôle |
|---|---|
| `BlueprintDefs.h/.cpp` | Identifiants, types de pins, types de données, nœuds, liens, variables, commentaires, fonctions et diagnostics. |
| `BlueprintGraph.h/.cpp` | Conteneur du graphe, CRUD, validation, sérialisation JSON, recherche, placement automatique, commentaires et fonctions/sous-graphes. |
| `BlueprintRuntime.h/.cpp` | Registre extensible, exécution déterministe, variables, appels de fonctions sérialisées, nœuds 2D/3D et débogage pas-à-pas. |
| `BlueprintReflection.h/.cpp` | Génération automatique de nœuds Get/Set à partir des métadonnées `ObjectReflection` rbfx. |
| `BlueprintTab.h/.cpp` | Canvas ImGui intégré : zoom, déplacement, câbles Bézier, palette contextuelle filtrée, commentaires, minimap, auto-layout, sauvegarde et débogage. |
| `TestBlueprint.cpp` | Tests Catch2 du graphe, JSON, recherche, layout, fonctions, runtime, réflexion et debugger. |

## Nœuds intégrés

Le registre fournit les nœuds de flux `Event.OnStart`, `Flow.Branch`, `Flow.Print`, les nœuds mathématiques `Math.AddFloat`, `Math.MultiplyFloat`, `Math.LessFloat`, les variables `Variable.Get` et `Variable.Set`, ainsi que `Function.Entry`, `Function.Return` et `Function.Call`. Les primitives de scène couvrent les positions, translations, rotations et échelles 2D/3D : `Scene.GetPosition2D/3D`, `SetPosition2D/3D`, `Translate2D/3D`, `GetRotation2D/3D`, `SetRotation2D/3D`, `GetScale2D/3D` et `SetScale2D/3D`. Le registre est ouvert : un module ou un jeu peut enregistrer ses propres nœuds avec un nom stable, une catégorie, une description, un mode d’exécution, des pins et une fonction C++.

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

La validation réalisée sur cette branche a donné **192 tests réussis sur 192** dans la première passe, puis les nouveaux scénarios avancés ont été recompilés et exécutés avec succès : recherche, commentaires, fonctions, auto-layout, sous-graphes sérialisés, débogage et mapping de réflexion. La compilation de l’éditeur produit également `build-editor/bin/Debug/Editor`.

## Utilisation dans l’éditeur

L’onglet **Blueprint** est enregistré automatiquement par `EditorApplication`. Il permet de créer un graphe de démonstration, de déplacer les nœuds, de zoomer et déplacer la vue, de rechercher les nœuds par type/titre/catégorie, d’afficher des commentaires et une minimap, d’appliquer un placement automatique, de valider le graphe, de l’enregistrer dans `Blueprints/Main.blueprint` et de l’exécuter avec le runtime intégré. La barre de débogage expose les commandes **Start**, **Step** et **Stop** et met en évidence le nœud courant.

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

Le socle fonctionnel est maintenant compilable et intégré, mais une version de production devra encore compléter l’écosystème de nœuds générés : composants rbfx spécialisés, sprites, corps physiques, caméras, lumières, matériaux, sons, entrées et événements. Les prochaines extensions naturelles sont les tableaux, timelines, nœuds latents, interfaces de fonctions, ports typés générés dans la palette, watch window et points d’arrêt persistants.

Le canvas peut encore évoluer vers une parité plus large avec Unreal Engine 5 : connexion interactive par glisser-déposer, menu contextuel au clic droit, suppression et copier-coller de nœuds, undo/redo via `UndoManager`, sélection multiple et reroutage de câbles.

> Cette version constitue une **extension C++ native compilable, testée et intégrée à rbfx**, avec un runtime indépendant de l’interface et une base solide pour poursuivre les fonctionnalités de production sans fragiliser le cœur 2D/3D.
