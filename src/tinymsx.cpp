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

unsigned char tinyMSX_readMemory(void* arg, unsigned short addr) { return ((TinyMSX*)arg)->readMemory(addr); }
void tinyMSX_writeMemory(void* arg, unsigned short addr, unsigned char value) { ((TinyMSX*)arg)->writeMemory(addr, value); }
unsigned char tinyMSX_inPort(void* arg, unsigned char port) { return ((TinyMSX*)arg)->inPort(port); }
void tinyMSX_outPort(void* arg, unsigned char port, unsigned char value) { ((TinyMSX*)arg)->outPort(port, value); }

TinyMSX::TinyMSX()
{
    this->cpu = new Z80(tinyMSX_readMemory, tinyMSX_writeMemory, tinyMSX_inPort, tinyMSX_outPort, this);
}

TinyMSX::~TinyMSX()
{
    if (this->cpu) delete this->cpu;
    this->cpu = NULL;
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