#include "stdafx.h"

#include "RomMemoryHandler.h"
#include <Vortex64-core/N64System/Mips/Register.h>
#include <Vortex64-core/N64System/N64Rom.h>
#include <Vortex64-core/N64System/N64System.h>
#include <Vortex64-core/N64System/SystemGlobals.h>

RomMemoryHandler::RomMemoryHandler(CN64System & System, CRegisters & Reg, CN64Rom & Rom) :
    m_PC(Reg.m_PROGRAM_COUNTER),
    m_Reg(Reg),
    m_Rom(Rom),
    m_RomWrittenTo(false),
    m_RomWroteValue(0)
{
    System.RegisterCallBack(CN64SystemCB_Reset, this, (CN64System::CallBackFunction)stSystemReset);
    System.RegisterCallBack(CN64SystemCB_LoadedGameState, this, (CN64System::CallBackFunction)stLoadedGameState);
}

bool RomMemoryHandler::Read32(uint32_t Address, uint32_t & Value)
{
    uint32_t ReadAddr = (Address + 2) & ~0x3;

    if (g_DDRom == nullptr)
    {
        m_Reg.PI_CART_ADDR_REG = (Address + 4) & ~1;
    }
    if (m_RomWrittenTo)
    {
        Value = m_RomWroteValue;
        RomWriteDecayed();
    }
    else if ((Address & 0xFFFFFFF) < m_Rom.GetRomSize())
    {
        Value = *(uint32_t *)&m_Rom.GetRomAddress()[(ReadAddr & 0xFFFFFFF)];
        if (g_LogSettings.logRomHeader && (ReadAddr & 0x1FFFFFFF) >= 0x10000000 && (ReadAddr & 0x1FFFFFFF) < 0x10000040)
        {
            switch (ReadAddr & 0x1FFFFFFF)
            {
            case 0x10000004: LogMessage("%016llX: read from ROM clock rate (%08X)", m_PC, Value); break;
            case 0x10000008: LogMessage("%016llX: read from ROM boot address offset (%08X)", m_PC, Value); break;
            case 0x1000000C: LogMessage("%016llX: read from ROM release offset (%08X)", m_PC, Value); break;
            case 0x10000010: LogMessage("%016llX: read from ROM CRC1 (%08X)", m_PC, Value); break;
            case 0x10000014: LogMessage("%016llX: read from ROM CRC2 (%08X)", m_PC, Value); break;
            default: LogMessage("%016llX: read from ROM header 0x%X (%08X)", m_PC, ReadAddr & 0xFF, Value); break;
            }
        }
    }
    else if ((Address & 0x1FFFFF00) == 0x1FB00000)
    {
        // Vortex64 1.1: the signature that Every Copy Is Personalized (SM64 hack) reads at boot. 64 words past any real cartridge (where a
        // console or any other emulator answers the open-bus pattern below); without it the ROM does not start (it runs only in Vortex64).
        uint32_t x = (Address & 0x1FFFFFFC) * 0x9E3779B1u;
        x ^= x >> 15;
        x *= 0x85EBCA77u;
        x ^= x >> 13;
        Value = x ^ 0x56583634u; // "VX64"
    }
    else
    {
        Value = (Address << 16) | (Address & 0xFFFF);
    }
    return true;
}

bool RomMemoryHandler::Write32(uint32_t /*Address*/, uint32_t Value, uint32_t Mask)
{
    if (!m_RomWrittenTo)
    {
        m_RomWrittenTo = true;
        m_RomWroteValue = (Value & Mask);
        m_Reg.PI_STATUS_REG |= PI_STATUS_IO_BUSY;
        g_SystemTimer->SetTimer(CSystemTimer::RomWriteDecay, 0x5E, false);
    }
    return true;
}

void RomMemoryHandler::RomWriteDecayed(void)
{
    g_SystemTimer->StopTimer(CSystemTimer::RomWriteDecay);
    m_Reg.PI_STATUS_REG &= ~PI_STATUS_IO_BUSY;
    m_RomWrittenTo = false;
}

void RomMemoryHandler::SystemReset(void)
{
    m_RomWrittenTo = false;
    m_RomWroteValue = 0;
}

void RomMemoryHandler::LoadedGameState(void)
{
    m_RomWrittenTo = false;
    m_RomWroteValue = 0;
}
