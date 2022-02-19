//
// Copyright (c) 2008-2022 the Urho3D project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#pragma once

#include "../Core/Context.h"
#include "../Resource/JSONValue.h"

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
    int GetNumLanguages() const { return (int) languages_.size(); }

    /// Return the index number of current language. The index is determined by the order of loading.
    /// @property
    int GetLanguageIndex() const { return languageIndex_; }

    /// Return the index number of language. The index is determined by the order of loading.
    int GetLanguageIndex(const ea::string& language);
    /// Return the name of current language.
    /// @property
    ea::string GetLanguage();
    /// Return the name of language.
    ea::string GetLanguage(int index);
    /// Set current language.
    void SetLanguage(int index);
    /// Set current language.
    void SetLanguage(const ea::string& language);
    /// Return a string in the current language. Returns EMPTY_STRING if id is empty. Returns id if translation is not found and logs a warning.
    ea::string Get(const ea::string& id, int index=-1);
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
    int languageIndex_;
    /// Storage strings: <Language <StringId, Value> >.
    ea::unordered_map<StringHash, ea::unordered_map<StringHash, ea::string> > strings_;
};

}
