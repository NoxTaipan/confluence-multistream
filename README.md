# Confluence Multistream

Personal fork of [sorayuki/obs-multi-rtmp](https://github.com/sorayuki/obs-multi-rtmp) (a native OBS Studio plugin for sending video to several RTMP destinations at once), rebranded and extended for [NOX TAIPAN](https://ko-fi.com/noxtaipan)'s **[Confluence Suite](https://github.com/NoxTaipan/confluence-beta)**: dark/green theme, IBM Plex Sans font, a live ES/EN toggle, and a dock row that shows/restarts a companion Confluence server.

- **Install / usage guide:** [CONFLUENCE-INSTALL.md](CONFLUENCE-INSTALL.md) ([English](CONFLUENCE-INSTALL.en.md))
- **No prebuilt binary is published** — `build_x64/` isn't committed and there are no [Releases](https://github.com/NoxTaipan/confluence-multistream/releases). Building from source requires Visual Studio ("Desktop development with C++") + CMake; see the install guide's rebuild section.
- The Confluence status row is wired to an absolute path hardcoded to the author's machine — it won't do anything for other installs, but plain RTMP multistreaming (this fork's actual feature, documented below) works standalone.

Everything below this line is the original fork's own technical README, describing the websocket API this fork adds on top of upstream `obs-multi-rtmp`.

---

# Fork of main obs-multi-rtmp adding OBS websocket server API support

Adds OBS websocket server API support to the obs-multi-rtmp plugin.

## Complete Websocket API Function List:
ListTargets - Get all configured RTMP targets  
GetTargetState - Get current state of a specific target  
StartTarget - Start a specific target  
StopTarget - Stop a specific target  
ToggleTarget - Toggle start/stop state of a target  
StartAll - Start all targets  
StopAll - Stop all targets  
AddTarget - Add a new target  
CloneTarget - Clone an existing target  
UpdateTargetName - Update target name  
UpdateStreamKey - Update target stream key  
UpdateServiceParam - Update service parameter  
DeleteTarget - Delete a target  
UpdateSyncStart - Sync start with OBS  
UpdateSyncStop - Sync stop with OBS  
GetTargetStats - Get stats of a specific target  

## Python test files:
Look in the folder python_tests for 9 test files that implement each of the API functions for confirmation of websocket functionality

## Notes
I could only compile this for Windows PC. If anyone can help compile and/or package for Mac OS, Linux or Windows installer please let me know!

## Added files:
- multi-output-widget.cpp  
- multi-output-widget.h  
- ws_vendor.cpp  
- ws_vendor.hpp  

## Altered files from source:
CMakeLists.txt  
  - Added websocket support:  
  - lines 41-60  
  - lines 79-82  
  - line 88 ??

pch.h  
  - altered line 27-30 & 37-40 for websocket support

obs-multi-rtmp.cpp  
  - Changes for websocket support:  
  - lines 12-14  
  - line 55-57  
  - line 71 & 87-92  
  - line 99-104  

push-widget.h  
  - Updated for websocket support:  
  - line 1-2  
  - line 6-7  
  - line 9-12  
  - line 19-27  

push-widget.cpp  
  - Updated for websocket support:  
  - line 13-18  
  - line 555  
  - ** I'm not sure if this is all the changes, compare them! **  

I used obs-websocket-5.6.3 for this version found here: https://github.com/obsproject/obs-websocket  
You specifically need to include obs-websocket-api.h  
I added it here: obs-multi-rtmp\\.deps\obs-studio-31.0.0\plugins\obs-websocket\obs-websocket-api.h  

# Build commands used for compiling on PC:
cls  
cd "C:\projects\v2 - 0.7.3\obs-multi-rtmp" -> adjust for your path!  
rmdir /s /q build  
mkdir build  
cd build  
cls  
cmake .. -G "Visual Studio 17 2022" -A x64 -DENABLE_QT=ON -DENABLE_FRONTEND_API=ON -DENABLE_WEBSOCKET=ON  
cmake --build . --config Release  
