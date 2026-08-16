# Production Profiler

Le module `Urho3D/Profiler/ProductionProfiler` fournit une collecte bornée et indépendante du backend pour instrumenter les frames, les scopes CPU hiérarchiques, les passes de rendu, les fonctions rbscript, la mémoire, le réseau et l’audio.

## Capture CPU

`BeginFrame(frameIndex)` ouvre une frame et `EndFrame()` ferme automatiquement les scopes encore actifs avant de publier la frame dans l’historique. `BeginScope`/`EndScope` sont utilisables directement ou via `ProfilerScope`, qui fournit une fermeture RAII. L’historique est borné par `SetHistoryFrames` afin de garantir une consommation mémoire contrôlée.

## Métriques spécialisées

`RecordGpuPass` agrège les durées des passes nommées. `RenderGraph::SetPassProfiler` fournit un sink optionnel qui mesure chaque callback exécuté avec une horloge monotone et transmet son nom au profiler. `RecordScriptFunction` est prévu pour les hooks d’exécution du VM rbscript. `TrackAllocation` et `TrackFree` suivent les octets courants, les pics, les allocations et les libérations par catégorie. `RecordNetwork` conserve les octets, pertes et RTT par connexion. `RecordAudio` agrège voix, échantillons et coût CPU par bus ou catégorie audio.

## Blueprint

`BlueprintRuntime` accepte un `ProductionProfiler` injecté par `SetProductionProfiler`. Les nœuds `Profiler.BeginScope`, `Profiler.EndScope` et `Profiler.GetFrameTime` utilisent ce service sans singleton global et signalent une erreur Blueprint explicite si aucun profiler n’est attaché.

## Rapports

`BuildReport()` produit une vue agrégée déterministe : temps moyen/minimum/maximum par frame, scopes CPU triés, passes GPU, fonctions script, catégories mémoire, connexions réseau et bus audio. Les données sont conçues pour alimenter ultérieurement `ProfilerTab`, l’export JSON et les diagnostics automatisés.

## Garanties

La collecte est désactivable avec `SetEnabled(false)`, les durées négatives sont bornées à zéro et les historiques sont tronqués conformément à la limite configurée. Le module reste utilisable dans les tests sans GPU, réseau actif ou périphérique audio.
