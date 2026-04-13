# RSDKv3 Web Build Guide

This shows how to build the RSDKv3 web version

## 1. Prerequisites

*   **[Emscripten SDK (emsdk)](https://github.com/emscripten-core/emsdk)**: Must be installed. https://github.com/emscripten-core/emsdk
*   **[Node.js](https://nodejs.org/)**: Required for the single-file compilation script.
*   **Assets**: 
    *   You will need the `Data.rsdk` placed in the root. (Origins version unsupported)
    *   And also the videos placed in the `videos/` folder.

## 2. Environment Setup (Ignore If alrighty have emsdk installed and sourced)

Before running any commands listed, you need source the emsdk environment:

```bash
source ./emsdk/emsdk_env.sh
```

## 3. Standard Build (No DEBUG)

This is where its just a normal version of the game. no debug code compiled.

```bash
# Change directory to "build-web"
cd build-web

# Configure it and Build
emcmake cmake .. -DPLATFORM=Emscripten -DRETRO_DEBUG=OFF
emmake make -j$(nproc)
```

**Output**: `build-web/RSDKv3.html`, `.js`, `.wasm`, and `.data`.

## 4. DEV Build (DEBUG)

This enables the Dev Menu selection in the pause menu, Hitbox toggles, fast forward, and pause

```bash
# Change directory to "build-web-DEV"
cd build-web-DEV

# Configure and Build
emcmake cmake .. -DPLATFORM=Emscripten -DRETRO_DEBUG=ON
emmake make -j$(nproc)
```

**Output**: `build-web-DEV/RSDKv3.html`, `.js`, `.wasm`, and `.data`.

## 5. Single-File HTML Generation

(Note: You need to have eather Standard Build or DEV Build compiled alrighty)

This combines the build files into one single standalone file that can be opened without a server 

(Good option for a Chromebook):

```bash
# Compile Standard Single File (Standard)
node compile.js build-web

# Generate DEV Single File (DEV)
node compile.js build_web_DEV
```

**Outputs**:
*   `RSDKv3_SingleFile.html` (~800MB)
*   `RSDKv3_DEV_SingleFile.html` (~800MB)

*Note: that its gonna take a long time to load these in due to large size added in by the videos. if you wanted them in smaller size and faster load time, remove the videos or rename the "videos" folder to something else.*

## 6. Debug Controls

| Key | Action |
|-----|--------|
| **ESC** | Open Dev Menu / Level Select |
| **F9** | Cycle Hitbox/Collision Overlay |
| **F10** | Toggle Palette Overlay |
| **F11** | Frame Step (while paused) |
| **F12** | Master Pause |
| **D** | Toggle Item Placement Mode (Debug Mode) |
| **Backspace** | Fast Forward (Hold) |

## 7. Video Soundtrack US/JP

Due to Chrome requires enabling experimental feature to switch audio tracks depending on which soundtrack you choose. It will have to be like this listed here:

*   **JP Soundtrack**: `Opening.mp4` (default)
*   **US Soundtrack**: `Opening_US.mp4`

Same as the Endings

*    `Bad_Ending.mp4` (default)
*    `Bad_Ending_US.mp4`
*    `Good_Ending.mp4` (default)
*    `Good_Ending_US.mp4`
