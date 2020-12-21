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
    void reset()
    {
        memset(&this->pages, 0, sizeof(this->pages));
    }

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

    inline unsigned char read(int num, int extra, unsigned short addr)
    {
        return this->pages[num].ptr[extra] ? this->pages[num].ptr[extra][addr & 0x3FFF] : 0xFF;
    }

    inline void write(int num, int extra, unsigned short addr, unsigned char value)
    {
        if (this->pages[num].isReadOnly[extra]) return;
        if (!this->pages[num].ptr[extra]) return;
        this->pages[num].ptr[extra][addr & 0x3FFF] = value;
    }
};

#endif // INCLUDE_MSXSLOT_HPP
