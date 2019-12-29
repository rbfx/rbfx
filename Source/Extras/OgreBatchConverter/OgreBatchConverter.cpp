// OgreBatchConverter.cpp : Defines the entry point for the console application.
//

#include <Urho3D/Core/Context.h>
#include <Urho3D/Core/ProcessUtils.h>
#include <Urho3D/IO/FileSystem.h>

#include <cstdio>

using namespace Urho3D;

SharedPtr<Context> context(new Context());
SharedPtr<FileSystem> fileSystem(new FileSystem(context));

int main(int argc, char** argv)
{
    // Take in account args and place on OgreImporter args
    const ea::vector<ea::string>& args = ParseArguments(argc, argv);
    ea::vector<ea::string> files;
    ea::string currentDir = fileSystem->GetCurrentDir();

    // Try to execute OgreImporter from same directory as this executable
    ea::string ogreImporterName = fileSystem->GetProgramDir() + "OgreImporter";

    printf("\n\nOgreBatchConverter requires OgreImporter.exe on same directory");
    printf("\nSearching Ogre file in Xml format in %s\n" ,currentDir.c_str());
    fileSystem->ScanDir(files, currentDir, "*.xml", SCAN_FILES, true);
    printf("\nFound %d files\n", files.size());
    #ifdef WIN32
    if (files.size()) fileSystem->SystemCommand("pause");
    #endif

    for (unsigned i = 0 ; i < files.size(); i++)
    {
        ea::vector<ea::string> cmdArgs;
        cmdArgs.push_back(files[i]);
        cmdArgs.push_back(ReplaceExtension(files[i], ".mdl"));
        cmdArgs.append(args);

        ea::string cmdPreview = ogreImporterName;
        for (unsigned j = 0; j < cmdArgs.size(); j++)
            cmdPreview += " " + cmdArgs[j];

        printf("\n%s", cmdPreview.c_str());
        fileSystem->SystemRun(ogreImporterName, cmdArgs);
    }

    printf("\nExit\n");
    #ifdef WIN32
    fileSystem->SystemCommand("pause");
    #endif

    return 0;
}

