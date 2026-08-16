# Multiplayer de production

Cette phase étend le transport réseau et la réplication rbfx existants sans créer de couche de transport concurrente. Les composants ajoutés sous `Source/Urho3D/Replica` utilisent les identifiants `NetworkId`, `Variant`, `StringVariantMap`, `NetworkFrame` et les connexions natives rbfx.

## Réplication déclarative

`ReplicatedProperty` décrit une propriété répliquée avec une valeur `Variant`, un état dirty et un indicateur de fiabilité. `ReplicatedPropertySet` enregistre les propriétés, capture les deltas dirty, applique des deltas entrants et permet de réinitialiser l’état dirty après émission. L’égalité est déterminée par `Variant`, ce qui conserve les types natifs rbfx et les valeurs imbriquées utilisées par Blueprint et rbscript.

## RPC typés

`RpcDispatcher` enregistre des handlers nommés et transporte des arguments `StringVariantMap` sur `AbstractConnection`. Chaque RPC est limité par une taille maximale, possède un mode fiable/non fiable, et peut exiger le rôle serveur. La validation du rôle est effectuée avant l’exécution du callback. Les nœuds Blueprint `Net.SendRPC` exposent cette façade via une connexion injectée ou la connexion serveur du subsystem `Network`.

## Intérêt réseau

`RelevancyManager` maintient un point d’observation et un rayon pour chaque connexion, ainsi qu’une règle spatiale par objet. Les objets always-relevant sont conservés indépendamment de la distance. Le filtrage est déterministe et utilise la distance au carré pour éviter les racines coûteuses dans les boucles d’intérêt.

## Snapshots et rollback

`SnapshotBuffer` conserve un historique borné de snapshots par `NetworkFrame`, interpole les valeurs numériques, maintient les valeurs discrètes et limite l’extrapolation à une fenêtre configurable. `RollbackManager` conserve les inputs et les états historiques, restaure l’état serveur confirmé puis rejoue uniquement les inputs postérieurs à cette confirmation avec un simulateur injecté.

## Serveur dédié

`DedicatedServer` encapsule le cycle de vie headless sur le subsystem `Network`, avec démarrage/arrêt idempotent, tick fixe borné et callback de simulation. Il est enregistré par `Engine` lorsque `URHO3D_NETWORK` est activé et peut être injecté dans `BlueprintRuntime`.

## Nœuds Blueprint

Les nœuds disponibles sont `Net.SendRPC`, `Net.SetRelevancy`, `Net.GetPing`, `Net.IsServer` et `Net.IsClient`. Ils ne dépendent d’aucun singleton: le runtime reçoit explicitement `RpcDispatcher`, `RelevancyManager`, `DedicatedServer`, `Network` et, si nécessaire, `AbstractConnection`.

## Validation

La phase ajoute des tests Catch2 couvrant les deltas répliqués, les règles d’autorisation RPC, le filtrage de pertinence, l’interpolation/extrapolation des snapshots, la réconciliation rollback et la présence des nœuds Blueprint. La suite globale atteint **255/255 scénarios réussis** sur Linux GCC avec `URHO3D_NETWORK=ON`.
