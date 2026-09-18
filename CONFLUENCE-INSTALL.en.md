🇬🇧 **English** | [🇪🇸 Español](CONFLUENCE-INSTALL.es.md)

# Reinstalling "Confluence Multistream" (after reinstalling OBS)

This is the obs-multi-rtmp fork (with websocket support) built for NOX TAIPAN, already with Confluence's dark/green theme and IBM Plex Sans font.

<img src="docs/screenshots/multistream.png" alt="Confluence Multistream" width="360" />

Besides starting/stopping RTMP destinations, this dock shows the Confluence server's status (green/gray dot) and has **Restart** and **Repair** buttons to restart it without leaving OBS — see `../confluence/README.md`, "Startup" section.

If you reinstall OBS or switch PCs, **you don't need to rebuild** — the build is already ready in `build_x64/rundir/Release/`. Just run:

```powershell
powershell -ExecutionPolicy Bypass -File install.ps1
```

(or right-click `install.ps1` → "Run with PowerShell")

This:
1. Requests administrator permissions (once).
2. Copies `obs-multi-rtmp.dll` to `obs-plugins\64bit\` and the data folder (locale + font) to `data\obs-plugins\obs-multi-rtmp\` of your OBS install.

Requirement: OBS has to be **closed** while the installer runs.

After that, two one-time manual steps (the script can't do these, they're done from OBS's UI):
- **Docks → Custom Browser Docks** → add `Confluence Stream Info` with URL `http://localhost:7773`, and `Confluence Chat` with URL `http://localhost:7773/currents.html`.
- Start Confluence for the first time with this same dock's **Restart** button (see above) — no OBS script is needed for this, `scripts/obs-autostart.lua` is now unused.

## If you need to rebuild from scratch

If the `.dll` no longer exists (for example you deleted `build_x64/`), you need the full toolchain: Visual Studio with the "Desktop development with C++" workload and CMake. Follow the **"Build commands"** section of this fork's original `README.md`, but name the build folder `build_x64` (not `build`) to match what `install.ps1` expects:

```powershell
mkdir build_x64
cd build_x64
cmake .. -G "Visual Studio 17 2022" -A x64 -DENABLE_QT=ON -DENABLE_FRONTEND_API=ON -DENABLE_WEBSOCKET=ON
cmake --build . --config Release
```

This leaves the `.dll` in `build_x64/rundir/Release/` (or `build_x64/Release/`, depending on the CMake version) for `install.ps1` to copy into OBS.

## Full suite

This plugin is one piece of **Confluence Suite** — each one installs separately, use whichever you need:

| Repo | What it is |
|---|---|
| [confluence-beta](https://github.com/NoxTaipan/confluence-beta) | The web panel (OBS dock): title/tags/category + unified Twitch/YouTube/Kick chat. |
| **confluence-multistream** (this repo) | Native OBS plugin for sending video to several RTMP destinations at once. |
| [confluence-streamdeck-beta](https://github.com/NoxTaipan/confluence-streamdeck-beta) | Elgato Stream Deck plugin to control all of the above from physical buttons. Optional. |

## Support

Everything I publish on GitHub — including this repo — is free and open source, always. If it's useful to you and you want to support its upkeep, buy me a coffee:

[![Ko-fi](https://img.shields.io/badge/Ko--fi-FF5E5B?style=for-the-badge&logo=ko-fi&logoColor=white)](https://ko-fi.com/noxtaipan)

Separately, on [Gumroad](https://noxtaipan.gumroad.com/) I sell other products — that one does cost money, to be clear.
