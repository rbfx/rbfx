//
// Copyright (c) 2017-2019 Rokas Kupstys.
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


#include <Urho3D/IO/FileWatcher.h>
#include <Urho3D/Resource/XMLFile.h>
#include <Urho3D/Scene/Serializable.h>
#include <Toolbox/IO/ContentUtilities.h>

#include "Converter.h"


namespace Urho3D
{

class Converter;

class Pipeline : public Serializable
{
    URHO3D_OBJECT(Pipeline, Serializable);
public:
    ///
    explicit Pipeline(Context* context);
    ///
    ~Pipeline() override;
    ///
    bool LoadJSON(const JSONValue& source) override;
    /// Watch directory for changed assets and automatically convert them.
    void EnableWatcher();
    /// Execute asset converters specified in `converterKinds` in worker threads. Returns immediately.
    void BuildCache(ConverterKinds converterKinds);

protected:
    /// Converts asset. Blocks calling thread.
    void ConvertAssets(const StringVector& resourceNames, ConverterKinds converterKinds);
    /// Watches for changed files and requests asset conversion if needed.
    void DispatchChangedAssets();

    /// Collection of top level converters defined in pipeline.
    Vector<SharedPtr<Converter>> converters_;
    /// List of file watchers responsible for watching game data folders for asset changes.
    FileWatcher watcher_;
};

}
