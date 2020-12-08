#include "../../src/tinymsx.h"

void usage() { puts("usage: tinymsx {sg1000 | msx} rom-file tick-count"); }

int main(int argc, char* argv[])
{
    if (argc < 3) {
        usage();
        return 1;
    }
    int type;
    if (0 == strcmp(argv[1], "sg1000")) {
        type = TINY_MSX_TYPE_SG1000;
    } else if (0 == strcmp(argv[1], "msx")) {
        type = TINY_MSX_TYPE_MSX1;
    } else {
        usage();
        return 1;
    }
    FILE* fp = fopen(argv[2], "rb");
    int tickCount = atoi(argv[3]);
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
    for (int i = 0; i < tickCount; i++) {
        msx.tick(0, 0);
    }
    return 0;
}
