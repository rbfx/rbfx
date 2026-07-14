// Copyright (c) 2008-2022 the Urho3D project.
// Copyright (c) 2023-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Core/Context.h"
#include "Urho3D/Resource/JSONValue.h"

#include <EASTL/optional.h>

#include <regex>

namespace Urho3D
{

/// %Localization subsystem. Stores all the strings in all languages.
class URHO3D_API Localization : public Object
{
    URHO3D_OBJECT(Localization, Object);

public:
    /// Construct.
    explicit Localization(Context* context);
    /// Destruct. Free all resources.
    ~Localization() override;

    /// Return the number of languages.
    /// @property
    int GetNumLanguages() const { return static_cast<int>(languages_.size()); }

    /// Return the index number of current language. The index is determined by the order of loading.
    /// @property
    int GetLanguageIndex() const { return languageIndex_; }

    /// Return the index number of language. The index is determined by the order of loading.
    int GetLanguageIndex(const ea::string& language);
    /// Return the name of current language.
    /// @property
    const ea::string& GetLanguage();
    /// Return the name of language.
    const ea::string& GetLanguage(int index);
    /// Set regex pattern for localizable strings.
    /// If empty, localization is attempted for all strings.
    /// If non-empty, localization is attempted only for strings that match regular expression pattern.
    void SetStringPattern(const ea::string& pattern);
    /// Set current language.
    void SetLanguage(int index);
    /// Set current language.
    void SetLanguage(const ea::string& language);
    /// Return a string in the current language. Returns EMPTY_STRING if id is empty. Returns id if translation is not
    /// found and logs a warning.
    ea::string Get(const ea::string& id, int index = -1);

    /// Clear all loaded strings.
    void Reset();
    /// Load strings from JSONFile. The file should be UTF8 without BOM.
    void LoadJSONFile(const ea::string& name, const ea::string& language = EMPTY_STRING);
    /// Load strings from JSONValue.
    void LoadMultipleLanguageJSON(const JSONValue& source);
    /// Load strings from JSONValue for specific language.
    void LoadSingleLanguageJSON(const JSONValue& source, const ea::string& language = EMPTY_STRING);

private:
    /// Language names.
    ea::vector<ea::string> languages_;
    /// Index of current language.
    int languageIndex_{-1};
    /// Localizable string pattern.
    ea::optional<std::regex> pattern_;
    /// Storage strings: <Language <StringId, Value> >.
    ea::unordered_map<StringHash, ea::unordered_map<StringHash, ea::string>> strings_;
};

} // namespace Urho3D
