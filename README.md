![Urho3D logo](https://raw.githubusercontent.com/urho3d/Urho3D/master/bin/Data/Textures/LogoLarge.png)

# Urho3D

[![Build Status](https://travis-ci.org/rokups/Urho3D.svg?branch=master)](https://travis-ci.org/rokups/Urho3D) [![Build status](https://ci.appveyor.com/api/projects/status/9b57do8manc0bfsq/branch/master?svg=true)](https://ci.appveyor.com/project/rokups/urho3d/branch/master) [![Join the chat at https://gitter.im/urho3d/Urho3D](https://badges.gitter.im/urho3d/Urho3D.svg)](https://gitter.im/urho3d/Urho3D?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)

**Urho3D** is a free lightweight, cross-platform 2D and 3D game engine implemented in C++ and released under the MIT license. Greatly inspired by OGRE and Horde3D.

Main website: [https://urho3d.github.io/](https://urho3d.github.io/)

## Differences
- This fork routinely merges from https://github.com/rokups/Urho3D.
- Update rates are fixed and a prioritized as having constant timing.  If you want to you can ignore using P_TIMESTEP in your update logic.
- Update rates are seperated from render update rates. There are no "Frame" events.  
  Rendering is viewed as a sampling of the current update state.  Transform Tweening is planned.
- Tweeks Subsystem.
- Other smaller changes as well as fixes.

## Further Goals
- Newton Game Dynamics Integration



## License
Licensed under the MIT license, see [LICENSE](https://github.com/urho3d/Urho3D/blob/master/LICENSE) for details.

## Contributing
Before making pull requests, please read the [Contribution checklist](https://urho3d.github.io/documentation/HEAD/_contribution_checklist.html) and [Coding conventions](https://urho3d.github.io/documentation/HEAD/_coding_conventions.html) pages from the documentation.

Urho3D is greatly inspired by OGRE (http://www.ogre3d.org) and Horde3D
(http://www.horde3d.org). Additional inspiration & research used:
- Rectangle packing by Jukka Jylänki (clb)
  http://clb.demon.fi/projects/rectangle-bin-packing
- Tangent generation from Terathon
  http://www.terathon.com/code/tangent.html
- Fast, Minimum Storage Ray/Triangle Intersection by Möller & Trumbore
  http://www.graphics.cornell.edu/pubs/1997/MT97.pdf
- Linear-Speed Vertex Cache Optimisation by Tom Forsyth
  http://home.comcast.net/~tom_forsyth/papers/fast_vert_cache_opt.html
- Software rasterization of triangles based on Chris Hecker's
  Perspective Texture Mapping series in the Game Developer magazine
  http://chrishecker.com/Miscellaneous_Technical_Articles
- Networked Physics by Glenn Fiedler
  http://gafferongames.com/game-physics/networked-physics/
- Euler Angle Formulas by David Eberly
  https://www.geometrictools.com/Documentation/EulerAngles.pdf
- Red Black Trees by Julienne Walker
  http://eternallyconfuzzled.com/tuts/datastructures/jsw_tut_rbtree.aspx
- Comparison of several sorting algorithms by Juha Nieminen
  http://warp.povusers.org/SortComparison/

Urho3D uses the following third-party libraries:
- Box2D 2.3.2 WIP (http://box2d.org)
- Bullet 2.86.1 (http://www.bulletphysics.org)
- Civetweb 1.7 (https://github.com/civetweb/civetweb)
- FreeType 2.8 (https://www.freetype.org)
- GLEW 1.13.0 (http://glew.sourceforge.net)
- kNet (https://github.com/juj/kNet)
- libcpuid 0.4.0+ (https://github.com/anrieff/libcpuid)
- LZ4 1.7.5 (https://github.com/lz4/lz4)
- MojoShader (https://icculus.org/mojoshader)
- Mustache 1.0 (https://mustache.github.io, https://github.com/kainjow/Mustache)
- Open Asset Import Library 4.1.0 (http://assimp.sourceforge.net)
- pugixml 1.7 (http://pugixml.org)
- rapidjson 1.1.0 (https://github.com/miloyip/rapidjson)
- Recast/Detour (https://github.com/memononen/recastnavigation)
- SDL 2.0.7 (https://www.libsdl.org)
- StanHull (https://codesuppository.blogspot.com/2006/03/john-ratcliffs-code-suppository-blog.html)
- stb_image 2.18 (https://nothings.org)
- stb_image_write 1.08 (https://nothings.org)
- stb_rect_pack 0.11 (https://nothings.org)
- stb_textedit 1.9 (https://nothings.org)
- stb_truetype 1.15 (https://nothings.org)
- stb_vorbis 1.13b (https://nothings.org)
- WebP (https://chromium.googlesource.com/webm/libwebp)
- imgui 1.52 (https://github.com/ocornut/imgui)
- easy_profiler 1.3.0 (https://github.com/yse/easy_profiler)
- nativefiledialog (https://github.com/mlabbe/nativefiledialog)
- IconFontCppHeaders (https://github.com/juliettef/IconFontCppHeaders)
- ImGuizmo (https://github.com/CedricGuillemet/ImGuizmo)
- deboost.context (https://github.com/septag/deboost.context)
- cr (https://github.com/fungos/cr)

DXT / ETC1 / PVRTC decompression code based on the Squish library and the Oolong Engine.
Jack and mushroom models from the realXtend project. (https://www.realxtend.org)
Ninja model and terrain, water, smoke, flare and status bar textures from OGRE.
BlueHighway font from Larabie Fonts.
Anonymous Pro font by Mark Simonson.
Font Awesome font from http://fontawesome.io/.
NinjaSnowWar sounds by Veli-Pekka Tätilä.
PBR textures from Substance Share. (https://share.allegorithmic.com)
IBL textures from HDRLab's sIBL Archive.
Dieselpunk Moto model by allexandr007.
Mutant & Kachujin models from Mixamo.
License / copyright information included with the assets as necessary. All other assets (including shaders) by Urho3D authors and licensed similarly as the engine itself.

## Documentation
Urho3D classes have been sparsely documented using Doxygen notation. To
generate documentation into the "Docs" subdirectory, open the Doxyfile in the
"Docs" subdirectory with doxywizard and click "Run doxygen" from the "Run" tab.
Get Doxygen from http://www.doxygen.org & Graphviz from http://www.graphviz.org.
See section "Documentation build" below on how to automate documentation
generation as part of the build process.

The documentation is also available online at
  https://urho3d.github.io/documentation/HEAD/index.html

Documentation on how to build Urho3D:
  https://urho3d.github.io/documentation/HEAD/_building.html
Documentation on how to use Urho3D as external library
  https://urho3d.github.io/documentation/HEAD/_using_library.html

Replace HEAD with a specific release version in the above links to obtain the
documentation pertinent to the specified release. Alternatively, use the
document-switcher in the documentation website to do so.

## History
The change history is available online at
  https://urho3d.github.io/documentation/HEAD/_history.html
