# CrossCountryTimer

![icon](data/crosscountrytimer.png)

The cross country timer app built with rayGui

---

## Build

Requires CMake 3.22+, C23 compiler(gcc 16), and (by default) a C++ compiler for whisper.cpp.

`third_party/raylib` and `third_party/whisper.cpp` are git submodules. After a clone:

```bash
git submodule update --init --recursive
```

Or clone with them already filled in:

```bash
git clone --recurse-submodules https://github.com/chimbiwide/CrossCountryTimer.git
```

On Debian/Ubuntu, raylib also needs X11 and OpenGL headers:

```bash
sudo apt install build-essential cmake git \
    libgl1-mesa-dev libx11-dev libxcursor-dev libxinerama-dev \
    libxrandr-dev libxi-dev libxext-dev libasound2-dev
```

Configure and compile from the repo root:

```bash
cmake -S . -B build
cmake --build build
```

The binary lands at `build/CrossCountryTimer`:

```bash
./build/CrossCountryTimer
```

Run it from the repo root so it can find `data/Google-Sans-Mono-Regular.ttf`.

### Windows (MinGW)

Needs CMake 3.22+, MinGW gcc/g++, and `mingw32-make` on `PATH`. There is no Visual Studio project. A plain `cmake -S . -B build` picks NMake and stops, because that generator wants the Visual Studio compiler.

From the repo root, in PowerShell:

```powershell
cmake --preset windows
cmake --build --preset windows -j
```

The preset is a Release build and writes `build\CrossCountryTimer.exe`. Start it from the repo root:

```powershell
.\build\CrossCountryTimer.exe
```

MinGW's runtime DLLs (`libgcc_s_seh-1.dll`, `libstdc++-6.dll`, `libwinpthread-1.dll`) have to be on `PATH`. It will work if launched from a terminal where `gcc` is available.


Compile without whisper:
```powershell
cmake --preset windows -DCCT_WITH_WHISPER=OFF
cmake --build --preset windows -j
```

Default build type on Linux is Debug. For a Release build:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

whisper.cpp is linked in by default (`CCT_WITH_WHISPER=ON`) and is the slow part of the compile. To skip it:

```bash
cmake -S . -B build -DCCT_WITH_WHISPER=OFF
cmake --build build
```

---

### TO-DO

1. Temperature
2. Wind
3. Precipitation
4. clouds
5. records

---

### Division

- Freshman
- Varsity
- JV 
- Middleman
