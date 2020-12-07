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
#include <string.h>
#include "tinymsx.h"

TinyMSX::TinyMSX(void* rom, size_t romSize, int colorMode)
{
    switch (colorMode) {
        case TINY_MSX_COLOR_MODE_RGB555:
            this->palette[0x0] = 0b0000000000000000;
            this->palette[0x1] = 0b0000000000000000;
            this->palette[0x2] = 0b0001001100101000;
            this->palette[0x3] = 0b0010111101101111;
            this->palette[0x4] = 0b0010100101011101;
            this->palette[0x5] = 0b0011110111011111;
            this->palette[0x6] = 0b0110100101001001;
            this->palette[0x7] = 0b0010001110111110;
            this->palette[0x8] = 0b0111110101001010;
            this->palette[0x9] = 0b0111110111101111;
            this->palette[0xA] = 0b0110101100001010;
            this->palette[0xB] = 0b0111001100110000;
            this->palette[0xC] = 0b0001001011000111;
            this->palette[0xD] = 0b0110010101110111;
            this->palette[0xE] = 0b0110011100111001;
            this->palette[0xF] = 0b0111111111111111;
            break;
        case TINY_MSX_COLOR_MODE_RGB565:
            this->palette[0x0] = 0b0000000000000000;
            this->palette[0x1] = 0b0000000000000000;
            this->palette[0x2] = 0b0010011001001000;
            this->palette[0x3] = 0b0101111011101111;
            this->palette[0x4] = 0b0101001010111101;
            this->palette[0x5] = 0b0111101110111111;
            this->palette[0x6] = 0b1101001010001001;
            this->palette[0x7] = 0b0100011101011110;
            this->palette[0x8] = 0b1111101010101010;
            this->palette[0x9] = 0b1111101111001111;
            this->palette[0xA] = 0b1101011000001010;
            this->palette[0xB] = 0b1110011001110000;
            this->palette[0xC] = 0b0010010110000111;
            this->palette[0xD] = 0b1100101011010111;
            this->palette[0xE] = 0b1100111001111001;
            this->palette[0xF] = 0b1111111111111111;
            break;
        default:
            memset(this->palette, 0, sizeof(this->palette));
    }
    this->rom = (unsigned char*)malloc(romSize);
    if (this->rom) memcpy(this->rom, rom, romSize);
    this->cpu = new Z80([](void* arg, unsigned short addr) { return ((TinyMSX*)arg)->readMemory(addr); }, [](void* arg, unsigned short addr, unsigned char value) { return ((TinyMSX*)arg)->writeMemory(addr, value); }, [](void* arg, unsigned char port) { return ((TinyMSX*)arg)->inPort(port); }, [](void* arg, unsigned char port, unsigned char value) { return ((TinyMSX*)arg)->outPort(port, value); }, this);
    this->cpu->setConsumeClockCallback([](void* arg, int clocks) { ((TinyMSX*)arg)->consumeClock(clocks); });
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
    if (this->cpu) memset(&this->cpu->reg, 0, sizeof(this->cpu->reg));
    memset(&this->vdp, 0, sizeof(this->vdp));
    memset(&this->ir, 0, sizeof(this->ir));
}

inline unsigned char TinyMSX::readMemory(unsigned short addr)
{
    if (addr < 0x8000) {
        return this->rom[addr];
    } else if (addr < 0xA000) {
        return 0; // unused in SG-1000
    } else {
        return this->ram[addr & 0x1FFF];
    }
}

inline void TinyMSX::writeMemory(unsigned short addr, unsigned char value)
{
    if (addr < 0x8000) {
        return;
    } else if (addr < 0xA000) {
        return;
    } else {
        this->ram[addr & 0x1FFF] = value;
    }
}

inline unsigned char TinyMSX::inPort(unsigned char port)
{
    switch (port) {
        case 0xC0:
        case 0xDC:
            return this->pad[0];
        case 0xC1:
        case 0xDD:
            return this->pad[1];
        case 0x98: // MSX
        case 0xBE: // SG-1000
            return this->vdpReadData();
        case 0x99: // MSX
        case 0xBF: // SG-1000
            return this->vdpReadStatus();
    }
    return 0;
}

inline unsigned char TinyMSX::vdpReadData()
{
    unsigned char result = this->vdp.readBuffer;
    this->readVideoMemory();
    return result;
}

inline unsigned char TinyMSX::vdpReadStatus()
{
    unsigned char result = this->vdp.stat;
    this->vdp.stat &= 0b01011111;
    return this->vdp.stat;
}

inline void TinyMSX::outPort(unsigned char port, unsigned char value)
{
    switch (port) {
        case 0x7E:
        case 0x7F:
            this->psgWrite(value);
            break;
        case 0x98: // MSX
        case 0xBE: // SG-1000
            this->vdpWriteData(value);
            break;
        case 0x99: // MSX
        case 0xBF: // SG-1000
            this->vdpWriteAddress(value);
            break;
    }
}

inline void TinyMSX::psgWrite(unsigned char value)
{
    // TODO: PSG write data procedure
}

inline void TinyMSX::vdpWriteData(unsigned char value)
{
    this->vdp.addr &= 0x3FFF;
    this->vdp.readBuffer = value;
    this->vdp.ram[this->vdp.addr++] = this->vdp.readBuffer;
}

inline void TinyMSX::vdpWriteAddress(unsigned char value)
{
    this->vdp.latch &= 1;
    this->vdp.tmpAddr[this->vdp.latch++] = value;
    if (2 == this->vdp.latch) {
        if (0b01000000 == (this->vdp.tmpAddr[0] & 0b11000000)) {
            this->updateVdpAddress();
        } else if (0b00000000 == (this->vdp.tmpAddr[0] & 0b11000000)) {
            this->updateVdpAddress();
            this->readVideoMemory();
        } else if (0b10000000 == (this->vdp.tmpAddr[0] & 0b11110000)) {
            this->updateVdpRegister();
        }
    }
}

inline void TinyMSX::updateVdpAddress()
{
    this->vdp.addr = this->vdp.tmpAddr[1];
    this->vdp.addr <<= 6;
    this->vdp.addr |= vdp.tmpAddr[0] & 0b00111111;
}

inline void TinyMSX::readVideoMemory()
{
    this->vdp.addr &= 0x3FFF;
    this->vdp.readBuffer = this->vdp.ram[this->vdp.addr++];
}

inline void TinyMSX::updateVdpRegister()
{
    this->vdp.reg[this->vdp.tmpAddr[0] & 0b00001111] = this->vdp.tmpAddr[1];
}

inline void TinyMSX::consumeClock(int clocks)
{
    while (clocks--) {
        this->ir.frameClock++;
        this->ir.lineClock++;
        this->checkUpdateScanline();
    }
}

inline void TinyMSX::checkUpdateScanline()
{
    switch (this->ir.lineClock) {
        case 224:
            this->ir.lineClock = 0;
            this->drawScanline(this->ir.lineNumber++);
            this->ir.lineNumber %= 262;
            break;
    }
}

inline void TinyMSX::drawScanline(int lineNumber)
{
    if (lineNumber < 192) {
        unsigned short lineBuffer[256];
        switch (this->getVideoMode()) {
            case 0: this->drawScanlineMode0(lineBuffer, lineNumber); break;
            case 1: this->drawScanlineModeX(lineBuffer, lineNumber); break;
            case 2: this->drawScanlineMode2(lineBuffer, lineNumber); break;
            case 4: this->drawScanlineMode3(lineBuffer, lineNumber); break;
            default: this->drawScanlineModeX(lineBuffer, lineNumber);
        }
        memcpy(&this->display[256 * lineNumber], lineBuffer, sizeof(lineBuffer));
        if (191 == lineNumber) {
            this->vdp.stat |= 0x80;
            this->cpu->generateIRQ(0);
        }
    }
}

inline void TinyMSX::drawScanlineMode0(unsigned short* lineBuffer, int lineNumber)
{
    // draw BG
    int pn = (this->vdp.reg[2] & 0b00001111) << 10;
    int ct = this->vdp.reg[3] << 6;
    int pg = (this->vdp.reg[4] & 0b00000111) << 11;
    int pixelLine = lineNumber % 8;
    unsigned char* nam = &this->vdp.ram[pn + lineNumber / 24 * 32];
    for (int i = 0; i < 32; i++) {
        unsigned char ptn = this->vdp.ram[pg + nam[i] * 8 + pixelLine];
        unsigned char c = this->vdp.ram[ct + nam[i] / 8];
        unsigned char cc[2];
        cc[1] = (c & 0xF0) >> 4;
        cc[0] = c & 0x0F;
        int cur = i * 8;
        this->display[cur++] = this->palette[cc[(ptn & 0b10000000) >> 7]];
        this->display[cur++] = this->palette[cc[(ptn & 0b01000000) >> 6]];
        this->display[cur++] = this->palette[cc[(ptn & 0b00100000) >> 5]];
        this->display[cur++] = this->palette[cc[(ptn & 0b00010000) >> 4]];
        this->display[cur++] = this->palette[cc[(ptn & 0b00001000) >> 3]];
        this->display[cur++] = this->palette[cc[(ptn & 0b00000100) >> 2]];
        this->display[cur++] = this->palette[cc[(ptn & 0b00000010) >> 1]];
        this->display[cur++] = this->palette[cc[ptn & 0b00000001]];
    }

    // draw Sprite
    bool si = this->vdp.reg[1] & 0b00000010 ? true : false;
    bool mag = this->vdp.reg[1] & 0b00000001 ? true : false;
    int sa = (this->vdp.reg[5] & 0b01111111) << 7;
    int sg = (this->vdp.reg[6] & 0b00000111) << 11;
} 

inline void TinyMSX::drawScanlineMode2(unsigned short* lineBuffer, int lineNumber)
{
}

inline void TinyMSX::drawScanlineMode3(unsigned short* lineBuffer, int lineNumber)
{
}

inline void TinyMSX::drawScanlineModeX(unsigned short* lineBuffer, int lineNumber)
{
    int bd = this->getColorBD();
    for (int i = 0; i < 256; i++) lineBuffer[i] = palette[bd];
}
