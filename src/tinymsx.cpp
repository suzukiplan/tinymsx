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

unsigned char TinyMSX_readMemory(void* arg, unsigned short addr) { return ((TinyMSX*)arg)->readMemory(addr); }
void TinyMSX_writeMemory(void* arg, unsigned short addr, unsigned char value) { ((TinyMSX*)arg)->writeMemory(addr, value); }
unsigned char TinyMSX_inPort(void* arg, unsigned char port) { return ((TinyMSX*)arg)->inPort(port); }
void TinyMSX_outPort(void* arg, unsigned char port, unsigned char value) { ((TinyMSX*)arg)->outPort(port, value); }

TinyMSX::TinyMSX(void* rom, size_t romSize)
{
    this->rom = (unsigned char*)malloc(romSize);
    if (this->rom) memcpy(this->rom, rom, romSize);
    this->cpu = new Z80(TinyMSX_readMemory, TinyMSX_writeMemory, TinyMSX_inPort, TinyMSX_outPort, this);
}

TinyMSX::~TinyMSX()
{
    if (this->cpu) delete this->cpu;
    this->cpu = NULL;
    if (this->rom) free(this->rom);
    this->rom = NULL;
}

unsigned char TinyMSX::readMemory(unsigned short addr)
{
    if (addr < 0x8000) {
        return this->rom[addr];
    } else if (addr < 0xA000) {
        return 0; // unused in SG-1000
    } else {
        return this->ram[addr & 0x1FFF];
    }
}

void TinyMSX::writeMemory(unsigned short addr, unsigned char value)
{
    if (addr < 0x8000) {
        return;
    } else if (addr < 0xA000) {
        return;
    } else {
        this->ram[addr & 0x1FFF] = value;
    }
}

unsigned char TinyMSX::inPort(unsigned char port)
{
    switch (port) {
        case 0xC0:
        case 0xDC:
            return this->pad[0];
        case 0xC1:
        case 0xDD:
            return this->pad[0];
        case 0xBE:
            return this->vdpReadData();
        case 0xBF:
            return this->vdpReadStatus();
    }
    return 0;
}

void TinyMSX::outPort(unsigned char port, unsigned char value)
{
    switch (port) {
        case 0x7E:
        case 0x7F:
            this->psgWrite(value);
            break;
        case 0xBE:
            this->vdpWriteData(value);
            break;
        case 0xBF:
            this->vdpWriteAddress(value);
            break;
    }
}