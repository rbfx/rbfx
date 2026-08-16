# rbfx Blueprint

## Vue d’ensemble

Cette branche ajoute à rbfx un premier socle de **visual scripting de type Blueprint** pour les projets 2D et 3D. Le système est séparé en deux parties : un modèle de graphe sérialisable et un runtime indépendant de l’interface de l’éditeur. Cette séparation permet d’exécuter un Blueprint dans le jeu, de le tester sans interface graphique et d’ajouter de nouveaux nœuds C++ sans modifier le canvas.

Le moteur rbfx conserve ses capacités natives de rendu 2D/3D, scènes, composants, physique, audio, ressources et éditeur. Le module Blueprint ajoute une ressource JSON lisible, un registre de nœuds, une validation des connexions, un exécuteur déterministe et un onglet de graphe dans l’éditeur.

## Architecture

| Élément | Rôle |
|---|---|
| `BlueprintDefs.h/.cpp` | Identifiants, types de pins, types de données, nœuds, liens, variables et diagnostics. |
| `BlueprintGraph.h/.cpp` | Conteneur du graphe, création/suppression, validation, sérialisation JSON et chargement. |
| `BlueprintRuntime.h/.cpp` | Registre extensible de nœuds et exécution des flux, valeurs pures et variables. |
| `BlueprintTab.h/.cpp` | Canvas ImGui intégré à l’éditeur : zoom, déplacement, ajout de nœuds, câbles Bézier, validation, sauvegarde et exécution. |
| `TestBlueprint.cpp` | Tests Catch2 du modèle de graphe, de la sérialisation et du runtime. |

## Nœuds intégrés

La première version fournit les nœuds suivants : `Event.OnStart`, `Flow.Branch`, `Flow.Print`, `Math.AddFloat`, `Math.MultiplyFloat`, `Math.LessFloat`, `Variable.Get` et `Variable.Set`. Le registre est volontairement ouvert : un module ou un jeu peut enregistrer ses propres nœuds avec un nom stable, une catégorie, une description, un mode d’exécution et une fonction C++.

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

La validation réalisée sur cette branche a donné **189 tests réussis sur 189**, dont les quatre tests Blueprint : validation des liens, aller-retour JSON, calcul mathématique et exécution d’un événement vers un nœud `Print`.

## Utilisation dans l’éditeur

L’onglet **Blueprint** est enregistré automatiquement par `EditorApplication`. Il permet de créer un graphe de démonstration, de déplacer les nœuds, de zoomer et déplacer la vue, de relier les pins d’exécution, d’ajouter quelques types de nœuds, de valider le graphe, de l’enregistrer dans `Blueprints/Main.blueprint` et de l’exécuter avec le runtime intégré.

Cette interface constitue le socle de l’éditeur. Elle n’est pas encore l’équivalent complet de l’éditeur Blueprint d’Unreal Engine : la recherche avancée, les menus contextuels, les sous-graphes, les macros, les nœuds générés par réflexion, les broches spécialisées 2D/3D, le débogage pas à pas et le placement automatique restent à développer.

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

La prochaine étape est de remplacer les nœuds de démonstration par une bibliothèque générée à partir de la réflexion rbfx. Elle devra exposer les entités, composants, scènes, transformations, sprites, corps physiques, caméras, lumières, matériaux, sons, entrées et événements. Ensuite viendront le compilateur de graphes, les sous-graphes, les fonctions Blueprint, les interfaces, les tableaux, les timelines, les nœuds latents et le débogage visuel.

Le canvas doit également évoluer vers une expérience plus proche d’Unreal Engine 5 : recherche contextuelle, catégories, raccourcis, sélection multiple, copier-coller, détachement de câbles, reroutage, commentaires, alignement, minimap, sauvegarde de la disposition, validation en direct et coloration des nœuds par type.

> Cette version est une **fondation compilable et testée**, pas encore un moteur Blueprint complet au niveau de production d’Unreal Engine. Elle est conçue pour être étendue par itérations contrôlées sans fragiliser le cœur 2D/3D de rbfx.
