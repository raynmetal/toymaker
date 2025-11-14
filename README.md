# ToyMaker Project

## Project Description

### Introduction

ToyMaker is an open source game engine developed and maintained by Zoheb Shujauddin.

### Documentation

See [raynmetal/game-of-ur's toymaker-fork tag](https://github.com/raynmetal/game-of-ur/releases/tag/toymaker-fork) which holds the commit history leading up to the splitting of the engine project from the game project.

Documentation for this game and its engine is available on this project's [github pages](https://raynmetal.github.io/toymaker/index.html).

### Motivation

I spent 2023-2024 studying C++, OpenGL, SDL, and 3D graphics by following the tutorials on [learncpp](https://www.learncpp.com/), [Lazy Foo](https://lazyfoo.net), and [Learn OpenGL](https://learnopengl.com/) among others.  Having written [Game of Ur](https://www.github.com/raynmetal/game-of-ur) with the game engine developed using my learnings, I wish to split ToyMaker off into its own project to continue to refine it further.

## Installation

Note that at the moment the project is only available on Windows.  I have not yet attempted to run or build it on any other platform.

### Building from source

#### Requirements

> [!NOTE]
> This application has not been tested with anything other than the Windows MinGW package available via MSYS2's package manager.

This project uses [CMake](https://cmake.org/) for its build system, so make sure to have that installed.

On your platform, download the following packages and place them somewhere discoverable by your compiler toolchain.

- [MinGW-w64](https://www.mingw-w64.org/) -- For the C++ standard library headers, and for the compiler toolchain for building native Windows applications.

- [SDL](https://www.libsdl.org/) -- For abstracting away platform specific tasks, like requesting a window for the application.

- [SDL Image](https://github.com/libsdl-org/SDL_image) -- For loading of images in various formats.

- [SDL TTF](https://github.com/libsdl-org/SDL_ttf) -- For loading and rendering fonts.

- [GLEW](https://github.com/nigels-com/glew) -- For exposing OpenGL functionality available on this platform.

- [Nlohmann JSON](https://json.nlohmann.me/) -- For serialization/deserialization of data to and from JSON.

- [GLM](https://github.com/g-truc/glm) -- For linear algebra functions resembling those in GLSL, and for quaternion math.

- [Assimp](https://github.com/assimp/assimp) -- For importing assets of various kinds, mainly 3D models.

If you'd like to generate and tinker with the documentation generated for the project, also install [Doxygen](https://www.doxygen.nl/).

Finally, [clone this repository,](https://github.com/raynmetal/toymaker) or download its snapshot.

#### Compiling

WIP

## Goals

WIP

## Contributing

I'm not planning to accept contributions to this project any time soon.  Feel free to fork the project and make it your own, though!

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
