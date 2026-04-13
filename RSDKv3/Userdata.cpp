#include "RetroEngine.hpp"
#if RETRO_PLATFORM == RETRO_WEB
#include <emscripten/emscripten.h>
#endif

#if RETRO_PLATFORM == RETRO_WIN && _MSC_VER
#include <Windows.h>
#include <codecvt>
#include "../dependencies/windows/ValveFileVDF/vdf_parser.hpp"

HKEY hKey;

LONG GetDWORDRegKey(HKEY hKey, const std::wstring &strValueName, DWORD &nValue, DWORD nDefaultValue)
{
    nValue = nDefaultValue;
    DWORD dwBufferSize(sizeof(DWORD));
    DWORD nResult(0);
    LONG nError = ::RegQueryValueExW(hKey, strValueName.c_str(), 0, NULL, reinterpret_cast<LPBYTE>(&nResult), &dwBufferSize);
    if (ERROR_SUCCESS == nError) {
        nValue = nResult;
    }
    return nError;
}

LONG GetStringRegKey(HKEY hKey, const std::wstring &strValueName, std::wstring &strValue, const std::wstring &strDefaultValue)
{
    strValue = strDefaultValue;
    WCHAR szBuffer[512];
    DWORD dwBufferSize = sizeof(szBuffer);
    ULONG nError;
    nError = RegQueryValueExW(hKey, strValueName.c_str(), 0, NULL, (LPBYTE)szBuffer, &dwBufferSize);
    if (ERROR_SUCCESS == nError) {
        strValue = szBuffer;
    }
    return nError;
}

inline std::string utf16ToUtf8(const std::wstring &utf16Str)
{
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> conv;
    return conv.to_bytes(utf16Str);
}

inline bool dirExists(const std::wstring &dirName_in)
{
    DWORD ftyp = GetFileAttributesW(dirName_in.c_str());
    if (ftyp == INVALID_FILE_ATTRIBUTES)
        return false; // something is wrong with your path!

    if (ftyp & FILE_ATTRIBUTE_DIRECTORY)
        return true; // this is a directory!

    return false; // this is not a directory!
}
#endif

int globalVariablesCount;
int globalVariables[GLOBALVAR_COUNT];
char globalVariableNames[GLOBALVAR_COUNT][0x20];

char gamePath[0x100];
int saveRAM[SAVEDATA_SIZE];
Achievement achievements[ACHIEVEMENT_COUNT];
LeaderboardEntry leaderboards[LEADERBOARD_COUNT];

#if RETRO_PLATFORM == RETRO_OSX || RETRO_PLATFORM == RETRO_LINUX
#include <sys/stat.h>
#include <sys/types.h>
#endif

int controlMode              = -1;
bool disableTouchControls    = false;
int disableFocusPause        = 0;
int disableFocusPause_Config = 0;

#if RETRO_USE_MOD_LOADER || !RETRO_USE_ORIGINAL_CODE
bool forceUseScripts        = false;
bool forceUseScripts_Config = false;
#endif

bool useSGame = false;

#if RETRO_PLATFORM == RETRO_WEB
bool webStorageInitialised = false;

void InitWebStorageMount()
{
    EM_ASM({
        if (!Module.rsdkWebStorageMounted) {
            FS.mkdir('/user');
            FS.mount(IDBFS, {}, '/user');
            Module.rsdkWebStorageMounted = true;
            Module.rsdkWebStorageSyncing = false;
            Module.rsdkWebStoragePendingSync = 0; // 0: none, 1: populate(true), 2: sync(false)
        }
        
        var doSync = function(populate) {
            if (Module.rsdkWebStorageSyncing || (typeof FS !== 'undefined' && FS.syncfs && FS.syncfs.inFlight)) {
                // If busy, track the most recent request type
                Module.rsdkWebStoragePendingSync = populate ? 1 : 2;
                return;
            }
            console.log('IDBFS sync requested, populate:', populate);
            Module.rsdkWebStorageSyncing = true;
            FS.syncfs(populate, function(err) {
                Module.rsdkWebStorageSyncing = false;
                if (err) {
                    console.error('IDBFS sync failed', err);
                } else {
                    console.log('IDBFS sync successful, populate:', populate);
                }
                
                if (populate) {
                    _SetWebStorageInitialised();
                }
                
                if (Module.rsdkWebStoragePendingSync) {
                    var nextPopulate = Module.rsdkWebStoragePendingSync === 1;
                    Module.rsdkWebStoragePendingSync = 0;
                    setTimeout(function() { Module.rsdkDoSync(nextPopulate); }, 10);
                }
            });
        };
        Module.rsdkDoSync = doSync;
        
        // Only trigger initial load if we just mounted
        if (Module.rsdkWebStorageMounted && !window._rsdkStorageInitialLoadTriggered) {
            window._rsdkStorageInitialLoadTriggered = true;
            doSync(true);
        }
    });
}

extern "C" {
EMSCRIPTEN_KEEPALIVE void SetWebStorageInitialised()
{
    webStorageInitialised = true;
}
}

void FlushWebStorage()
{
    EM_ASM({
        if (Module.rsdkWebStorageMounted && Module.rsdkDoSync) {
            Module.rsdkDoSync(false);
        }
    });
}
#else
void FlushWebStorage() {}
#endif

void InitUserdata()
{
#if RETRO_PLATFORM == RETRO_WIN && _MSC_VER
    // ... reg keys ...
#endif

#if RETRO_PLATFORM == RETRO_WEB
    InitWebStorageMount();
    sprintf(gamePath, "/user/");
    if (!webStorageInitialised) return;
#else
    sprintf(gamePath, "%s", BASE_PATH);
#endif

    char buffer[0x200];
    sprintf(buffer, "%ssettings.ini", gamePath);
    FileIO *file = fOpen(buffer, "rb");
    IniParser ini;
    if (!file) {
        // ... create default settings ...
        ini = IniParser();
        ini.SetComment("Dev", "DevMenuComment", "Enable this flag to activate dev menu via the ESC key");
        ini.SetBool("Dev", "DevMenu", Engine.devMenu = false);
        ini.SetBool("Dev", "EngineDebugMode", engineDebugMode = false);
        ini.SetBool("Dev", "TxtScripts", forceUseScripts = false);
        forceUseScripts_Config = forceUseScripts;
        ini.SetInteger("Dev", "StartingCategory", Engine.startList = 0);
        ini.SetInteger("Dev", "StartingScene", Engine.startStage = 0);
        ini.SetInteger("Dev", "FastForwardSpeed", Engine.fastForwardSpeed = 8);
        ini.SetBool("Dev", "UseHQModes", Engine.useHQModes = true);
        ini.SetString("Dev", "DataFile", Engine.dataFile);
        StrCopy(Engine.dataFile, "Data.rsdk");

        ini.SetInteger("Game", "Language", Engine.language = RETRO_EN);
        ini.SetInteger("Game", "GameType", Engine.gameTypeID = 0);
        ini.SetInteger("Game", "OriginalControls", controlMode = -1);
        ini.SetBool("Game", "DisableTouchControls", disableTouchControls = false);
        ini.SetInteger("Game", "DisableFocusPause", disableFocusPause = 0);
        disableFocusPause_Config = disableFocusPause;

#if RETRO_USING_SDL2
        ini.SetComment("Keyboard 1", "IK1Comment",
                       "Keyboard Mappings for P1 (Based on: https://wiki.libsdl.org/SDL2/SDLScancodeLookup)");
        ini.SetInteger("Keyboard 1", "Up", inputDevice[INPUT_UP].keyMappings = SDL_SCANCODE_UP);
        ini.SetInteger("Keyboard 1", "Down", inputDevice[INPUT_DOWN].keyMappings = SDL_SCANCODE_DOWN);
        ini.SetInteger("Keyboard 1", "Left", inputDevice[INPUT_LEFT].keyMappings = SDL_SCANCODE_LEFT);
        ini.SetInteger("Keyboard 1", "Right", inputDevice[INPUT_RIGHT].keyMappings = SDL_SCANCODE_RIGHT);
        ini.SetInteger("Keyboard 1", "A", inputDevice[INPUT_BUTTONA].keyMappings = SDL_SCANCODE_Z);
        ini.SetInteger("Keyboard 1", "B", inputDevice[INPUT_BUTTONB].keyMappings = SDL_SCANCODE_X);
        ini.SetInteger("Keyboard 1", "C", inputDevice[INPUT_BUTTONC].keyMappings = SDL_SCANCODE_C);
        ini.SetInteger("Keyboard 1", "Start", inputDevice[INPUT_START].keyMappings = SDL_SCANCODE_RETURN);

        ini.SetInteger("Controller 1", "Up", inputDevice[INPUT_UP].contMappings = SDL_CONTROLLER_BUTTON_DPAD_UP);
        ini.SetInteger("Controller 1", "Down", inputDevice[INPUT_DOWN].contMappings = SDL_CONTROLLER_BUTTON_DPAD_DOWN);
        ini.SetInteger("Controller 1", "Left", inputDevice[INPUT_LEFT].contMappings = SDL_CONTROLLER_BUTTON_DPAD_LEFT);
        ini.SetInteger("Controller 1", "Right", inputDevice[INPUT_RIGHT].contMappings = SDL_CONTROLLER_BUTTON_DPAD_RIGHT);
        ini.SetInteger("Controller 1", "A", inputDevice[INPUT_BUTTONA].contMappings = SDL_CONTROLLER_BUTTON_A);
        ini.SetInteger("Controller 1", "B", inputDevice[INPUT_BUTTONB].contMappings = SDL_CONTROLLER_BUTTON_B);
        ini.SetInteger("Controller 1", "C", inputDevice[INPUT_BUTTONC].contMappings = SDL_CONTROLLER_BUTTON_X);
        ini.SetInteger("Controller 1", "Start", inputDevice[INPUT_START].contMappings = SDL_CONTROLLER_BUTTON_START);
#endif

#if RETRO_USING_SDL1
        ini.SetComment("Keyboard 1", "IK1Comment", "Keyboard Mappings for P1 (Based on: https://www.libsdl.org/release/SDL-1.2.15/docs/html/sdlkey.html)");
        ini.SetInteger("Keyboard 1", "Up", inputDevice[INPUT_UP].keyMappings = SDLK_UP);
        ini.SetInteger("Keyboard 1", "Down", inputDevice[INPUT_DOWN].keyMappings = SDLK_DOWN);
        ini.SetInteger("Keyboard 1", "Left", inputDevice[INPUT_LEFT].keyMappings = SDLK_LEFT);
        ini.SetInteger("Keyboard 1", "Right", inputDevice[INPUT_RIGHT].keyMappings = SDLK_RIGHT);
        ini.SetInteger("Keyboard 1", "A", inputDevice[INPUT_BUTTONA].keyMappings = SDLK_z);
        ini.SetInteger("Keyboard 1", "B", inputDevice[INPUT_BUTTONB].keyMappings = SDLK_x);
        ini.SetInteger("Keyboard 1", "C", inputDevice[INPUT_BUTTONC].keyMappings = SDLK_c);
        ini.SetInteger("Keyboard 1", "Start", inputDevice[INPUT_START].keyMappings = SDLK_RETURN);

        ini.SetInteger("Controller 1", "Up", inputDevice[INPUT_UP].contMappings = 1);
        ini.SetInteger("Controller 1", "Down", inputDevice[INPUT_DOWN].contMappings = 2);
        ini.SetInteger("Controller 1", "Left", inputDevice[INPUT_LEFT].contMappings = 3);
        ini.SetInteger("Controller 1", "Right", inputDevice[INPUT_RIGHT].contMappings = 4);
        ini.SetInteger("Controller 1", "A", inputDevice[INPUT_BUTTONA].contMappings = 5);
        ini.SetInteger("Controller 1", "B", inputDevice[INPUT_BUTTONB].contMappings = 6);
        ini.SetInteger("Controller 1", "C", inputDevice[INPUT_BUTTONC].contMappings = 7);
        ini.SetInteger("Controller 1", "Start", inputDevice[INPUT_START].contMappings = 8);
#endif

        ini.SetBool("Window", "FullScreen", Engine.startFullScreen = DEFAULT_FULLSCREEN);
        ini.SetBool("Window", "Borderless", Engine.borderless = false);
        ini.SetBool("Window", "VSync", Engine.vsync = false);
        ini.SetInteger("Window", "ScalingMode", Engine.scalingMode = 0);
        ini.SetInteger("Window", "WindowScale", Engine.windowScale = 2);
        ini.SetInteger("Window", "ScreenWidth", SCREEN_XSIZE = DEFAULT_SCREEN_XSIZE);
        SCREEN_XSIZE_CONFIG = SCREEN_XSIZE;
        ini.SetInteger("Window", "RefreshRate", Engine.refreshRate = 60);
        ini.SetInteger("Window", "DimLimit", Engine.dimLimit = 300);
        ini.SetBool("Window", "HardwareRenderer", false);

        ini.SetFloat("Audio", "BGMVolume", 1.0f);
        ini.SetFloat("Audio", "SFXVolume", 1.0f);
        bgmVolume = MAX_VOLUME;
        sfxVolume = MAX_VOLUME;

        ini.Write(buffer, false);
    }
    else {
        fClose(file);
        ini = IniParser(buffer, false);

        if (!ini.GetBool("Dev", "DevMenu", &Engine.devMenu)) Engine.devMenu = false;
        if (!ini.GetBool("Dev", "EngineDebugMode", &engineDebugMode)) engineDebugMode = false;
        if (!ini.GetBool("Dev", "TxtScripts", &forceUseScripts)) forceUseScripts = false;
        forceUseScripts_Config = forceUseScripts;
        if (!ini.GetInteger("Dev", "StartingCategory", &Engine.startList)) Engine.startList = 0;
        if (!ini.GetInteger("Dev", "StartingScene", &Engine.startStage)) Engine.startStage = 0;
        if (!ini.GetInteger("Dev", "FastForwardSpeed", &Engine.fastForwardSpeed)) Engine.fastForwardSpeed = 8;
        if (!ini.GetBool("Dev", "UseHQModes", &Engine.useHQModes)) Engine.useHQModes = true;
        if (!ini.GetString("Dev", "DataFile", Engine.dataFile)) StrCopy(Engine.dataFile, "Data.rsdk");

        if (!ini.GetInteger("Game", "Language", &Engine.language)) Engine.language = RETRO_EN;
        if (!ini.GetInteger("Game", "GameType", &Engine.gameTypeID)) Engine.gameTypeID = 0;
        if (!ini.GetInteger("Game", "OriginalControls", &controlMode)) controlMode = -1;
        if (!ini.GetBool("Game", "DisableTouchControls", &disableTouchControls)) disableTouchControls = false;
        if (!ini.GetInteger("Game", "DisableFocusPause", &disableFocusPause)) disableFocusPause = 0;
        disableFocusPause_Config = disableFocusPause;

#if RETRO_USING_SDL2
        if (!ini.GetInteger("Keyboard 1", "Up", &inputDevice[INPUT_UP].keyMappings)) inputDevice[INPUT_UP].keyMappings = SDL_SCANCODE_UP;
        if (!ini.GetInteger("Keyboard 1", "Down", &inputDevice[INPUT_DOWN].keyMappings)) inputDevice[INPUT_DOWN].keyMappings = SDL_SCANCODE_DOWN;
        if (!ini.GetInteger("Keyboard 1", "Left", &inputDevice[INPUT_LEFT].keyMappings)) inputDevice[INPUT_LEFT].keyMappings = SDL_SCANCODE_LEFT;
        if (!ini.GetInteger("Keyboard 1", "Right", &inputDevice[INPUT_RIGHT].keyMappings)) inputDevice[INPUT_RIGHT].keyMappings = SDL_SCANCODE_RIGHT;
        if (!ini.GetInteger("Keyboard 1", "A", &inputDevice[INPUT_BUTTONA].keyMappings)) inputDevice[INPUT_BUTTONA].keyMappings = SDL_SCANCODE_Z;
        if (!ini.GetInteger("Keyboard 1", "B", &inputDevice[INPUT_BUTTONB].keyMappings)) inputDevice[INPUT_BUTTONB].keyMappings = SDL_SCANCODE_X;
        if (!ini.GetInteger("Keyboard 1", "C", &inputDevice[INPUT_BUTTONC].keyMappings)) inputDevice[INPUT_BUTTONC].keyMappings = SDL_SCANCODE_C;
        if (!ini.GetInteger("Keyboard 1", "Start", &inputDevice[INPUT_START].keyMappings)) inputDevice[INPUT_START].keyMappings = SDL_SCANCODE_RETURN;

        if (!ini.GetInteger("Controller 1", "Up", &inputDevice[INPUT_UP].contMappings))
            inputDevice[INPUT_UP].contMappings = SDL_CONTROLLER_BUTTON_DPAD_UP;
        if (!ini.GetInteger("Controller 1", "Down", &inputDevice[INPUT_DOWN].contMappings))
            inputDevice[INPUT_DOWN].contMappings = SDL_CONTROLLER_BUTTON_DPAD_DOWN;
        if (!ini.GetInteger("Controller 1", "Left", &inputDevice[INPUT_LEFT].contMappings))
            inputDevice[INPUT_LEFT].contMappings = SDL_CONTROLLER_BUTTON_DPAD_LEFT;
        if (!ini.GetInteger("Controller 1", "Right", &inputDevice[INPUT_RIGHT].contMappings))
            inputDevice[INPUT_RIGHT].contMappings = SDL_CONTROLLER_BUTTON_DPAD_RIGHT;
        if (!ini.GetInteger("Controller 1", "A", &inputDevice[INPUT_BUTTONA].contMappings))
            inputDevice[INPUT_BUTTONA].contMappings = SDL_CONTROLLER_BUTTON_A;
        if (!ini.GetInteger("Controller 1", "B", &inputDevice[INPUT_BUTTONB].contMappings))
            inputDevice[INPUT_BUTTONB].contMappings = SDL_CONTROLLER_BUTTON_B;
        if (!ini.GetInteger("Controller 1", "C", &inputDevice[INPUT_BUTTONC].contMappings))
            inputDevice[INPUT_BUTTONC].contMappings = SDL_CONTROLLER_BUTTON_X;
        if (!ini.GetInteger("Controller 1", "Start", &inputDevice[INPUT_START].contMappings))
            inputDevice[INPUT_START].contMappings = SDL_CONTROLLER_BUTTON_START;
#endif

#if RETRO_USING_SDL1
        if (!ini.GetInteger("Keyboard 1", "Up", &inputDevice[INPUT_UP].keyMappings)) inputDevice[INPUT_UP].keyMappings = SDLK_UP;
        if (!ini.GetInteger("Keyboard 1", "Down", &inputDevice[INPUT_DOWN].keyMappings)) inputDevice[INPUT_DOWN].keyMappings = SDLK_DOWN;
        if (!ini.GetInteger("Keyboard 1", "Left", &inputDevice[INPUT_LEFT].keyMappings)) inputDevice[INPUT_LEFT].keyMappings = SDLK_LEFT;
        if (!ini.GetInteger("Keyboard 1", "Right", &inputDevice[INPUT_RIGHT].keyMappings)) inputDevice[INPUT_RIGHT].keyMappings = SDLK_RIGHT;
        if (!ini.GetInteger("Keyboard 1", "A", &inputDevice[INPUT_BUTTONA].keyMappings)) inputDevice[INPUT_BUTTONA].keyMappings = SDLK_z;
        if (!ini.GetInteger("Keyboard 1", "B", &inputDevice[INPUT_BUTTONB].keyMappings)) inputDevice[INPUT_BUTTONB].keyMappings = SDLK_x;
        if (!ini.GetInteger("Keyboard 1", "C", &inputDevice[INPUT_BUTTONC].keyMappings)) inputDevice[INPUT_BUTTONC].keyMappings = SDLK_c;
        if (!ini.GetInteger("Keyboard 1", "Start", &inputDevice[INPUT_START].keyMappings)) inputDevice[INPUT_START].keyMappings = SDLK_RETURN;

        if (!ini.GetInteger("Controller 1", "Up", &inputDevice[INPUT_UP].contMappings)) inputDevice[INPUT_UP].contMappings = 1;
        if (!ini.GetInteger("Controller 1", "Down", &inputDevice[INPUT_DOWN].contMappings)) inputDevice[INPUT_DOWN].contMappings = 2;
        if (!ini.GetInteger("Controller 1", "Left", &inputDevice[INPUT_LEFT].contMappings)) inputDevice[INPUT_LEFT].contMappings = 3;
        if (!ini.GetInteger("Controller 1", "Right", &inputDevice[INPUT_RIGHT].contMappings)) inputDevice[INPUT_RIGHT].contMappings = 4;
        if (!ini.GetInteger("Controller 1", "A", &inputDevice[INPUT_BUTTONA].contMappings)) inputDevice[INPUT_BUTTONA].contMappings = 5;
        if (!ini.GetInteger("Controller 1", "B", &inputDevice[INPUT_BUTTONB].contMappings)) inputDevice[INPUT_BUTTONB].contMappings = 6;
        if (!ini.GetInteger("Controller 1", "C", &inputDevice[INPUT_BUTTONC].contMappings)) inputDevice[INPUT_BUTTONC].contMappings = 7;
        if (!ini.GetInteger("Controller 1", "Start", &inputDevice[INPUT_START].contMappings)) inputDevice[INPUT_START].contMappings = 8;
#endif

        if (!ini.GetBool("Window", "FullScreen", &Engine.startFullScreen)) Engine.startFullScreen = DEFAULT_FULLSCREEN;
        if (!ini.GetBool("Window", "Borderless", &Engine.borderless)) Engine.borderless = false;
        if (!ini.GetBool("Window", "VSync", &Engine.vsync)) Engine.vsync = false;
        if (!ini.GetInteger("Window", "ScalingMode", &Engine.scalingMode)) Engine.scalingMode = 0;
        if (!ini.GetInteger("Window", "WindowScale", &Engine.windowScale)) Engine.windowScale = 2;
        if (!ini.GetInteger("Window", "ScreenWidth", &SCREEN_XSIZE)) SCREEN_XSIZE = DEFAULT_SCREEN_XSIZE;
        SCREEN_XSIZE_CONFIG = SCREEN_XSIZE;
        if (!ini.GetInteger("Window", "RefreshRate", &Engine.refreshRate)) Engine.refreshRate = 60;
        if (!ini.GetInteger("Window", "DimLimit", &Engine.dimLimit)) Engine.dimLimit = 300;
        if (Engine.dimLimit >= 0) Engine.dimLimit *= Engine.refreshRate;

        bool hwRender = false;
        ini.GetBool("Window", "HardwareRenderer", &hwRender);
        renderType = hwRender ? RENDER_HW : RENDER_SW;

        float bv = 1.0f, sv = 1.0f;
        ini.GetFloat("Audio", "BGMVolume", &bv);
        ini.GetFloat("Audio", "SFXVolume", &sv);
        bgmVolume = bv * MAX_VOLUME;
        sfxVolume = sv * MAX_VOLUME;
    }

    sprintf(buffer, "%sUdata.bin", gamePath);
    file = fOpen(buffer, "rb");
    if (file) {
        fClose(file);
        ReadUserdata();
    }
    else {
        WriteUserdata();
    }
}

void WriteSettings()
{
    char buffer[0x200];
    sprintf(buffer, "%ssettings.ini", gamePath);
    IniParser ini(buffer, false);

    ini.SetBool("Dev", "DevMenu", Engine.devMenu);
    ini.SetBool("Dev", "EngineDebugMode", engineDebugMode);
    ini.SetBool("Dev", "TxtScripts", forceUseScripts);
    ini.SetInteger("Dev", "StartingCategory", Engine.startList);
    ini.SetInteger("Dev", "StartingScene", Engine.startStage);
    ini.SetInteger("Dev", "FastForwardSpeed", Engine.fastForwardSpeed);
    ini.SetBool("Dev", "UseHQModes", Engine.useHQModes);
    ini.SetString("Dev", "DataFile", Engine.dataFile);

    ini.SetInteger("Game", "Language", Engine.language);
    ini.SetInteger("Game", "GameType", Engine.gameTypeID);
    ini.SetInteger("Game", "OriginalControls", controlMode);
    ini.SetBool("Game", "DisableTouchControls", disableTouchControls);
    ini.SetInteger("Game", "DisableFocusPause", disableFocusPause);

    ini.SetInteger("Keyboard 1", "Up", inputDevice[INPUT_UP].keyMappings);
    ini.SetInteger("Keyboard 1", "Down", inputDevice[INPUT_DOWN].keyMappings);
    ini.SetInteger("Keyboard 1", "Left", inputDevice[INPUT_LEFT].keyMappings);
    ini.SetInteger("Keyboard 1", "Right", inputDevice[INPUT_RIGHT].keyMappings);
    ini.SetInteger("Keyboard 1", "A", inputDevice[INPUT_BUTTONA].keyMappings);
    ini.SetInteger("Keyboard 1", "B", inputDevice[INPUT_BUTTONB].keyMappings);
    ini.SetInteger("Keyboard 1", "C", inputDevice[INPUT_BUTTONC].keyMappings);
    ini.SetInteger("Keyboard 1", "Start", inputDevice[INPUT_START].keyMappings);

    ini.SetInteger("Controller 1", "Up", inputDevice[INPUT_UP].contMappings);
    ini.SetInteger("Controller 1", "Down", inputDevice[INPUT_DOWN].contMappings);
    ini.SetInteger("Controller 1", "Left", inputDevice[INPUT_LEFT].contMappings);
    ini.SetInteger("Controller 1", "Right", inputDevice[INPUT_RIGHT].contMappings);
    ini.SetInteger("Controller 1", "A", inputDevice[INPUT_BUTTONA].contMappings);
    ini.SetInteger("Controller 1", "B", inputDevice[INPUT_BUTTONB].contMappings);
    ini.SetInteger("Controller 1", "C", inputDevice[INPUT_BUTTONC].contMappings);
    ini.SetInteger("Controller 1", "Start", inputDevice[INPUT_START].contMappings);

    ini.SetBool("Window", "FullScreen", Engine.isFullScreen);
    ini.SetBool("Window", "Borderless", Engine.borderless);
    ini.SetBool("Window", "VSync", Engine.vsync);
    ini.SetInteger("Window", "ScalingMode", Engine.scalingMode);
    ini.SetInteger("Window", "WindowScale", Engine.windowScale);
    ini.SetInteger("Window", "ScreenWidth", SCREEN_XSIZE_CONFIG);
    ini.SetInteger("Window", "RefreshRate", Engine.refreshRate);
    ini.SetInteger("Window", "DimLimit", Engine.dimLimit / (Engine.refreshRate > 0 ? Engine.refreshRate : 60));
    ini.SetBool("Window", "HardwareRenderer", renderType == RENDER_HW);

    ini.SetFloat("Audio", "BGMVolume", (float)bgmVolume / (float)MAX_VOLUME);
    ini.SetFloat("Audio", "SFXVolume", (float)sfxVolume / (float)MAX_VOLUME);

    ini.Write(buffer, false);
    FlushWebStorage();
}

void ReadUserdata()
{
    char buffer[0x200];
    sprintf(buffer, "%sUData.bin", gamePath);
    FileIO *userFile = fOpen(buffer, "rb");
    if (!userFile) return;
    int buf = 0;
    for (int a = 0; a < ACHIEVEMENT_COUNT; ++a) {
        fRead(&buf, 4, 1, userFile);
        achievements[a].status = buf;
    }
    for (int l = 0; l < LEADERBOARD_COUNT; ++l) {
        fRead(&buf, 4, 1, userFile);
        leaderboards[l].score = buf;
    }
    fClose(userFile);
}

void WriteUserdata()
{
    char buffer[0x200];
    sprintf(buffer, "%sUData.bin", gamePath);
    FileIO *userFile = fOpen(buffer, "wb");
    if (!userFile) return;
    for (int a = 0; a < ACHIEVEMENT_COUNT; ++a) fWrite(&achievements[a].status, 4, 1, userFile);
    for (int l = 0; l < LEADERBOARD_COUNT; ++l) fWrite(&leaderboards[l].score, 4, 1, userFile);
    fClose(userFile);
    FlushWebStorage();
}

bool ReadSaveRAMData()
{
    useSGame = false;
    char buffer[0x180];

    sprintf(buffer, "%sSData.bin", gamePath);

    saveRAM[33] = bgmVolume;
    saveRAM[34] = sfxVolume;

    FileIO *saveFile = fOpen(buffer, "rb");
    if (!saveFile) {
        sprintf(buffer, "%sSGame.bin", gamePath);
        saveFile = fOpen(buffer, "rb");
        if (!saveFile)
            return false;
        useSGame = true;
    }
    fRead(saveRAM, 4, SAVEDATA_SIZE, saveFile);

    fClose(saveFile);
    return true;
}

bool WriteSaveRAMData()
{
    char buffer[0x180];
    if (!useSGame) {
        sprintf(buffer, "%sSData.bin", gamePath);
    }
    else {
        sprintf(buffer, "%sSGame.bin", gamePath);
    }

    FileIO *saveFile = fOpen(buffer, "wb");
    if (!saveFile)
        return false;

    fWrite(saveRAM, 4, SAVEDATA_SIZE, saveFile);

    fClose(saveFile);
    FlushWebStorage();
    return true;
}

void AwardAchievement(int id, int status)
{
    if (id < 0 || id >= ACHIEVEMENT_COUNT) return;
    achievements[id].status = status;
    WriteUserdata();
}
void SetAchievement(int id, int done) { AwardAchievement(id, done); }
void SetLeaderboard(int id, int result) { leaderboards[id].score = result; WriteUserdata(); }

void LoadUserdata() { InitUserdata(); }
