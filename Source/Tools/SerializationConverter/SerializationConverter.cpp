//
// Copyright (c) 2017-2020 the rbfx project.
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

// Command line utility always uses console.
#define URHO3D_WIN32_CONSOLE

#include <Urho3D/Core/CommandLine.h>
#include <Urho3D/Engine/Application.h>
#include <Urho3D/Engine/EngineDefs.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/IO/BinaryArchive.h>
#include <Urho3D/IO/File.h>
#include <Urho3D/IO/Serializer.h>
#include <Urho3D/IO/Deserializer.h>
#include <Urho3D/Resource/XMLFile.h>
#include <Urho3D/Resource/JSONFile.h>
#include <Urho3D/Resource/XMLArchive.h>
#include <Urho3D/Resource/JSONArchive.h>
#include <Urho3D/Scene/Serializable.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/UI/UIElement.h>
#include <Urho3D/UI/Font.h>


using namespace Urho3D;

class ConverterApplication : public Application
{
    URHO3D_OBJECT(ConverterApplication, Application);
public:
    explicit ConverterApplication(Context* context) : Application(context)
    {
    }

    void Setup() override
    {
        engineParameters_[EP_ENGINE_CLI_PARAMETERS] = false;
        engineParameters_[EP_SOUND] = false;
        engineParameters_[EP_HEADLESS] = true;

        auto& app = GetCommandLineParser();
        app.add_option("-t,--type", type_, "Name of type that handles serialization of specified files.")->required();
        app.add_option("-i,--input-type", inputType_, "Serialization format of input file.")->set_default_str("old");
        app.add_option("-o,--output-type", outputType_, "Serialization format of output file.")-> set_default_str("new");
        app.add_option("input", input_, "Input file (xml/json/binary).")->required();
        app.add_option("output", output_, "Output file (xml/json/binary).")->required();
    }

    void Start() override
    {
        if (type_ == "Font")
        {
            // Font::SaveXML requires point size parameter.
            PrintLine("Conversions for 'Font' type are not supported.", true);
            engine_->Exit();
        }

        SharedPtr<Serializable> converter = StaticCast<Serializable>(context_->CreateObject(type_));

        bool loaded = false, read = false, saved = false;
        do
        {
            if (GetExtension(input_) == ".xml")
            {
                XMLFile file(context_);

                if (!(read = file.LoadFile(input_)))
                    break;

                if (inputType_ == "old")
                    loaded = converter->LoadXML(file.GetRoot());
                else if (inputType_ == "new")
                {
                    XMLInputArchive archive(&file);
                    loaded = converter->Serialize(archive);
                }
            }
            else if (GetExtension(input_) == ".json")
            {
                JSONFile file(context_);

                if (!(read = file.LoadFile(input_)))
                    break;

                if (inputType_ == "old")
                    loaded = converter->LoadJSON(file.GetRoot());
                else if (inputType_ == "new")
                {
                    JSONInputArchive archive(&file);
                    loaded = converter->Serialize(archive);
                }
            }
            else
            {
                File file(context_);

                if (!(read = file.Open(input_)))
                    break;

                if (inputType_ == "old")
                    loaded = converter->Load(file);
                else if (inputType_ == "new")
                {
                    BinaryInputArchive archive(context_, file);
                    loaded = converter->Serialize(archive);
                }
            }

            if (!loaded)
                break;

            if (GetExtension(output_) == ".xml")
            {

                if (outputType_ == "old")
                {
                    if (converter->GetTypeName() == "Scene")
                    {
                        File file(context_, output_, FILE_WRITE);
                        saved = StaticCast<Scene>(converter)->SaveXML(file);
                    }
                    else if (converter->GetTypeName() == "Node")
                    {
                        File file(context_, output_, FILE_WRITE);
                        saved = StaticCast<Node>(converter)->SaveXML(file);
                    }
                    else if (converter->GetTypeName() == "UIElement")
                    {
                        File file(context_, output_, FILE_WRITE);
                        saved = StaticCast<UIElement>(converter)->SaveXML(file);
                    }
                    else
                    {
                        PrintLine("Root XML tag of output file may be invalid!");
                        XMLFile file(context_);
                        XMLElement root = file.GetOrCreateRoot("root");
                        if ((saved = converter->SaveXML(root)))
                            file.SaveFile(output_);
                    }
                }
                else if (outputType_ == "new")
                {
                    XMLFile file(context_);
                    XMLOutputArchive archive(&file);
                    saved = converter->Serialize(archive);
                }
            }
            else if (GetExtension(output_) == ".json")
            {
                JSONFile file(context_);

                if (outputType_ == "old")
                    saved = converter->SaveJSON(file.GetRoot());
                else if (outputType_ == "new")
                {
                    JSONOutputArchive archive(&file);
                    saved = converter->Serialize(archive);
                }

                if (saved)
                    saved = file.SaveFile(output_);
            }
            else
            {
                File file(context_, output_, FILE_WRITE);

                if (outputType_ == "old")
                    saved = converter->Save(file);
                else if (outputType_ == "new")
                {
                    BinaryOutputArchive archive(context_, file);
                    saved = converter->Serialize(archive);
                }
            }
        } while (false);

        if (!read)
            PrintLine(Format("Reading of '{}' failed.", input_), true);
        else if (!loaded)
            PrintLine(Format("Loading of '{}' failed.", input_), true);
        else if (!saved)
            PrintLine(Format("Saving of '{}' failed.", output_), true);
        else
            PrintLine("Conversion succeeded.");

        engine_->Exit();
    }

    ea::string type_;
    ea::string inputType_{"old"};
    ea::string outputType_{"new"};
    ea::string input_;
    ea::string output_;
};

URHO3D_DEFINE_APPLICATION_MAIN(ConverterApplication);
