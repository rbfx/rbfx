//
// Copyright (c) 2018 Rokas Kupstys
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

#include <fstream>
#include <thread>
#include <cppast/libclang_parser.hpp>
#include <CLI11/CLI11.hpp>
#include "Pass/BuildMetaAST.h"
#include "Pass/UnknownTypesPass.h"
#include "Pass/CSharp/Urho3DTypeMaps.h"
#include "Pass/CSharp/Urho3DCustomPassEarly.h"
#include "Pass/CSharp/MoveGlobalsPass.h"
#include "Pass/CSharp/ConvertToPropertiesPass.h"
#include "Pass/CSharp/ImplementInterfacesPass.h"
#include "Pass/CSharp/FixDefaultValuesPass.h"
#include "Pass/CSharp/GenerateClassWrappers.h"
#include <Pass/CSharp/OverrideConstantsPass.h>
#include "Pass/CSharp/GenerateCApiPass.h"
#include "Pass/CSharp/RenameMembersPass.h"
#include "Pass/CSharp/Urho3DCustomPass.h"
#include "Pass/CSharp/GeneratePInvokePass.h"
#include "Pass/CSharp/GenerateCSharpApiPass.h"
#include <spdlog/spdlog.h>


namespace Urho3D
{

GeneratorContext* generator = nullptr;

}

using namespace Urho3D;


int main(int argc, char* argv[])
{
    struct CommandLineOptions
    {
        std::string rulesFile;
        std::string sourceDir;
        std::string outputDirCpp;
        std::string outputDirCs;
        std::vector<std::string> includes;
        std::vector<std::string> defines;
        std::vector<std::string> options;
    };

    generator = new GeneratorContext();

    CLI::App app{"CSharp bindings generator"};

    const size_t maxModules = 16;
    app.require_subcommand(1, maxModules);
    CommandLineOptions options[maxModules];

    for (auto i = 0; i < maxModules; i++)
    {
        auto* bindApp = app.add_subcommand(fmt::format("bind{}", i), "Generate module bindings");

        bindApp->add_flag("--static", generator->isStatic_, "Generate bindings for static library.");
        bindApp->add_option("-I", options[i].includes, "Target include paths.");
        bindApp->add_option("-D", options[i].defines, "Target preprocessor definitions.");
        bindApp->add_option("-O", options[i].options, "Target compiler options.");

        bindApp->add_option("rules", options[i].rulesFile, "Path to rules json file")->required()->check(CLI::ExistingFile);
        bindApp->add_option("source", options[i].sourceDir, "Path to source directory")->required()->check(CLI::ExistingDirectory);
        bindApp->add_option("output", options[i].outputDirCpp, "Path to output directory")->required();

        bindApp->set_callback([i, &options]() {
            auto& opt = options[i];
            auto sourceDir = str::AddTrailingSlash(opt.sourceDir);
            auto outputDirCs = str::AddTrailingSlash(opt.outputDirCpp) + "CSharp/";
            auto outputDirCpp = str::AddTrailingSlash(opt.outputDirCpp) + "Native/";

            Urho3D::CreateDirsRecursive(outputDirCpp);
            Urho3D::CreateDirsRecursive(outputDirCs);

            generator->sourceDir_ = sourceDir;
            generator->outputDirCpp_ = outputDirCpp;
            generator->outputDirCs_ = outputDirCs;

            // Generate bindings
            generator->LoadCompileConfig(opt.includes, opt.defines, opt.options);
#if _WIN32
            generator->config_.set_flags(cppast::cpp_standard::cpp_14, {
                cppast::compile_flag::ms_compatibility | cppast::compile_flag::ms_extensions
            });
#else
            generator->config_.set_flags(cppast::cpp_standard::cpp_11, {cppast::compile_flag::gnu_extensions});
#endif
            generator->LoadRules(opt.rulesFile);
            generator->Generate();
        });
    }

    std::vector<std::string> cmdLines;
    do
    {
        if (argc != 2)
            break;

        std::string parametersPath;
        CLI::App app{"CSharp bindings generator"};
        app.add_option("parameters", parametersPath)->required()->check(CLI::ExistingFile);;

        try
        {
            app.parse(argc, argv);
        }
        catch(const CLI::ParseError &e)
        {
            // First parameter is not file path with parameters. Could be a --help flag for example.
            break;
        }

        cmdLines.emplace_back(argv[0]);
        std::ifstream infile(parametersPath);
        std::string line;
        while (std::getline(infile, line))
        {
            str::rtrim(line);
            if (!line.empty())
                cmdLines.push_back(line);
        }

        char** newArgv = argv = new char*[cmdLines.size()];
        for (const auto& ln : cmdLines)
            *newArgv++ = (char*)ln.c_str();
        argc = (int)cmdLines.size();
    } while (false);

    spdlog::set_level(spdlog::level::debug);
    spdlog::stdout_color_mt("console");

    generator->AddCppPass<BuildMetaAST>();
    generator->AddApiPass<Urho3DTypeMaps>();
    generator->AddApiPass<UnknownTypesPass>();
    generator->AddApiPass<DiscoverInterfacesPass>();
    generator->AddApiPass<ImplementInterfacesPass>();
    generator->AddApiPass<GenerateClassWrappers>();
    generator->AddApiPass<OverrideConstantsPass>();
    generator->AddApiPass<Urho3DCustomPassEarly>();
    generator->AddApiPass<MoveGlobalsPass>();
    generator->AddApiPass<GenerateCApiPass>();
    generator->AddApiPass<RenameMembersPass>();
    generator->AddApiPass<FixDefaultValuesPass>();
    generator->AddApiPass<Urho3DCustomPassLate>();
    generator->AddApiPass<GeneratePInvokePass>();
    generator->AddApiPass<ConvertToPropertiesPass>();
    generator->AddApiPass<GenerateCSharpApiPass>();

    CLI11_PARSE(app, argc, argv);
}
