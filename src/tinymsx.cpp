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

#define MASTER_CLOCK 10738635 // Unused in this emulator
#define CPU_CLOCK 3579545     // Master Clock div 3
#define VDP_CLOCK 5370863     // 342 * 262 * 59.94 (Actually: Master Clock div 2)
#define PSG_CLOCK 44100       // Output sampling rate

#define STATE_CHUNK_CPU "CP"
#define STATE_CHUNK_RAM "RA"
#define STATE_CHUNK_VDP "VD"
#define STATE_CHUNK_SN7 "S7"
#define STATE_CHUNK_AY3 "A3"
#define STATE_CHUNK_IOS "IO"
#define STATE_CHUNK_SLT "SL"
#define STATE_CHUNK_GM2 "G2"

static void detectBlank(void* arg) { ((TinyMSX*)arg)->cpu->generateIRQ(0x07); }
static void detectBreak(void* arg) { ((TinyMSX*)arg)->cpu->requestBreak(); }

TinyMSX::TinyMSX(int type, const void* rom, size_t romSize, size_t ramSize, int colorMode)
{
    this->type = type;
    if (NULL != rom && 0 < romSize) {
        this->rom = (unsigned char*)malloc(romSize);
        if (this->rom) {
            memcpy(this->rom, rom, romSize);
            this->romSize = romSize;
        } else {
            this->romSize = 0;
        }
    } else {
        this->rom = NULL;
        this->romSize = 0;
    }
    this->ramSize = ramSize;
    this->cpu = new Z80([](void* arg, unsigned short addr) { return ((TinyMSX*)arg)->readMemory(addr); }, [](void* arg, unsigned short addr, unsigned char value) { return ((TinyMSX*)arg)->writeMemory(addr, value); }, [](void* arg, unsigned char port) { return ((TinyMSX*)arg)->inPort(port); }, [](void* arg, unsigned char port, unsigned char value) { return ((TinyMSX*)arg)->outPort(port, value); }, this);
    this->cpu->setConsumeClockCallback([](void* arg, int clocks) { ((TinyMSX*)arg)->consumeClock(clocks); });
    this->vdp.initialize(colorMode, this, detectBlank, detectBreak);
    memset(&this->bios, 0, sizeof(this->bios));
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
    memset(&this->cpu->reg, 0, sizeof(this->cpu->reg));
    memset(&this->cpu->reg.pair, 0xFF, sizeof(this->cpu->reg.pair));
    memset(&this->cpu->reg.back, 0xFF, sizeof(this->cpu->reg.back));
    this->cpu->reg.SP = 0xF000;
    this->cpu->reg.IX = 0xFFFF;
    this->cpu->reg.IY = 0xFFFF;
    this->vdp.reset();
    memset(this->io, 0xFF, sizeof(this->io));
    memset(this->ram, 0, sizeof(this->ram));
    memset(&this->ay8910, 0, sizeof(this->ay8910));
    if (this->isSG1000()) {
        this->sn76489.reset(CPU_CLOCK, PSG_CLOCK);
        this->ramSize = 0x800;
    } else if (this->isMSX1()) {
        this->ay8910.reset();
        this->slot.reset();
        this->slot.add(0, 0, &this->bios.main[0x0000], true);
        this->slot.add(0, 1, &this->bios.main[0x4000], true);
        if (this->rom) {
            this->slot.add(1, 0, this->rom, true);
            if (0x4000 < this->romSize) this->slot.add(1, 1, &this->rom[0x4000], true);
        }
        if (this->ramSize < 0x4000) this->ramSize = 0x4000;
        switch (this->ramSize / 0x4000) {
            case 1:
                this->slot.add(3, 3, &this->ram[0x0000], false);
                break;
            case 2:
                this->slot.add(3, 2, &this->ram[0x0000], false);
                this->slot.add(3, 3, &this->ram[0x4000], false);
                break;
            case 3:
                this->slot.add(3, 1, &this->ram[0x0000], false);
                this->slot.add(3, 2, &this->ram[0x4000], false);
                this->slot.add(3, 3, &this->ram[0x8000], false);
                break;
            case 4:
                this->slot.add(3, 0, &this->ram[0x0000], false);
                this->slot.add(3, 1, &this->ram[0x4000], false);
                this->slot.add(3, 2, &this->ram[0x8000], false);
                this->slot.add(3, 3, &this->ram[0xC000], false);
                break;
        }
        // initialize Page n = Slot n
        this->slot.setupPage(0, 0);
        this->slot.setupPage(1, 1);
        this->slot.setupPage(2, 2);
        this->slot.setupPage(3, 3);
        // initialize default slot condition: 0-0, 1-0, 2-0, 3-0
        this->slot.setupSlot(0, 0b00000000);
        this->slot.setupSlot(1, 0b00000001);
        this->slot.setupSlot(2, 0b00000010);
        this->slot.setupSlot(3, 0b00000011);
    } else if (this->isMSX1_GameMaster2()) {
        this->ay8910.reset();
        this->slotGM2.reset();
        this->slotGM2.init(this->rom);
        this->slotGM2.add(0, 0, &this->bios.main[0x0000], true);
        this->slotGM2.add(0, 1, &this->bios.main[0x4000], true);
        if (this->rom) {
            this->slotGM2.add(1, 0, this->rom, true);
            if (0x4000 < this->romSize) this->slotGM2.add(1, 1, &this->rom[0x4000], true);
        }
        if (this->ramSize < 0x4000) this->ramSize = 0x4000;
        switch (this->ramSize / 0x4000) {
            case 1:
                this->slotGM2.add(3, 3, &this->ram[0x0000], false);
                break;
            case 2:
                this->slotGM2.add(3, 2, &this->ram[0x0000], false);
                this->slotGM2.add(3, 3, &this->ram[0x4000], false);
                break;
            case 3:
                this->slotGM2.add(3, 1, &this->ram[0x0000], false);
                this->slotGM2.add(3, 2, &this->ram[0x4000], false);
                this->slotGM2.add(3, 3, &this->ram[0x8000], false);
                break;
            case 4:
                this->slotGM2.add(3, 0, &this->ram[0x0000], false);
                this->slotGM2.add(3, 1, &this->ram[0x4000], false);
                this->slotGM2.add(3, 2, &this->ram[0x8000], false);
                this->slotGM2.add(3, 3, &this->ram[0xC000], false);
                break;
        }
        // initialize Page n = Slot n
        this->slotGM2.setupPage(0, 0);
        this->slotGM2.setupPage(1, 1);
        this->slotGM2.setupPage(2, 2);
        this->slotGM2.setupPage(3, 3);
        // initialize default slot condition: 0-0, 1-0, 2-0, 3-0
        this->slotGM2.setupSlot(0, 0b00000000);
        this->slotGM2.setupSlot(1, 0b00000001);
        this->slotGM2.setupSlot(2, 0b00000010);
        this->slotGM2.setupSlot(3, 0b00000011);
    }
    memset(this->soundBuffer, 0, sizeof(this->soundBuffer));
    this->soundBufferCursor = 0;
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
        case TINYMSX_TYPE_MSX1_GameMaster2:
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
            this->ay8910.setPads(this->pad[0], this->pad[1]);
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
            if (this->romSize <= addr) {
                return 0;
            } else {
                return this->rom[addr];
            }
        } else if (addr < 0xA000) {
            return 0; // unused in SG-1000
        } else {
            return this->ram[addr & 0x07FF];
        }
    } else if (this->isMSX1()) {
        return 0xFFFF == addr ? this->slot.readSecondaryStatus() : this->slot.read(addr);
    } else if (this->isMSX1_GameMaster2()) {
        return 0xFFFF == addr ? this->slotGM2.readSecondaryStatus() : this->slotGM2.read(addr);
    } else {
        return 0; // unknown system
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
            this->ram[addr & 0x07FF] = value;
        }
    } else if (this->isMSX1()) {
        if (0xFFFF == addr) {
            this->slot.changeSecondarySlots(value);
        } else {
            this->slot.write(addr, value);
        }
    } else if (this->isMSX1_GameMaster2()) {
        if (0xFFFF == addr) {
            this->slotGM2.changeSecondarySlots(value);
        } else {
            this->slotGM2.write(addr, value);
        }
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
            case '\n': this->setupSpecialKeyV(n, 7, 7); break;
            case ' ': this->setupSpecialKeyV(n, 0, 8); break;
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
                printf("ignore an unknown input port $%02X\n", port);
                return this->io[port];
        }
    } else if (this->isMSX1() || this->isMSX1_GameMaster2()) {
        switch (port) {
            case 0x98:
                return this->vdp.readData();
            case 0x99:
                return this->vdp.readStatus();
            case 0xA2:
                return this->ay8910.read();
            case 0xA8:
                if (this->isMSX1()) {
                    return this->slot.readPrimaryStatus();
                } else if (this->isMSX1_GameMaster2()) {
                    return this->slotGM2.readPrimaryStatus();
                } else {
                    return 0xFF;
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
                    if ((this->io[0xAA] & 0x0F) == this->specialKeyY[0]) {
                        result |= bit[this->specialKeyX[0]];
                    }
                }
                if (this->pad[0] & 0b10000000) {
                    if ((this->io[0xAA] & 0x0F) == this->specialKeyY[1]) {
                        result |= bit[this->specialKeyX[1]];
                    }
                }
                return ~result;
            }
            case 0xAA: return 0xFF;
            default:
                printf("ignore an unknown input port $%02X\n", port);
                return this->io[port];
        }
    }
    return 0;
}

inline void TinyMSX::outPort(unsigned char port, unsigned char value)
{
    this->io[port] = value;
    if (this->isSG1000()) {
        switch (port) {
            case 0x7E:
            case 0x7F:
                this->sn76489.write(value);
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
                printf("ignore an unknown out port $%02X <- $%02X\n", port, value);
        }
    } else if (this->isMSX1() || this->isMSX1_GameMaster2()) {
        switch (port) {
            case 0x98:
                this->vdp.writeData(value);
                break;
            case 0x99:
                this->vdp.writeAddress(value);
                break;
            case 0xA0:
                this->ay8910.latch(value);
                break;
            case 0xA1:
                this->ay8910.write(value);
                break;
            case 0xA8:
                if (this->isMSX1()) {
                    this->slot.changePrimarySlots(value);
                } else if (this->isMSX1_GameMaster2()) {
                    this->slotGM2.changePrimarySlots(value);
                }
                break;
            case 0xAA: // to access the register that control the keyboard CAP LED, two signals to data recorder and a matrix row (use the port C of PPI)
                break;
            case 0xAB: // to access the ports control register. (Write only)
                break;
            case 0xFC:
            case 0xFD:
            case 0xFE:
            case 0xFF:
                if (this->isMSX1()) {
                    this->slot.setupPage(3 - (port & 0b11), value);
                } else if (this->isMSX1_GameMaster2()) {
                    this->slotGM2.setupPage(3 - (port & 0b11), value);
                }
                break;
            default:
                printf("ignore an unknown out port $%02X <- $%02X\n", port, value);
        }
    }
}

inline void TinyMSX::consumeClock(int clocks)
{
    // execute PSG
    if (this->isSG1000()) {
        this->sn76489.ctx.bobo += clocks * PSG_CLOCK;
        while (0 < this->sn76489.ctx.bobo) {
            this->sn76489.ctx.bobo -= CPU_CLOCK;
            this->sn76489.execute(&this->soundBuffer[this->soundBufferCursor], &this->soundBuffer[this->soundBufferCursor + 1]);
            this->soundBufferCursor += 2;
        }
    } else if (this->isMSX1() || this->isMSX1_GameMaster2()) {
        this->ay8910.ctx.bobo += clocks * PSG_CLOCK;
        while (0 < this->ay8910.ctx.bobo) {
            this->ay8910.ctx.bobo -= CPU_CLOCK;
            this->ay8910.execute(&this->soundBuffer[this->soundBufferCursor], &this->soundBuffer[this->soundBufferCursor + 1]);
            this->soundBufferCursor += 2;
        }
    }
    // execute VDP
    this->vdp.ctx.bobo += clocks * VDP_CLOCK;
    while (0 < this->vdp.ctx.bobo) {
        this->vdp.ctx.bobo -= CPU_CLOCK;
        this->vdp.tick();
    }
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

bool TinyMSX::loadBiosFromMemory(void* bios, size_t size)
{
    if (size != 0x8000) return false;
    memcpy(this->bios.main, bios, size);
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
    for (int i = 0; i < this->ramSize; i++) {
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
        ptr += writeSaveState(this->tmpBuffer, ptr, STATE_CHUNK_SN7, sizeof(this->sn76489.ctx), &this->sn76489.ctx);
    } else if (this->isMSX1()) {
        ptr += writeSaveState(this->tmpBuffer, ptr, STATE_CHUNK_AY3, sizeof(this->ay8910.ctx), &this->ay8910.ctx);
        ptr += writeSaveState(this->tmpBuffer, ptr, STATE_CHUNK_SLT, sizeof(this->slot.ctx), &this->slot.ctx);
    } else if (this->isMSX1_GameMaster2()) {
        ptr += writeSaveState(this->tmpBuffer, ptr, STATE_CHUNK_AY3, sizeof(this->ay8910.ctx), &this->ay8910.ctx);
        ptr += writeSaveState(this->tmpBuffer, ptr, STATE_CHUNK_GM2, sizeof(this->slotGM2.ctx), &this->slotGM2.ctx);
    }
    ptr += writeSaveState(this->tmpBuffer, ptr, STATE_CHUNK_IOS, sizeof(this->io), &this->io);
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
        } else if (0 == strncmp(ch, STATE_CHUNK_SN7, 2)) {
            memcpy(&this->sn76489.ctx, d, ds);
        } else if (0 == strncmp(ch, STATE_CHUNK_AY3, 2)) {
            memcpy(&this->ay8910.ctx, d, ds);
        } else if (0 == strncmp(ch, STATE_CHUNK_SLT, 2)) {
            memcpy(&this->slot.ctx, d, ds);
        } else if (0 == strncmp(ch, STATE_CHUNK_GM2, 2)) {
            memcpy(&this->slotGM2.ctx, d, ds);
            this->slotGM2.reloadBank();
        } else if (0 == strncmp(ch, STATE_CHUNK_IOS, 2)) {
            memcpy(this->io, d, ds);
        } else {
            // ignore unknown chunk
        }
        d += ds;
        s -= ds;
    }
}
