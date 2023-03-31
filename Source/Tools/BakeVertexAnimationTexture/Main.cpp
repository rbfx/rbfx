//
// Copyright (c) 2023-2023 the rbfx project.
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

#include "Baker.h"

#include <Urho3D/Core/Context.h>
#include <Urho3D/Core/ProcessUtils.h>

#ifdef WIN32
    #include <windows.h>
#endif

using namespace Urho3D;

int main(int argc, char** argv);
void Run(ea::vector<ea::string>& arguments);

void Help(const ea::string& reason)
{
    ErrorExit(
        Format("{}"
        "\nUsage: BakeVertexAnimationTexture -options <input mdl> <input ani> <output folder>\n"
        "\n"
        "Options:\n"
        "--diffuse <diffuse texture> Bake diffuse texture into vertex colors.\n"
        "--precise Create a high precision (16 bit) texture instead of 8 bit.\n", reason));
}



int main(int argc, char** argv)
{
    ea::vector<ea::string> arguments;

#ifdef WIN32
    arguments = ParseArguments(GetCommandLineW());
#else
    arguments = ParseArguments(argc, argv);
#endif

    Run(arguments);
    return 0;
}

void Run(ea::vector<ea::string>& arguments)
{
    SharedPtr<Context> context{new Context()};
    auto* fileSystem = new FileSystem(context);
    context->RegisterSubsystem(fileSystem);
    // context->RegisterSubsystem(new Log(context));

    Options options;
    while (arguments.size() > 0)
    {
        ea::string arg = arguments.front();
        arguments.pop_front();

        if (arg.starts_with("-"))
        {
            if (arg == "--precise")
            {
                options.precise_ = true;
            }
            else if (arg == "--diffuse")
            {
                ea::string arg = arguments.front();
                arguments.pop_front();

                if (!options.diffuse_.empty())
                    Help("Diffuse texture already defined: " + arg + " " + options.diffuse_);
                if (!fileSystem->Exists(arg))
                    Help("Texture file not found: " + arg);
                options.diffuse_ = arg;
            }
            else
            {
                Help("Unknown argument " + arg);
            }
        }
        else
        {
            auto ext = GetExtension(arg);
            if (ext == ".mdl")
            {
                if (!options.inputModel_.empty())
                    Help("Model file already defined: " + arg + " " + options.inputModel_);
                if (!fileSystem->Exists(arg))
                    Help("Model file not found: "+arg);
                options.inputModel_ = arg;
            }
            else if (ext == ".ani")
            {
                if (!options.inputAnimation_.empty())
                    Help("Animation file already defined: " + arg + " " + options.inputAnimation_);
                if (!fileSystem->Exists(arg))
                    Help("Animation file not found: " + arg);
                options.inputAnimation_ = arg;
            }
            else
            {
                if (!options.outputFolder_.empty())
                    Help("Output folder already defined: " + arg + " " + options.outputFolder_);
                if (!fileSystem->DirExists(arg))
                {
                    if (!fileSystem->CreateDir(arg))
                    {
                        Help("Can't create directory: " + arg);
                    }
                }
                options.outputFolder_ = AddTrailingSlash(arg);
            }
        }
    }
    if (options.inputModel_.empty())
        Help("Model file not defined");
    if (options.inputAnimation_.empty())
        Help("Animation file not defined");
    if (options.outputFolder_.empty())
        Help("Output folder not defined");

    Baker baker{context, options};

    baker.Bake();


}
