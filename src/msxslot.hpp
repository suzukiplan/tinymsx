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
  private:
    struct Slot {
        unsigned char* ptr[4];
        bool isReadOnly[4];
    } slots[4];

  public:
    struct Context {
        unsigned char page[4];
        unsigned char slot[4];
    } ctx;

    void reset()
    {
        memset(&this->slots, 0, sizeof(this->slots));
        memset(&this->ctx, 0, sizeof(this->ctx));
    }

    inline void setupPage(int index, int slotNumber) { this->ctx.page[index] = slotNumber; }
    inline void setupSlot(int index, int slotStatus) { this->ctx.slot[index] = slotStatus; }
    inline bool hasSlot(int ps, int ss) { return this->slots[ps].ptr[ss] ? true : false; }
    inline int primaryNumber(int page) { return this->ctx.slot[this->ctx.page[page]] & 0b11; }
    inline int secondaryNumber(int page) { return (this->ctx.slot[this->ctx.page[page]] & 0b1100) >> 2; }

    inline void add(int ps, int ss, unsigned char* data, bool isReadOnly)
    {
        this->slots[ps].ptr[ss] = data;
        this->slots[ps].isReadOnly[ss] = isReadOnly;
    }

    inline unsigned char readPrimaryStatus()
    {
        unsigned char result = 0;
        for (int i = 0; i < 4; i++) {
            result <<= 2;
            result |= this->ctx.slot[3 - i] & 0b00000011;
        }
        return result;
    }

    inline void changePrimarySlots(unsigned char value)
    {
#ifdef DEBUG
        unsigned char previous = this->readPrimaryStatus();
#endif
        for (int i = 0; i < 4; i++) {
            this->ctx.slot[i] &= 0b11111100;
            this->ctx.slot[i] |= value & 0b00000011;
            value >>= 2;
        }
#ifdef DEBUG
        unsigned char current = this->readPrimaryStatus();
        if (previous != current) {
            printf("Primary Slot Changed: %d-%d:%d-%d:%d-%d:%d-%d\n", primaryNumber(0), secondaryNumber(0), primaryNumber(1), secondaryNumber(1), primaryNumber(2), secondaryNumber(2), primaryNumber(3), secondaryNumber(3));
        }
#endif
    }

    inline unsigned char readSecondaryStatus()
    {
        unsigned char result = 0;
        for (int i = 0; i < 4; i++) {
            result <<= 2;
            result |= (this->ctx.slot[i] & 0b00001100) >> 2;
        }
        return ~result;
    }

    inline void changeSecondarySlots(unsigned char value)
    {
#ifdef DEBUG
        unsigned char previous = this->readSecondaryStatus();
#endif
        for (int i = 0; i < 4; i++) {
            int sn = value & 0b00000011;
            value >>= 2;
            if (this->hasSlot(i, sn)) {
                this->ctx.slot[i] &= 0b11110011;
                this->ctx.slot[i] |= sn << 2;
            }
        }
#ifdef DEBUG
        unsigned char current = this->readSecondaryStatus();
        if (previous != current) {
            printf("Secondary Slot Changed: %d-%d:%d-%d:%d-%d:%d-%d\n", primaryNumber(0), secondaryNumber(0), primaryNumber(1), secondaryNumber(1), primaryNumber(2), secondaryNumber(2), primaryNumber(3), secondaryNumber(3));
        }
#endif
    }

    inline unsigned char read(unsigned short addr)
    {
        int pn = addr / 0x4000;
        int ps = this->primaryNumber(pn);
        int ss = this->secondaryNumber(pn);
        int sa = 0;
        for (int i = pn - 1; 0 <= i; i--) {
            if (ps == this->primaryNumber(i) && ss == this->secondaryNumber(i)) {
                sa++;
            } else {
                break;
            }
        }
        ss += sa;
        return this->read(ps & 3, ss & 3, addr);
    }

    inline unsigned char read(int ps, int ss, unsigned short addr)
    {
        return this->slots[ps].ptr[ss] ? this->slots[ps].ptr[ss][addr & 0x3FFF] : 0xFF;
    }

    inline void write(unsigned short addr, unsigned char value)
    {
        int pn = addr / 0x4000;
        int ps = this->primaryNumber(pn);
        int ss = this->secondaryNumber(pn);
        int sa = 0;
        for (int i = pn - 1; 0 <= i; i--) {
            if (ps == this->primaryNumber(i) && ss == this->secondaryNumber(i)) {
                sa++;
            } else {
                break;
            }
        }
        ss += sa;
        this->write(ps & 3, ss & 3, addr, value);
    }

    inline void write(int ps, int ss, unsigned short addr, unsigned char value)
    {
        if (this->slots[ps].isReadOnly[ss]) return;
        if (!this->slots[ps].ptr[ss]) return;
        this->slots[ps].ptr[ss][addr & 0x3FFF] = value;
    }
};

#endif // INCLUDE_MSXSLOT_HPP
