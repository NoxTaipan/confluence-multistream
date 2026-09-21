[🇬🇧 English](CONFLUENCE-INSTALL.en.md) | 🇪🇸 **Español**

# Reinstalar "Confluence Multistream" (después de reinstalar OBS)

Este es el fork de obs-multi-rtmp (con soporte de websocket) compilado para NOX TAIPAN, ya con el tema oscuro/verde y la fuente IBM Plex Sans de Confluence.

> **¿Clonaste este repo para probarlo vos mismo (no sos NOX TAIPAN)?** Esta guía está escrita como recordatorio personal para reinstalar después de formatear/reinstalar OBS, no como instalador para terceros. Dos cosas que necesitas saber antes de seguir:
> 1. **No hay binario compilado en el repo.** La carpeta `build_x64/` está en `.gitignore` (solo existe en la PC del autor) y no hay [Releases](https://github.com/NoxTaipan/confluence-multistream/releases) publicados. Andá directo a la sección **["Si necesitas recompilar desde cero"](#si-necesitas-recompilar-desde-cero)** — vas a necesitar compilarlo vos con Visual Studio + CMake.
> 2. **La fila de estado de Confluence (punto verde/gris, botones Restart/Repair) no te va a funcionar.** Depende de un path absoluto hardcodeado a la PC del autor (`ConfluenceDir()` en `src/multi-output-widget.cpp`) y de tener corriendo tu propia copia del servidor de Confluence (repo privado, no publicado). El multistreo RTMP normal — la función principal de este fork — funciona igual sin eso.

<img src="docs/screenshots/multistream.png" alt="Confluence Multistream" width="360" />

Además de arrancar/detener destinos RTMP, este dock muestra el estado del servidor de Confluence (punto verde/gris) y tiene botones **Restart** y **Repair** para reiniciarlo sin salir de OBS — esto usa el "Arranque" del hub privado `confluence` (no publicado en GitHub).

Si reinstalas OBS o cambias de PC (siendo NOX TAIPAN), **no hace falta recompilar** — el build ya está listo en `build_x64/rundir/Release/`. Solo corre:

```powershell
powershell -ExecutionPolicy Bypass -File install.ps1
```

(o clic derecho sobre `install.ps1` → "Ejecutar con PowerShell")

Esto:
1. Pide permisos de administrador (una vez).
2. Copia `obs-multi-rtmp.dll` a `obs-plugins\64bit\` y la carpeta de datos (locale + fuente) a `data\obs-plugins\obs-multi-rtmp\` de tu instalación de OBS.

Requisito: OBS tiene que estar **cerrado** mientras corre el instalador.

Después, dos pasos manuales de un solo uso (no los puede hacer el script, se hacen desde la UI de OBS):
- **Docks → Custom Browser Docks** → agrega `Confluence Stream Info` con URL `http://localhost:7773`, y `Confluence Chat` con URL `http://localhost:7773/currents.html`.
- Arrancar Confluence una primera vez con el botón **Restart** de este mismo dock (ver arriba) — no hace falta ningún script de OBS para esto, `scripts/obs-autostart.lua` quedó en desuso.

## Si necesitas recompilar desde cero

Si el `.dll` ya no existe (por ejemplo, borraste `build_x64/`), hace falta el toolchain completo: Visual Studio con el workload "Desktop development with C++" y CMake. Seguí la sección **"Build commands"** del `README.md` original de este fork, pero llama a la carpeta de build `build_x64` (no `build`) para que coincida con lo que espera `install.ps1`:

```powershell
mkdir build_x64
cd build_x64
cmake .. -G "Visual Studio 17 2022" -A x64 -DENABLE_QT=ON -DENABLE_FRONTEND_API=ON -DENABLE_WEBSOCKET=ON
cmake --build . --config Release
```

Esto deja el `.dll` en `build_x64/rundir/Release/` (o `build_x64/Release/`, según la versión de CMake) para que `install.ps1` lo copie a OBS.

> ⚡ En vez de correr `install.ps1` de este repo directamente, también podés correr `install.bat` de [confluence-beta](https://github.com/NoxTaipan/confluence-beta) y elegir "Solo Multistream" (o "Todo") — apenas encuentra un `build_x64/rundir/Release/obs-multi-rtmp.dll` acá, corre este mismo `install.ps1` por vos. No puede compilar el plugin por vos (no hay forma de saltarse el paso de Visual Studio + CMake de arriba), solo copiar uno ya compilado a OBS. Guía completa de todas las formas de instalar el suite entero: [INSTALL.md](https://github.com/NoxTaipan/confluence-beta/blob/master/INSTALL.md) ([English](https://github.com/NoxTaipan/confluence-beta/blob/master/INSTALL.en.md)).

## Suite completa

Este plugin es una pieza de **Confluence Suite** — cada una se instala por separado, usa lo que necesites:

| Repo | Qué es |
|---|---|
| [confluence-beta](https://github.com/NoxTaipan/confluence-beta) | El panel web (OBS dock): título/tags/categoría + chat unificado de Twitch/YouTube/Kick. |
| **confluence-multistream** (este repo) | Plugin nativo de OBS para mandar el video a varios destinos RTMP a la vez. |
| [confluence-streamdeck-beta](https://github.com/NoxTaipan/confluence-streamdeck-beta) | Plugin de Elgato Stream Deck para controlar todo lo anterior desde botones físicos. Opcional. |

## Soporte

Todo lo que publico en GitHub — incluido este repo — es gratis y de código abierto, siempre. Si te sirve y querés apoyar el mantenimiento, invitame un café:

[![Ko-fi](https://img.shields.io/badge/Ko--fi-FF5E5B?style=for-the-badge&logo=ko-fi&logoColor=white)](https://ko-fi.com/noxtaipan)

Aparte, en [Gumroad](https://noxtaipan.gumroad.com/) vendo otros productos — eso sí tiene costo, para que quede claro.
