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
    struct Page {
        unsigned char* ptr[4];
        bool isReadOnly[4];
    } pages[4];

  public:
    struct Context {
        unsigned char page[4];
        unsigned char slot[4];
    } ctx;

    void reset()
    {
        memset(&this->pages, 0, sizeof(this->pages));
        memset(&this->ctx, 0, sizeof(this->ctx));
    }

    inline void setupPage(int index, int slotNumber) { this->ctx.page[index] = slotNumber; }
    inline void setupSlot(int index, int slotStatus) { this->ctx.slot[index] = slotStatus; }
    inline bool hasSlot(int ps, int ss) { return this->pages[ps].ptr[ss] ? true : false; }

    bool add(int ps, int ss, unsigned char* data, bool isReadOnly)
    {
        this->pages[ps].ptr[ss] = data;
        this->pages[ps].isReadOnly[ss] = isReadOnly;
        return true;
    }

    inline unsigned char readPrimaryStatus()
    {
        unsigned char result = 0;
        result |= (this->ctx.slot[3] & 0b00000011) << 6;
        result |= (this->ctx.slot[2] & 0b00000011) << 4;
        result |= (this->ctx.slot[1] & 0b00000011) << 2;
        result |= (this->ctx.slot[0] & 0b00000011);
        return result;
    }

    inline void changePrimarySlots(unsigned char value)
    {
#ifdef DEBUG
        unsigned char previous = this->readPrimaryStatus();
#endif
        for (int i = 0; i < 4; i++) {
            this->ctx.slot[i] &= 0b11111100;
        }
        this->ctx.slot[3] |= value >> 6 & 3;
        this->ctx.slot[2] |= value >> 4 & 3;
        this->ctx.slot[1] |= value >> 2 & 3;
        this->ctx.slot[0] |= value & 3;
#ifdef DEBUG
        unsigned char current = this->readPrimaryStatus();
        if (previous != current) {
            printf("Primary Slot Changed: %d-%d:%d-%d:%d-%d:%d-%d\n", getPrimarySlotNumber(0), getSecondarySlotNumber(0), getPrimarySlotNumber(1), getSecondarySlotNumber(1), getPrimarySlotNumber(2), getSecondarySlotNumber(2), getPrimarySlotNumber(3), getSecondarySlotNumber(3));
        }
#endif
    }

    inline unsigned char readSecondaryStatus()
    {
        unsigned char result = 0;
        result |= (this->ctx.slot[0] & 0b00001100) >> 2;
        result |= (this->ctx.slot[1] & 0b00001100);
        result |= (this->ctx.slot[2] & 0b00001100) << 2;
        result |= (this->ctx.slot[3] & 0b00001100) << 4;
        return ~result;
    }

    inline void changeSecondarySlots(unsigned char value)
    {
#ifdef DEBUG
        unsigned char previous = this->readSecondaryStatus();
#endif
        this->changeSecondarySlot(0, value & 0b00000011);
        this->changeSecondarySlot(1, value & 0b00001100 >> 2);
        this->changeSecondarySlot(2, value & 0b00110000 >> 4);
        this->changeSecondarySlot(3, value & 0b11000000 >> 6);
#ifdef DEBUG
        unsigned char current = this->readSecondaryStatus();
        if (previous != current) {
            printf("Secondary Slot Changed: %d-%d:%d-%d:%d-%d:%d-%d\n", getPrimarySlotNumber(0), getSecondarySlotNumber(0), getPrimarySlotNumber(1), getSecondarySlotNumber(1), getPrimarySlotNumber(2), getSecondarySlotNumber(2), getPrimarySlotNumber(3), getSecondarySlotNumber(3));
        }
#endif
    }

    inline void changeSecondarySlot(int pn, int sn)
    {
        if (this->hasSlot(pn, sn)) {
            this->ctx.slot[pn] &= 0b11110011;
            this->ctx.slot[pn] |= sn ? 0x80 : 0;
            this->ctx.slot[pn] |= sn << 2;
        }
    }

    inline int getPrimarySlotNumber(int page) { return this->ctx.slot[this->ctx.page[page]] & 0b11; }
    inline int getSecondarySlotNumber(int page) { return (this->ctx.slot[this->ctx.page[page]] & 0b1100) >> 2; }

    inline unsigned char read(unsigned short addr)
    {
        int pn = addr / 0x4000;
        int ps = this->getPrimarySlotNumber(pn);
        int ss = this->getSecondarySlotNumber(pn);
        int sa = 0;
        for (int i = 0; i < pn; i++) {
            if (ps == this->getPrimarySlotNumber(i) && ss == this->getSecondarySlotNumber(i)) {
                sa++;
            }
        }
        ss += sa;
        return this->read(ps & 3, ss & 3, addr);
    }

    inline unsigned char read(int ps, int ss, unsigned short addr)
    {
        return this->pages[ps].ptr[ss] ? this->pages[ps].ptr[ss][addr & 0x3FFF] : 0xFF;
    }

    inline void write(unsigned short addr, unsigned char value)
    {
        int pn = addr / 0x4000;
        int ps = this->getPrimarySlotNumber(pn);
        int ss = this->getSecondarySlotNumber(pn);
        int sa = 0;
        for (int i = 0; i < pn; i++) {
            if (ps == this->getPrimarySlotNumber(i) && ss == this->getSecondarySlotNumber(i)) {
                sa++;
            }
        }
        ss += sa;
        this->write(ps & 3, ss & 3, addr, value);
    }

    inline void write(int ps, int ss, unsigned short addr, unsigned char value)
    {
        if (this->pages[ps].isReadOnly[ss]) return;
        if (!this->pages[ps].ptr[ss]) return;
        this->pages[ps].ptr[ss][addr & 0x3FFF] = value;
    }
};

#endif // INCLUDE_MSXSLOT_HPP
