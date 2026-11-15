// Vortex64-Windows: Windows counterpart of ScreenSaverInhibit.cpp (D-Bus on Linux).
// While a game runs, the display and the system must not go idle: SetThreadExecutionState(ES_CONTINUOUS | ...)
// is per thread, so Set() must be called from the GUI thread (it is: MainWindow calls it from its slots).
#include "ScreenSaverInhibit.h"

#include <windows.h>

ScreenSaverInhibitor::ScreenSaverInhibitor(QObject * Parent) :
    QObject(Parent)
{
}

ScreenSaverInhibitor::~ScreenSaverInhibitor()
{
    if (m_HaveCookie)
    {
        Release();
    }
}

void ScreenSaverInhibitor::Set(bool Inhibit)
{
    m_Wanted = Inhibit;
    if (Inhibit)
    {
        if (!m_HaveCookie)
        {
            Request();
        }
    }
    else if (m_HaveCookie)
    {
        Release();
    }
}

void ScreenSaverInhibitor::Request()
{
    if (SetThreadExecutionState(ES_CONTINUOUS | ES_DISPLAY_REQUIRED | ES_SYSTEM_REQUIRED) != 0)
    {
        m_HaveCookie = true;
        m_ServiceName = QStringLiteral("SetThreadExecutionState");
    }
}

void ScreenSaverInhibitor::Release()
{
    SetThreadExecutionState(ES_CONTINUOUS);
    m_HaveCookie = false;
    m_ServiceName.clear();
}
