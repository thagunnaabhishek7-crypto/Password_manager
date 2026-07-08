# PS Manager — SFML Build Setup

This project uses **SFML 3.1.0** and **CMake**. SFML is fetched automatically at build time via CMake's `FetchContent` — you do **not** need to manually download or install SFML itself. You only need a compiler, CMake, and (on Linux) a handful of system libraries that SFML needs to compile.

Follow the section for your OS.

---

## Windows (VS Code + CMake Tools)

VS Code doesn't come with a C++ compiler, so you still need Visual Studio's *Build Tools* installed underneath it — you just won't open the Visual Studio IDE itself day-to-day.

### 1. Install Visual Studio Build Tools (compiler only, not the full IDE)
Download from https://visualstudio.microsoft.com/downloads/ → scroll to **"Tools for Visual Studio"** → **Build Tools for Visual Studio**.
During install, check **"Desktop development with C++"**. This gives you MSVC, the Windows SDK, and CMake/Ninja under the hood.

> ⚠️ **Use MSVC, not MinGW.** Mixing MinGW with SFML's fetched binaries can cause missing-DLL / ABI issues. Stick to the MSVC toolchain installed above.

### 2. Install Git
Download from https://git-scm.com/download/win.

### 3. Install VS Code
Download from https://code.visualstudio.com/.

### 4. Install VS Code extensions
Open VS Code → Extensions (`Ctrl+Shift+X`) → install:
- **C/C++** (Microsoft)
- **CMake Tools** (Microsoft)

### 5. Clone the repo
Open a terminal (or VS Code's integrated terminal, `` Ctrl+` ``):
```powershell
git clone <REPO_URL>
cd PS_MANAGER
code .
```
The last command opens the folder in VS Code.

### 6. Select a kit (compiler)
Press `Ctrl+Shift+P` → **CMake: Select a Kit** → choose the **Visual Studio Build Tools 2022 - amd64** entry (or similar, listing MSVC). If nothing shows up, press `Ctrl+Shift+P` → **CMake: Scan for Kits** first.

### 7. Configure
Press `Ctrl+Shift+P` → **CMake: Configure**.
This triggers `FetchContent` to download and build SFML — first run takes a few minutes, watch the **Output** panel (select "CMake/Build" from the dropdown) for progress or errors.

### 8. Build
Press `Ctrl+Shift+P` → **CMake: Build**, or click **Build** in the blue status bar at the bottom.

### 9. Run it
Click **Run** in the status bar, or:
```powershell
.\build\bin\ps_manager.exe
```

### Command-line equivalent (if you prefer not to click through the UI)
```powershell
cmake -B build -S .
cmake --build build
.\build\bin\ps_manager.exe
```

---

## Fedora Linux

### 1. Install build tools and dependencies
```bash
sudo dnf install -y gcc-c++ cmake git
sudo dnf install -y libX11-devel libXrandr-devel libXcursor-devel libXi-devel \
    systemd-devel freetype-devel flac-devel libvorbis-devel \
    mesa-libGL-devel mesa-libEGL-devel mbedtls-devel libssh2-devel openssl-devel
```

### 2. Clone the repo
```bash
git clone <REPO_URL>
cd PS_MANAGER
```

### 3. Configure and build
```bash
cmake -B build -S .
cmake --build build
```

### 4. Run it
```bash
./build/bin/ps_manager
```

---

## Ubuntu / Debian Linux

### 1. Install build tools and dependencies
```bash
sudo apt update
sudo apt install -y g++ cmake git pkg-config
sudo apt install -y libxrandr-dev libxcursor-dev libxi-dev libudev-dev \
    libfreetype-dev libflac-dev libvorbis-dev libgl1-mesa-dev libegl1-mesa-dev \
    libmbedtls-dev libssh2-1-dev libssl-dev
```

### 2. Clone the repo
```bash
git clone <REPO_URL>
cd PS_MANAGER
```

### 3. Configure and build
```bash
cmake -B build -S .
cmake --build build
```

### 4. Run it
```bash
./build/bin/ps_manager
```

---

## macOS

### 1. Install Xcode Command Line Tools
```bash
xcode-select --install
```

### 2. Install CMake and Git (via Homebrew)
```bash
brew install cmake git
```

### 3. Clone the repo
```bash
git clone <REPO_URL>
cd PS_MANAGER
```

### 4. Configure and build
```bash
cmake -B build -S .
cmake --build build
```

### 5. Run it
```bash
./build/bin/ps_manager
```

---

## How you'll know it worked

Running the executable should open a window. If you see a build error, check:

- **"Could NOT find X"** during `cmake -B build -S .` → a system dependency is missing (Linux only). Install the missing `-devel`/`-dev` package it names and re-run.
- **"undefined reference to `main`"** during build → one of the `.cpp` files listed in `CMakeLists.txt`'s `add_executable()` doesn't actually contain a `main()` function.
- **No error, but no `build/bin/` folder** → the build didn't actually complete; re-run `cmake --build build` and read the full output for the first error.

---

## Notes

- Don't commit the `build/` folder — it's machine- and OS-specific and is already excluded via `.gitignore`.
- SFML version is pinned in the root `CMakeLists.txt` (`GIT_TAG 3.1.0`). Don't change this without syncing with the team, since mismatched SFML versions across machines can cause subtle bugs.
- If you add new `.cpp` files to the project, remember to add them to the `add_executable(...)` list in `CMakeLists.txt`, or they won't be compiled.
