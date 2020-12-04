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
#include <stdio.h>
#include "z80.hpp"

class TinyMSX {
    private:
        unsigned char pad[2];
    public:
        struct VideoDisplayProcessor {
            unsigned char ram[0x4000];      // video memory
            unsigned short p[16];           // palette register
            unsigned char r[64];            // controll register
            unsigned char s[10];            // status register
            unsigned int addr;              // address counter (17bit)
            unsigned char reserved[2];      // reserved area
        } vdp;
        unsigned char ram[0x2000];
        Z80* cpu;
        unsigned char* rom;
        size_t romSize;
        TinyMSX(void* rom, size_t romSize);
        ~TinyMSX();

        // Z80 callback functions
        unsigned char readMemory(unsigned short addr);
        void writeMemory(unsigned short addr, unsigned char value);
        unsigned char inPort(unsigned char port);
        void outPort(unsigned char port, unsigned char value);

    private:
        inline unsigned char vdpReadData();
        inline unsigned char vdpReadStatus();
        inline void vdpWriteData(unsigned char value);
        inline void vdpWriteAddress(unsigned char value);
        inline void psgWrite(unsigned char value);
};
