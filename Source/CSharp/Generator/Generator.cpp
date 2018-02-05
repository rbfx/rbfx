//
// Copyright (c) 2008-2018 the Urho3D project.
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

#include <cppast/libclang_parser.hpp>
#include <CLI11/CLI11.hpp>
#include <Urho3D/Urho3DAll.h>
#include <Pass/GatherInfoPass.h>
#include <Pass/UnknownTypesPass.h>
#include <Pass/CSharp/GenerateCApiPass.h>
#include <Pass/CSharp/GenerateClassWrappers.h>
#include <Pass/CSharp/GeneratePInvokePass.h>
#include "GeneratorContext.h"


int main(int argc, char* argv[])
{
    String compileCommands;
    String rulesFile;
    String sourceDir;
    String outputDir;
    SharedPtr<Context> context;

    CLI::App app{"CSharp bindings generator"};
    app.add_option("rules", rulesFile, "Path to rules xml file")->check(CLI::ExistingFile);
    app.add_option("source", sourceDir, "Path to source directory")->check(CLI::ExistingDirectory);
    app.add_option("compile-commands", compileCommands, "Path to compile_commands.json")->check(CLI::ExistingFile);
    app.add_option("output", outputDir, "Path to output directory");

    CLI11_PARSE(app, argc, argv);
    sourceDir = AddTrailingSlash(sourceDir);
    outputDir = AddTrailingSlash(outputDir);

    context = new Context();
    context->RegisterSubsystem(new FileSystem(context));
    context->RegisterSubsystem(new Log(context));
    context->GetLog()->SetLevel(LOG_DEBUG);

    // Register factories
    context->RegisterFactory<GatherInfoPass>();
    context->RegisterFactory<UnknownTypesPass>();
    context->RegisterFactory<GenerateCApiPass>();
    context->RegisterFactory<GenerateClassWrappers>();
    context->RegisterFactory<GeneratePInvokePass>();

    // Generate bindings
    auto* generator = new GeneratorContext(context);
    context->RegisterSubsystem(generator);

    if (!generator->LoadCompileConfig(compileCommands))
    {
        URHO3D_LOGERROR("Loading compile commands failed.");
        return -1;
    }

#if _WIN32
    generator->GetCompilerConfig().set_flags(cppast::cpp_standard::cpp_11, {
        cppast::compile_flag::ms_compatibility | cppast::compile_flag::ms_extensions
    });
#else
    generator->GetCompilerConfig().set_flags(cppast::cpp_standard::cpp_11, {cppast::compile_flag::gnu_extensions});
#endif

    generator->LoadRules(rulesFile);
    generator->ParseFiles(sourceDir);

    generator->AddPass<GatherInfoPass>();
    generator->AddPass<UnknownTypesPass>();
    generator->AddPass<GenerateClassWrappers>();
    generator->AddPass<GenerateCApiPass>();
    generator->AddPass<GeneratePInvokePass>();

    generator->Generate(outputDir);
}
