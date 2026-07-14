# ToyMaker Project

## Project Description

### Introduction

ToyMaker is an open source 3D game engine developed and maintained by Zoheb Shujauddin, primarily written in C++ on top of SDL3 and OpenGL.

### Features

- Rendering using [Blinn-Phong lighting](https://en.wikipedia.org/wiki/Blinn%E2%80%93Phong_reflection_model) in a fixed [deferred shading](https://learnopengl.com/advanced-lighting/deferred-shading) pipeline.

    - Supports basic key-value based materials, where keys are declared by [ToyMaker::BaseRenderStage implementations](https://raynmetal.github.io/toymaker/classToyMaker_1_1BaseRenderStage.html) during initialization.

- Scene data organized and updated via the engine's own single-threaded [Entity-Component-System implementation.](https://raynmetal.github.io/toymaker/md_docs_2toymaker-engine_2core_2ecs__system.html)

- Extendable resource loading, tracking, and serialization through [ToyMaker's Resource Database](https://raynmetal.github.io/toymaker/group__ToyMakerResourceDB.html)

- [Scene management](https://raynmetal.github.io/toymaker/md_docs_2toymaker-engine_2scene__system.html) through its scene-tree style API, with JSON serialization.  Additionally
supports:

    - Multiple viewports through [ToyMaker::ViewportNode](https://raynmetal.github.io/toymaker/classToyMaker_1_1ViewportNode.html), where each viewport may share its parent's [ECS "World"](https://raynmetal.github.io/toymaker/classToyMaker_1_1ECSWorld.html) or
    create one of its own.

    - Object-oriented, component based application logic scripting, a la many popular engines, through [ToyMaker Aspects](https://raynmetal.github.io/toymaker/classToyMaker_1_1SimObjectAspect.html)

- Input abstraction, management, and serialization via the engine's [input system.](https://raynmetal.github.io/toymaker/md_docs_2toymaker-engine_2input__system.html)

    - Input events are buffered on receipt, and only sent to listeners when the input's timestamp matches the simulation window currently being processed by the engine.  This ensures
    that user inputs are processed predictably even during update compute lags.

    - note: Currently requires knowledge of [SDL's input event definitions](https://github.com/libsdl-org/SDL/blob/main/include/SDL3/SDL_events.h).

- [Observer pattern](https://en.wikipedia.org/wiki/Observer_pattern) implementation in the form of [ToyMaker Signals](https://raynmetal.github.io/toymaker/group__ToyMakerSignals.html), allowing decoupling event data publishers and consumers.

- A [spatial query system](https://raynmetal.github.io/toymaker/group__ToyMakerSpatialQuerySystem.html) using [Octrees](https://en.wikipedia.org/wiki/Octree) for fast retrieval of scene entities based on spatial parameters (e.g., ray casts,
bounding volume collisions, etc.)

- A [physics system](https://raynmetal.github.io/toymaker/md_docs_2toymaker-engine_2physics__system.html) using an implementation of [Extended Position-Based Dynamics(XPBD)](https://matthias-research.github.io/pages/publications/PBDBodies.pdf)
with support for collisions between capsules, spheres, and cuboids.

    - In addition to the spatial query system's octree, it uses [Sweep and Prune](https://raynmetal.github.io/toymaker/classToyMaker_1_1SweepPrune.html) to accelerate broad phase collision
    checks per
    physics substep.

### Documentation

See [raynmetal/game-of-ur's toymaker-fork tag](https://github.com/raynmetal/game-of-ur/releases/tag/toymaker-fork) which holds the commit history leading up to the splitting of the engine project from the game project.

Documentation the engine is available on this project's [github pages.](https://raynmetal.github.io/toymaker/index.html)

Open, ongoing, and completed tasks and issues are tracked on this project's [Trello board.](https://trello.com/b/ALP2KjNp/toymaker-engine)

### Motivation

I spent 2023-2024 studying C++, OpenGL, SDL, and 3D graphics by following the tutorials on [learncpp](https://www.learncpp.com/), [Lazy Foo](https://lazyfoo.net), and
[Learn OpenGL](https://learnopengl.com/) among others.  I used my learnings to write [Game of Ur](https://github.com/raynmetal/game-of-ur), and later split this engine
out from it.

Now I just want to find out how far I can go with this.

## Installation

### Building from source

#### Requirements

This project uses [CMake](https://cmake.org/) for its build system, so make sure to have that installed.

On your platform, download the following packages and place them somewhere discoverable by your compiler toolchain.  This project has been compiled successfully with [MinGW-w64](https://www.mingw-w64.org/)
on Windows, and [clang](https://clang.llvm.org/) on Linux.

- [SDL3](https://www.libsdl.org/) -- For abstracting away platform specific tasks, like requesting a window for the application.

- [SDL3 Image](https://github.com/libsdl-org/SDL_image) -- For loading of images in various formats.

- [SDL3 TTF](https://github.com/libsdl-org/SDL_ttf) -- For loading and rendering fonts.

- [GLEW](https://github.com/nigels-com/glew) -- For exposing OpenGL functionality available on this platform.

- [Nlohmann JSON](https://json.nlohmann.me/) -- For serialization/deserialization of data to and from JSON.

- [GLM](https://github.com/g-truc/glm) -- For linear algebra functions resembling those in GLSL, and for quaternion math.

- [Assimp](https://github.com/assimp/assimp) -- For importing assets of various kinds, mainly 3D models.

- [Doctest](https://github.com/doctest/doctest/) -- For building and running tests.

If you'd like to generate and tinker with the documentation generated for the project, also install [Doxygen](https://www.doxygen.nl/).

Finally, [clone this repository,](https://github.com/raynmetal/toymaker) or download its snapshot.

#### Compiling

## Goals

## Contributing

I'm not planning to accept contributions to this project any time soon.  Feel free to fork it and make it your own, though!

## LICENSE

raynmetal/toymaker is distributed under the terms of the [MIT License](LICENSE.txt).

This program makes extensive use of the following libraries:

- [SDL](https://www.libsdl.org/)

- [SDL Image](https://github.com/libsdl-org/SDL_image)

- [SDL TTF](https://github.com/libsdl-org/SDL_ttf)

- [GLEW](https://github.com/nigels-com/glew)

- [Nlohmann JSON](https://json.nlohmann.me/)

- [GLM](https://github.com/g-truc/glm)

- [Assimp](https://github.com/assimp/assimp)

