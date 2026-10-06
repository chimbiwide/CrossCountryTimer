# CrossCountryTimer

![icon](data/crosscountrytimer.png)

The cross country timer app built with rayGui

---

## Build

Requires CMake 3.22+, C23 compiler(gcc 16), and (by default) a C++ compiler for whisper.cpp.

`third_party/raylib`, `third_party/SDL`, `third_party/whisper.cpp`, `third_party/xlsxio`, `third_party/zlib`, and `third_party/expat` are git submodules. After a clone:

```bash
git submodule update --init --recursive
```

Or clone with them already filled in:

```bash
git clone --recurse-submodules https://github.com/chimbiwide/CrossCountryTimer.git
```

.xlsx reading and writing is built in from xlsxio. zlib, minizip, and expat are compiled with the app and linked statically.

raylib runs on its SDL3 backend, which is what makes gamepad rumble work. SDL3 (`third_party/SDL`) is compiled with the app and linked statically.

On Debian/Ubuntu, raylib and SDL also need X11, OpenGL, and udev headers:

```bash
sudo apt install build-essential cmake git \
    libgl1-mesa-dev libx11-dev libxcursor-dev libxinerama-dev \
    libxrandr-dev libxi-dev libxext-dev libxfixes-dev libasound2-dev \
    libudev-dev
```

On Fedora:

```bash
sudo dnf install gcc gcc-c++ cmake git \
    mesa-libGL-devel libX11-devel libXcursor-devel libXinerama-devel \
    libXrandr-devel libXi-devel libXext-devel libXfixes-devel alsa-lib-devel \
    systemd-devel
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

The SDL3 backend is the default (`CCT_WITH_SDL=ON`). To build on raylib's GLFW backend instead, which has no gamepad rumble:

```bash
cmake -S . -B build -DCCT_WITH_SDL=OFF
cmake --build build
```

---

### Division

- Freshman
- Varsity
- JV 
- Middleman

New column for divsion: F, V, J, M

---

### TODO

1. Wind direction (N,S,W,E...)
2. Make wind a textbok (15G20)
3. Scrolling function broken on windows
4. Scoring teams (vertical team scoring + horizonal scoreing)
5. infitine teams
6. record loading
