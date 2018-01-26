@echo off

::set up folders
if not exist Build\Win64_Shared mkdir Build\Win64_Shared
cd Build\Win64_Shared

::run cmake on Urho3D Source
call cmake ..\.. -DBUILD_SHARED_LIBS=1 -DURHO3D_ANGELSCRIPT=0 -DURHO3D_LUA=0 -DURHO3D_URHO2D=0 -DURHO3D_SAMPLES=0 -DURHO3D_LUA=0 -DURHO3D_OPENGL=1 -G "Visual Studio 15 2017 Win64"

::build Urho
start "" "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\MSBuild\15.0\Bin\msbuild.exe" Urho3D.sln /p:Configuration=Release /maxcpucount:4
start "" "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\MSBuild\15.0\Bin\msbuild.exe" Urho3D.sln /p:Configuration=Debug /maxcpucount:4
