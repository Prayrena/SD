# SD Engine Projects

This repository is a monorepo for a custom C++ game engine and the projects that use it. The engine lives in `Engine/`, and each game or test project lives beside it so the existing Visual Studio project references continue to work.

## Repository Layout

```text
SD/
  Engine/
  DFS2/
  Doomenstein/
  Libra/
  MathUnitTests/
  MathVisualTests/
  ModelViewer/
  SimpleMiner/
  Starship/
  Thesis/
  Vaporum/
```

Each project expects the shared engine at this sibling path:

```text
..\Engine\Code\Engine\Engine.vcxproj
```

Keep this folder structure when cloning or moving the repository.

## Requirements

- Windows
- Visual Studio 2022 or newer
- MSVC toolset compatible with the project files
- Git LFS for large art, audio, font, and model assets

## Opening A Project

Open the solution for the game or test project you want to work on, for example:

```text
SimpleMiner\SimpleMiner.sln
DFS2\DFS2.sln
MathUnitTests\MathUnitTests.sln
```

The solution includes the game project and references the shared engine project in `Engine/`.

## GitHub Publishing Notes

Start this repository as private until third-party code, DLLs, music, textures, models, and other assets have been audited for redistribution rights.

Build outputs, Visual Studio caches, generated CMake files, and game executables are ignored. Required source files and runtime data assets should stay in the repository, with large binary assets stored through Git LFS.

## Future Split Option

If the engine and games later need independent public histories or releases, split them into separate repositories and include the engine in each game through a Git submodule or package-style dependency. For now, the monorepo keeps local development simple and matches the current project references.
