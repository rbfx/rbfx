# rbfx World Fabric

`rbfx World Fabric` est le backbone sémantique déterministe qui relie les systèmes de production du moteur : réflexion rbfx et rbscript, Blueprint, streaming de mondes, RenderGraph, réseau, profilage, génération procédurale, tests, collaboration, accessibilité, localisation et packaging. Il ne remplace aucun de ces systèmes ; il leur fournit une représentation commune des ressources, services, graphes, cellules et artefacts afin de rendre leurs dépendances observables, validables et reproductibles.

> **Principe central :** un système de production ne doit pas seulement exécuter une opération ; il doit pouvoir déclarer ce qu’elle représente, de quoi elle dépend, quel digest elle produit et comment elle se corrèle aux autres opérations.

## Objectifs et garanties

World Fabric fournit des identifiants stables par clé sémantique, des dépendances typées, un tri topologique déterministe, la détection explicite des cycles, un digest structurel et des événements de modification. Les identifiants utilisent un hash FNV-1a 64 bits avec gestion des collisions ; ils restent donc indépendants des adresses mémoire, de l’ordre d’insertion et de la session de l’éditeur.

Le graphe est conçu pour fonctionner sans GPU, réseau ou éditeur actif. Les services sont injectés explicitement dans `BlueprintRuntime`, ce qui évite les singletons globaux et permet de les remplacer dans les tests, les serveurs dédiés et les outils de build.

| Garantie | Contrat de production |
| --- | --- |
| Identité | Une clé sémantique donne un `WorldFabricId` stable et collision-safe. |
| Ordonnancement | Le tri topologique est stable ; les nœuds équivalents sont ordonnés par identifiant. |
| Validation | Les nœuds inconnus, arêtes dupliquées et cycles sont rejetés avec un diagnostic. |
| Reproductibilité | Les digests parcourent les données dans un ordre déterministe. |
| Isolation | Chaque intégration peut être activée ou omise sans dépendance à un runtime global. |
| Observabilité | Les mutations, annotations de profilage et changements de services peuvent être consommés par des outils. |

## Modèle de graphe

Le cœur est défini par `Urho3D/WorldFabric/WorldFabric.h` et son implémentation. Un `WorldFabricNode` représente une entité sémantique : composant réfléchi, type rbscript, asset, cellule de streaming, passe de rendu, tâche de build, package ou test. Une `WorldFabricDependency` porte la relation typée entre deux nœuds et peut conserver une métadonnée de production.

Les opérations principales sont :

```cpp
WorldFabricId AddNode(const ea::string& key, WorldFabricNodeType type);
bool AddDependency(WorldFabricId from, WorldFabricId to, WorldFabricDependencyType type);
bool RemoveNode(WorldFabricId id);
bool BuildTopologicalOrder(ea::vector<WorldFabricId>& order, ea::string* error = nullptr) const;
unsigned long long ComputeDigest() const;
```

Une clé doit être stable et descriptive, par exemple `rbscript/type/Player`, `world/cell/12/-4`, `render/pass/TemporalUpscaling` ou `package/Windows/x64`. Les appels qui dépendent de données d’exécution doivent enregistrer ces données dans les métadonnées plutôt que dans la clé, afin de préserver une identité lisible.

## Intégrations du backbone

### Réflexion rbfx et rbscript

`WorldFabricReflection` projette les types, propriétés et méthodes exposés par `ObjectReflection` ainsi que le registre de types rbscript dans le graphe. Un même type peut donc être référencé par C++, Blueprint et rbscript au moyen d’un nœud sémantique commun. Cette projection permet de vérifier les dépendances de compilation et d’identifier les changements de contrat lors d’un hot reload.

### Blueprint

`BlueprintRuntime` possède des liaisons optionnelles vers les services World Fabric. Les nœuds déclarent la présence obligatoire du service au moment de l’exécution et retournent un diagnostic Blueprint stable lorsqu’il n’est pas injecté.

| Nœud Blueprint | Fonction |
| --- | --- |
| `WorldFabric.AddNode` | Ajouter un nœud sémantique et retourner son identifiant 64 bits. |
| `WorldFabric.AddDependency` | Déclarer une dépendance typée entre deux nœuds. |
| `WorldFabric.Validate` | Calculer l’ordre topologique et signaler les cycles. |
| `WorldFabric.GetDigest` | Lire le digest structurel courant. |
| `WorldFabric.SyncWorldPartition` | Synchroniser les cellules et assets du monde actif. |
| `HotReload.Reload` | Capturer, migrer et recharger un état versionné. |
| `Deterministic.GetFrame` / `GetDigest` | Lire la frame et l’empreinte de simulation. |
| `Deterministic.Restore` | Restaurer un snapshot de simulation borné. |
| `Build.Execute` | Exécuter le graphe de tâches déterministe avec cache. |
| `Procedural.Generate` | Générer des cellules procédurales à partir d’un seed. |
| `Tests.RunGameplay` | Exécuter les tests de gameplay filtrés par tags. |
| `Collab.SubmitOperation` / `LockNode` / `GetRevision` | Soumettre une opération, verrouiller un nœud et lire la révision. |
| `Accessibility.SetFeature` / `IsFeatureEnabled` / `SetTextScale` | Modifier ou lire les préférences d’accessibilité. |
| `Localize.Translate` / `SetLocale` | Résoudre une traduction et sélectionner la locale active. |
| `Profiler.AnnotateNode` / `GetNodeStats` | Corréler une mesure avec un nœud sémantique. |
| `Export.GetPlatform` / `GetCapabilities` | Décrire les capacités de l’adaptateur d’export choisi. |

Les identifiants 64 bits sont transportés dans les pins Blueprint comme des entiers non signés convertis explicitement en `Variant`. Cette règle évite les pertes de précision et les conversions implicites entre les graphes visuels et le C++.

### WorldPartition et streaming

`WorldFabricWorldPartition` synchronise les cellules du `WorldPartition`, leurs états de chargement, leurs niveaux de détail et leurs assets. Une cellule est représentée par une clé stable dérivée de ses coordonnées et de son identifiant de monde. Les transitions de chargement et de déchargement deviennent des événements de graphe pouvant être consommés par le build graph, le profiler ou les tests de gameplay.

### RenderGraph et production de contenu

Les passes de rendu, shaders, matériaux, VFX et assets peuvent être déclarés comme nœuds dépendants. Le digest World Fabric complète les digests de ressources : il permet d’identifier qu’un package a été produit avec une structure de graphe précise, même si les fichiers individuels ont le même contenu.

### Hot reload et simulation déterministe

`HotReloadManager` capture l’état versionné, applique une migration déclarée puis calcule un digest de l’état rechargé. `DeterministicSimulation` avance par pas fixes, conserve un historique borné de snapshots, restaure une frame et rejoue les entrées. Les deux services partagent le backbone afin que les rechargements et les rollbacks puissent être diagnostiqués avec les mêmes identifiants sémantiques.

### Build Graph, génération procédurale et tests

`BuildGraph` ordonne des tâches par dépendances, rejette les cycles et mémorise les résultats par digest. `ProceduralWorldGenerator` génère des cellules et des LOD de manière déterministe à partir d’un seed. `GameplayTestRunner` trie les cas par identifiant, applique le filtre de tags et fournit un rapport indépendant de l’ordre d’enregistrement.

### Collaboration, accessibilité et localisation

`WorldFabricCollaboration` conserve les opérations versionnées, les verrous clients, l’historique et la fusion. `WorldFabricAccessibility` centralise les préférences de sous-titres, contraste élevé, modes daltoniens, échelle de texte et mouvement réduit. `WorldFabricLocalization` gère les catalogues déterministes, la locale active et le fallback. Les digests de ces services peuvent être utilisés par la CI et le packaging pour détecter une divergence de configuration.

## Profilage corrélé

`WorldFabricProfiler` associe une durée à un `WorldFabricId` et à un canal (`CPU`, `GPU`, `rbscript` ou canal personnalisé). Il conserve les appels, le total, le minimum, le maximum et la moyenne, puis retourne les résultats dans l’ordre `node/channel`. Lorsqu’un `ProductionProfiler` est injecté, l’annotation est également relayée vers le scope CPU, la passe GPU ou la fonction rbscript correspondante.

```cpp
WorldFabricProfiler profiler(&worldFabric, &productionProfiler);
profiler.AnnotateNode(renderPassId, 1.42, "GPU");
WorldFabricNodeProfile stats;
profiler.GetNodeStats(renderPassId, "GPU", stats);
```

Cette corrélation évite les rapports isolés : un pic de temps peut être relié à une passe RenderGraph, une cellule streaming, un appel Blueprint ou une fonction rbscript. `ComputeDigest()` permet de comparer deux captures et de détecter une divergence dans les statistiques agrégées.

## Packaging reproductible et export multiplateforme

`PackageBuildProfile` et `PackageManifest` transportent le champ `worldFabricDigest`. Il est sérialisé comme chaîne décimale exacte afin d’éviter les pertes de précision dans les parseurs JSON qui représentent les nombres par `double`. La lecture accepte également les anciens manifests numériques pour préserver la compatibilité.

`PlatformExportAdapter` définit un contrat commun pour Linux, Windows, macOS et WebAssembly. Chaque adaptateur expose les architectures et les capacités threads, GPU, réseau, AOT et code dynamique. Le CLI `PackageBuilder` sélectionne l’adaptateur à partir du profil, rejette les architectures incompatibles et ne produit le manifeste qu’après validation.

| Cible | Architectures | Threads | GPU | Réseau | Code dynamique |
| --- | --- | ---: | ---: | ---: | ---: |
| Linux | x64, arm64 | Oui | Oui | Oui | Oui |
| Windows | x64, arm64 | Oui | Oui | Oui | Oui |
| macOS | x64, arm64 | Oui | Oui | Oui | Oui |
| WebAssembly | wasm32 | Non | Oui | Oui | Non |

Les adaptateurs décrivent des capacités de packaging ; ils ne prétendent pas remplacer les toolchains natives, SDK ou certificats de chaque plateforme. Les backends console et Android peuvent être ajoutés derrière la même interface lorsque leurs toolchains et contraintes de distribution sont disponibles.

## Déterminisme et CI

Un pipeline reproductible doit enregistrer le digest World Fabric avec le commit source, le profil de plateforme, le seed procédural, la version des outils et les digests d’assets. En CI, les étapes recommandées sont : reconfiguration CMake, compilation de `Tests`, exécution de `ctest --output-on-failure`, compilation Release de `PackageBuilder`, validation du profil et comparaison du manifeste produit.

La commande de validation du dépôt est :

```bash
./script/validate_production.sh
```

Pour une vérification ciblée :

```bash
cmake -S . -B build -G Ninja -DURHO3D_TESTING=ON -DURHO3D_EDITOR=OFF \
  -DURHO3D_PLAYER=OFF -DURHO3D_CSHARP=OFF
cmake --build build --target Tests -j2
ctest --test-dir build --output-on-failure
```

Les services World Fabric utilisent des historiques bornés et des conteneurs déterministes. Les callbacks de collaboration, hot reload, build et tests ne doivent pas dépendre d’une horloge système non injectée, d’une adresse mémoire ou de l’ordre d’itération d’une table de hachage.

## Organisation du code

| Répertoire | Responsabilité |
| --- | --- |
| `Source/Urho3D/WorldFabric/` | Graphe, réflexion, streaming, hot reload, simulation, build, génération, tests, collaboration, accessibilité, localisation et profilage corrélé. |
| `Source/Urho3D/Blueprint/` | Runtime Blueprint et nœuds d’intégration des services. |
| `Source/Urho3D/Resource/` | PackageBuilder et adaptateurs d’export. |
| `Source/Urho3D/Profiler/` | ProductionProfiler CPU/GPU/mémoire/réseau/audio/scripts. |
| `Source/Tests/` | Tests Catch2 du backbone et de ses intégrations. |
| `Source/Tools/PackageBuilder/` | CLI de construction et de validation des manifests. |

## État de validation

La suite Linux validée de cette itération contient **305 tests passants sur 305**, dont les tests de registre Blueprint Phase 5, de corrélation World Fabric–profiler, de digest de manifeste et d’adaptateurs d’export. Le binaire Release `PackageBuilder` est également compilé et son aide CLI est disponible avec `PackageBuilder --help`.

World Fabric est donc une couche d’intégration déterministe et testée. La conformité complète à chaque SDK propriétaire, aux consoles et aux services de distribution reste dépendante des toolchains et licences propres à ces plateformes ; elle doit être vérifiée dans les runners CI correspondants plutôt que simulée dans le dépôt Linux.
