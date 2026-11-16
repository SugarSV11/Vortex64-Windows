#include "FirstRun.h"
#include "../Vortex/Language.h"

#include <Common/path.h>
#include <Vortex64-core/Multilanguage.h>
#include <Vortex64-core/Settings.h>
#include <Vortex64-core/Settings/SettingsID.h>

void ApplyFirstRunDefaults(MainWindow *)
{
    if (g_Lang->IsLanguageLoaded())
    {
        return;
    }
    // Spanish, as the first language (saves it for the next runs and loads the core's language file)
    Vortex::ApplyLanguage("es");

    // GLideN64 when its plugin is installed, otherwise the registered default stays
#ifdef _WIN32
    static const char * const GlideN64File = "GLideN64.dll";
#else
    static const char * const GlideN64File = "libGLideN64.so";
#endif
    CPath Plugin(g_Settings->LoadStringVal(Directory_Plugin).c_str(), GlideN64File);
    if (Plugin.Exists())
    {
        g_Settings->SaveString(Plugin_GFX_Current, GlideN64File);
    }
}
