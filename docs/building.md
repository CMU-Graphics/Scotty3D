---
layout: default
title: "Building Scotty3D"
nav_order: 3
permalink: /build/
---

# Building Scotty3D

![Ubuntu Build Status](https://github.com/CMU-Graphics/Scotty3D/workflows/Ubuntu/badge.svg) ![MacOS Build Status](https://github.com/CMU-Graphics/Scotty3D/workflows/MacOS/badge.svg) ![Windows Build Status](https://github.com/CMU-Graphics/Scotty3D/workflows/Windows/badge.svg)

To get a copy of the codebase, see [Git Setup](/Scotty3D/git).

Note: the first build on any platform will be very slow, as it must compile most dependencies. Subsequent builds will only need to re-compile your edited Scotty3D code.

### Linux

The following packages (ubuntu/debian) are required, as well as CMake and either gcc or clang:
```
sudo apt install pkg-config libgtk-3-dev libsdl2-dev
```

The version of CMake packaged with apt may be too old (we are using the latest version). If this is the case, you can install the latest version through pip:
```
pip install cmake
export PATH=$PATH:/usr/local/bin
```

Finally, to build the project:
```
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo ..
make -j4
```

The same process should also work modified for your distro/package manager of choice. Note that if you are using Wayland, you may need to set the environment variable ``SDL_VIDEODRIVER=wayland`` when running ``Scotty3D`` for acceptable performance.

Notes:
- You can instead use ``cmake -DCMAKE_BUILD_TYPE=Debug ..`` to build in debug mode, which, while far slower, makes the debugging experience much more intuitive.
- You can replace ``4`` with the number of build processes to run in parallel (set to the number of cores in your machine for maximum utilization).
- If you have both gcc and clang installed and want to build with clang, you should run ``CC=clang CXX=clang++ cmake ..`` instead.

### Windows

The windows build is easiest to set up using the Visual Studio compiler (for now). To get the compiler, download and install the Visual Studio 2019 Build Tools [here](https://visualstudio.microsoft.com/downloads/#build-tools-for-visual-studio-2019). If you want to instead use the full Visual Studio IDE, you can download Visual Studio Community 2019 [here](https://visualstudio.microsoft.com/downloads/). Be sure to install the "Desktop development with C++" component.

You can download CMake for windows [here](https://cmake.org/download/).

Once the Visual Studio compiler (MSVC) is installed, you can access it by running "Developer Command Prompt for VS 2019," which opens a terminal with the utilities in scope. The compiler is called ``cl``. You can also import these utilities in any terminal session by running the script installed at ``C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat``.

We also provide a simple script, ``build_win.bat``, that will automatically import the compiler and build the project. You should be able to simply run it in the project root to build. ``Scotty3D.exe`` will be generated under ``build/RelWithDebInfo/``.

If you want to build manually, the steps (assuming MSVC is in scope) are:
```
mkdir build
cd build
cmake ..
cmake --build . --config RelWithDebInfo
```

You can also use ``--config Debug`` to build in debug mode, which, while far slower, makes the debugging experience much more intuitive. If you swap this, be sure to make a new build directory for it.

Finally, also note that ``cmake ..`` generates a Visual Studio solution file in the current directory. You can open this solution (``Scotty3D.sln``) in Visual Studio itself and use its interface to build, run, and debug the project. (Using the Visual Studio debugger or the provided VSCode launch options for debugging is highly recommended.)

### MacOS

The following packages are required, as well as CMake and clang. You can install them with [homebrew](https://brew.sh/):
```
brew install pkg-config sdl2
```

To build the project:
```
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo ..
make -j4
```

Notes:
- You can instead use ``cmake -DCMAKE_BUILD_TYPE=Debug ..`` to build in debug mode, which, while far slower, makes the debugging experience much more intuitive.
- You can replace ``4`` with the number of build processes to run in parallel (set to the number of cores in your machine for maximum utilization).
