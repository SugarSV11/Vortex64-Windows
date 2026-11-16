#include "PluginDirCheck.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>

namespace Vortex
{
static QByteArray Digest(const QString & Path)
{
    QFile File(Path);
    if (!File.open(QIODevice::ReadOnly))
    {
        return QByteArray();
    }
    QCryptographicHash Hash(QCryptographicHash::Sha256);
    Hash.addData(&File);
    return Hash.result();
}

QStringList PluginsDifferentFromOwn(const QString & SelectedDirectory, const QString & OwnDirectory)
{
    QStringList Different;
    const QDir Selected(SelectedDirectory), Own(OwnDirectory);
    if (SelectedDirectory.isEmpty() || OwnDirectory.isEmpty() || QFileInfo(Selected.absolutePath()).canonicalFilePath() == QFileInfo(Own.absolutePath()).canonicalFilePath())
    {
        return Different;
    }
#ifdef _WIN32
    static const char * const Names[] = {"Vortex64-video.dll", "Vortex64-audio.dll", "Vortex64-input.dll", "Vortex64-rsp.dll", "Vortex64-video-qtconfig.dll", "Vortex64-audio-qtconfig.dll", "Vortex64-input-qtconfig.dll", "GLideN64.dll"};
#else
    static const char * const Names[] = {"libVortex64-video.so", "libVortex64-audio.so", "libVortex64-input.so", "libVortex64-rsp.so", "libVortex64-video-qtconfig.so", "libVortex64-audio-qtconfig.so", "libVortex64-input-qtconfig.so", "libGLideN64.so"};
#endif
    for (const char * Name : Names)
    {
        const QString A = Selected.filePath(QString::fromLatin1(Name)), B = Own.filePath(QString::fromLatin1(Name));
        if (!QFileInfo::exists(A) || !QFileInfo::exists(B))
        {
            continue;
        }
        if (QFileInfo(A).size() != QFileInfo(B).size() || Digest(A) != Digest(B))
        {
            Different << QString::fromLatin1(Name);
        }
    }
    return Different;
}
} // namespace Vortex
