// The check of the plugin directory (Vortex/PluginDirCheck.h): the plugins of another folder that are not the ones of the application are found.
//
//   plugindir-test
#ifdef _WIN32
#define PJ64_TEST_LIB(n) n ".dll"
#else
#define PJ64_TEST_LIB(n) "lib" n ".so"
#endif
#include "../Vortex/PluginDirCheck.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <cstdio>

static int g_Failures = 0;
#define CHECK(cond)                                                \
    do                                                             \
    {                                                              \
        if (!(cond))                                               \
        {                                                          \
            printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
            g_Failures++;                                          \
        }                                                          \
    } while (0)

static void Make(const QString & Dir, const char * Name, const QByteArray & Content)
{
    QDir().mkpath(Dir);
    QFile File(Dir + "/" + Name);
    File.open(QIODevice::WriteOnly);
    File.write(Content);
}

int main(int argc, char ** argv)
{
    QCoreApplication App(argc, argv);
    QTemporaryDir Work;
    const QString Own = Work.path() + "/own", Old = Work.path() + "/old", Same = Work.path() + "/same";
    for (const char * Name : {PJ64_TEST_LIB("Vortex64-video"), PJ64_TEST_LIB("Vortex64-input"), PJ64_TEST_LIB("Vortex64-rsp")})
    {
        Make(Own, Name, QByteArray("current ") + Name);
        Make(Same, Name, QByteArray("current ") + Name);
        Make(Old, Name, QByteArray("older ") + Name);
    }
    Make(Old, PJ64_TEST_LIB("Vortex64-audio"), "audio only there");  // (not in the own folder: not compared)
    Make(Old, PJ64_TEST_LIB("ThirdParty"), "somebody else's");       // (a plugin of another author)
    Make(Same, PJ64_TEST_LIB("ThirdParty"), "somebody else's");

    CHECK(Vortex::PluginsDifferentFromOwn(Own, Own).isEmpty());               // the folder of the application itself
    CHECK(Vortex::PluginsDifferentFromOwn(Own + "/../own", Own).isEmpty());   // (the same folder written another way)
    CHECK(Vortex::PluginsDifferentFromOwn(Same, Own).isEmpty());              // a copy of the same plugins
    CHECK(Vortex::PluginsDifferentFromOwn(Work.path() + "/missing", Own).isEmpty());
    const QStringList Different = Vortex::PluginsDifferentFromOwn(Old, Own);
    CHECK(Different.size() == 3 && Different.contains(PJ64_TEST_LIB("Vortex64-video")) && Different.contains(PJ64_TEST_LIB("Vortex64-input")) && Different.contains(PJ64_TEST_LIB("Vortex64-rsp")));
    CHECK(!Different.contains(PJ64_TEST_LIB("Vortex64-audio")) && !Different.contains(PJ64_TEST_LIB("ThirdParty")));
    // the same size, another content
    Make(Same, PJ64_TEST_LIB("Vortex64-video"), "CURRENT libVortex64-video.so");
    CHECK(Vortex::PluginsDifferentFromOwn(Same, Own) == QStringList({PJ64_TEST_LIB("Vortex64-video")}));
    printf("%s plugindir\n", g_Failures == 0 ? "PASS" : "FAILED");
    return g_Failures == 0 ? 0 : 1;
}
