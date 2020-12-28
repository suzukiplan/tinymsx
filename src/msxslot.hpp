/**
 * SUZUKI PLAN - TinyMSX - MSX Slot System
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
#ifndef INCLUDE_MSXSLOT_HPP
#define INCLUDE_MSXSLOT_HPP

#include <stdio.h>
#include <string.h>

class MsxSlot
{
  protected:
    struct Slot {
        unsigned char* ptr[16];
        bool isReadOnly[16];
    } slots[4];

  public:
    struct Context {
        unsigned char page[4];
        unsigned char slot[4];
    } ctx;

    virtual void reset()
    {
        memset(&this->slots, 0, sizeof(this->slots));
        memset(&this->ctx, 0, sizeof(this->ctx));
    }

    virtual inline void setupPage(int index, int slotNumber) { this->ctx.page[index] = slotNumber; }
    virtual inline void setupSlot(int index, int slotStatus) { this->ctx.slot[index] = slotStatus; }
    virtual inline bool hasSlot(int ps, int ss) { return this->slots[ps].ptr[ss] ? true : false; }
    virtual inline int primaryNumber(int page) { return this->ctx.slot[this->ctx.page[page]] & 0b11; }
    virtual inline int secondaryNumber(int page) { return this->ctx.slot[this->ctx.page[page]] & 0b1100; }

    virtual inline void add(int ps, int ss, unsigned char* data, bool isReadOnly)
    {
        ss <<= 2;
        for (int i = 0; i < 4; i++, data += 0x1000, ss++) {
            this->slots[ps].ptr[ss] = data;
            this->slots[ps].isReadOnly[ss] = isReadOnly;
        }
    }

    virtual inline unsigned char readPrimaryStatus()
    {
        unsigned char result = 0;
        for (int i = 0; i < 4; i++) {
            result <<= 2;
            result |= this->ctx.slot[3 - i] & 0b00000011;
        }
        return result;
    }

    virtual inline void changePrimarySlots(unsigned char value)
    {
        for (int i = 0; i < 4; i++, value >>= 2) {
            this->ctx.slot[i] &= 0b11111100;
            this->ctx.slot[i] |= value & 0b00000011;
        }
    }

    virtual inline unsigned char readSecondaryStatus()
    {
        unsigned char result = 0;
        for (int i = 0; i < 4; i++) {
            result <<= 2;
            result |= (this->ctx.slot[i] & 0b00001100) >> 2;
        }
        return ~result;
    }

    virtual inline void changeSecondarySlots(unsigned char value)
    {
        for (int i = 0; i < 4; i++, value >>= 2) {
            int sn = (value & 0b00000011) << 2;
            if (this->hasSlot(i, sn)) {
                this->ctx.slot[i] &= 0b11110011;
                this->ctx.slot[i] |= sn;
            }
        }
    }

    virtual inline unsigned char read(unsigned short addr)
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
        return this->slots[ps].ptr[ss] ? this->slots[ps].ptr[ss][addr & 0x0FFF] : 0xFF;
    }

    virtual inline void write(unsigned short addr, unsigned char value)
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
        if (this->slots[ps].isReadOnly[ss]) return;
        if (!this->slots[ps].ptr[ss]) return;
        this->slots[ps].ptr[ss][addr & 0x0FFF] = value;
    }
};

#endif // INCLUDE_MSXSLOT_HPP
