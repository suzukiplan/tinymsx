#include "../../src/tinymsx.h"

void usage() { puts("usage: tinymsx {sg1000 | msx} rom-file tick-count [bitmap-file]"); }

struct BitmapHeader {
    unsigned int size;
    unsigned int reserved;
    unsigned int offset;
    unsigned int biSize;
    int width;
    int height;
    unsigned short planes;
    unsigned short bitCount;
    unsigned int compression;
    unsigned int sizeImage;
    unsigned int dpmX;
    unsigned int dpmY;
    unsigned int numberOfPalettes;
    unsigned int cir;
};

void saveBitmap(const char* filename, unsigned short* display, int width, int height)
{
    struct BitmapHeader hed;
    memset(&hed, 0, sizeof(hed));
    hed.offset = sizeof(hed) + 2;
    hed.biSize = 40;
    hed.width = width;
    hed.height = height;
    hed.planes = 1;
    hed.bitCount = 32;
    hed.compression = 0;
    hed.sizeImage = width * height * 4;
    hed.numberOfPalettes = 0;
    hed.cir = 0;
    FILE* fp = fopen(filename, "wb");
    if (!fp) return;
    printf("writing %s\n", filename);
    fwrite("BM", 1, 2, fp);
    fwrite(&hed, 1, 52, fp);
    for (int y = 0; y < height; y++) {
        int posY = height - y - 1;
        unsigned int line[width];
        for (int x = 0; x < width; x++) {
            unsigned short p = display[posY * width + x];
            unsigned char r = (p & 0b0111110000000000) >> 7;
            unsigned char g = (p & 0b0000001111100000) >> 2;
            unsigned char b = (p & 0b0000000000011111) << 3;
            line[x] = b;
            line[x] <<= 8;
            line[x] |= g;
            line[x] <<= 8;
            line[x] |= r;
        }
        fwrite(line, 1, sizeof(line), fp);
    }
    fclose(fp);
}

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
    const char* bmp = argc < 4 ? NULL : argv[4];
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
    if (bmp) {
        saveBitmap(bmp, msx.display, 256, 192);
    }
    return 0;
}
