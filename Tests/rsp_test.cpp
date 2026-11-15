// Vortex64-Windows: tests of the RSP plugin through its public (Zilmar-style) ABI, the way the emulator uses it:
// the DLL is loaded, given a fake RSP_INFO (DMEM/IMEM/registers in plain memory) and asked to run small RSP programs.
// Covers: plugin identification, scalar instructions, branches with delay slots, loads/stores of every width,
// vector instructions (VADD/VMULF/VAND), and halting (BREAK, interrupt on break).
#include <Common/DynamicLibrary.h>
#include <Common/path.h>
#include <Vortex64-plugin-spec/Rsp.h>

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

static int g_Failures = 0;
static int g_Checks = 0;
#define CHECK(cond)                                                    \
    do                                                                 \
    {                                                                  \
        g_Checks++;                                                    \
        if (!(cond))                                                   \
        {                                                              \
            printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);     \
            g_Failures++;                                              \
        }                                                              \
    } while (0)

typedef void(__cdecl * GetDllInfoFn)(PLUGIN_INFO *);
typedef void(__cdecl * InitiateRSPFn)(RSP_INFO, uint32_t *);
typedef void(__cdecl * VoidFn)(void);
typedef uint32_t(__cdecl * DoRspCyclesFn)(uint32_t);

// the "hardware" the plugin sees
static std::vector<uint8_t> g_Rdram(0x400000), g_Dmem(0x1000), g_Imem(0x1000), g_Header(0x1000);
static uint32_t MI_INTR, SP_MEM_ADDR, SP_DRAM_ADDR, SP_RD_LEN, SP_WR_LEN, SP_STATUS, SP_DMA_FULL, SP_DMA_BUSY, SP_PC, SP_SEMAPHORE;
static uint32_t DPC_START, DPC_END, DPC_CURRENT, DPC_STATUS, DPC_CLOCK, DPC_BUFBUSY, DPC_PIPEBUSY, DPC_TMEM;
static int g_Interrupts = 0;
static void __cdecl CheckInterrupts() { g_Interrupts++; }
static void __cdecl Nothing() {}

static DoRspCyclesFn DoRspCycles_ = nullptr;

// With MemoryBswaped the memory is kept as host-order 32-bit words: a word written with plain stores is the
// big-endian word the RSP sees. Instructions and data are therefore written as uint32_t.
static void PutWord(std::vector<uint8_t> & Mem, uint32_t Offset, uint32_t Value)
{
    memcpy(&Mem[Offset], &Value, 4);
}
static uint32_t GetWord(const std::vector<uint8_t> & Mem, uint32_t Offset)
{
    uint32_t V;
    memcpy(&V, &Mem[Offset], 4);
    return V;
}

static bool Run(const std::vector<uint32_t> & Program, uint32_t MaxCycles = 100000)
{
    for (size_t i = 0; i < Program.size(); i++)
    {
        PutWord(g_Imem, (uint32_t)i * 4, Program[i]);
    }
    SP_PC = 0;
    SP_STATUS = 0; // not halted: the RSP runs
    DoRspCycles_(MaxCycles);
    return (SP_STATUS & 0x1) != 0; // halted
}

static void TestPluginInfo(void * Lib)
{
    GetDllInfoFn GetDllInfo = (GetDllInfoFn)DynamicLibraryGetProc(Lib, "GetDllInfo");
    CHECK(GetDllInfo != nullptr);
    if (GetDllInfo == nullptr)
    {
        return;
    }
    PLUGIN_INFO Info;
    memset(&Info, 0, sizeof(Info));
    GetDllInfo(&Info);
    CHECK(Info.Type == PLUGIN_TYPE_RSP);
    CHECK(Info.Version == RSP_SPECS_VERSION);
    CHECK(strlen(Info.Name) > 0);
    printf("  plugin: \"%s\" (spec 0x%04X)\n", Info.Name, Info.Version);
}

static void TestScalar()
{
    printf("[scalar instructions]\n");
    // ADDIU $1,$0,0x1234 ; SW $1,0x10($0) ; ADDIU $2,$1,5 ; SW $2,0x14($0) ; BREAK
    CHECK(Run({0x24011234, 0xAC010010, 0x24220005, 0xAC020014, 0x0000000D}));
    CHECK(GetWord(g_Dmem, 0x10) == 0x1234);
    CHECK(GetWord(g_Dmem, 0x14) == 0x1239);
    CHECK((SP_STATUS & 0x3) == 0x3); // halted and broke

    // loop: sum of 1..10 with BNE and its delay slot
    //  ADDIU $1,$0,10 ; ADDIU $2,$0,0 ; loop: ADDU $2,$2,$1 ; ADDIU $1,$1,-1 ; BNE $1,$0,loop ; NOP ; SW $2,0x20($0) ; BREAK
    CHECK(Run({0x2401000A, 0x24020000, 0x00411021, 0x2421FFFF, 0x1420FFFD, 0x00000000, 0xAC020020, 0x0000000D}));
    CHECK(GetWord(g_Dmem, 0x20) == 55);

    // logic and shifts: LUI/ORI build 0xDEADBEEF; SLL by 4; SRL by 8; SRA of a negative value
    //  LUI $1,0xDEAD ; ORI $1,$1,0xBEEF ; SLL $2,$1,4 ; SRL $3,$1,8 ; SRA $4,$1,8 ; XOR $5,$1,$1 ; SW x4 ; BREAK
    CHECK(Run({0x3C01DEAD, 0x3421BEEF, 0x00011100, 0x00011A02, 0x00012203, 0x00212826, 0xAC010030, 0xAC020034, 0xAC030038, 0xAC04003C, 0xAC050040, 0x0000000D}));
    CHECK(GetWord(g_Dmem, 0x30) == 0xDEADBEEF);
    CHECK(GetWord(g_Dmem, 0x34) == 0xEADBEEF0);
    CHECK(GetWord(g_Dmem, 0x38) == 0x00DEADBE);
    CHECK(GetWord(g_Dmem, 0x3C) == 0xFFDEADBE);
    CHECK(GetWord(g_Dmem, 0x40) == 0);
}

static void TestLoadStoreWidths()
{
    printf("[load/store widths]\n");
    PutWord(g_Dmem, 0x100, 0x11223344);
    // LW/LH/LB (sign extended) and LHU/LBU, stored back as words
    //  LB  $1,0x100($0)  -> 0x11          LBU $2,0x103($0) -> 0x44
    //  LH  $3,0x102($0)  -> 0x3344         LHU $4,0x100($0) -> 0x1122
    //  SB  $1,0x110($0)  SH $3,0x112($0)   SW $2,0x120 ...
    CHECK(Run({0x80010100, 0x90020103, 0x84030102, 0x94040100, 0xAC010120, 0xAC020124, 0xAC030128, 0xAC04012C, 0x0000000D}));
    CHECK(GetWord(g_Dmem, 0x120) == 0x11);
    CHECK(GetWord(g_Dmem, 0x124) == 0x44);
    CHECK(GetWord(g_Dmem, 0x128) == 0x3344);
    CHECK(GetWord(g_Dmem, 0x12C) == 0x1122);
    // sign extension: LB of 0x80 is -128
    PutWord(g_Dmem, 0x140, 0x80FF7F00);
    CHECK(Run({0x80010140, 0x80020141, 0xAC010150, 0xAC020154, 0x0000000D}));
    CHECK(GetWord(g_Dmem, 0x150) == 0xFFFFFF80);
    CHECK(GetWord(g_Dmem, 0x154) == 0xFFFFFFFF);
    // SB/SH write exactly the addressed bytes
    PutWord(g_Dmem, 0x160, 0xAAAAAAAA);
    CHECK(Run({0x24010055, 0xA0010161, 0x24020BCD, 0xA4020162, 0x0000000D}));
    CHECK(GetWord(g_Dmem, 0x160) == 0xAA550BCD); // SB wrote byte 1, SH wrote bytes 2-3 of the big-endian word
}

// COP2: vector registers. Registers are loaded from DMEM with LQV, combined, stored with SQV.
static void TestVector()
{
    printf("[vector instructions]\n");
    // two vectors of eight 16-bit lanes (big endian halves, as the RSP sees them)
    const int16_t A[8] = {1, 2, 3, -4, 30000, -30000, 100, 0};
    const int16_t B[8] = {10, 20, 30, 40, 5000, -5000, -100, 0};
    auto PutVec = [](uint32_t Off, const int16_t * V) {
        for (int i = 0; i < 8; i += 2)
        {
            // lanes i and i+1 form one big-endian word: (V[i] << 16) | (uint16_t)V[i+1]
            PutWord(g_Dmem, Off + i * 2, ((uint32_t)(uint16_t)V[i] << 16) | (uint16_t)V[i + 1]);
        }
    };
    auto GetLane = [](uint32_t Off, int Lane) -> int16_t {
        const uint32_t W = GetWord(g_Dmem, Off + (Lane & ~1) * 2);
        return (int16_t)((Lane & 1) ? (W & 0xFFFF) : (W >> 16));
    };
    PutVec(0x200, A);
    PutVec(0x210, B);
    // LQV $v1[0],0x200 ; LQV $v2[0],0x210 ; VADD $v3,$v1,$v2 ; VAND $v4,$v1,$v2 ; VMULF $v5,$v1,$v2 ; SQV x3 ; BREAK
    //  LQV vt,element,offset(base): opcode 0x32, base=$0, vt, funct(rd field)=0x04, element<<7, offset>>4
    auto LQV = [](int vt, uint32_t Off) -> uint32_t { return (0x32u << 26) | (vt << 16) | (0x04u << 11) | ((Off >> 4) & 0x7F); };
    auto SQV = [](int vt, uint32_t Off) -> uint32_t { return (0x3Au << 26) | (vt << 16) | (0x04u << 11) | ((Off >> 4) & 0x7F); };
    auto COP2 = [](uint32_t Funct, int vd, int vs, int vt) -> uint32_t { return (0x12u << 26) | (1u << 25) | (vt << 16) | (vs << 11) | (vd << 6) | Funct; };
    const uint32_t VMULF = 0x00, VAND = 0x28, VADD = 0x10;
    CHECK(Run({LQV(1, 0x200), LQV(2, 0x210), COP2(VADD, 3, 1, 2), COP2(VAND, 4, 1, 2), COP2(VMULF, 5, 1, 2),
               SQV(3, 0x220), SQV(4, 0x230), SQV(5, 0x240), 0x0000000D}));
    for (int i = 0; i < 8; i++)
    {
        int Sum = A[i] + B[i];
        if (Sum > 32767) Sum = 32767;
        if (Sum < -32768) Sum = -32768;
        CHECK(GetLane(0x220, i) == (int16_t)Sum);
        CHECK(GetLane(0x230, i) == (int16_t)(A[i] & B[i]));
        // VMULF: (a*b*2 + 0x8000) >> 16, clamped
        int64_t M = ((int64_t)A[i] * B[i] * 2 + 0x8000) >> 16;
        if (M > 32767) M = 32767;
        if (M < -32768) M = -32768;
        CHECK(GetLane(0x240, i) == (int16_t)M);
    }
}

static void TestHalt()
{
    printf("[halting]\n");
    // BREAK halts the RSP, sets the "broke" status bit and (with the interrupt-on-break bit set) raises an RSP interrupt
    g_Interrupts = 0;
    MI_INTR = 0;
    PutWord(g_Imem, 0, 0x0000000D); // BREAK
    SP_PC = 0;
    SP_STATUS = 0x40; // SP_STATUS_INTR_BREAK enabled, not halted
    DoRspCycles_(1000);
    CHECK((SP_STATUS & 0x3) == 0x3); // halted + broke
    CHECK((MI_INTR & 0x1) != 0);     // SP interrupt pending in MI_INTR
    CHECK(g_Interrupts >= 1);        // the emulator was asked to check interrupts
    // (DoRspCycles itself does not look at the halt bit: the emulator only calls it for a running RSP)
}

int main()
{
    setvbuf(stdout, nullptr, _IONBF, 0);
    const std::string Dir = CPath(CPath::MODULE_DIRECTORY).GetDirectory();
    const std::string Path = Dir + "/Vortex64-rsp.dll";
    void * Lib = DynamicLibraryOpen(Path.c_str());
    CHECK(Lib != nullptr);
    if (Lib == nullptr)
    {
        printf("cannot load %s\n", Path.c_str());
        printf("FAILED: %d checks, %d failed\n", g_Checks, g_Failures);
        return 1;
    }
    printf("[plugin]\n");
    TestPluginInfo(Lib);

    InitiateRSPFn InitiateRSP = (InitiateRSPFn)DynamicLibraryGetProc(Lib, "InitiateRSP");
    VoidFn RomOpen = (VoidFn)DynamicLibraryGetProc(Lib, "RomOpen");
    VoidFn RomClosed = (VoidFn)DynamicLibraryGetProc(Lib, "RomClosed");
    DoRspCycles_ = (DoRspCyclesFn)DynamicLibraryGetProc(Lib, "DoRspCycles");
    CHECK(InitiateRSP && RomOpen && RomClosed && DoRspCycles_);
    if (!(InitiateRSP && RomOpen && RomClosed && DoRspCycles_))
    {
        return 1;
    }

    RSP_INFO Info;
    memset(&Info, 0, sizeof(Info));
    Info.MemoryBswaped = 1;
    Info.HEADER = g_Header.data();
    Info.RDRAM = g_Rdram.data();
    Info.DMEM = g_Dmem.data();
    Info.IMEM = g_Imem.data();
    Info.MI_INTR_REG = &MI_INTR;
    Info.SP_MEM_ADDR_REG = &SP_MEM_ADDR;
    Info.SP_DRAM_ADDR_REG = &SP_DRAM_ADDR;
    Info.SP_RD_LEN_REG = &SP_RD_LEN;
    Info.SP_WR_LEN_REG = &SP_WR_LEN;
    Info.SP_STATUS_REG = &SP_STATUS;
    Info.SP_DMA_FULL_REG = &SP_DMA_FULL;
    Info.SP_DMA_BUSY_REG = &SP_DMA_BUSY;
    Info.SP_PC_REG = &SP_PC;
    Info.SP_SEMAPHORE_REG = &SP_SEMAPHORE;
    Info.DPC_START_REG = &DPC_START;
    Info.DPC_END_REG = &DPC_END;
    Info.DPC_CURRENT_REG = &DPC_CURRENT;
    Info.DPC_STATUS_REG = &DPC_STATUS;
    Info.DPC_CLOCK_REG = &DPC_CLOCK;
    Info.DPC_BUFBUSY_REG = &DPC_BUFBUSY;
    Info.DPC_PIPEBUSY_REG = &DPC_PIPEBUSY;
    Info.DPC_TMEM_REG = &DPC_TMEM;
    Info.CheckInterrupts = CheckInterrupts;
    Info.ProcessDList = Nothing;
    Info.ProcessAList = Nothing;
    Info.ProcessRdpList = Nothing;
    Info.ShowCFB = Nothing;

    uint32_t CycleCount = 0;
    InitiateRSP(Info, &CycleCount);
    RomOpen();

    TestScalar();
    TestLoadStoreWidths();
    TestVector();
    TestHalt();

    RomClosed();
    DynamicLibraryClose(Lib);
    printf("%s: %d checks, %d failed\n", g_Failures == 0 ? "PASS" : "FAILED", g_Checks, g_Failures);
    return g_Failures == 0 ? 0 : 1;
}
