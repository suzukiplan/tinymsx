#ifndef INCLUDE_MSXSLOT_HPP
#define INCLUDE_MSXSLOT_HPP

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

    void setupPage(int index, int slotNumber) { this->ctx.page[index] = slotNumber; }
    void setupSlot(int index, int slotStatus) { this->ctx.slot[index] = slotStatus; }

    bool add(int num, int extra, unsigned char* data, bool isReadOnly)
    {
        this->pages[num].ptr[extra] = data;
        this->pages[num].isReadOnly[extra] = isReadOnly;
        return true;
    }

    inline bool hasSlot(int num, int extra)
    {
        return this->pages[num].ptr[extra] ? true : false;
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
        for (int i = 0; i < 4; i++) {
            this->ctx.slot[i] &= 0b11111100;
        }
        this->ctx.slot[3] |= value >> 6 & 3;
        this->ctx.slot[2] |= value >> 4 & 3;
        this->ctx.slot[1] |= value >> 2 & 3;
        this->ctx.slot[0] |= value & 3;
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
        this->setSecondaryPage(0, value & 0b00000011);
        this->setSecondaryPage(1, value & 0b00001100 >> 2);
        this->setSecondaryPage(2, value & 0b00110000 >> 4);
        this->setSecondaryPage(3, value & 0b11000000 >> 6);
    }

    inline void setSecondaryPage(int slot, int extra)
    {
        if (this->hasSlot(slot, extra)) {
            this->ctx.slot[slot] &= 0b11110011;
            this->ctx.slot[slot] |= extra ? 0x80 : 0;
            this->ctx.slot[slot] |= extra << 2;
        }
    }

    inline int getPrimarySlotNumber(int page) { return this->ctx.slot[this->ctx.page[page]] & 0b11; }
    inline int getSecondarySlotNumber(int page) { return (this->ctx.slot[this->ctx.page[page]] & 0b1100) >> 2; }

    inline unsigned char read(unsigned short addr)
    {
        int pn = addr / 0x4000;
        int ps = this->getPrimarySlotNumber(pn);
        int ss = this->getSecondarySlotNumber(pn);
        switch (pn) {
            case 0:
            case 1:
                if (ps == this->getPrimarySlotNumber(pn - 1) && ss == this->getSecondarySlotNumber(pn - 1)) {
                    ss++;
                }
                break;
            case 2:
                if (ps == this->getPrimarySlotNumber(pn - 1) && ss == this->getSecondarySlotNumber(pn - 1)) {
                    if (ps == this->getPrimarySlotNumber(pn - 2) && ss == this->getSecondarySlotNumber(pn - 2)) {
                        ss += 2;
                    } else {
                        ss++;
                    }
                }
                break;
            case 3:
                if (ps == this->getPrimarySlotNumber(pn - 1) && ss == this->getSecondarySlotNumber(pn - 1)) {
                    if (ps == this->getPrimarySlotNumber(pn - 2) && ss == this->getSecondarySlotNumber(pn - 2)) {
                        if (ps == this->getPrimarySlotNumber(pn - 3) && ss == this->getSecondarySlotNumber(pn - 3)) {
                            ss += 3;
                        } else {
                            ss += 2;
                        }
                    } else {
                        ss++;
                    }
                }
                break;
        }
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
        switch (pn) {
            case 0:
            case 1:
                if (ps == this->getPrimarySlotNumber(pn - 1) && ss == this->getSecondarySlotNumber(pn - 1)) {
                    ss++;
                }
                break;
            case 2:
                if (ps == this->getPrimarySlotNumber(pn - 1) && ss == this->getSecondarySlotNumber(pn - 1)) {
                    if (ps == this->getPrimarySlotNumber(pn - 2) && ss == this->getSecondarySlotNumber(pn - 2)) {
                        ss += 2;
                    } else {
                        ss++;
                    }
                }
                break;
            case 3:
                if (ps == this->getPrimarySlotNumber(pn - 1) && ss == this->getSecondarySlotNumber(pn - 1)) {
                    if (ps == this->getPrimarySlotNumber(pn - 2) && ss == this->getSecondarySlotNumber(pn - 2)) {
                        if (ps == this->getPrimarySlotNumber(pn - 3) && ss == this->getSecondarySlotNumber(pn - 3)) {
                            ss += 3;
                        } else {
                            ss += 2;
                        }
                    } else {
                        ss++;
                    }
                }
                break;
        }
        this->write(ps, ss, addr, value);
    }

    inline void write(int ps, int ss, unsigned short addr, unsigned char value)
    {
        if (this->pages[ps].isReadOnly[ss]) return;
        if (!this->pages[ps].ptr[ss]) return;
        this->pages[ps].ptr[ss][addr & 0x3FFF] = value;
    }
};

#endif // INCLUDE_MSXSLOT_HPP
