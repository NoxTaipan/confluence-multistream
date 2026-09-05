# Reinstalar "Confluence Multistream" (despues de reinstalar OBS)

Este es el fork de obs-multi-rtmp (con soporte de websocket) compilado para NOX TAIPAN, ya con el tema oscuro/verde y la fuente IBM Plex Sans de Confluence.

Si reinstalas OBS o cambias de PC, **no hace falta recompilar** — el build ya está listo en `build/rundir/Release/`. Solo corre:

```powershell
powershell -ExecutionPolicy Bypass -File install.ps1
```

(o clic derecho sobre `install.ps1` → "Ejecutar con PowerShell")

Esto:
1. Pide permisos de administrador (una vez).
2. Copia `obs-multi-rtmp.dll` a `obs-plugins\64bit\` y la carpeta de datos (locale + fuente) a `data\obs-plugins\obs-multi-rtmp\` de tu instalacion de OBS.

Requisito: OBS tiene que estar **cerrado** mientras corre el instalador.

Después, dos pasos manuales de un solo uso (no los puede hacer el script, se hacen desde la UI de OBS):
- **Docks → Custom Browser Docks** → agrega "Confluence" con URL `http://localhost:7773` (para el panel de info).
- **Tools → Scripts → "+"** → selecciona `../confluence/scripts/obs-autostart.lua` (para que el servidor arranque solo).

## Si necesitas recompilar desde cero

Si el `.dll` ya no existe (por ejemplo, borraste `build/`), hace falta el toolchain completo: Visual Studio con el workload "Desktop development with C++", CMake, y luego:

```powershell
cd build
cmake --build . --config Release
```

(el primer `cmake ..` con todos los flags de configuracion esta documentado en el `README.md` original de este fork, seccion "Build commands").
