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

TinyMSX::TinyMSX(void* rom, size_t romSize)
{
    this->rom = (unsigned char*)malloc(romSize);
    if (this->rom) memcpy(this->rom, rom, romSize);
    this->cpu = new Z80([](void* arg, unsigned short addr) { return ((TinyMSX*)arg)->readMemory(addr); }, [](void* arg, unsigned short addr, unsigned char value) { return ((TinyMSX*)arg)->writeMemory(addr, value); }, [](void* arg, unsigned char port) { return ((TinyMSX*)arg)->inPort(port); }, [](void* arg, unsigned char port, unsigned char value) { return ((TinyMSX*)arg)->outPort(port, value); }, this);
    this->cpu->setConsumeClockCallback([](void* arg, int clocks) { ((TinyMSX*)arg)->consumeClock(clocks); });
}

TinyMSX::~TinyMSX()
{
    if (this->cpu) delete this->cpu;
    this->cpu = NULL;
    if (this->rom) free(this->rom);
    this->rom = NULL;
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
            return this->pad[0];
        case 0xBE:
            return this->vdpReadData();
        case 0xBF:
            return this->vdpReadStatus();
    }
    return 0;
}

inline unsigned char TinyMSX::vdpReadData()
{
    this->vdp.addr &= 0x3FFF;
    return this->vdp.ram[this->vdp.addr++];
}

inline unsigned char TinyMSX::vdpReadStatus()
{
    return this->vdp.stat;
}

inline void TinyMSX::outPort(unsigned char port, unsigned char value)
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

inline void TinyMSX::psgWrite(unsigned char value)
{
    // TODO: PSG write data procedure
}

inline void TinyMSX::vdpWriteData(unsigned char value)
{
    this->vdp.addr &= 0x3FFF;
    this->vdp.ram[this->vdp.addr++] = value;
}

inline void TinyMSX::vdpWriteAddress(unsigned char value)
{
    this->vdp.latch &= 1;
    this->vdp.tmpAddr[this->vdp.latch++] = value;
    if (2 == this->vdp.latch) {
        if (0b01000000 == (this->vdp.tmpAddr[0] & 0b11000000)) {
            this->updateVdpAddress();
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

inline void TinyMSX::updateVdpRegister()
{
    this->vdp.reg[this->vdp.tmpAddr[0] & 0b00001111] = this->vdp.tmpAddr[1];
}

inline void TinyMSX::consumeClock(int clocks)
{
    // TODO: consume clock procedure
}