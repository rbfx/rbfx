// Copyright (c) 2008-2022 the Urho3D project.
// Copyright (c) 2023-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Precompiled.h"

#include "Urho3D/Resource/Localization.h"

#include "Urho3D/IO/Log.h"
#include "Urho3D/Resource/JSONFile.h"
#include "Urho3D/Resource/ResourceCache.h"
#include "Urho3D/Resource/ResourceEvents.h"

#include "Urho3D/DebugNew.h"

namespace Urho3D
{

Localization::Localization(Context* context)
    : Object(context)
{
}

Localization::~Localization() = default;

int Localization::GetLanguageIndex(const ea::string& language)
{
    if (language.empty())
    {
        URHO3D_LOGWARNING("Cannot get localization language index: language name is empty");
        return -1;
    }

    if (GetNumLanguages() == 0)
    {
        URHO3D_LOGWARNING("Cannot get localization language index: no loaded languages");
        return -1;
    }

    for (int i = 0; i < GetNumLanguages(); i++)
    {
        if (languages_[i] == language)
            return i;
    }

    return -1;
}

const ea::string& Localization::GetLanguage()
{
    if (languageIndex_ == -1)
    {
        URHO3D_LOGWARNING("Cannot get localization language: no loaded languages");
        return EMPTY_STRING;
    }
    return languages_[languageIndex_];
}

const ea::string& Localization::GetLanguage(int index)
{
    if (GetNumLanguages() == 0)
    {
        URHO3D_LOGWARNING("Cannot get localization language: no loaded languages");
        return EMPTY_STRING;
    }

    if (index < 0 || index >= GetNumLanguages())
    {
        URHO3D_LOGWARNING("Cannot get localization language: index out of range");
        return EMPTY_STRING;
    }

    return languages_[index];
}

void Localization::SetStringPattern(const ea::string& pattern)
{
    try
    {
        if (pattern.empty())
            pattern_ = ea::nullopt;
        else
            pattern_ = std::regex{pattern.c_str()};
    }
    catch (const std::exception& e)
    {
        URHO3D_LOGERROR("Cannot set localizable string pattern: {}", e.what());
        pattern_ = ea::nullopt;
    }
}

void Localization::SetLanguage(int index)
{
    if (GetNumLanguages() == 0)
    {
        URHO3D_LOGWARNING("Cannot set localization language: no loaded languages");
        return;
    }

    if (index < 0 || index >= GetNumLanguages())
    {
        URHO3D_LOGWARNING("Cannot set localization language: index out of range");
        return;
    }

    if (index != languageIndex_)
    {
        languageIndex_ = index;
        VariantMap& eventData = GetEventDataMap();
        SendEvent(E_CHANGELANGUAGE, eventData);
    }
}

void Localization::SetLanguage(const ea::string& language)
{
    if (language.empty())
    {
        URHO3D_LOGWARNING("Cannot set localization language: language name is empty");
        return;
    }

    if (GetNumLanguages() == 0)
    {
        URHO3D_LOGWARNING("Cannot set localization language: no loaded languages");
        return;
    }

    const int index = GetLanguageIndex(language);
    if (index == -1)
    {
        URHO3D_LOGWARNING("Cannot set localization language: language not found");
        return;
    }

    SetLanguage(index);
}

ea::string Localization::Get(const ea::string& id, int index)
{
    if (id.empty())
        return EMPTY_STRING;

    if (GetNumLanguages() == 0)
    {
        URHO3D_LOGWARNING("Cannot get localized string: no loaded languages");
        return id;
    }

    if (index >= static_cast<int>(languages_.size()))
    {
        URHO3D_LOGWARNING("Cannot get localized string: invalid language index");
        return id;
    }

    if (pattern_ && !std::regex_match(id.begin(), id.end(), *pattern_))
    {
        // Quietly ignore strings that don't match the pattern
        return id;
    }

    const ea::string& language = index < 0 ? GetLanguage() : languages_[index];
    const ea::string& result = strings_[language][StringHash(id)];
    if (result.empty())
    {
        if (pattern_)
            URHO3D_LOGWARNING("Cannot get localized string for \"{}\" ({})", id, language);
        else
            URHO3D_LOGTRACE("Cannot get localized string for \"{}\" ({})", id, language);
        return id;
    }
    return result;
}

void Localization::Reset()
{
    languages_.clear();
    languageIndex_ = -1;
    strings_.clear();
}

void Localization::LoadJSONFile(const ea::string& name, const ea::string& language)
{
    auto* cache = GetSubsystem<ResourceCache>();
    auto* jsonFile = cache->GetResource<JSONFile>(name);
    if (jsonFile)
    {
        if (language.empty())
            LoadMultipleLanguageJSON(jsonFile->GetRoot());
        else
            LoadSingleLanguageJSON(jsonFile->GetRoot(), language);
    }
}

void Localization::LoadMultipleLanguageJSON(const JSONValue& source)
{
    for (const auto& i : source)
    {
        ea::string id = i.first;
        if (id.empty())
        {
            URHO3D_LOGWARNING("Error loading localization JSON: string ID is empty");
            continue;
        }

        const JSONValue& value = i.second;
        if (value.IsObject())
        {
            for (const auto& j : value)
            {
                const ea::string& language = j.first;
                if (language.empty())
                {
                    URHO3D_LOGWARNING("Error loading localization JSON: language name is empty for \"{}\"", id);
                    continue;
                }

                const ea::string& string = j.second.GetString();
                if (string.empty())
                {
                    URHO3D_LOGWARNING(
                        "Error loading localization JSON: translation is empty for \"{}\" ({})", id, language);
                    continue;
                }

                if (strings_[StringHash(language)][StringHash(id)] != EMPTY_STRING)
                    URHO3D_LOGWARNING("Overriding translation for \"{}\" ({})", id, language);

                strings_[StringHash(language)][StringHash(id)] = string;
                if (!languages_.contains(language))
                    languages_.push_back(language);
                if (languageIndex_ == -1)
                    languageIndex_ = 0;
            }
        }
        else
            URHO3D_LOGWARNING("Error loading localization JSON values for \"{}\"", id);
    }
}

void Localization::LoadSingleLanguageJSON(const JSONValue& source, const ea::string& language)
{
    for (const auto& i : source)
    {
        ea::string id = i.first;
        if (id.empty())
        {
            URHO3D_LOGWARNING("Error loading localization JSON: string ID is empty");
            continue;
        }

        const JSONValue& value = i.second;
        if (value.IsString())
        {
            if (value.GetString().empty())
            {
                URHO3D_LOGWARNING(
                    "Error loading localization JSON: translation is empty for \"{}\" ({})", id, language);
                continue;
            }

            if (strings_[StringHash(language)][StringHash(id)] != EMPTY_STRING)
                URHO3D_LOGWARNING("Overriding translation for \"{}\" ({})", id, language);

            strings_[StringHash(language)][StringHash(id)] = value.GetString();
            if (!languages_.contains(language))
                languages_.push_back(language);
        }
        else
            URHO3D_LOGWARNING("Error loading localization JSON values for \"{}\" ({})", id, language);
    }
}

} // namespace Urho3D
