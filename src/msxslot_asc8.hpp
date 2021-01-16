
/**
 * SUZUKI PLAN - TinyMSX - MSX Slot System for ASC8
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
#ifndef INCLUDE_MSXSLOT_ASC8_HPP
#define INCLUDE_MSXSLOT_ASC8_HPP

#include "msxslot.hpp"
#include <stdio.h>
#include <string.h>

class MsxSlotASC8 : public MsxSlot
{
  public:
    unsigned char* rom;
    struct Context {
        unsigned char page[4];
        unsigned char slot[4];
        unsigned char seg[4];
    } ctx;

    void init(unsigned char* rom) { this->rom = rom; }

    void reset()
    {
        memset(&this->slots, 0, sizeof(this->slots));
        memset(&this->ctx, 0, sizeof(this->ctx));
        this->reloadBank();
    }

    inline void write(unsigned short addr, unsigned char value)
    {
        int pn = (addr & 0b1100000000000000) >> 14;
        int sa = (addr & 0b0011000000000000) >> 12;
        int ps = this->primaryNumber(pn);
        int ss = this->secondaryNumber(pn);
        while (0 < pn--) {
            if (ps == this->primaryNumber(pn) && ss == this->secondaryNumber(pn)) {
                sa += 0b0100;
            }
        }
        ss += sa;
        if (1 == ps && 0x6000 <= addr && addr < 0x8000) this->switchBank((addr - 0x6000) / 0x800, value);
        if (this->slots[ps].isReadOnly[ss]) return;
        if (!this->slots[ps].ptr[ss]) return;
        this->slots[ps].ptr[ss][addr & 0x0FFF] = value;
    }

    inline void switchBank(int segNo, unsigned char value)
    {
        this->ctx.seg[segNo & 0b11] = value;
        this->ctx.seg[0] = 0;
        this->reloadBank();
    }

    inline void reloadBank()
    {
        for (int i = 0; i < 8; i++) {
            int seg = this->ctx.seg[i / 2];
            seg <<= 13;
            this->slots[1].isReadOnly[i] = true;
            this->slots[1].ptr[i] = &this->rom[seg + (i & 1 ? 0x1000 : 0)];
        }
    }
};

#endif // INCLUDE_MSXSLOT_ASC8_HPP
