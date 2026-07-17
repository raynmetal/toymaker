# ToyMaker

## About

**ToyMaker is an open source, extensible, single-threaded 3D game engine written in C++ on top of [SDL3](https://wiki.libsdl.org/SDL3/FrontPage)
and [OpenGL.](https://www.opengl.org/)**  Scene data is described in JSON and managed through a combination of ECS and scene
tree APIs.  Demo projects using ToyMaker have been compiled successfully for Windows and Linux.

ToyMaker is currently in very early alpha; all APIs seen here today are subject to change until an eventual 1.0 release.  It
also does _not_ yet feature a level editor.

### Examples

![Fill Container 1](https://media3.giphy.com/media/v1.Y2lkPTc5MGI3NjExdGpmMGI1bWF0eHVpanplMGVmcXdyc2cybHpidDB0Z2l1eXQ4MmdxeiZlcD12MV9pbnRlcm5hbF9naWZfYnlfaWQmY3Q9Zw/HLqkqke2YgdZUamEdI/giphy.gif)

![Fill Container 2](https://media2.giphy.com/media/v1.Y2lkPTc5MGI3NjExZnVsNzU3c2QyOWtkamUxemNyOGR3Mml1YzJjMjRlaWFiZnB5ZzF1NiZlcD12MV9pbnRlcm5hbF9naWZfYnlfaWQmY3Q9Zw/YzVg8ozl9jqvc6I6c3/giphy.gif)

![Fill Container 3](https://media3.giphy.com/media/v1.Y2lkPTc5MGI3NjExaXI0cXhwc29zOWVjZDA4d3l6MHhvd3hndTQxMDl4c2kwa3d1Z2h4dCZlcD12MV9pbnRlcm5hbF9naWZfYnlfaWQmY3Q9Zw/1k08fjqesbcxVYGpgX/giphy.gif)

`Examples/FillContainer` -- shows spheres spawning at a steady rate filling an invisible container, with several collision checks
and corrections taking place every frame.

![Slope Friction](https://media3.giphy.com/media/v1.Y2lkPTc5MGI3NjExbXZqZnZlZ2ZqNXYzeWsxZTNncDlrZGduOTBoYmNqd2E0NW5ib200cSZlcD12MV9pbnRlcm5hbF9naWZfYnlfaWQmY3Q9Zw/hB2tfGISPc9946Vr0v/giphy.gif)

`Examples/SlopeFriction` -- demos friction forces causing a moving block to slow and come to a stop sliding down a gentle
incline.

![Many Lights](https://media3.giphy.com/media/v1.Y2lkPTc5MGI3NjExdTQ5MXlxMHprZzl3OGRmZnJvc2xuZzJvaGNsMWs3c201dGYycThmMyZlcD12MV9pbnRlcm5hbF9naWZfYnlfaWQmY3Q9Zw/hUsAyQAKHBP7KFUr2I/giphy.gif)

`Examples/ManyLights` -- scene containing 441 point lights and 441 models (2 meshes each) rendered with instancing for both.

![One One Collision](https://media4.giphy.com/media/v1.Y2lkPTc5MGI3NjExemZlYTg0enp6Y2ZudzVpZHozanV3NHQ5cm0xNXBsaTB0N3RtY3hjZCZlcD12MV9pbnRlcm5hbF9naWZfYnlfaWQmY3Q9Zw/4m0mQ5owt4ez6mrmJ2/giphy.gif)

`Examples/LeftRightCollision` -- shows single one-to-one collision between two spheres.

![Interactive Center Text](https://media0.giphy.com/media/v1.Y2lkPTc5MGI3NjExbDJ1aTl3Y3dzMGZ4NGx5d2YzeHJtNzV6cTIycHJhZG96aTl0YTllOSZlcD12MV9pbnRlcm5hbF9naWZfYnlfaWQmY3Q9Zw/MLEgE7qNc53t5HznIb/giphy.gif)

`Examples/CenterText` -- demo featuring clickable text that spins on interaction.

### Features

- Rendering using Blinn-Phong lighting in a fixed deferred shading pipeline.

    - Supports basic key-value based [materials](https://raynmetal.github.io/toymaker/classToyMaker_1_1Material.html), where keys
    are declared by [ToyMaker::BaseRenderStage implementations](https://raynmetal.github.io/toymaker/classToyMaker_1_1BaseRenderStage.html)
    during initialization.

- Scene data organized and updated via the engine's own single-threaded [Entity-Component-System implementation.](https://raynmetal.github.io/toymaker/md_docs_2toymaker-engine_2core_2ecs__system.html)

- Extensible resource loading, tracking, and serialization through [ToyMaker's Resource Database](https://raynmetal.github.io/toymaker/group__ToyMakerResourceDB.html)

- [Scene management](https://raynmetal.github.io/toymaker/md_docs_2toymaker-engine_2scene__system.html) through its scene-tree style
API with JSON serialization.  Additionally supports:

    - Multiple viewports through [ToyMaker::ViewportNode](https://raynmetal.github.io/toymaker/classToyMaker_1_1ViewportNode.html), where
    each viewport may share its parent's [ECS "World"](https://raynmetal.github.io/toymaker/classToyMaker_1_1ECSWorld.html) or create one
    of its own.

    - Object-oriented, component based application logic scripting, a la many popular engines, through [ToyMaker Aspects](https://raynmetal.github.io/toymaker/classToyMaker_1_1SimObjectAspect.html)

- Input abstraction, management, and serialization via the engine's [input system.](https://raynmetal.github.io/toymaker/md_docs_2toymaker-engine_2input__system.html)

    - Input events are buffered on receipt, and only sent to listeners when the input's timestamp matches the simulation window
    currently being processed by the engine.  This ensures that user inputs are processed predictably even during update compute lags.

> [!NOTE]
> Currently requires knowledge of [SDL's input event definitions](https://github.com/libsdl-org/SDL/blob/main/include/SDL3/SDL_events.h)
> to set bindings correctly.

- Observer pattern implementation in the form of [ToyMaker Signals](https://raynmetal.github.io/toymaker/group__ToyMakerSignals.html),
allowing decoupling of event data publishers and consumers.

- A [spatial query system](https://raynmetal.github.io/toymaker/group__ToyMakerSpatialQuerySystem.html) using octrees for fast retrieval
of scene entities based on spatial parameters (e.g., ray casts, bounding volume collisions, etc.)

- A [physics system](https://raynmetal.github.io/toymaker/md_docs_2toymaker-engine_2physics__system.html) using an implementation
of [Extended Position-Based Dynamics(XPBD)](https://matthias-research.github.io/pages/publications/PBDBodies.pdf) with support for
collisions between capsules, spheres, and cuboids.

    - In addition to the spatial query system's octree, it uses [Sweep and Prune](https://raynmetal.github.io/toymaker/classToyMaker_1_1SweepPrune.html)
    to accelerate broad phase collision checks per physics substep.

### Motivation

I spent 2023-2024 studying C++, OpenGL, SDL, and 3D graphics by following the tutorials on [learncpp](https://www.learncpp.com/), [Lazy Foo](https://lazyfoo.net),
and [Learn OpenGL](https://learnopengl.com/) among others.  I used my learnings to write [Game of Ur](https://github.com/raynmetal/game-of-ur),
then split this engine out from it.  The goal now is to take this as far as I can while learning as much as I can along the way.

### Documentation

See [raynmetal/game-of-ur's toymaker-fork tag](https://github.com/raynmetal/game-of-ur/releases/tag/toymaker-fork) which holds the commit
history leading up to the splitting of the engine project from the game project.

Documentation for the engine is available on this project's [github pages.](https://raynmetal.github.io/toymaker/index.html)

Open, ongoing, and completed tasks and issues are tracked on this project's [Trello board.](https://trello.com/b/ALP2KjNp/toymaker-engine)

## Building & Running

### Requirements

This project uses [CMake](https://cmake.org/) for its build system, so make sure to have that installed.

On your platform, download the following packages and place them somewhere discoverable by your compiler toolchain.  This project
has been successfully compiled with [MinGW-w64](https://www.mingw-w64.org/) on Windows, and [clang](https://clang.llvm.org/) on
Linux.

- [SDL3](https://www.libsdl.org/) -- For abstracting away platform specific tasks, like requesting a window for the application.

- [SDL3 Image](https://github.com/libsdl-org/SDL_image) -- For loading of images in various formats.

- [SDL3 TTF](https://github.com/libsdl-org/SDL_ttf) -- For loading and rendering fonts.

- [GLEW](https://github.com/nigels-com/glew) -- For exposing OpenGL functionality available on this platform.

- [Nlohmann JSON](https://json.nlohmann.me/) -- For serialization/deserialization of data to and from JSON.

- [GLM](https://github.com/g-truc/glm) -- For linear algebra functions resembling those in GLSL, and for quaternion math.

- [Assimp](https://github.com/assimp/assimp) -- For importing assets of various kinds, mainly 3D models.

- [Doctest](https://github.com/doctest/doctest/) -- For building and running tests.

If you'd like to generate and tinker with the documentation generated for the project, also install [Doxygen](https://www.doxygen.nl/).

### Presets

The project is bundled with a few CMake presets used with `cmake --preset ${PRESET}`

- `Examples_FullBuild_Release` -- Compiles all example projects under `Examples/` _and_
the ToyMaker library, with optimizations enabled and debug symbols disabled.

- `Library_Release` -- Compiles only the engine library using sources found in
`ToyMaker_Main/` and `ToyMaker_Builtins/`, with optimizations enabled and debug symbols
disabled.

- `Examples_AppBuild_Release` -- Compiles a release version of the projects under
`Examples/`, linking them to ToyMaker binaries already installed in the local system.

Replacing `Release` with `Debug` in any of the above enables the generation of debug
symbols and suppresses many compiler optimizations.

### Building

First, clone this repository.

```bash
git clone https://github.com/raynmetal/toymaker.git
# OR, using ssh
git clone git@github.com:raynmetal/toymaker.git
```

Create a directory for storing files generated during the build process.  The instructions here assume that the
build directory is `../toymaker_build/` relative to the project root.

```bash
# on Linux and Mac
mkdir toymaker_build
```

Enter the project directory and generate the build configuration using CMake.  The instructions here use
the `FullBuild_Examples` preset (see the Presets section), that generates binaries both for the ToyMaker
library as well as the example projects under `Examples/`.

> [!IMPORTANT]
> For now, the `toymaker_configure_executable` CMake script picks up created and removed
> data files (JSON scene descriptions, images, models, etc.) only when this command is run.
>
> Be sure to rerun it whenever making such a change if you use ToyMaker in a project of your own, or
> when attempting to modify any of the examples.

```bash
cd toymaker
cmake --preset Examples_FullBuild_Debug -B ../toymaker_build
```

Finally head over to the project directory and actually build the project.

```bash
cd ../toymaker_build
cmake --build .
```

### Running Examples

Starting in the build directory root, navigate to the directory containing the demo you would like to
run (say, FillContainer).

```bash
cd Examples/FillContainer
```

Then simply run the generated executable (likely prefixed with `Example_`).

```bash
./Example_FillContainer
```

> [!WARNING]
> This command will not work if you try running it while in a different directory from the executable, as in:
>
> ```bash
> toymaker_build/Examples/FillContainer/Example_FillContainer
> ```
>
> Running the above will lead to an immediate crash, as the program won't be able to find the data files it expects
> to find in the directory where the command was invoked.  This is an issue and will be addressed in a future update.

### Running Tests

Tests are compiled along with the rest of the library when ToyMaker is itself the project being built (as opposed
to when it is a vendored library in another project).  The test sources are found under `tests/` under the project
root.

To run them, go to the root of the build directory, then invoke:

```bash
tests/TestDriver
```

ToyMaker uses [Doctest](https://github.com/doctest/doctest) for testing.  See instructions there to learn how to run
tests selectively, or with options besides the default.

## Contributing

I'm not planning to accept contributions to this project any time soon.  Feel free to fork it and make it your own,
though!

## LICENSE

[raynmetal/toymaker](https://github.com/raynmetal/toymaker) is distributed under the terms of the [MIT License](LICENSE.txt).

This program makes extensive use of the following libraries:

- [SDL](https://www.libsdl.org/)

- [SDL Image](https://github.com/libsdl-org/SDL_image)

- [SDL TTF](https://github.com/libsdl-org/SDL_ttf)

- [GLEW](https://github.com/nigels-com/glew)

- [Nlohmann JSON](https://json.nlohmann.me/)

- [GLM](https://github.com/g-truc/glm)

- [Assimp](https://github.com/assimp/assimp)

