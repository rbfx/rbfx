# libjuice - UDP Interactive Connectivity Establishment

[![License: MPL 2.0](https://img.shields.io/badge/License-MPL_2.0-blue.svg)](https://www.mozilla.org/en-US/MPL/2.0/)
[![Build and test](https://github.com/paullouisageneau/libjuice/actions/workflows/build.yml/badge.svg)](https://github.com/paullouisageneau/libjuice/actions/workflows/build.yml)
[![Gitter](https://badges.gitter.im/libjuice/community.svg)](https://gitter.im/libjuice/community?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)
[![Discord](https://img.shields.io/discord/903257095539925006?logo=discord)](https://discord.gg/jXAP8jp3Nn)
[![AUR package](https://repology.org/badge/version-for-repo/aur/libjuice.svg)](https://repology.org/project/libjuice/versions)
[![Vcpkg package](https://repology.org/badge/version-for-repo/vcpkg/libjuice.svg)](https://repology.org/project/libjuice/versions)

libjuice :lemon::sweat_drops: (_JUICE is a UDP Interactive Connectivity Establishment library_) allows to open bidirectionnal User Datagram Protocol (UDP) streams with Network Address Translator (NAT) traversal.

The library is a simplified implementation of the Interactive Connectivity Establishment (ICE) protocol, client-side and server-side, written in C without dependencies for POSIX platforms (including GNU/Linux, Android, Apple macOS and iOS) and Microsoft Windows. The client supports only a single component over UDP per session in a standard single-gateway network topology, as this should be sufficient for the majority of use cases nowadays.

libjuice is licensed under MPL 2.0, see [LICENSE](https://github.com/paullouisageneau/libjuice/blob/master/LICENSE).

libjuice is available on [AUR](https://aur.archlinux.org/packages/libjuice/) and [vcpkg](https://vcpkg.info/port/libjuice). Bindings are available for [Rust](https://github.com/VollmondT/juice-rs).

For a STUN/TURN server application based on libjuice, see [Violet](https://github.com/paullouisageneau/violet).

## Compatibility

The library implements a simplified but fully compatible ICE agent ([RFC5245](https://www.rfc-editor.org/rfc/rfc5245.html) then [RFC8445](https://www.rfc-editor.org/rfc/rfc8445.html)) featuring:
- STUN protocol ([RFC5389](https://www.rfc-editor.org/rfc/rfc5389.html) then [RFC8489](https://www.rfc-editor.org/rfc/rfc8489.html))
- TURN relaying ([RFC5766](https://www.rfc-editor.org/rfc/rfc5766.html) then [RFC8656](https://www.rfc-editor.org/rfc/rfc8656.html))
- SDP-based interface ([RFC8839](https://www.rfc-editor.org/rfc/rfc8839.html))
- IPv4 and IPv6 dual-stack support
- Optional multiplexing on a single UDP port

The limitations compared to a fully-featured ICE agent are:
- Only UDP is supported as transport protocol and other protocols are ignored.
- Only one component is supported, which is sufficient for WebRTC Data Channels and multiplexed RTP+RTCP.
- Candidates are gathered without binding to each network interface, which behaves identically to the full implementation on most client systems.

It also implements a lightweight STUN/TURN server ([RFC8489](https://www.rfc-editor.org/rfc/rfc8489.html) and [RFC8656](https://www.rfc-editor.org/rfc/rfc8656.html)). The server can be disabled at compile-time with the `NO_SERVER` flag.

## Dependencies

None!

Optionally, [Nettle](https://www.lysator.liu.se/~nisse/nettle/) can provide SHA1 and SHA256 algorithms instead of the internal implementation.

## Building

### Clone repository

```bash
$ git clone https://github.com/paullouisageneau/libjuice.git
$ cd libjuice
```

### Build with CMake

The CMake library targets `libjuice` and `libjuice-static` respectively correspond to the shared and static libraries. The default target will build the library and tests. It exports the targets with namespace `LibJuice::LibJuice` and `LibJuice::LibJuiceStatic` to link the library from another CMake project.

#### POSIX-compliant operating systems (including Linux and Apple macOS)

```bash
$ cmake -B build
$ cd build
$ make -j2
```

The option `USE_NETTLE` allows to use the Nettle library instead of the internal implementation for HMAC-SHA1:
```bash
$ cmake -B build -DUSE_NETTLE=1
$ cd build
$ make -j2
```

#### Microsoft Windows with MinGW cross-compilation

```bash
$ cmake -B build -DCMAKE_TOOLCHAIN_FILE=/usr/share/mingw/toolchain-x86_64-w64-mingw32.cmake # replace with your toolchain file
$ cd build
$ make -j2
```

#### Microsoft Windows with Microsoft Visual C++

```bash
$ cmake -B build -G "NMake Makefiles"
$ cd build
$ nmake
```

### Build directly with Make (Linux only)

```bash
$ make
```

The option `USE_NETTLE` allows to use the Nettle library instead of the internal implementation for HMAC-SHA1:
```bash
$ make USE_NETTLE=1
```

## Example

See [test/connectivity.c](https://github.com/paullouisageneau/libjuice/blob/master/test/connectivity.c) for a complete local connection example.

See [test/server.c](https://github.com/paullouisageneau/libjuice/blob/master/test/server.c) for a server example.

