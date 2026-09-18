#include "i18n.h"
#include "pch.h"
#include <util/text-lookup.h>

namespace {

lookup_t* g_lookupEn = nullptr;
lookup_t* g_lookupEs = nullptr;
bool g_spanish = false;

lookup_t* LoadLookup(const char* localeFile)
{
    char* path = obs_module_file(localeFile);
    if (!path) return nullptr;
    lookup_t* lookup = text_lookup_create(path);
    bfree(path);
    return lookup;
}

void EnsureLookups()
{
    static bool tried = false;
    if (tried) return;
    tried = true;
    g_lookupEn = LoadLookup("locale/en-US.ini");
    g_lookupEs = LoadLookup("locale/es-ES.ini");
}

} // namespace

const char* Tr(const char* key)
{
    EnsureLookups();

    lookup_t* lookup = g_spanish ? g_lookupEs : g_lookupEn;
    const char* out = nullptr;
    if (lookup && text_lookup_getstr(lookup, key, &out) && out)
        return out;

    // Clave no encontrada en el lookup forzado (por ejemplo si algun .ini
    // quedo desactualizado) - cae al idioma global de OBS en vez de mostrar
    // un boton en blanco.
    return obs_module_text(key);
}

void SetConfluenceLanguageSpanish(bool spanish)
{
    g_spanish = spanish;
}

bool IsConfluenceLanguageSpanish()
{
    return g_spanish;
}
