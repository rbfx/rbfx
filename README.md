[![Build Status](https://github.com/rokups/rbfx/workflows/Build/badge.svg)](https://github.com/rokups/rbfx/actions)
[![Discord Chat](https://img.shields.io/discord/560082228928053258.svg?logo=discord)](https://discord.gg/XKs73yf)
[![Support on Patreon](https://img.shields.io/badge/dynamic/json?color=%23e85b46&label=Patreon&query=data.attributes.patron_count&suffix=%20patrons&url=https%3A%2F%2Fwww.patreon.com%2Fapi%2Fcampaigns%2F9697078&logo=patreon)](https://www.patreon.com/eugeneko)

**The Rebel Fork** aka **rbfx** is an indie game engine/framework.
It is an experimental fork of [Urho3D](https://github.com/urho3d/Urho3D) game engine distributed under [MIT license](https://github.com/rbfx/rbfx/blob/master/LICENSE).

**The Rebel Fork** is:

* **Free and Open Source Software**, and it will stay this way;
* Suitable for 3D games and applications;
* Moderately lightweight and modular;
* Supported for Windows, Linux, MacOS, Android, iOS, Web and XBox (via UWP);
* Just a C++ library with a couple of tools;
* There are optional experimental C# bindings.

**Note**: The Framework is not yet released and is undergoing active development.
Backward compatibility is (mostly) preserved on resource level, but C++ API is prone to changes.


## Using the Framework

There are two template projects that you can use as example.

[Empty Project](https://github.com/rbfx/empty-project) is the absolute minimum of code
that is required to get things running on Desktop platforms.
Check it out if you don't care about recommended high-level workflow and want to do things your way.

[Sample Project](https://github.com/rbfx/sample-project) demonstrates recommended workflow
which enables certain high-level features like writing your code once and then running it
both from Editor and standalone. Sample Project is also Mobile and Web friendly
and is deployed to [itch.io](https://eugeneko.itch.io/sample-project).

Building the project is usually straighforward on Desktop platforms: standard CMake configure and build.
On Mobile and Web platforms extra steps may be needed.
If you cannot figure it out, check how our GitHub Actions are configured.
Chech documentation for more information.


## Supported Platforms

| Graphics API/Platform | Windows | UWP | Linux | MacOS | iOS | Android | Web |
| --------------------- |:-------:|:---:|:-----:|:-----:|:---:|:-------:|:---:|
| Direct3D 11.1         | ✔       | ✔   |       |       |     |         |     |
| Direct3D 12           | ✔       | ✔   |       |       |     |         |     |
| Vulkan 1.0            | ✔       |     | ✔     | ✔*    | ✔*  | ✔       |     |
| OpenGL 4.1            | ✔       |     | ✔     | ✔     |     |         |     |
| OpenGL ES 3.0         |         |     |       |       | ✔   | ✔       |     |
| WebGL 2.0             |         |     |       |       |     |         | ✔   |

(*) `Vulkan` is supported on MacOS and iOS via `MoltenVK`. Additional setup is required.


## Links

* [Latest Binaries for Desktop Platforms](https://github.com/rbfx/rbfx/releases/tag/latest)

* [Documentation](https://rbfx.github.io/index.html)

* [Project Repository on GitHub](https://github.com/rbfx/rbfx)

* [Documentation Repository](https://github.com/rbfx/rbfx-docs) for documentation issues and pull requests

* [Discord Server](https://discord.gg/XKs73yf)


## Reasons to use

There are multiple game engines out there, both proprietary and free.
Here are some reasons why you may want to try this one:

* It's "code first" framework with full control over code execution,
    unlike Unity-like game engines with "IDE first" approach and script sandboxes.

* It's portable and relatively lightweight framework that can be used like any other third-party dependency,
    unlike huge mainstream game engines.

* It's a fork of the mature and stable Urho3D engine (which was released in 2011),
    so it's more feature-rich and well tested than many of the new non-mainstream game engines.

* If you already use Urho3D, you may want to try this framework if you like Urho3D
    but you are not fully satisfied with current Urho3D feature set.


## Reasons NOT to use

**Don't** use the Framework if:

* You are not ready to do your own research.
    Due to small community size, we don't have as much documentation, tutorials and other onboarding materials.
    Consider using mainstream engines with bigger communities.

* You are not ready to write code when you need some feature.
    Due to small community and maintainers team size, we don't have as much "ready to use" codebase.
    Consider using mainstream engines with user store and ready-to-use assets.

* You want to have cutting-edge graphics or technology for AAA game.
    We try to provide decent level of technology, but we don't aim to be on the level of AAA graphics fidelity.
    Consider using commercial mainstream engines backed by paid full-time developers.

* You are happy with Urho3D.
    This framework is not intended to be a replacement of Urho3D.
    Consider using [U3D](https://github.com/u3d-community/U3D) or [Urho3D](https://github.com/urho3d/Urho3D).

* You want C#-oriented Urho3D.
    While this framework *does* have C# bindings, C# is not a first-class citizen here and its support is lacking.
    Consider using [Urho.Net](https://github.com/Urho-Net).


## Screenshots

![](https://github.com/rbfx/rbfx-docs/blob/master/images/showcase/screenshot-00.png?raw=true)
