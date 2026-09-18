#pragma once

// Traduccion forzada ES/EN para el dock de Confluence Multistream,
// independiente del idioma global de OBS. Reutiliza los mismos archivos
// data/locale/en-US.ini y data/locale/es-ES.ini que ya usa obs_module_text() -
// una sola fuente de verdad para las traducciones, no un diccionario aparte.
//
// Tr(key) devuelve el string en el idioma forzado actual (ver
// SetConfluenceLanguageSpanish); si esa clave no existe en el .ini cargado,
// cae de vuelta a obs_module_text(key) (el idioma global de OBS) para no
// dejar nunca un boton en blanco.
const char* Tr(const char* key);

void SetConfluenceLanguageSpanish(bool spanish);
bool IsConfluenceLanguageSpanish();
