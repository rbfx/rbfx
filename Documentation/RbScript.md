# rbscript

## Présentation

**rbscript** est le langage de gameplay textuel natif de rbfx. Il complète le C++ moteur et le système Blueprint sans remplacer l’un ou l’autre : le C++ reste utilisé pour le cœur du moteur et les extensions bas niveau, Blueprint sert au scripting visuel, et rbscript sert au code de gameplay typé, lisible et versionnable.

Le langage utilise une syntaxe à accolades de style C++, avec des fonctions déclarées par `fn`, un type de retour introduit par `->`, des scripts héritant d’un type rbfx et des gestionnaires d’événements déclarés par `on`. Il ne repose pas sur l’indentation significative de GDScript.

> Un fichier `.rbscript` suit le pipeline `source → lexer → AST → type checker → bytecode → VM`, puis peut appeler des fonctions natives rbfx ou des fonctions Blueprint exportées.

## Architecture native

| Module | Responsabilité |
|---|---|
| `RbScriptDefs.h/.cpp` | Tokens, positions source, spans et diagnostics structurés. |
| `RbScriptLexer.h/.cpp` | Tokenisation des mots-clés, identifiants, nombres, chaînes, commentaires et opérateurs. |
| `RbScriptAst.h` | Représentation des modules, scripts, champs, paramètres, fonctions, événements, instructions et expressions. |
| `RbScriptParser.h/.cpp` | Parseur récursif descendant avec précédence des expressions et diagnostics de syntaxe. |
| `RbScriptType.h/.cpp` | Types intégrés, alias, génériques, signatures de fonctions et vérification statique. |
| `RbScriptCompiler.h/.cpp` | Compilation de l’AST en instructions, constantes, fonctions compilées et informations de débogage. |
| `RbScriptVM.h/.cpp` | Machine virtuelle à pile, frames d’appels, valeurs typées, appels natifs, événements et limites d’exécution. |
| `RbScriptReflection.h/.cpp` | Projection des métadonnées `ObjectReflection` rbfx dans le registre de types rbscript. |
| `RbScriptBindings.h/.cpp` | Fonctions natives de scène, navigation de nœuds, composants, monde, entrée et appels Blueprint. |
| `RbScriptBlueprintInterop.h/.cpp` | Conversion de valeurs et appels bidirectionnels entre rbscript et Blueprint. |
| `RbScriptResource.h/.cpp` | Ressource `.rbscript`, compilation au chargement, cache du bytecode, sauvegarde et rechargement conditionnel. |
| `RbScriptTab.h/.cpp` | Onglet de code Foundation avec édition multiline, coloration lexicale, diagnostics, aperçu des tokens, compilation et undo/redo. |

Le module est ajouté à `Source/Urho3D/CMakeLists.txt` et est donc compilé comme une partie native d’Urho3D/rbfx. Il n’utilise pas un interpréteur externe ni une enveloppe C++ séparée.

## Syntaxe de base

Un module peut déclarer un script de gameplay avec une base rbfx, des champs et des fonctions :

```rbscript
module PlayerGameplay;

use rbfx::Input;
use rbfx::Math;

script PlayerController : Node
{
    [[editable]] var speed: f32 = 6.0f;
    var health: i32 = 100;

    fn move(direction: Vector3) -> void
    {
        const velocity: Vector3 = direction * speed;
        owner().translate(velocity);
    }

    fn damage(amount: i32) -> void
    {
        health = health - amount;
        if (health <= 0)
        {
            emit defeated;
        }
    }

    on Ready()
    {
        emit initialized;
    }
}
```

Les fonctions asynchrones utilisent le même style à accolades :

```rbscript
async fn respawn(delay: f32) -> void
{
    await Delay(delay);
    health = 100;
    emit respawned;
}
```

Les attributs utilisent la forme à doubles crochets. L’attribut `[[blueprint_callable]]` exporte une fonction rbscript vers le registre d’interopérabilité Blueprint :

```rbscript
[[blueprint_callable]]
fn compute_damage(base: f32, multiplier: f32) -> f32
{
    return base * multiplier;
}
```

Les déclarations peuvent utiliser les types intégrés `void`, `bool`, `i32`, `u32`, `f32`, `f64`, `String`, `Vector2`, `Vector3`, `Quaternion`, `Color`, `Node`, `Component`, `Resource` et `Variant`. Les alias `Float`, `Int`, `NodeBehavior` et `NodeRef` sont résolus par le registre. Les formes génériques `Array<T>`, `Map<K, V>` et `Optional<T>` sont également enregistrées.

## Pipeline de compilation

Le compilateur de ressource applique les étapes suivantes dans l’ordre :

1. `RbScriptLexer` convertit le texte en tokens et conserve les positions ligne, colonne et offset.
2. `RbScriptParser` construit un `RbScriptModule` avec les scripts, fonctions, champs, événements, instructions et expressions.
3. `RbScriptTypeChecker` résout les alias et génériques, vérifie les initialisations, les paramètres, les retours, les opérateurs et les appels.
4. `RbScriptCompiler` produit un `RbScriptChunk`, son pool de constantes, ses fonctions compilées, ses noms de paramètres et ses informations de ligne.
5. `RbScriptVM` exécute le chunk avec une pile de valeurs, une pile de frames et un gestionnaire de fonctions natives.

Une compilation échouée ne remplace pas le dernier chunk valide déjà conservé par `RbScriptResource`. Ce comportement permet à l’éditeur et au jeu de conserver une version exécutable pendant la correction d’une erreur de syntaxe ou de typage.

## Types partagés avec rbfx

Les valeurs scalaires, chaînes et types mathématiques rbfx sont transportés dans `RbScriptValue`. Les conversions interopérables comprennent notamment `bool`, entiers, flottants, `String`, `Vector2`, `Vector3`, `Quaternion`, `Color` et `Variant`. Les objets rbfx sont reliés au moteur par les callbacks natifs et par les métadonnées `ObjectReflection`, plutôt que par des pointeurs non typés exposés directement au script.

Le registre peut être enrichi depuis un `Context` rbfx :

```cpp
RbScriptTypeRegistry registry;
registry.RegisterFromReflection(context);
```

Cette opération indexe les types et propriétés disponibles dans la réflexion du moteur afin que les diagnostics, l’autocomplétion et les bindings utilisent le même vocabulaire que le C++ et l’éditeur.

## API gameplay native

Les fonctions natives enregistrées par `RbScriptBindings` fournissent les points d’accès principaux au gameplay : navigation vers le nœud propriétaire, recherche d’enfant, récupération d’un composant, accès au monde ou à la scène, entrée utilisateur et temporisation. Les fonctions sont représentées comme des signatures dans le registre et invoquées par le VM via un gestionnaire d’appels hôte.

Les fonctions natives peuvent être étendues par le jeu en enregistrant un callback portant un nom stable et en contrôlant explicitement ses arguments, sa valeur de retour et les références de scène. Les erreurs de résolution ou de conversion sont rapportées dans les diagnostics VM au lieu d’être ignorées.

## Interopérabilité Blueprint

L’interopérabilité fonctionne dans les deux directions.

| Direction | Mécanisme |
|---|---|
| Blueprint → rbscript | Le runtime Blueprint utilise l’invoker rbscript pour appeler une fonction compilée par son nom, avec le mapping des paramètres dans l’ordre déclaré. Le nœud générique `Function.RbScript` expose cette passerelle. |
| rbscript → Blueprint | Le binding `blueprint::call` demande au runtime Blueprint d’invoquer une fonction Blueprint et renvoie une valeur `Variant` convertie en `RbScriptValue`. |
| rbscript → rbfx | Le VM délègue les fonctions externes au gestionnaire de bindings natifs. |
| rbscript → Blueprint exporté | Les fonctions marquées `[[blueprint_callable]]` sont converties par `RbScriptBlueprintInterop` en définitions de nœuds avec pins d’exécution et de données. |

Les signatures exportées conservent les noms de paramètres et leurs types afin que la création des pins reste déterministe. Les fonctions non marquées restent utilisables par rbscript, mais ne sont pas publiées automatiquement comme nœuds Blueprint.

## Ressource et rechargement

`RbScriptResource` est une ressource rbfx native enregistrée dans `ResourceCache`. Le cache peut donc ouvrir les fichiers `.rbscript` depuis les chemins de ressources et les packages. La ressource conserve le texte source, le module AST, les diagnostics et le chunk compilé.

La sauvegarde écrit le texte source avec les APIs `File` rbfx. Le rechargement conditionnel compare l’état du fichier et ne remplace le chunk actif qu’après une compilation complète réussie. Les erreurs restent disponibles par `GetDiagnostics()` afin que l’éditeur, les outils de build et le jeu puissent les afficher de manière uniforme.

## Éditeur Foundation

L’onglet **rbscript** est enregistré automatiquement dans `EditorApplication`. Le navigateur de ressources reconnaît l’extension `.rbscript` comme `RbScriptResource` et route cette ressource vers `RbScriptTab`.

L’onglet prend en charge l’ouverture de plusieurs ressources, l’édition via `InputTextMultiline`, la compilation automatique optionnelle, la sauvegarde, l’aperçu lexical coloré, l’affichage des diagnostics ligne/colonne, les raccourcis de compilation et de sauvegarde, ainsi que les snapshots undo/redo du texte. Une liste de suggestions fournit les mots-clés et les types intégrés connus du registre rbscript.

## Tests et validation

La suite Catch2 du dépôt contient les tests du lexer, du parseur, du système de types, du compilateur, du VM, des bindings, de l’interopérabilité et de `RbScriptResource`. La validation Linux réalisée sur cette branche a donné **223 tests réussis sur 223**. Le binaire de l’éditeur `build-editor/bin/Debug/Editor` a également été construit avec succès après l’intégration de `RbScriptTab`.

Les commandes de validation sont :

```bash
cmake -S . -B build -G Ninja \
  -DURHO3D_TESTING=ON \
  -DURHO3D_EDITOR=OFF \
  -DURHO3D_PLAYER=OFF \
  -DURHO3D_CSHARP=OFF
cmake --build build --target Tests -j2
ctest --test-dir build --output-on-failure
```

Pour l’éditeur :

```bash
cmake -S . -B build-editor \
  -DURHO3D_EDITOR=ON \
  -DURHO3D_PLAYER=OFF \
  -DURHO3D_TOOLS=ON \
  -DURHO3D_TESTS=OFF \
  -DURHO3D_SAMPLES=OFF
cmake --build build-editor --target Editor -j2
```

La validation automatisée effectuée ici est Linux x86_64. Le code est conçu autour des APIs portables C++17 et rbfx, mais Windows et macOS doivent encore être compilés dans leurs environnements natifs avant de déclarer une validation officielle sur ces plateformes.

## Limites connues et prochaines extensions

Le socle actuel fournit le langage, le typage, le bytecode, le VM, les bindings, l’interopérabilité, la ressource et l’éditeur. Les extensions de production restantes concernent principalement un débogueur rbscript visuel avec breakpoints et inspection de variables, une autocomplétion sémantique fondée sur l’AST et la réflexion, des tableaux et dictionnaires pleinement manipulables à l’exécution, les structs et enums utilisateur, des delegates/signals plus riches, ainsi que des bibliothèques gameplay dédiées à la physique, l’animation, l’audio et la caméra.

Ces extensions peuvent être ajoutées sans modifier le contrat fondamental : le C++ garde la responsabilité du moteur, Blueprint reste le graphe visuel et rbscript reste le langage textuel partagé par la réflexion rbfx et l’interopérabilité Blueprint.

## Références internes

Les implémentations et tests correspondants se trouvent dans `Source/Urho3D/RbScript/`, `Source/Tests/`, `Source/Urho3D/Blueprint/` et `Source/Editor/Foundation/`. Le dépôt public est [rbfx-blueprint](https://github.com/robert-sarah/rbfx-blueprint).
