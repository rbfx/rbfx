[![Build Status](https://github.com/rokups/rbfx/workflows/Build/badge.svg)](https://github.com/rokups/rbfx/actions)
[![Discord Chat](https://img.shields.io/discord/560082228928053258.svg?logo=discord)](https://discord.gg/XKs73yf)
[![Support on Patreon](https://img.shields.io/badge/dynamic/json?color=%23e85b46&label=Patreon&query=data.attributes.patron_count&suffix=%20patrons&url=https%3A%2F%2Fwww.patreon.com%2Fapi%2Fcampaigns%2F9697078&logo=patreon)](https://www.patreon.com/eugeneko)

**Rebel Fork Framework** aka **rbfx** is an experimental fork of [Urho3D](https://github.com/urho3d/Urho3D) game engine distributed under [MIT license](https://github.com/rbfx/rbfx/blob/master/LICENSE).

**Rebel Fork Framework** is:

* **Free and Open Source Software**, and it will stay this way;
* Suitable for 3D games and applications;
* Moderately lightweight and modular;
* Supported for Windows, Linux, MacOS, Android, iOS, Web and XBox (via UWP);
* Just a C++ library with a couple of tools;
* Also, there are experimental, optional C# bindings.

**Note**: The Framework is not yet released and is undergoing active development.
Backward compatibility is (mostly) preserved on resource level, but C++ API is prone to changes.


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

* You are not ready to write code when you need some feature.
    Just use [Unity](https://unity.com/) or any other mainstream engine with user store and ready-to-use assets.

* You want to have cutting-edge graphics or technology for AAA game.
    Just use [Unreal Engine](https://www.unrealengine.com/) or any other graphics-oriented game engine.

* You are happy with Urho3D.
    This framework is not intended to be a replacement of Urho3D.
    Just keep using [Urho3D](https://github.com/urho3d/Urho3D).

* You want Urho3D but for C#.
    While this framework *does* have C# bindings, C# is not a first-class citizen here.
    Try using [Urho.Net](https://github.com/Urho-Net) or any other C#-friendly engine.


## Using the Framework

It is recommended to check out [Sample Project on GitHub](https://github.com/rbfx/sample-project).

This project demonstrates full pipeline: from code and assets to build and publishing.

This project is automatically published to [itch.io](https://eugeneko.itch.io/sample-project).

If you want to build the project or the Framework for your platform,
it's recommended to check out how GitHub Actions are configured.


## Supported Platforms

| Graphics API/Platform | Windows | UWP | Linux | MacOS | iOS | Android | Web |
| --------------------- |:-------:|:---:|:-----:|:-----:|:---:|:-------:|:---:|
| D3D11                 | ✔       | ✔   |       |       |     |         |     |
| OpenGL 2 / 3.1        | ✔       |     | ✔     | ✔     |     |         |     |
| OpenGL ES 2 / 3       |         |     |       |       | ✔   | ✔       |     |
| WebGL                 |         |     |       |       |     |         | ✔   |


## Links

* [Documentation](https://rbfx.github.io/index.html)

* [Project Repository on GitHub](https://github.com/rbfx/rbfx)

* [Documentation Repository](https://github.com/rbfx/rbfx-docs) for documentation issues and pull requests

* [Discord Server](https://discord.gg/XKs73yf)


## Screenshots

![](https://rbfx.github.io/screenshot-00.png)
