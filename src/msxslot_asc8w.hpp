
/**
 * SUZUKI PLAN - TinyMSX - MSX Slot System for ASC8 + SRAM (Wizardry type)
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
#ifndef INCLUDE_MSXSLOT_ASC8W_HPP
#define INCLUDE_MSXSLOT_ASC8W_HPP

#include "msxslot.hpp"
#include <stdio.h>
#include <string.h>

class MsxSlotASC8W : public MsxSlot
{
  public:
    unsigned char* rom;
    struct Context {
        unsigned char page[4];
        unsigned char slot[4];
        unsigned char seg[4];
        unsigned char sram[0x2000];
    } ctx;

    void init(unsigned char* rom)
    {
        this->rom = rom;
        memset(&this->ctx, 0, sizeof(this->ctx));
    }

    void reset()
    {
        unsigned char sram[0x2000];
        memcpy(sram, this->ctx.sram, 0x2000);
        memset(&this->slots, 0, sizeof(this->slots));
        memset(&this->ctx, 0, sizeof(this->ctx));
        memcpy(this->ctx.sram, sram, 0x2000);
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
        if (1 == ps) {
            switch (addr) {
                case 0x6000: this->switchBank(0, value); break;
                case 0x6800: this->switchBank(1, value); break;
                case 0x7000: this->switchBank(2, value); break;
                case 0x7800: this->switchBank(3, value); break;
                default:
                    if (0x4000 <= addr && addr < 0xC000 && this->slots[ps].isReadOnly[ss]) printf("INVALID WRITE: $%04X = $%02X\n", addr, value);
            }
        }
        if (this->slots[ps].isReadOnly[ss]) return;
        if (!this->slots[ps].ptr[ss]) return;
        this->slots[ps].ptr[ss][addr & 0x0FFF] = value;
    }

    inline void switchBank(int segNo, unsigned char value)
    {
        if (this->ctx.seg[segNo & 0b11] != value) {
            this->ctx.seg[segNo & 0b11] = value;
            this->reloadBank();
        }
    }

    inline void reloadBank()
    {
        for (int i = 0; i < 8; i++) {
            int seg = this->ctx.seg[i / 2];
            if (seg & 0x80) {
                this->slots[1].isReadOnly[i] = i < 4 ? true : false;
                this->slots[1].ptr[i] = &this->ctx.sram[i & 1 ? 0x1000 : 0];
            } else {
                seg <<= 13;
                this->slots[1].isReadOnly[i] = true;
                this->slots[1].ptr[i] = &this->rom[seg + (i & 1 ? 0x1000 : 0)];
            }
        }
    }
};

#endif // INCLUDE_MSXSLOT_ASC8W_HPP
