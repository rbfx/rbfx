![rbfx-logo](https://user-images.githubusercontent.com/19151258/57008846-a292be00-6bfb-11e9-8303-d79e6dd36038.png)

**rbfx** is a free lightweight, cross-platform 2D and 3D game engine implemented in C++ and released under the MIT license. Greatly inspired by OGRE and Horde3D.

This project is a fork of [urho3d.github.io](http://urho3d.github.io/).

## License

Licensed under the MIT license, see [LICENSE](https://github.com/urho3d/Urho3D/blob/master/LICENSE) for details.

## Features overview

* Audio
* Graphics (d3d9. d3d11, OpenGL 2.0, OpenGL 3.2, GLES2, GLES3)
* Input (keyboard/mouse/touch)
* Inverse Kinematics
* Navigation
* Physics (2D/3D)
* UI (retained mode game UI + ImGui for tools)
* 2D subsystem
* Networking
* C# support
* WYSIWYG editor with hot code reload (C++ and C#)

## Screenshots

![editor](https://user-images.githubusercontent.com/19151258/49943614-09376980-fef1-11e8-88fe-8c26fcf30a59.jpg)

## Dependencies

rbfx uses the following third-party libraries:
- Box2D 2.3.2 WIP (http://box2d.org)
- Bullet 2.86.1 (http://www.bulletphysics.org)
- Civetweb 1.7 (https://github.com/civetweb/civetweb)
- FreeType 2.8 (https://www.freetype.org)
- GLEW 1.13.0 (http://glew.sourceforge.net)
- SLikeNet (https://github.com/SLikeSoft/SLikeNet)
- libcpuid 0.4.0+ (https://github.com/anrieff/libcpuid)
- LZ4 1.7.5 (https://github.com/lz4/lz4)
- MojoShader (https://icculus.org/mojoshader)
- Open Asset Import Library 4.1.0 (http://assimp.sourceforge.net)
- pugixml 1.9 (http://pugixml.org)
- rapidjson 1.1.0 (https://github.com/miloyip/rapidjson)
- Recast/Detour (https://github.com/recastnavigation/recastnavigation)
- SDL 2.0.7 (https://www.libsdl.org)
- StanHull (https://codesuppository.blogspot.com/2006/03/john-ratcliffs-code-suppository-blog.html)
- stb_image 2.18 (https://nothings.org)
- stb_image_write 1.08 (https://nothings.org)
- stb_rect_pack 0.11 (https://nothings.org)
- stb_textedit 1.9 (https://nothings.org)
- stb_truetype 1.15 (https://nothings.org)
- stb_vorbis 1.13b (https://nothings.org)
- WebP (https://chromium.googlesource.com/webm/libwebp)
- imgui 1.70 (https://github.com/ocornut/imgui/tree/docking)
- tracy (https://bitbucket.org/wolfpld/tracy/)
- nativefiledialog (https://github.com/mlabbe/nativefiledialog)
- IconFontCppHeaders (https://github.com/juliettef/IconFontCppHeaders)
- ImGuizmo 1.61 (https://github.com/CedricGuillemet/ImGuizmo)
- cr (https://github.com/fungos/cr)
- crunch (https://github.com/Unity-Technologies/crunch/)
- CLI11 v1.5.1 (https://github.com/CLIUtils/CLI11/)
- fmt 4.1.0 (http://fmtlib.net)
- spdlog 1.3.1 (https://github.com/gabime/spdlog)
- EASTL 3.13.04 (https://github.com/electronicarts/EASTL/)

rbfx optionally uses the following external third-party libraries:
- Mono (http://www.mono-project.com/download/stable/)

## Supported Platforms

| Platform |      API     | Build Status | Comment |
| -------- | ------------ | ------------ | ------- |
| Windows | D3D9, D3D11   | [![Build status](https://ci.appveyor.com/api/projects/status/9b57do8manc0bfsq/branch/master?svg=true)](https://ci.appveyor.com/project/rokups/rbfx/branch/master) | :heavy_check_mark:  |
| Linux   | OpenGL 2/3.1  | [![Build Status](https://travis-ci.org/rokups/rbfx.svg?branch=master)](https://travis-ci.org/rokups/rbfx) | :heavy_check_mark: |
| MacOS   | OpenGL 2/3.1  | [![Build Status](https://travis-ci.org/rokups/rbfx.svg?branch=master)](https://travis-ci.org/rokups/rbfx) | :heavy_check_mark: |
| iOS     | OpenGL 2/3.1  | - | Untested, planned |
| Android | OpenGL ES 2/3 | - | Untested, experimental |
| Web     | WebGL         | [![Build Status](https://travis-ci.org/rokups/rbfx.svg?branch=master)](https://travis-ci.org/rokups/rbfx) | Experimental, builds on WSL/Unix |
