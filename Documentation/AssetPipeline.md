# Asset Pipeline

Le pipeline d’assets de rbfx-blueprint fournit une base déterministe pour l’import, la cuisson et l’invalidation des contenus 2D et 3D. Il ne force pas un format propriétaire: chaque format est branché par une `AssetImporterRule` et un callback spécialisé qui reçoit les données source, les réglages versionnés et le chemin de sortie.

## Réglages versionnés

`AssetImportSettings` contient une version de schéma, un nom d’importeur et une map de propriétés `Variant`. Les propriétés scalaires usuelles (`bool`, `int`, `float`, `double` et `string`) sont conservées en JSON avec leur type explicite. Le hash est déterministe: les clés sont triées avant de combiner leur nom, leur type et leur valeur. Une modification de réglage force donc une nouvelle variante de cache.

`AssetImportSettingsResource` expose les mêmes données sous forme de `Resource` rbfx. Il est enregistré dans les factories globales et peut être chargé et sauvegardé par le cycle `BeginLoad`/`Save` standard de l’engine.

## Cache et invalidation

`AssetCache` indexe chaque sortie avec l’identifiant source, le hash du contenu et le hash combiné des réglages et de la version de l’importeur. Une importation identique retourne l’entrée cuite sans rappeler le callback. Les dépendances conservées dans l’entrée permettent d’invalider les sorties qui consomment un asset modifié.

## Graphe de dépendances

`AssetDependencyGraph` maintient les arêtes source-vers-dépendances, refuse les auto-références et détecte les cycles avant validation. `CollectDirty` remonte les arêtes inverses afin de produire l’ensemble déterministe des assets à recuire lorsqu’une texture, un matériau, un mesh ou une scène change.

## Exemple minimal

```cpp
AssetImporter importer;
importer.RegisterRule({"mesh", "MeshImporter", 1,
    [](const ea::string& sourceId, const ea::string& sourceData,
        const AssetImportSettings& settings, const ea::string& outputPath,
        ea::vector<ea::string>& dependencies, ea::string& error)
    {
        // Décoder sourceData, écrire outputPath et déclarer les dépendances.
        return true;
    }});

AssetImportSettings settings;
settings.importer = "MeshImporter";
settings.properties["GenerateLODs"] = Variant(true);
const AssetImportResult result = importer.Import(
    "Models/hero.mesh", sourceData, settings, "Cooked/hero.model");
```

Cette fondation est volontairement indépendante du format de stockage final. Les phases de packaging et de cuisson multiplateforme peuvent ainsi ajouter des importeurs glTF, FBX, textures, audio, shaders et scènes sans modifier le contrat de cache.
