# CrossCountryTimer

## Build

Needs CMake 3.22+, a C11 compiler, and (by default) a C++ compiler for whisper.cpp.

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

Default build type is Debug. For a Release build:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

whisper.cpp is linked in by default (`CCT_WITH_WHISPER=ON`) and is the slow part of the compile. To skip it:

```bash
cmake -S . -B build -DCCT_WITH_WHISPER=OFF
cmake --build build
```
