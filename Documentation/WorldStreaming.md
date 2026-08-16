# World Streaming and LOD

La phase de streaming de monde ajoute une fondation déterministe pour les grands niveaux 2D et 3D. Elle ne remplace pas le chargement de scènes rbfx : elle orchestre les cellules, leurs transitions et les budgets, tandis que l’intégration de chargement effective reste fournie par l’engine ou par l’éditeur.

## StreamingCell

`StreamingCell` décrit une cellule de monde avec un identifiant stable, des coordonnées de grille, un centre, un rayon d’influence et une ressource de scène optionnelle. Son état est strictement contrôlé par une machine de transitions :

| État | Transitions autorisées |
|---|---|
| `Unloaded` | `Loading` |
| `Loading` | `Loaded` ou `Failed` |
| `Loaded` | `Unloading` |
| `Unloading` | `Unloaded` ou `Failed` |
| `Failed` | `Unloaded` après `ResetFailure()` |

Chaque cycle de chargement augmente une révision monotone. Cette révision permet aux intégrations asynchrones d’ignorer le résultat tardif d’une opération obsolète et d’éviter les doubles chargements ou déchargements.

## WorldPartition

`WorldPartition` maintient les cellules enregistrées, le rayon d’intérêt, le nombre maximal de cellules résidentes et une file d’opérations ordonnée. `Update(observerPosition)` calcule les cellules désirées, élimine les opérations redondantes et donne la priorité aux cellules les plus proches de l’observateur. Les opérations sont consommées par `PopNextOperation()` puis confirmées par `CompleteOperation()`.

Les applications peuvent aussi appeler `RequestLoad(cellId)` et `RequestUnload(cellId)` pour les transitions explicites de gameplay. Ces méthodes sont idempotentes lorsqu’une cellule est déjà dans l’état demandé et renvoient une erreur descriptive en cas d’identifiant inconnu ou de transition concurrente incompatible.

> Le composant ne crée pas de thread caché et ne charge pas directement des fichiers dans son cœur. Cette séparation rend le budget, la planification et la politique d’annulation testables, puis permet de brancher un scheduler IO, un job system ou une politique d’éditeur sans changer les contrats de gameplay.

## LODGroup

`LODGroup` trie ses niveaux par index, valide les distances de coupure et sélectionne un niveau avec hystérésis. La distance est exprimée en unités monde; le niveau 0 est le plus détaillé et les seuils croissants sélectionnent des niveaux moins détaillés. L’hystérésis empêche les oscillations lorsque la caméra reste autour d’une coupure.

La sélection utilise la distance au carré dans les intégrations de streaming afin d’éviter les racines carrées inutiles. Les niveaux sont indépendants du type de rendu : ils peuvent représenter un mesh 3D, une variante de sprite 2D, une représentation impostor ou une désactivation complète.

## Nœuds Blueprint

Les nœuds suivants utilisent un `WorldPartition` injecté avec `BlueprintRuntime::SetWorldPartition()` :

| Nœud | Fonction |
|---|---|
| `World.LoadCell` | Demande le chargement explicite d’une cellule et renvoie `queued`. |
| `World.UnloadCell` | Demande le déchargement explicite d’une cellule et renvoie `queued`. |
| `World.SetStreamingRadius` | Modifie le rayon courant et renvoie la valeur effective. |

Une erreur d’intégration est publiée dans les diagnostics Blueprint avec un code `BPWORLDxxx`; aucun singleton global n’est utilisé.

## Cycle d’intégration recommandé

L’engine met à jour la partition à chaque changement de position de l’observateur, consomme les opérations selon sa capacité IO, lance le chargement ou le déchargement réel, puis appelle `CompleteOperation()` avec le résultat. Les cellules `Failed` doivent être journalisées et peuvent être réinitialisées après correction de la ressource ou modification de la politique de retry.

Les tests `TestWorldStreaming.cpp` couvrent les transitions, l’hystérésis, la priorité de proximité, le budget résident, la déduplication et la protection contre la suppression d’une cellule en vol. `TestBlueprintWorld.cpp` couvre le pont Blueprint et les diagnostics de cellule inconnue.
