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

(Note: You need to have eather Standard Build or DEV Build compiled alrighty and make sure that you haven't compiled it with ogv files inside "videos")

This combines the build files into one single standalone file that can be opened without a server 

(Good option for a Chromebook):

```bash
# Compile Standard Single File (Standard)
node compile.js build-web

# Generate DEV Single File (DEV)
node compile.js build_web_DEV
```

**Outputs**:
*   `RSDKv3_SingleFile.html` (~259.3MB)
*   `RSDKv3_DEV_SingleFile.html` (~259.3MB)

*Note: t̶h̶a̶t̶ ̶i̶t̶s̶ ̶g̶o̶n̶n̶a̶ ̶t̶a̶k̶e̶ ̶a̶ ̶l̶o̶n̶g̶ ̶t̶i̶m̶e̶ ̶t̶o̶ ̶l̶o̶a̶d̶ ̶t̶h̶e̶s̶e̶ ̶i̶n̶ ̶d̶u̶e̶ ̶t̶o̶ ̶l̶a̶r̶g̶e̶ ̶s̶i̶z̶e̶ ̶a̶d̶d̶e̶d̶ ̶i̶n̶ ̶b̶y̶ ̶t̶h̶e̶ ̶v̶i̶d̶e̶o̶s̶.̶ ̶i̶f̶ ̶y̶o̶u̶ ̶w̶a̶n̶t̶e̶d̶ ̶t̶h̶e̶m̶ ̶i̶n̶ ̶s̶m̶a̶l̶l̶e̶r̶ ̶s̶i̶z̶e̶ ̶a̶n̶d̶ ̶f̶a̶s̶t̶e̶r̶ ̶l̶o̶a̶d̶ ̶t̶i̶m̶e̶,̶ ̶r̶e̶m̶o̶v̶e̶ ̶t̶h̶e̶ ̶v̶i̶d̶e̶o̶s̶ ̶o̶r̶ ̶r̶e̶n̶a̶m̶e̶ ̶t̶h̶e̶ ̶"̶v̶i̶d̶e̶o̶s̶"̶ ̶f̶o̶l̶d̶e̶r̶ ̶t̶o̶ ̶s̶o̶m̶e̶t̶h̶i̶n̶g̶ ̶e̶l̶s̶e̶.̶ Edited: This is caused by having ogv file compiled inside "RSDKv3.data"*

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
