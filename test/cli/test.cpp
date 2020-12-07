#include "../../src/tinymsx.h"

int main(int argc, char* argv[])
{
    if (argc < 2) {
        puts("usage: tinymsx rom-file");
        return 1;
    }
    FILE* fp = fopen(argv[1], "rb");
    if (!fp) {
        puts("File not found");
        return 1;
    }
    void* rom;
    fseek(fp, 0, SEEK_END);
    size_t romSize = ftell(fp);
    rom = malloc(romSize);
    fseek(fp, 0, SEEK_SET);
    fread(rom, 1, romSize, fp);
    fclose(fp);
    TinyMSX msx(TINY_MSX_TYPE_MSX1, rom, romSize, TINY_MSX_COLOR_MODE_RGB555);
    msx.cpu->setDebugMessage([](void* arg, const char* msg) {
        TinyMSX* msx = (TinyMSX*)arg;
        printf("[%04X (%3d)] %s\n", msx->cpu->reg.PC, msx->ir.lineNumber, msg);
    });
    msx.tick(0, 0);
    return 0;
}
