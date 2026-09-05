# Instalador de "Confluence Multistream" (fork de obs-multi-rtmp con soporte websocket)
# Reinstala el plugin compilado en una instalacion de OBS Studio nueva/reinstalada.
# Uso: clic derecho -> "Ejecutar con PowerShell", o: powershell -File install.ps1

$ErrorActionPreference = 'Stop'

function Test-Admin {
    $id = [Security.Principal.WindowsIdentity]::GetCurrent()
    $p = New-Object Security.Principal.WindowsPrincipal($id)
    return $p.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
}

if (-not (Test-Admin)) {
    Start-Process powershell -Verb RunAs -ArgumentList @('-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', "`"$PSCommandPath`"")
    exit
}

$obsPath = "C:\Program Files\obs-studio"
if (-not (Test-Path $obsPath)) {
    Write-Host "No se encontro OBS en '$obsPath'. Edita la variable `$obsPath en este script si esta en otra ruta." -ForegroundColor Red
    Read-Host "Presiona Enter para salir"
    exit 1
}

if (Get-Process obs64 -ErrorAction SilentlyContinue) {
    Write-Host "OBS esta abierto. Cierralo y vuelve a correr este instalador." -ForegroundColor Yellow
    Read-Host "Presiona Enter para salir"
    exit 1
}

$src = Join-Path $PSScriptRoot "build\rundir\Release"
if (-not (Test-Path (Join-Path $src "obs-multi-rtmp.dll"))) {
    Write-Host "No se encontro el build compilado en '$src'. Compila el plugin primero (ver README)." -ForegroundColor Red
    Read-Host "Presiona Enter para salir"
    exit 1
}

$destPlugin = Join-Path $obsPath "obs-plugins\64bit"
$destData = Join-Path $obsPath "data\obs-plugins\obs-multi-rtmp"

Copy-Item -Path (Join-Path $src "obs-multi-rtmp.dll") -Destination $destPlugin -Force
New-Item -ItemType Directory -Path $destData -Force | Out-Null
Copy-Item -Path (Join-Path $src "obs-multi-rtmp\*") -Destination $destData -Recurse -Force

Write-Host ""
Write-Host "Listo. Plugin instalado en:" -ForegroundColor Green
Write-Host "  $destPlugin\obs-multi-rtmp.dll"
Write-Host "  $destData"
Write-Host ""
Write-Host "Pasos manuales que faltan (una sola vez):" -ForegroundColor Cyan
Write-Host "  1. Abre OBS. Docks -> Custom Browser Docks -> agrega 'Confluence' con URL http://localhost:7773"
Write-Host "  2. Tools -> Scripts -> '+' -> selecciona confluence\scripts\obs-autostart.lua"
Write-Host ""
Read-Host "Presiona Enter para cerrar"
