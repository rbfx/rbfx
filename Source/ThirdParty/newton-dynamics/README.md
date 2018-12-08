![emscripten logo](https://raw.githubusercontent.com/MADEAPPS/newton-dynamics/master/applications/newtonLogo/newtonLogo.png)

Newton dynamics is a realistic, cross-platform physics simulation library. It can easily be integrated into games and game engines and provides top of its class performance and simulation stability.
Ongoing development and a permissive license makes Newton Dynamics a top choice for all kinds of projects from scientific projects to game engines.

* Links to **demos**, **tutorial**, **FAQ**, etc: <https://github.com/MADEAPPS/newton-dynamics/wiki>
* Main project page: <http://newtondynamics.com>
* Forums and public discussion: <http://newtondynamics.com/forum/index.php>

Compiling Newton
================

## Windows and OSX
Project files for Visual Studio and XCode are provided, mainly

* Newton Core and packages: [sdk/projects/](sdk/projects/)
* demo sandbox: [applications/demosSandbox/projects/](applications/demosSandbox/projects/)

## Linux
There are Unix makefiles in the project folders mentioned above, but it is *highly* recommended to use CMake instead.

Newton Core does not have any third party dependencies.

To build the demo sandbox the following packages need to be installed:
* OpenGL
* glfw3
* OpenAL
* TinyXML

All of these should be available on any major Linux distribution (with associated `-dev` or `-devel` packages).

Alternatively they can be found in the folder [sdk/thirdParty/](sdk/thirdParty/) and built from source.


License
=======
Newton Dynamics is licensed under the zlib open source license, with little if any practical difference between them.

See `LICENSE` for the full content of the licenses.

Authors
=======
Newton Dynamics is a project of Julio Jerez and Alain Suero. See `AUTHORS` for a full list of contributors.
