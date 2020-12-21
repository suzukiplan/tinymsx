/**
 * SUZUKI PLAN - TinyMSX
 * -----------------------------------------------------------------------------
 * The MIT License (MIT)
 * 
 * Copyright (c) 2020 Yoji Suzuki.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 * -----------------------------------------------------------------------------
 */
#include "tinymsx.h"
#include <string.h>

#define CPU_RATE 3579545
#define SAMPLE_RATE 44100.0
#define PSG_SHIFT 16

#define STATE_CHUNK_CPU "CP"
#define STATE_CHUNK_RAM "RA"
#define STATE_CHUNK_VDP "VD"
#define STATE_CHUNK_PSG "PS"
#define STATE_CHUNK_MEM "MR"

static void detectBlank(void* arg) { ((TinyMSX*)arg)->cpu->generateIRQ(0); }
static void detectBreak(void* arg) { ((TinyMSX*)arg)->cpu->requestBreak(); }

TinyMSX::TinyMSX(int type, const void* rom, size_t romSize, int colorMode)
{
    this->type = type;
    this->initBIOS();
    this->rom = (unsigned char*)malloc(romSize);
    if (this->rom) {
        memcpy(this->rom, rom, romSize);
        this->romSize = romSize;
    } else {
        this->romSize = 0;
    }
    this->cpu = new Z80([](void* arg, unsigned short addr) { return ((TinyMSX*)arg)->readMemory(addr); }, [](void* arg, unsigned short addr, unsigned char value) { return ((TinyMSX*)arg)->writeMemory(addr, value); }, [](void* arg, unsigned char port) { return ((TinyMSX*)arg)->inPort(port); }, [](void* arg, unsigned char port, unsigned char value) { return ((TinyMSX*)arg)->outPort(port, value); }, this);
    this->cpu->setConsumeClockCallback([](void* arg, int clocks) { ((TinyMSX*)arg)->consumeClock(clocks); });
    this->vdp.initialize(colorMode, this, detectBlank, detectBreak);
    reset();
}

TinyMSX::~TinyMSX()
{
    if (this->cpu) delete this->cpu;
    this->cpu = NULL;
    if (this->rom) free(this->rom);
    this->rom = NULL;
}

void TinyMSX::reset()
{
    if (this->cpu) {
        memset(&this->cpu->reg, 0, sizeof(this->cpu->reg));
        this->cpu->reg.PC = this->getInitAddr();
        printf("Init addr: $%04X\n", this->cpu->reg.PC);
    }
    this->vdp.reset();
    memset(&this->mem, 0, sizeof(this->mem));
    memset(this->ram, 0, sizeof(this->ram));
    memset(&this->sn76489, 0, sizeof(this->sn76489));
    memset(&this->ay8910, 0, sizeof(this->ay8910));
    if (this->isSG1000()) {
        unsigned char levels[16] = {255, 180, 127, 90, 63, 44, 31, 22, 15, 10, 7, 5, 3, 2, 1, 0};
        memcpy(this->psgLevels, &levels, sizeof(levels));
    } else if (this->isMSX1()) {
        unsigned char levels[16] = {0, 1, 2, 3, 5, 7, 11, 15, 22, 31, 44, 63, 90, 127, 180, 255};
        memcpy(this->psgLevels, &levels, sizeof(levels));
        for (int i = 0; i < 3; i++) this->ay8910.count[i] = 0x1000;
        this->ay8910.noise.seed = 0xffff;
        this->ay8910.noise.count = 0x40;
        this->ay8910.env.pause = 1;
        this->slots.reset();
        this->slots.add(0, 0, this->bios.main, true);
        this->slots.add(0, 1, this->bios.main, true);
        //this->slots.add(0, 1, &this->bios.main[0x4000], true);
        this->slots.add(1, 0, this->rom, true);
        if (0x4000 < this->romSize) this->slots.add(1, 1, &this->rom[0x4000], true);
        if (0x8000 < this->romSize) this->slots.add(1, 2, &this->rom[0x8000], true);
        if (0xC000 < this->romSize) this->slots.add(1, 3, &this->rom[0xC000], true);
        //this->slots.add(3, 0, this->bios.logo, true);
        this->slots.add(3, 3, this->ram, false);
        this->mem.page[0] = 0;
        this->mem.page[1] = 1;
        this->mem.page[2] = 2;
        this->mem.page[3] = 3;
        this->mem.slot[0] = 0b10000000;
        this->mem.slot[1] = 0b10000001;
        this->mem.slot[2] = 0b10000101;
        this->mem.slot[3] = 0b10001111;
    }
    this->psgClock = CPU_RATE / SAMPLE_RATE * (1 << PSG_SHIFT);
    memset(this->soundBuffer, 0, sizeof(this->soundBuffer));
    this->soundBufferCursor = 0;
}

unsigned short TinyMSX::getInitAddr()
{
#if 0
    return 0;
#else
    if (this->isMSX1()) {
        return (this->rom[3] * 256) | this->rom[2];
    } else {
        return 0;
    }
#endif
}

void TinyMSX::tick(unsigned char pad1, unsigned char pad2)
{
    this->pad[0] = 0;
    this->pad[1] = 0;
    switch (this->type) {
        case TINYMSX_TYPE_SG1000:
            this->pad[0] |= pad1 & TINYMSX_JOY_UP ? 0 : 0b00000001;
            this->pad[0] |= pad1 & TINYMSX_JOY_DW ? 0 : 0b00000010;
            this->pad[0] |= pad1 & TINYMSX_JOY_LE ? 0 : 0b00000100;
            this->pad[0] |= pad1 & TINYMSX_JOY_RI ? 0 : 0b00001000;
            this->pad[0] |= pad1 & TINYMSX_JOY_T1 ? 0 : 0b00010000;
            this->pad[0] |= pad1 & TINYMSX_JOY_T2 ? 0 : 0b00100000;
            this->pad[0] |= pad2 & TINYMSX_JOY_UP ? 0 : 0b01000000;
            this->pad[0] |= pad2 & TINYMSX_JOY_DW ? 0 : 0b10000000;
            this->pad[1] |= pad2 & TINYMSX_JOY_LE ? 0 : 0b00000001;
            this->pad[1] |= pad2 & TINYMSX_JOY_RI ? 0 : 0b00000010;
            this->pad[1] |= pad2 & TINYMSX_JOY_T1 ? 0 : 0b00000100;
            this->pad[1] |= pad2 & TINYMSX_JOY_T2 ? 0 : 0b00001000;
            this->pad[1] |= 0b11110000;
            break;
        case TINYMSX_TYPE_MSX1:
            this->pad[0] |= pad1 & TINYMSX_JOY_UP ? 0b00000001 : 0;
            this->pad[0] |= pad1 & TINYMSX_JOY_DW ? 0b00000010 : 0;
            this->pad[0] |= pad1 & TINYMSX_JOY_LE ? 0b00000100 : 0;
            this->pad[0] |= pad1 & TINYMSX_JOY_RI ? 0b00001000 : 0;
            this->pad[0] |= pad1 & TINYMSX_JOY_T1 ? 0b00010000 : 0;
            this->pad[0] |= pad1 & TINYMSX_JOY_T2 ? 0b00100000 : 0;
            this->pad[0] |= pad1 & TINYMSX_JOY_S1 ? 0b01000000 : 0;
            this->pad[0] |= pad1 & TINYMSX_JOY_S2 ? 0b10000000 : 0;
            this->pad[1] |= pad2 & TINYMSX_JOY_UP ? 0b00000001 : 0;
            this->pad[1] |= pad2 & TINYMSX_JOY_DW ? 0b00000010 : 0;
            this->pad[1] |= pad2 & TINYMSX_JOY_LE ? 0b00000100 : 0;
            this->pad[1] |= pad2 & TINYMSX_JOY_RI ? 0b00001000 : 0;
            this->pad[1] |= pad2 & TINYMSX_JOY_T1 ? 0b00010000 : 0;
            this->pad[1] |= pad1 & TINYMSX_JOY_T2 ? 0b00100000 : 0;
            break;
    }
    if (this->cpu) {
        this->cpu->execute(0x7FFFFFFF);
    }
}

void* TinyMSX::getSoundBuffer(size_t* size)
{
    *size = this->soundBufferCursor * 2;
    this->soundBufferCursor = 0;
    return this->soundBuffer;
}

inline unsigned char TinyMSX::readMemory(unsigned short addr)
{
    if (this->isSG1000()) {
        if (addr < 0x8000) {
            if (romSize <= addr) {
                return 0;
            } else {
                return this->rom[addr];
            }
        } else if (addr < 0xA000) {
            return 0; // unused in SG-1000
        } else {
            return this->ram[addr & 0x1FFF];
        }
    } else if (this->isMSX1()) {
        if (0xFFFF == addr) {
            unsigned char result = 0;
            result |= (this->mem.slot[0] & 0b00001100) >> 2;
            result |= (this->mem.slot[1] & 0b00001100);
            result |= (this->mem.slot[2] & 0b00001100) << 2;
            result |= (this->mem.slot[3] & 0b00001100) << 4;
            return ~result;
        }
        int pn = addr / 0x4000;
        return this->slots.read(this->getSlotNumber(pn), this->getExtSlotNumber(pn), addr);
    } else {
        return 0; // unknown system
    }
}

inline void TinyMSX::setExtraPage(int slot, int extra)
{
    if (this->slots.hasSlot(slot, extra)) {
        this->mem.slot[slot] &= 0b11110011;
        this->mem.slot[slot] |= extra ? 0x80 : 0;
        this->mem.slot[slot] |= extra << 2;
    }
}

inline void TinyMSX::writeMemory(unsigned short addr, unsigned char value)
{
    if (this->isSG1000()) {
        if (addr < 0x8000) {
            return;
        } else if (addr < 0xA000) {
            return;
        } else {
            this->ram[addr & 0x1FFF] = value;
        }
    } else if (this->isMSX1()) {
        if (0xFFFF == addr) {
            unsigned char previous = this->readMemory(0xFFFF);
            this->setExtraPage(0, value & 0b00000011);
            this->setExtraPage(1, value & 0b00001100 >> 2);
            this->setExtraPage(2, value & 0b00110000 >> 4);
            this->setExtraPage(3, value & 0b11000000 >> 6);
            if (this->readMemory(0xFFFF) != previous) {
                printf("[$%04X] extra slot changed: %d %d %d %d\n", cpu->reg.PC, mem.slot[0] >> 2 & 3, mem.slot[1] >> 2 & 3, mem.slot[2] >> 2 & 3, mem.slot[3] >> 2 & 3);
            }
            return;
        }
        int pn = addr / 0x4000;
        this->slots.write(this->getSlotNumber(pn), this->getExtSlotNumber(pn), addr, value);
    }
}

void TinyMSX::setupSpecialKey1(unsigned char ascii, bool isTenKey) { this->setupSpecialKey(0, ascii, isTenKey); }
void TinyMSX::setupSpecialKey2(unsigned char ascii, bool isTenKey) { this->setupSpecialKey(1, ascii, isTenKey); }

void TinyMSX::setupSpecialKey(int n, unsigned char ascii, bool isTenKey)
{
    if (isTenKey) {
        switch (ascii) {
            case '*': this->setupSpecialKeyV(n, 0, 9); break;
            case '+': this->setupSpecialKeyV(n, 1, 9); break;
            case '/': this->setupSpecialKeyV(n, 2, 9); break;
            case '0': this->setupSpecialKeyV(n, 3, 9); break;
            case '1': this->setupSpecialKeyV(n, 4, 9); break;
            case '2': this->setupSpecialKeyV(n, 5, 9); break;
            case '3': this->setupSpecialKeyV(n, 6, 9); break;
            case '4': this->setupSpecialKeyV(n, 7, 9); break;
            case '5': this->setupSpecialKeyV(n, 0, 10); break;
            case '6': this->setupSpecialKeyV(n, 1, 10); break;
            case '7': this->setupSpecialKeyV(n, 2, 10); break;
            case '8': this->setupSpecialKeyV(n, 3, 10); break;
            case '9': this->setupSpecialKeyV(n, 4, 10); break;
            case '-': this->setupSpecialKeyV(n, 5, 10); break;
            case ',': this->setupSpecialKeyV(n, 6, 10); break;
            case '.': this->setupSpecialKeyV(n, 7, 10); break;
            default: this->setupSpecialKeyV(n, 255, 255); break;
        }
    } else {
        switch (isalpha(ascii) ? toupper(ascii) : ascii) {
            case '0': this->setupSpecialKeyV(n, 0, 0); break;
            case '1': this->setupSpecialKeyV(n, 1, 0); break;
            case '2': this->setupSpecialKeyV(n, 2, 0); break;
            case '3': this->setupSpecialKeyV(n, 3, 0); break;
            case '4': this->setupSpecialKeyV(n, 4, 0); break;
            case '5': this->setupSpecialKeyV(n, 5, 0); break;
            case '6': this->setupSpecialKeyV(n, 6, 0); break;
            case '7': this->setupSpecialKeyV(n, 7, 0); break;
            case '8': this->setupSpecialKeyV(n, 0, 1); break;
            case '9': this->setupSpecialKeyV(n, 1, 1); break;
            case '-': this->setupSpecialKeyV(n, 2, 1); break;
            case '^': this->setupSpecialKeyV(n, 3, 1); break;
            case '\\': this->setupSpecialKeyV(n, 4, 1); break;
            case '@': this->setupSpecialKeyV(n, 5, 1); break;
            case '[': this->setupSpecialKeyV(n, 6, 1); break;
            case '+': this->setupSpecialKeyV(n, 7, 1); break;
            default: this->setupSpecialKeyV(n, 255, 255); break;
        }
    }
}

inline unsigned char TinyMSX::inPort(unsigned char port)
{
    if (this->isSG1000()) {
        switch (port) {
            case 0xC0:
            case 0xDC:
                return this->pad[0];
            case 0xC1:
            case 0xDD:
                return this->pad[1];
            case 0xBE:
                return this->vdp.readData();
            case 0xBF:
                return this->vdp.readStatus();
            case 0xD9: // unknown (read from 007: it will occur when pushed a trigger at the title)
            case 0xDE: // SC-3000 keyboard port (ignore)
            case 0xDF: // SC-3000 keyboard port (ignore)
            case 0xE0: // unknown (read from Othello Multivision BIOS)
            case 0xE1: // unknown (read from Othello Multivision BIOS)
            case 0xE2: // unknown (read from Othello Multivision BIOS)
            case 0xE3: // unknown (read from Othello Multivision BIOS)
                return 0xFF;
            default:
                printf("unknown input port $%02X\n", port);
                exit(-1);
        }
    } else if (this->isMSX1()) {
        switch (port) {
            case 0x98:
                return this->vdp.readData();
            case 0x99:
                return this->vdp.readStatus();
            case 0xA2:
                return this->psgRead();
            case 0xA8: {
                unsigned char result = 0;
                result |= (this->mem.slot[3] & 0b00000011) << 6;
                result |= (this->mem.slot[2] & 0b00000011) << 4;
                result |= (this->mem.slot[1] & 0b00000011) << 2;
                result |= (this->mem.slot[0] & 0b00000011);
                return result;
            }
            case 0xA9: {
                // to read the keyboard matrix row specified via the port AAh. (PPI's port B is used)
                static unsigned char bit[8] = {
                    0b00000001,
                    0b00000010,
                    0b00000100,
                    0b00001000,
                    0b00010000,
                    0b00100000,
                    0b01000000,
                    0b10000000};
                unsigned char result = 0;
                if (this->pad[0] & 0b01000000) {
                    if ((this->mem.portAA & 0x0F) == this->specialKeyY[0]) {
                        result |= bit[this->specialKeyX[0]];
                    }
                }
                if (this->pad[0] & 0b10000000) {
                    if ((this->mem.portAA & 0x0F) == this->specialKeyY[1]) {
                        result |= bit[this->specialKeyX[1]];
                    }
                }
                return ~result;
            }
            case 0xAA: return 0xFF;
            default:
                printf("unknown input port $%02X\n", port);
                exit(-1);
        }
    }
    return 0;
}

inline void TinyMSX::outPort(unsigned char port, unsigned char value)
{
    if (this->isSG1000()) {
        switch (port) {
            case 0x7E:
            case 0x7F:
                this->psgWrite(value);
                break;
            case 0xBE:
                this->vdp.writeData(value);
                break;
            case 0xBF:
                this->vdp.writeAddress(value);
                break;
            case 0xDE: // keyboard port (ignore)
            case 0xDF: // keyboard port (ignore)
                break;
            default:
                printf("unknown out port $%02X <- $%02X\n", port, value);
                exit(-1);
        }
    } else if (this->isMSX1()) {
        switch (port) {
            case 0x98:
                this->vdp.writeData(value);
                break;
            case 0x99:
                this->vdp.writeAddress(value);
                break;
            case 0xA0:
                this->psgLatch(value);
                break;
            case 0xA1:
                this->psgWrite(value);
                break;
            case 0xA8: {
                for (int i = 0; i < 4; i++) {
                    this->mem.slot[i] &= 0b11111100;
                }
                this->mem.slot[3] |= value >> 6 & 3;
                this->mem.slot[2] |= value >> 4 & 3;
                this->mem.slot[1] |= value >> 2 & 3;
                this->mem.slot[0] |= value & 3;
                fprintf(stderr, "[$%04X] primary slot changed: %d %d %d %d\n", cpu->reg.PC, mem.slot[0] & 3, mem.slot[1] & 3, mem.slot[2] & 3, mem.slot[3] & 3);
                break;
            }
            case 0xAA: // to access the register that control the keyboard CAP LED, two signals to data recorder and a matrix row (use the port C of PPI)
                this->mem.portAA = value;
                break;
            case 0xAB: // to access the ports control register. (Write only)
                break;
            case 0xFC: this->changeMemoryMap(3, value); break;
            case 0xFD: this->changeMemoryMap(2, value); break;
            case 0xFE: this->changeMemoryMap(1, value); break;
            case 0xFF: this->changeMemoryMap(0, value); break;
            default:
                printf("unknown out port $%02X <- $%02X\n", port, value);
                exit(-1);
        }
    }
}

inline void TinyMSX::changeMemoryMap(int page, unsigned char map)
{
    this->mem.page[page & 3] = map;
}

inline void TinyMSX::psgLatch(unsigned char value)
{
    if (this->isMSX1()) {
        this->ay8910.latch = value & 0x1F;
    }
}

inline void TinyMSX::psgWrite(unsigned char value)
{
    if (this->isSG1000()) {
        // SN76489
        if (value & 0x80) {
            this->sn76489.i = (value >> 4) & 7;
            this->sn76489.r[this->sn76489.i] = value & 0x0f;
        } else {
            this->sn76489.r[this->sn76489.i] |= (value & 0x3f) << 4;
        }
        switch (this->sn76489.r[6] & 3) {
            case 0: this->sn76489.np = 1; break;
            case 1: this->sn76489.np = 2; break;
            case 2: this->sn76489.np = 4; break;
            case 3: this->sn76489.np = this->sn76489.r[4]; break;
        }
        this->sn76489.nx = (this->sn76489.r[6] & 0x04) ? 0x12000 : 0x08000;
    } else {
        // AY-3-8910
        int reg = this->ay8910.latch;
        this->ay8910.reg[reg] = value;
        if (reg < 6) {
            value &= reg & 1 ? 0x0F : 0xFF;
            int c = reg >> 1;
            this->ay8910.freq[c] = ((this->ay8910.reg[c * 2 + 1] & 15) << 8) + this->ay8910.reg[c * 2];
        } else if (6 == reg) {
            this->ay8910.noise.freq = (value & 0x1F) << 1;
        } else if (7 == reg) {
            this->ay8910.tmask[0] = value & 1;
            this->ay8910.tmask[1] = value & 2;
            this->ay8910.tmask[2] = value & 4;
            this->ay8910.nmask[0] = value & 8;
            this->ay8910.nmask[1] = value & 16;
            this->ay8910.nmask[2] = value & 32;
        } else if (reg < 11) {
            this->ay8910.volume[reg - 8] = (value & 0x1F) << 1;
        } else if (reg < 13) {
            this->ay8910.env.freq = (this->ay8910.reg[12] << 8) + this->ay8910.reg[11];
        } else if (13 == reg) {
            value &= 0x0F;
            this->ay8910.env.cont = (value >> 3) & 1;
            this->ay8910.env.attack = (value >> 2) & 1;
            this->ay8910.env.alternate = (value >> 1) & 1;
            this->ay8910.env.hold = value & 1;
            this->ay8910.env.face = this->ay8910.env.attack;
            this->ay8910.env.pause = 0;
            this->ay8910.env.count = 0x10000 - this->ay8910.env.freq;
            this->ay8910.env.ptr = this->ay8910.env.face ? 0 : 0x1F;
        }
    }
}

inline unsigned char TinyMSX::psgRead()
{
    if (this->isMSX1()) {
        switch (this->ay8910.latch) {
            case 0x0E: return ~(this->pad[0] & 0x3F);
            case 0x0F: return ~(this->pad[1] & 0x3F);
            default: return this->ay8910.reg[this->ay8910.latch];
        }
    }
    return 0xFF;
}

inline void TinyMSX::sn76489Calc(short* left, short* right)
{
    for (int i = 0; i < 3; i++) {
        int regidx = i << 1;
        if (this->sn76489.r[regidx]) {
            unsigned int cc = this->psgClock + this->sn76489.c[i];
            while ((cc & 0x80000000) == 0) {
                cc -= (this->sn76489.r[regidx] << (PSG_SHIFT + 4));
                this->sn76489.e[i] ^= 1;
            }
            this->sn76489.c[i] = cc;
        } else {
            this->sn76489.e[i] = 1;
        }
    }
    if (this->sn76489.np) {
        unsigned int cc = this->psgClock + this->sn76489.c[3];
        while ((cc & 0x80000000) == 0) {
            cc -= (this->sn76489.np << (PSG_SHIFT + 4));
            this->sn76489.ns >>= 1;
            if (this->sn76489.ns & 1) {
                this->sn76489.ns = this->sn76489.ns ^ this->sn76489.nx;
                this->sn76489.e[3] = 1;
            } else {
                this->sn76489.e[3] = 0;
            }
        }
        this->sn76489.c[3] = cc;
    }
    int w = 0;
    if (this->sn76489.e[0]) w += this->psgLevels[this->sn76489.r[1]];
    if (this->sn76489.e[1]) w += this->psgLevels[this->sn76489.r[3]];
    if (this->sn76489.e[2]) w += this->psgLevels[this->sn76489.r[5]];
    if (this->sn76489.e[3]) w += this->psgLevels[this->sn76489.r[7]];
    w <<= 4;
    w = (short)w;
    w *= 45;
    w /= 10;
    if (32767 < w) {
        w = 32767;
    } else if (w < -32768) {
        w = -32768;
    }
    *left = (short)w;
    *right = (short)w;
}

inline short TinyMSX::ay8910Calc()
{
    // Envelope
    this->ay8910.env.count += 5;
    while (this->ay8910.env.count >= 0x10000 && this->ay8910.env.freq != 0) {
        if (!this->ay8910.env.pause) {
            if (this->ay8910.env.face) {
                this->ay8910.env.ptr = (this->ay8910.env.ptr + 1) & 0x3f;
            } else {
                this->ay8910.env.ptr = (this->ay8910.env.ptr + 0x3f) & 0x3f;
            }
        }
        if (this->ay8910.env.ptr & 0x20) {
            if (this->ay8910.env.cont) {
                if (this->ay8910.env.alternate ^ this->ay8910.env.hold) this->ay8910.env.face ^= 1;
                if (this->ay8910.env.hold) this->ay8910.env.pause = 1;
                this->ay8910.env.ptr = this->ay8910.env.face ? 0 : 0x1f;
            } else {
                this->ay8910.env.pause = 1;
                this->ay8910.env.ptr = 0;
            }
        }
        this->ay8910.env.count -= this->ay8910.env.freq;
    }

    // Noise
    this->ay8910.noise.count += 5;
    if (this->ay8910.noise.count & 0x40) {
        if (this->ay8910.noise.seed & 1) {
            this->ay8910.noise.seed ^= 0x24000;
        }
        this->ay8910.noise.seed >>= 1;
        this->ay8910.noise.count -= this->ay8910.noise.freq ? this->ay8910.noise.freq : (1 << 1);
    }
    int noise = this->ay8910.noise.seed & 1;

    // Tone
    for (int i = 0; i < 3; i++) {
        this->ay8910.count[i] += 5;
        if (this->ay8910.count[i] & 0x1000) {
            if (this->ay8910.freq[i] > 1) {
                this->ay8910.edge[i] = !this->ay8910.edge[i];
                this->ay8910.count[i] -= this->ay8910.freq[i];
            } else {
                this->ay8910.edge[i] = 1;
            }
        }
        if ((this->ay8910.tmask[i] || this->ay8910.edge[i]) && (this->ay8910.nmask[i] || noise)) {
            this->ay8910.ch_out[i] += (this->psgLevels[(this->ay8910.volume[i] & 0x1F) >> 1] << 4);
        }
        this->ay8910.ch_out[i] >>= 1;
    }
    int w = this->ay8910.ch_out[0];
    w += this->ay8910.ch_out[1];
    w += this->ay8910.ch_out[2];
    if (32767 < w) w = 32767;
    if (w < -32768) w = -32768;
    return (short)w;
}

inline void TinyMSX::psgExec(int clocks)
{
    if (this->isSG1000()) {
        this->sn76489.b += clocks * SAMPLE_RATE;
        while (0 <= this->sn76489.b) {
            this->sn76489.b -= CPU_RATE;
            this->sn76489Calc(&this->soundBuffer[this->soundBufferCursor], &this->soundBuffer[this->soundBufferCursor + 1]);
            this->soundBufferCursor += 2;
        }
    } else if (this->isMSX1()) {
        static const int interval = CPU_RATE * 256.0 / SAMPLE_RATE * (32.0 / 44.1);
        this->ay8910.clocks += clocks << 8;
        while (interval <= this->ay8910.clocks) {
            this->ay8910.clocks -= interval;
            short w = this->ay8910Calc();
            this->soundBuffer[this->soundBufferCursor++] = w;
            this->soundBuffer[this->soundBufferCursor++] = w;
        }
    }
}

inline void TinyMSX::consumeClock(int clocks)
{
    this->psgExec(clocks);
    while (clocks--) {
        this->vdp.tick();
        this->vdp.tick();
    }
}

inline void TinyMSX::initBIOS()
{
}

bool TinyMSX::loadSpecificSizeFile(const char* path, void* buffer, size_t size)
{
    FILE* fp = fopen(path, "rb");
    if (!fp) {
        return false;
    }
    if (-1 == fseek(fp, 0, SEEK_END)) {
        fclose(fp);
        return false;
    }
    long fileSize = ftell(fp);
    if (fileSize != size) {
        fclose(fp);
        return false;
    }
    if (-1 == fseek(fp, 0, SEEK_SET)) {
        fclose(fp);
        return false;
    }
    if (size != fread(buffer, 1, size, fp)) {
        fclose(fp);
        return false;
    }
    fclose(fp);
    return true;
}

bool TinyMSX::loadBiosFromFile(const char* path) { return this->loadSpecificSizeFile(path, this->bios.main, 0x8000); }
bool TinyMSX::loadLogoFromFile(const char* path) { return this->loadSpecificSizeFile(path, this->bios.logo, 0x4000); }

bool TinyMSX::loadBiosFromMemory(void* bios, size_t size)
{
    if (size != 0x8000) return false;
    memcpy(this->bios.main, bios, size);
    return true;
}

bool TinyMSX::loadLogoFromMemory(void* logo, size_t size)
{
    if (size != 0x4000) return false;
    memcpy(this->bios.logo, logo, size);
    return true;
}

static int writeSaveState(unsigned char* buf, int ptr, const char* c, size_t s, void* data)
{
    buf[ptr++] = c[0];
    buf[ptr++] = c[1];
    unsigned short size = s & 0xFFFF;
    memcpy(&buf[ptr], &size, 2);
    ptr += 2;
    memcpy(&buf[ptr], data, size);
    return (int)(s + 4);
}

size_t TinyMSX::calcAvairableRamSize()
{
    size_t result = 0;
    for (int i = 0; i < sizeof(this->ram); i++) {
        if (this->ram[i]) {
            result = i + 1;
        }
    }
    return result;
}

const void* TinyMSX::saveState(size_t* size)
{
    int ptr = 0;
    ptr += writeSaveState(this->tmpBuffer, ptr, STATE_CHUNK_CPU, sizeof(this->cpu->reg), &this->cpu->reg);
    ptr += writeSaveState(this->tmpBuffer, ptr, STATE_CHUNK_RAM, this->calcAvairableRamSize(), this->ram);
    ptr += writeSaveState(this->tmpBuffer, ptr, STATE_CHUNK_VDP, sizeof(this->vdp.ctx), &this->vdp.ctx);
    if (this->isSG1000()) {
        ptr += writeSaveState(this->tmpBuffer, ptr, STATE_CHUNK_PSG, sizeof(this->sn76489), &this->sn76489);
    } else if (this->isMSX1()) {
        ptr += writeSaveState(this->tmpBuffer, ptr, STATE_CHUNK_PSG, sizeof(this->ay8910), &this->ay8910);
    }
    ptr += writeSaveState(this->tmpBuffer, ptr, STATE_CHUNK_MEM, sizeof(this->mem), &this->mem);
    *size = ptr;
    return this->tmpBuffer;
}

void TinyMSX::loadState(const void* data, size_t size)
{
    reset();
    char* d = (char*)data;
    int s = (int)size;
    unsigned short ds;
    while (4 < s) {
        memcpy(&ds, &d[2], 2);
        const char* ch = &d[0];
        d += 4;
        s -= 4;
        if (s < ds) break; // invalid size
        if (0 == strncmp(ch, STATE_CHUNK_CPU, 2)) {
            memcpy(&((Z80*)this->cpu)->reg, d, ds);
        } else if (0 == strncmp(ch, STATE_CHUNK_RAM, 2)) {
            memcpy(this->ram, d, ds);
        } else if (0 == strncmp(ch, STATE_CHUNK_VDP, 2)) {
            this->vdp.reset();
            memcpy(&this->vdp.ctx, d, ds);
        } else if (0 == strncmp(ch, STATE_CHUNK_PSG, 2)) {
            if (this->isSG1000()) {
                memcpy(&this->sn76489, d, ds);
            } else if (this->isMSX1()) {
                memcpy(&this->ay8910, d, ds);
            }
        } else if (0 == strncmp(ch, STATE_CHUNK_MEM, 2)) {
            memcpy(&this->mem, d, ds);
        } else {
            // ignore unknown chunk
        }
        d += ds;
        s -= ds;
    }
}
