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
#ifndef INCLUDE_TINYMSX_H
#define INCLUDE_TINYMSX_H
#include <stdio.h>
#include "z80.hpp"
#include "tinymsx_def.h"
#include "msxslot.hpp"
#include "msxslot_asc8.hpp"
#include "msxslot_asc8w.hpp"
#include "msxslot_asc8x.hpp"
#include "msxslot_gm2.hpp"
#include "tms9918a.hpp"
#include "sn76489.hpp"
#include "ay8910.hpp"

class TinyMSX {
    private:
        struct MsxBIOS {
            unsigned char main[0x8000];
        } bios;
        int type;
        unsigned char pad[2];
        unsigned char specialKeyX[2];
        unsigned char specialKeyY[2];
        unsigned char* rom;
        size_t romSize;
        size_t ramSize;
        short soundBuffer[65536];
        unsigned short soundBufferCursor;
        unsigned char tmpBuffer[1024 * 1024];
    public:
        TMS9918A* tms9918;
        SN76489 sn76489;
        AY8910 ay8910;
        unsigned char io[0x100];
        unsigned char ram[0x10000];
        MsxSlot slot;
        MsxSlotGM2 slotGM2;
        MsxSlotASC8 slotASC8;
        MsxSlotASC8W slotASC8W;
        MsxSlotASC8X slotASC8X;
        Z80* cpu;
        TinyMSX(int type, const void* rom, size_t romSize, size_t ramSize, int colorMode);
        ~TinyMSX();
        bool loadBiosFromFile(const char* path);
        bool loadBiosFromMemory(void* bios, size_t size);
        void setupSpecialKey1(unsigned char ascii, bool isTenKey = false);
        void setupSpecialKey2(unsigned char ascii, bool isTenKey = false);
        void reset();
        void tick(unsigned char pad1, unsigned char pad2);
        unsigned short* getDisplayBuffer() { return this->tms9918->display; }
        unsigned short getBackdropColor() { return this->tms9918->getBackdropColor(); }
        void* getSoundBuffer(size_t* size);
        const void* saveState(size_t* size);
        void loadState(const void* data, size_t size);
        inline bool isSG1000() { return this->type == TINYMSX_TYPE_SG1000; }
        inline bool isMSX1() { return this->type == TINYMSX_TYPE_MSX1; }
        inline bool isMSX1_GameMaster2() { return this->type == TINYMSX_TYPE_MSX1_GameMaster2; }
        inline bool isMSX1_ASC8() { return this->type == TINYMSX_TYPE_MSX1_ASC8; }
        inline bool isMSX1_ASC8W() { return this->type == TINYMSX_TYPE_MSX1_ASC8W; }
        inline bool isMSX1_ASC8X() { return this->type == TINYMSX_TYPE_MSX1_ASC8X; }
        inline bool isMSX1Family() { return this->isMSX1() || this->isMSX1_GameMaster2() || this->isMSX1_ASC8() || this->isMSX1_ASC8W() || this->isMSX1_ASC8X(); }

    private:
        inline void setupSpecialKeyV(int n, int x, int y) {
            this->specialKeyX[n] = x;
            this->specialKeyY[n] = y;
        }
        void setupSpecialKey(int n, unsigned char ascii, bool isTenKey);
        inline unsigned char readMemory(unsigned short addr);
        inline void writeMemory(unsigned short addr, unsigned char value);
        inline unsigned char inPort(unsigned char port);
        inline void outPort(unsigned char port, unsigned char value);
        inline void consumeClock(int clocks);
        inline bool loadSpecificSizeFile(const char* path, void* buffer, size_t size);
        size_t calcAvairableRamSize();

        inline void slot_reset() {
            if (this->isMSX1()) this->slot.reset();
            else if (this->isMSX1_GameMaster2()) this->slotGM2.reset();
            else if (this->isMSX1_ASC8()) this->slotASC8.reset();
            else if (this->isMSX1_ASC8W()) this->slotASC8W.reset();
            else if (this->isMSX1_ASC8X()) this->slotASC8X.reset();
        }

        inline void slot_setupPage(int index, int slotNumber) {
            if (this->isMSX1()) this->slot.setupPage(index, slotNumber);
            else if (this->isMSX1_GameMaster2()) this->slotGM2.setupPage(index, slotNumber);
            else if (this->isMSX1_ASC8()) this->slotASC8.setupPage(index, slotNumber);
            else if (this->isMSX1_ASC8W()) this->slotASC8W.setupPage(index, slotNumber);
            else if (this->isMSX1_ASC8X()) this->slotASC8X.setupPage(index, slotNumber);
        }

        inline void slot_setupSlot(int index, int slotStatus) {
            if (this->isMSX1()) this->slot.setupSlot(index, slotStatus);
            else if (this->isMSX1_GameMaster2()) this->slotGM2.setupSlot(index, slotStatus);
            else if (this->isMSX1_ASC8()) this->slotASC8.setupSlot(index, slotStatus);
            else if (this->isMSX1_ASC8W()) this->slotASC8W.setupSlot(index, slotStatus);
            else if (this->isMSX1_ASC8X()) this->slotASC8X.setupSlot(index, slotStatus);
        }
    
        inline void slot_add(int ps, int ss, unsigned char* data, bool isReadOnly) {
            if (this->isMSX1()) this->slot.add(ps, ss, data, isReadOnly);
            else if (this->isMSX1_GameMaster2()) this->slotGM2.add(ps, ss, data, isReadOnly);
            else if (this->isMSX1_ASC8()) this->slotASC8.add(ps, ss, data, isReadOnly);
            else if (this->isMSX1_ASC8W()) this->slotASC8W.add(ps, ss, data, isReadOnly);
            else if (this->isMSX1_ASC8X()) this->slotASC8X.add(ps, ss, data, isReadOnly);
        }

        inline unsigned char slot_readPrimaryStatus() {
            if (this->isMSX1()) return this->slot.readPrimaryStatus();
            else if (this->isMSX1_GameMaster2()) return this->slotGM2.readPrimaryStatus();
            else if (this->isMSX1_ASC8()) return this->slotASC8.readPrimaryStatus();
            else if (this->isMSX1_ASC8W()) return this->slotASC8W.readPrimaryStatus();
            else if (this->isMSX1_ASC8X()) return this->slotASC8X.readPrimaryStatus();
            else return 0xFF;
        }

        inline void slot_changePrimarySlots(unsigned char value) {
            if (this->isMSX1()) this->slot.changePrimarySlots(value);
            else if (this->isMSX1_GameMaster2()) this->slotGM2.changePrimarySlots(value);
            else if (this->isMSX1_ASC8()) this->slotASC8.changePrimarySlots(value);
            else if (this->isMSX1_ASC8W()) this->slotASC8W.changePrimarySlots(value);
            else if (this->isMSX1_ASC8X()) this->slotASC8X.changePrimarySlots(value);
        }

        inline unsigned char slot_readSecondaryStatus() {
            if (this->isMSX1()) return this->slot.readSecondaryStatus();
            else if (this->isMSX1_GameMaster2()) return this->slotGM2.readSecondaryStatus();
            else if (this->isMSX1_ASC8()) return this->slotASC8.readSecondaryStatus();
            else if (this->isMSX1_ASC8W()) return this->slotASC8W.readSecondaryStatus();
            else if (this->isMSX1_ASC8X()) return this->slotASC8X.readSecondaryStatus();
            else return 0xFF;
        }

        inline void slot_changeSecondarySlots(unsigned char value) {
            if (this->isMSX1()) this->slot.changeSecondarySlots(value);
            else if (this->isMSX1_GameMaster2()) this->slotGM2.changeSecondarySlots(value);
            else if (this->isMSX1_ASC8()) this->slotASC8.changeSecondarySlots(value);
            else if (this->isMSX1_ASC8W()) this->slotASC8W.changeSecondarySlots(value);
            else if (this->isMSX1_ASC8X()) this->slotASC8X.changeSecondarySlots(value);
        }

        inline unsigned char slot_read(unsigned short addr) {
            if (this->isMSX1()) return this->slot.read(addr);
            else if (this->isMSX1_GameMaster2()) return this->slotGM2.read(addr);
            else if (this->isMSX1_ASC8()) return this->slotASC8.read(addr);
            else if (this->isMSX1_ASC8W()) return this->slotASC8W.read(addr);
            else if (this->isMSX1_ASC8X()) return this->slotASC8X.read(addr);
            else return 0xFF;
        }

        inline void slot_write(unsigned short addr, unsigned char value) {
            if (this->isMSX1()) this->slot.write(addr, value);
            else if (this->isMSX1_GameMaster2()) this->slotGM2.write(addr, value);
            else if (this->isMSX1_ASC8()) this->slotASC8.write(addr, value);
            else if (this->isMSX1_ASC8W()) this->slotASC8W.write(addr, value);
            else if (this->isMSX1_ASC8X()) this->slotASC8X.write(addr, value);
        }
};

#endif
