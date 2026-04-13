# RSDKv3 Web Build Guide

This guide covers how to build the Retro Engine v3 for the web using Emscripten, including the special single-file "serverless" HTML versions.

## 1. Prerequisites

*   **Emscripten SDK (emsdk)**: Must be installed and configured in the `emsdk/` directory.
*   **Node.js**: Required for the single-file compilation script.
*   **Assets**: 
    *   Place `Data.rsdk` in the project root.
    *   Place your `videos/` folder in the project root.

## 2. Environment Setup

Before running any build commands, you must source the Emscripten environment:

```bash
source ./emsdk/emsdk_env.sh
```

## 3. Standard Build (Production)

This build is optimized for performance and has all debug features disabled.

```bash
# Create directory if it doesn't exist
mkdir -p build-web
cd build-web

# Configure and Build
emcmake cmake .. -DPLATFORM=Emscripten -DRETRO_DEBUG=OFF
emmake make -j$(nproc)
```

**Output**: `build-web/RSDKv3.html`, `.js`, `.wasm`, and `.data`.

## 4. DEV Build (Debugging)

This build enables the Dev Menu, Hitbox toggles, and high-speed fast forward.

```bash
# Create directory if it doesn't exist
mkdir -p build_web_DEV
cd build_web_DEV

# Configure and Build
emcmake cmake .. -DPLATFORM=Emscripten -DRETRO_DEBUG=ON
emmake make -j$(nproc)
```

**Output**: `build_web_DEV/RSDKv3.html`, `.js`, `.wasm`, and `.data`.

## 5. Single-File HTML Generation

To combine the `.html`, `.js`, `.wasm`, and `.data` files into one single standalone file that can be opened without a web server:

```bash
# Generate Standard Single File
node compile.js build-web

# Generate DEV Single File
node compile.js build_web_DEV
```

**Outputs**:
*   `RSDKv3_SingleFile.html` (~800MB)
*   `RSDKv3_DEV_SingleFile.html` (~800MB)

*Note: These files are very large. When opening them, please wait 5-10 seconds for the browser to reassemble the data in memory.*

## 6. Debug Controls (DEV Build Only)

| Key | Action |
|-----|--------|
| **ESC** | Open Dev Menu / Level Select |
| **F9** | Cycle Hitbox/Collision Overlay |
| **F10** | Toggle Palette Overlay |
| **F11** | Frame Step (while paused) |
| **F12** | Master Pause |
| **D** | Toggle Item Placement Mode (Debug Mode) |
| **Backspace** | Fast Forward (Hold) |

## 7. Video Soundtrack Support

The engine supports multi-track audio via file suffixes. If you want specific videos for different soundtrack settings, name them as follows in your `videos/` folder:

*   **JP Soundtrack**: `Opening.mp4` (default)
*   **US Soundtrack**: `Opening_US.mp4`
