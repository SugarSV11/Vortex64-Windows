// Vortex64-Windows: unit/integration tests of the platform layer and the core that do not need a GUI,
// a window, a GPU or a commercial ROM (ROMs are synthetic and header-only).
//
//   - memory management (VirtualAlloc layer, incl. executable memory as used by the JIT)
//   - path layer (drive letters, directory enumeration - regression of the dirent shim bug)
//   - INI files (round trip, shrinking a value must not leave stray bytes)
//   - ROM loading: z64 / n64 / v64 byte orders must produce the same image; header validation
//   - application start-up (AppInit) with user data isolated in a temporary directory
#include <Common/File.h>
#include <Common/IniFile.h>
#include <Common/MemoryManagement.h>
#include <Common/Platform.h>
#include <Common/path.h>
#include <Vortex64-cli/LinuxNotification.h>
#include <Vortex64-core/AppInit.h>
#include <Vortex64-core/N64System/N64Rom.h>
#include <Vortex64-core/Settings.h>

#ifdef _WIN32
#include <process.h>
#endif
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

namespace fs = std::filesystem;

static int g_Failures = 0;
static int g_Checks = 0;
#define CHECK(cond)                                                                 \
    do                                                                              \
    {                                                                               \
        g_Checks++;                                                                 \
        if (!(cond))                                                                \
        {                                                                           \
            printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);                  \
            g_Failures++;                                                           \
        }                                                                           \
    } while (0)

static std::vector<uint8_t> ReadAll(const fs::path & p)
{
    std::ifstream f(p, std::ios::binary);
    return std::vector<uint8_t>((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
}

// ------------------------------------------------------------------------------------------------
static void TestMemoryManagement()
{
    printf("[memory management]\n");
    const size_t Size = 0x100000;
    uint8_t * Base = (uint8_t *)AllocateAddressSpace(Size);
    CHECK(Base != nullptr);
    if (Base == nullptr)
    {
        return;
    }
    uint8_t * Rw = (uint8_t *)CommitMemory(Base, 0x10000, MEM_READWRITE);
    CHECK(Rw == Base);
    memset(Rw, 0xAB, 0x10000);
    CHECK(Rw[0] == 0xAB && Rw[0xFFFF] == 0xAB);
    MEM_PROTECTION Old = MEM_NOACCESS;
    CHECK(ProtectMemory(Rw, 0x10000, MEM_READONLY, &Old));
    CHECK(Old == MEM_READWRITE);
    CHECK(ProtectMemory(Rw, 0x10000, MEM_READWRITE));
    // a second block committed further into the reservation
    CHECK(CommitMemory(Base + 0x20000, 0x10000, MEM_READWRITE) == Base + 0x20000);
    CHECK(DecommitMemory(Base + 0x20000, 0x10000));
    CHECK(FreeAddressSpace(Base, Size));

    // executable memory as the recompiler uses it: write machine code, run it
    uint8_t * Code = (uint8_t *)AllocateAddressSpace(0x10000);
    CHECK(Code != nullptr);
    if (Code != nullptr)
    {
        uint8_t * Exec = (uint8_t *)CommitMemory(Code, 0x1000, MEM_EXECUTE_READWRITE);
        CHECK(Exec == Code);
        // mov eax, 42 ; ret
        const uint8_t Program[] = {0xB8, 0x2A, 0x00, 0x00, 0x00, 0xC3};
        memcpy(Exec, Program, sizeof(Program));
        typedef int (*Fn)();
        CHECK(((Fn)Exec)() == 42);
        CHECK(FreeAddressSpace(Code, 0x10000));
    }
}

// ------------------------------------------------------------------------------------------------
static void TestPaths(const fs::path & Tmp)
{
    printf("[paths]\n");
    // drive letters are absolute and must not get a leading '/'
    CPath Abs("C:/Users/test/file.txt");
    CHECK(!Abs.IsRelative());
    std::string Dir, Name, Ext;
    Abs.GetComponents(&Dir, &Name, &Ext);
    CHECK(Dir == "C:/Users/test");
    CHECK(Name == "file" && Ext == "txt");

    // the running executable's directory exists and contains this very executable
    CPath Module(CPath::MODULE_DIRECTORY);
    CHECK(Module.DirectoryExists());

    // enumeration (regression: the dirent shim once searched "bin*" instead of "bin\*" and returned 1 entry)
    const fs::path Enum = Tmp / "enum";
    fs::create_directories(Enum / "subdir");
    for (const char * n : {"a.dll", "b.dll", "c.txt"})
    {
        std::ofstream(Enum / n) << "x";
    }
    CPath Files((Enum.generic_string() + "/").c_str(), "*.dll");
    int Count = 0;
    for (bool Ok = Files.FindFirst(); Ok; Ok = Files.FindNext())
    {
        Count++;
    }
    CHECK(Count == 2);
    CPath Subdirs((Enum.generic_string() + "/").c_str(), "*");
    int Dirs = 0;
    for (bool Ok = Subdirs.FindFirst(CPath::FIND_ATTRIBUTE_SUBDIR); Ok; Ok = Subdirs.FindNext())
    {
        Dirs++;
    }
    CHECK(Dirs == 1);

    // directory creation (with intermediates) and existence
    CPath NewDir((Tmp / "x" / "y" / "z").generic_string().c_str(), "file.bin");
    CHECK(NewDir.DirectoryCreate());
    CHECK(NewDir.DirectoryExists());
}

// ------------------------------------------------------------------------------------------------
static void TestIniFile(const fs::path & Tmp)
{
    printf("[ini file]\n");
    const fs::path Ini = Tmp / "test.ini";
    {
        CIniFile File(Ini.generic_string().c_str());
        File.SetAutoFlush(true);
        File.SaveString("Plugin", "Audio", "a long first value that will be replaced by something shorter");
        File.SaveNumber("Plugin", "Number", 12345);
        File.SaveString("Other", "Key", "keep me");
    }
    {
        CIniFile File(Ini.generic_string().c_str());
        CHECK(File.GetString("Plugin", "Audio", "") == "a long first value that will be replaced by something shorter");
        CHECK(File.GetNumber("Plugin", "Number", 0) == 12345);
        File.SaveString("Plugin", "Audio", "short"); // shrinking in place
    }
    {
        CIniFile File(Ini.generic_string().c_str());
        CHECK(File.GetString("Plugin", "Audio", "") == "short");
        CHECK(File.GetNumber("Plugin", "Number", 0) == 12345);
        CHECK(File.GetString("Other", "Key", "") == "keep me");
    }
    // nothing may be left behind after the shrink: every line of the file is a section header, a key=value or empty
    std::vector<uint8_t> Raw = ReadAll(Ini);
    std::string Text(Raw.begin(), Raw.end());
    size_t Pos = 0;
    bool Stray = false;
    while (Pos < Text.size())
    {
        size_t End = Text.find('\n', Pos);
        std::string Line = Text.substr(Pos, End == std::string::npos ? std::string::npos : End - Pos);
        while (!Line.empty() && (Line.back() == '\r'))
        {
            Line.pop_back();
        }
        if (!Line.empty() && Line[0] != '[' && Line.find('=') == std::string::npos)
        {
            printf("  stray line in ini: [%s]\n", Line.c_str());
            Stray = true;
        }
        if (End == std::string::npos)
        {
            break;
        }
        Pos = End + 1;
    }
    CHECK(!Stray);
}

// ------------------------------------------------------------------------------------------------
// Synthetic header-only ROM in the three byte orders (z64 big endian, v64 16-bit swapped, n64 32-bit swapped)
static std::vector<uint8_t> MakeZ64(size_t Size)
{
    std::vector<uint8_t> R(Size, 0);
    R[0] = 0x80; R[1] = 0x37; R[2] = 0x12; R[3] = 0x40; // z64 magic
    const char * Title = "VORTEX TEST ROM     "; // 20 bytes at 0x20
    memcpy(&R[0x20], Title, 20);
    for (size_t i = 0x40; i < Size; i++)
    {
        R[i] = (uint8_t)((i * 31 + 7) & 0xFF);
    }
    return R;
}
static std::vector<uint8_t> ToV64(const std::vector<uint8_t> & Z)
{
    std::vector<uint8_t> R(Z);
    for (size_t i = 0; i + 1 < R.size(); i += 2)
        std::swap(R[i], R[i + 1]);
    return R;
}
static std::vector<uint8_t> ToN64(const std::vector<uint8_t> & Z)
{
    std::vector<uint8_t> R(Z);
    for (size_t i = 0; i + 3 < R.size(); i += 4)
    {
        std::swap(R[i], R[i + 3]);
        std::swap(R[i + 1], R[i + 2]);
    }
    return R;
}
static void WriteFile(const fs::path & p, const std::vector<uint8_t> & d)
{
    std::ofstream f(p, std::ios::binary);
    f.write((const char *)d.data(), (std::streamsize)d.size());
}

static void TestRomLoading(const fs::path & Tmp)
{
    printf("[rom loading]\n");
    uint8_t Z64[4] = {0x80, 0x37, 0x12, 0x40}, V64[4] = {0x37, 0x80, 0x40, 0x12}, N64[4] = {0x40, 0x12, 0x37, 0x80}, Bad[4] = {1, 2, 3, 4};
    CHECK(CN64Rom::IsValidRomImage(Z64));
    CHECK(CN64Rom::IsValidRomImage(V64));
    CHECK(CN64Rom::IsValidRomImage(N64));
    CHECK(!CN64Rom::IsValidRomImage(Bad));

    const size_t Size = 0x100000;
    const std::vector<uint8_t> Z = MakeZ64(Size);
    WriteFile(Tmp / "t.z64", Z);
    WriteFile(Tmp / "t.v64", ToV64(Z));
    WriteFile(Tmp / "t.n64", ToN64(Z));
    {
        std::vector<uint8_t> NotARom(Size, 0x55);
        WriteFile(Tmp / "bad.z64", NotARom);
    }

    std::vector<std::vector<uint8_t>> Images;
    for (const char * n : {"t.z64", "t.v64", "t.n64"})
    {
        CN64Rom Rom;
        const bool Loaded = Rom.LoadN64Image((Tmp / n).generic_string().c_str());
        CHECK(Loaded);
        if (!Loaded)
        {
            continue;
        }
        CHECK(Rom.GetRomSize() == Size);
        CHECK(Rom.GetRomName() == "VORTEX TEST ROM");
        Images.emplace_back(Rom.GetRomAddress(), Rom.GetRomAddress() + Rom.GetRomSize());
    }
    // all three byte orders end up as the same image in memory
    CHECK(Images.size() == 3);
    if (Images.size() == 3)
    {
        CHECK(Images[0] == Images[1]);
        CHECK(Images[0] == Images[2]);
        // and that image is the big-endian ROM seen as host-order words (CN64Rom keeps it byte swapped per 32-bit word)
        std::vector<uint8_t> Expected = ToN64(Z);
        CHECK(Images[0] == Expected);
    }
    // a file that is not a ROM is refused
    CN64Rom BadRom;
    CHECK(!BadRom.LoadN64Image((Tmp / "bad.z64").generic_string().c_str()));
    // a file that does not exist is refused
    CN64Rom MissingRom;
    CHECK(!MissingRom.LoadN64Image((Tmp / "missing.z64").generic_string().c_str()));
}

// ------------------------------------------------------------------------------------------------
int main()
{
    setvbuf(stdout, nullptr, _IONBF, 0);
    const fs::path Tmp = fs::temp_directory_path() / ("vortex64-core-test-" + std::to_string(_getpid()));
    fs::create_directories(Tmp);
    // user data (settings, saves, logs) of this test run is isolated
    fs::create_directories(Tmp / "appdata");
    fs::create_directories(Tmp / "localappdata");
    _putenv_s("APPDATA", (Tmp / "appdata").string().c_str());
    _putenv_s("LOCALAPPDATA", (Tmp / "localappdata").string().c_str());

    TestMemoryManagement();
    TestPaths(Tmp);
    TestIniFile(Tmp);

    printf("[application start-up]\n");
    static CLinuxNotification Notify;
    const std::string Base = CPath(CPath::MODULE_DIRECTORY).GetDirectory();
    char * Argv[] = {(char *)"core_test", nullptr};
    const bool Init = AppInit(&Notify, Base.c_str(), 1, Argv);
    CHECK(Init);
    if (Init)
    {
        CHECK(g_Settings != nullptr);
        TestRomLoading(Tmp);
        AppCleanup();
    }

    std::error_code Ec;
    fs::remove_all(Tmp, Ec);
    printf("%s: %d checks, %d failed\n", g_Failures == 0 ? "PASS" : "FAILED", g_Checks, g_Failures);
    return g_Failures == 0 ? 0 : 1;
}
