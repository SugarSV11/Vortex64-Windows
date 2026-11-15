// Writes a 4 MB header-only z64 ROM (not a game: zeros after a valid header) used by the automated tests.
#include <cstdio>
#include <cstring>
#include <vector>
int main(int argc, char ** argv)
{
    if (argc < 2)
    {
        fprintf(stderr, "usage: make-fake-rom <output.z64>\n");
        return 2;
    }
    std::vector<unsigned char> Rom(4 * 1024 * 1024, 0);
    const unsigned char Magic[4] = {0x80, 0x37, 0x12, 0x40};
    memcpy(Rom.data(), Magic, 4);
    memcpy(&Rom[0x20], "VORTEX FAKE TEST", 16);
    FILE * f = fopen(argv[1], "wb");
    if (f == nullptr)
    {
        return 1;
    }
    fwrite(Rom.data(), 1, Rom.size(), f);
    fclose(f);
    return 0;
}
