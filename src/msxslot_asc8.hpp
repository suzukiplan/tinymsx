
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

#include <stdio.h>
#include <string.h>

class MsxSlotASC8
{
  private:
    struct Slot {
        unsigned char* ptr[16];
        bool isReadOnly[16];
    } slots[4];

  public:
    unsigned char* rom;
    struct Context {
        unsigned char page[4];
        unsigned char slot[4];
        unsigned char seg[4];
        unsigned char reserved[4];
    } ctx;
    unsigned int bankSwitchStart;
    unsigned int bankSwitchEnd;
    unsigned int bankSwitchInterval;

    void init(unsigned char* rom) {
        this->rom = rom;
        this->bankSwitchStart = 0x6000;   // 0x4000 (KONAMI)
        this->bankSwitchEnd = 0x8000;     // 0xC000 (KONAMI)
        this->bankSwitchInterval = 0x800; // 0x2000 (KONAMI)
    }

    inline void setupPage(int index, int slotNumber) { this->ctx.page[index] = slotNumber; }
    inline void setupSlot(int index, int slotStatus) { this->ctx.slot[index] = slotStatus; }
    inline bool hasSlot(int ps, int ss) { return this->slots[ps].ptr[ss] ? true : false; }
    inline int primaryNumber(int page) { return this->ctx.slot[this->ctx.page[page]] & 0b11; }
    inline int secondaryNumber(int page) { return this->ctx.slot[this->ctx.page[page]] & 0b1100; }

    void reset()
    {
        memset(&this->slots, 0, sizeof(this->slots));
        memset(&this->ctx, 0, sizeof(this->ctx));
        this->reloadBank();
    }

    inline void add(int ps, int ss, unsigned char* data, bool isReadOnly)
    {
        ss <<= 2;
        for (int i = 0; i < 4; i++, data += 0x1000, ss++) {
            this->slots[ps].ptr[ss] = data;
            this->slots[ps].isReadOnly[ss] = isReadOnly;
        }
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
        for (int i = 0; i < 4; i++, value >>= 2) {
            this->ctx.slot[i] &= 0b11111100;
            this->ctx.slot[i] |= value & 0b00000011;
        }
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
        for (int i = 0; i < 4; i++, value >>= 2) {
            int sn = (value & 0b00000011) << 2;
            if (this->hasSlot(i, sn)) {
                this->ctx.slot[i] &= 0b11110011;
                this->ctx.slot[i] |= sn;
            }
        }
    }

    inline unsigned char read(unsigned short addr)
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
        if (1 == ps && this->bankSwitchStart <= addr && addr < this->bankSwitchEnd) this->switchBank((addr - this->bankSwitchStart) / this->bankSwitchInterval, value);
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
            seg <<= 13;
            this->slots[1].isReadOnly[i] = true;
            this->slots[1].ptr[i] = &this->rom[seg + (i & 1 ? 0x1000 : 0)];
        }
    }
};

#endif // INCLUDE_MSXSLOT_ASC8_HPP
