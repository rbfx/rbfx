Steam Audio SDK
===============

.. toctree::
   :maxdepth: 2
   :hidden:
   :caption: Steam Audio SDK

   getting-started
   guide
   build-instructions
   integration
   reference

Steam Audio is a cross-platform software toolkit for spatial audio. You can use it to add rich, immersive positional and environmental audio effects to games and VR/AR apps. Steam Audio offers a wide variety of features, including:

-  **Render 3D positional audio.** Steam Audio binaurally renders direct sound using HRTFs to accurately model the direction of a sound source relative to the listener. Users can get an impression of the height of the source, as well as whether the source is in front of or behind them.

-  **Render 3D audio for Ambisonic content.** Steam Audio can also spatialize Ambisonics audio clips, rotating them based on the listener’s orientation in the virtual world. This is useful for creating spatialized ambiences that are fixed in place relative to the virtual world.

-  **Use custom HRTFs using SOFA files.** In addition to its built-in HRTF, Steam Audio can spatialize point sources and Ambisonics sources using any HRTF specified by the user. These files should be in the AES standard `SOFA format <https://www.sofaconventions.org/>`_.

-  **Model occlusion of sound sources.** Steam Audio can quickly model raycast occlusion of direct sound by solid objects. Steam Audio also models partial occlusion for non-point sources, and transmission of occluded sound through the occluding geometry.

-  **Model reflections and related environmental audio effects.** Steam Audio can model how sound is reflected by solid objects. This can be used to model many kinds of environmental audio effects, including slap echoes, flutter echoes, occlusion of sound by buildings, and more.

-  **Model propagation of sound along multiple paths.** Steam Audio can model how sound propagates from the source to the listener, along multiple paths. This can be used to correctly spatialize sound that reaches the listener after traveling along hallways, around corners, through doors and windows, and more.

-  **Create environmental audio effects tailored to your scene.** Steam Audio analyzes the size, shape, layout, and material properties of rooms and objects in your scene. It uses this information to automatically calculate occlusion, reflections, and other environmental effects by simulating the physics of sound.

-  **Automate the process of authoring reverbs and other environmental audio effects.** With Steam Audio, you don’t have to manually place effect filters throughout your scene, and you don’t have to manually tweak the filters everywhere. Steam Audio uses an automated real-time or pre-computation based process where environmental audio properties are calculated (using physics principles) throughout your scene.

-  **Change acoustics in real-time in response to moving geometry.** Steam Audio can model how moving geometry impacts occlusion, reflections, and other environmental effects. For example, opening doors can result in an occluded sound becoming unoccluded; making a room larger during gameplay can increase the amount of reverb in it.

-  **Render high-quality convolution reverbs.** Steam Audio can calculate convolution reverb, which results in compelling environments that sound more realistic than with parametric reverb. This is particularly true for outdoor spaces, where parametric reverbs have several limitations.

-  **Use multi-core CPU and GPU acceleration.** Steam Audio can use multi-core CPUs to accelerate simulation of environmental effects. Steam Audio can also use supported GPUs for accelerating simulation and rendering of environmental effects, while ensuring that its audio workloads do not adversely impact other GPU workloads or frame rate stability.

-  **Balance physical realism and artistic intent.** Steam Audio lets designers modify the results of physics-based simulation before they are used for rendering audio effects. This lets designers treat the simulation results as a high-quality starting point, while providing many ways to tweak the results to achieve their artistic goals. For example, designers can adjust the material-dependent EQ filters used to model transmission of sound through geometry, or make a reverb tail longer or shorter while retaining the characteristics of the early reflections.

This document describes the Steam Audio SDK, which is a C library that you can use to add spatial audio capabilities to any application. For example, if you want to use Steam Audio with a custom game engine, a custom audio middleware tool, or a Digital Audio Workstation (DAW), the Steam Audio SDK is the right tool for the job. Since the SDK provides a C API, you can use it from any programming language that can call functions in external libraries, such as C/C++, C#, Java, Rust, and many more.

If you are using Unity as your game engine, refer to the documentation for the Steam Audio Unity integration.

If you are using Unreal Engine 4 as your game engine, refer to the documentation for the Steam Audio Unreal Engine integration.

If you are using FMOD Studio as your audio middleware, refer to the documentation for the Steam Audio FMOD Studio integration.
