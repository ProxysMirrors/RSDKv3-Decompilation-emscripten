if(NOT EMSCRIPTEN)
    message(FATAL_ERROR "platforms/Emscripten.cmake requires the Emscripten toolchain")
endif()

add_executable(RetroEngine ${RETRO_FILES})

# Web target uses the SDL2 port from Emscripten.
set(RETRO_SDL_VERSION "2")

# Emscripten specific flags
target_compile_options(RetroEngine PRIVATE 
    -O3
    -sUSE_SDL=2
    -sUSE_VORBIS=1
    -sUSE_OGG=1
)

target_link_options(RetroEngine PRIVATE
    -O3
    -sUSE_SDL=2
    -sUSE_VORBIS=1
    -sUSE_OGG=1
    -sLEGACY_GL_EMULATION=1
    -lidbfs.js
    -sALLOW_MEMORY_GROWTH=1
    -sMAXIMUM_MEMORY=2GB
    -sMALLOC=emmalloc
    -sMODULARIZE=0
    -sEXPORT_NAME="Module"
    -sEXPORT_ALL=0
    -sEXPORTED_FUNCTIONS=_SetWebStorageInitialised,_TryResumeAudioDevice,_RefreshWindow,_main,_malloc,_free,_SDL_UpdateTexture
    -sEXPORTED_RUNTIME_METHODS=ccall,cwrap,UTF8ToString,FS,HEAPU8
    --shell-file ${CMAKE_SOURCE_DIR}/platforms/web/shell.html
    "SHELL:--preload-file ${CMAKE_SOURCE_DIR}/Data.rsdk@/Data.rsdk"
    "SHELL:--preload-file ${CMAKE_SOURCE_DIR}/videos@videos"
)

set_target_properties(RetroEngine PROPERTIES
    SUFFIX ".html"
)
