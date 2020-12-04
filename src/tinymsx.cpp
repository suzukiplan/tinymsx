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

TinyMSX::TinyMSX(void* rom, size_t romSize, size_t ramSize)
{
    this->rom = (unsigned char*)malloc(romSize);
    if (this->rom) memcpy(this->rom, rom, romSize);
    this->ram = (unsigned char*)malloc(ramSize);
    this->cpu = new Z80(TinyMSX_readMemory, TinyMSX_writeMemory, TinyMSX_inPort, TinyMSX_outPort, this);
}

TinyMSX::~TinyMSX()
{
    if (this->cpu) delete this->cpu;
    this->cpu = NULL;
    if (this->ram) free(this->ram);
    this->ram = NULL;
    if (this->rom) free(this->rom);
    this->rom = NULL;
}

unsigned char TinyMSX::readMemory(unsigned short addr)
{
    return 0; // TODO: need implement
}

void TinyMSX::writeMemory(unsigned short addr, unsigned char value)
{
    // TODO: need implement
}

unsigned char TinyMSX::inPort(unsigned char port)
{
    return 0; // TODO: need implement
}

void TinyMSX::outPort(unsigned char port, unsigned char value)
{
    // TODO: need implement
}