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
            line[x] = 0xFF;
            line[x] <<= 8;
            line[x] |= r;
            line[x] <<= 8;
            line[x] |= g;
            line[x] <<= 8;
            line[x] |= b;
        }
        fwrite(line, 1, sizeof(line), fp);
    }
    fclose(fp);
}

static void print_dump(FILE* fp, const char* title, unsigned char* buf, unsigned short sp, unsigned short sz)
{
    fprintf(fp, "\n\n[%s] $%04X - $%04X\n", title, sp, sp + sz - 1);

    int ptr = sp;

    while (ptr - sp < sz) {
        fprintf(fp, "[$%04X]", ptr);
        for (int i = 0; i < 16; i++) {
            if (i == 8) {
                fprintf(fp, " - %02X", buf[ptr + i]);
            } else {
                fprintf(fp, " %02X", buf[ptr + i]);
            }
            ptr++;
            if (sz <= ptr - sp) {
                fprintf(fp, "\n");
                return;
            }
        }
        fprintf(fp, "\n");
    }
}

int main(int argc, char* argv[])
{
    if (argc < 3) {
        usage();
        return 1;
    }
    int type;
    if (0 == strcmp(argv[1], "sg1000")) {
        type = TINYMSX_TYPE_SG1000;
    } else if (0 == strcmp(argv[1], "msx")) {
        type = TINYMSX_TYPE_MSX1;
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
    TinyMSX msx(type, rom, romSize, 0x8000, TINYMSX_COLOR_MODE_RGB555);
    if (TINYMSX_TYPE_MSX1 == type) {
        if (!msx.loadBiosFromFile("../../bios/cbios_main_msx1.rom")) {
            puts("load BIOS error");
            exit(-1);
        }
    }
    msx.cpu->setDebugMessage([](void* arg, const char* msg) {
        static int count;
        TinyMSX* msx = (TinyMSX*)arg;
        int pn = msx->cpu->reg.PC / 0x4000;
        printf("%08d %3d (VI:%02X) (%d-%d %d-%d, %d-%d, %d-%d) %s\n",
               ++count,
               msx->tms9918.ctx.countV,
               msx->tms9918.getVideoMode(),
               msx->slot.primaryNumber(0),
               msx->slot.secondaryNumber(0),
               msx->slot.primaryNumber(1),
               msx->slot.secondaryNumber(1),
               msx->slot.primaryNumber(2),
               msx->slot.secondaryNumber(2),
               msx->slot.primaryNumber(3),
               msx->slot.secondaryNumber(3),
               msg);
    });
#if 0
    msx.cpu->addBreakPoint(0x4042, [](void* arg) {
        TinyMSX* msx = (TinyMSX*)arg;
        printf("detect break point: $%04X\n", msx->cpu->reg.PC);
        exit(0);
    });
#endif
    for (int i = 0; i < tickCount; i++) {
        msx.tick(0xFF, 0xFF);
    }
    if (bmp) {
        saveBitmap(bmp, msx.getDisplayBuffer(), 256, 192);
    }

    {
        FILE* fp = fopen("tms9918.dmp", "wb");
        if (fp) {
            switch (msx.tms9918.getVideoMode()) {
                case 2: {
                    unsigned short pn = ((int)(msx.tms9918.ctx.reg[2] & 0b00001111)) << 10;
                    unsigned short ct = ((int)(msx.tms9918.ctx.reg[3] & 0b10000000)) << 6;
                    unsigned short pg = ((int)(msx.tms9918.ctx.reg[4] & 0b00000100)) << 11;
                    unsigned short sa = ((int)(msx.tms9918.ctx.reg[5] & 0b01111111)) << 7;
                    unsigned short sg = ((int)(msx.tms9918.ctx.reg[6] & 0b00000111)) << 11;
                    fprintf(fp, "Video MODE: $%02X\n", 2);
                    fprintf(fp, "TC: $%X, BD: $%X\n", msx.tms9918.ctx.reg[7] / 16, msx.tms9918.ctx.reg[7] % 16);
                    fprintf(fp, "PN: $%04X  CT: $%04X  PG: $%04X  SA: $%04X  SG: $%04X\n", pn, ct, pg, sa, sg);
                    print_dump(fp, "Pattern Name Table", msx.tms9918.ctx.ram, pn, 768);
                    print_dump(fp, "Color Table", msx.tms9918.ctx.ram, ct, 6144);
                    print_dump(fp, "Character Pattern Generator", msx.tms9918.ctx.ram, pg, 6144);
                    print_dump(fp, "Sprite Attribute", msx.tms9918.ctx.ram, sa, 128);
                    print_dump(fp, "Sprite Pattern Generator", msx.tms9918.ctx.ram, sg, 2048);
                    break;
                }
                default: {
                    // dump as Mode 0
                    unsigned short pn = ((int)(msx.tms9918.ctx.reg[2] & 0b00001111)) << 10;
                    unsigned short ct = ((int)msx.tms9918.ctx.reg[3]) << 6;
                    unsigned short pg = ((int)(msx.tms9918.ctx.reg[4] & 0b00000111)) << 11;
                    unsigned short sa = ((int)(msx.tms9918.ctx.reg[5] & 0b01111111)) << 7;
                    unsigned short sg = ((int)(msx.tms9918.ctx.reg[6] & 0b00000111)) << 11;
                    fprintf(fp, "Video MODE: $%02X\n", msx.tms9918.getVideoMode());
                    fprintf(fp, "TC: $%X, BD: $%X\n", msx.tms9918.ctx.reg[7] / 16, msx.tms9918.ctx.reg[7] % 16);
                    fprintf(fp, "PN: $%04X  CT: $%04X  PG: $%04X  SA: $%04X  SG: $%04X\n", pn, ct, pg, sa, sg);
                    print_dump(fp, "Pattern Name Table", msx.tms9918.ctx.ram, pn, 768);
                    print_dump(fp, "Color Table", msx.tms9918.ctx.ram, ct, 32);
                    print_dump(fp, "Character Pattern Generator", msx.tms9918.ctx.ram, pg, 2048);
                    print_dump(fp, "Sprite Attribute", msx.tms9918.ctx.ram, sa, 128);
                    print_dump(fp, "Sprite Pattern Generator", msx.tms9918.ctx.ram, sg, 2048);
                }
            }
            fclose(fp);
        }
    }
    return 0;
}
