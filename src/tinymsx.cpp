#include "tinymsx.h"

static unsigned char readMemory(void* arg, unsigned short addr)
{
    return 0;
}

static void writeMemory(void* arg, unsigned short addr, unsigned char value)
{
}

static unsigned char inPort(void* arg, unsigned char port)
{
    return 0;
}

static void outPort(void* arg, unsigned char port, unsigned char value)
{
}

TinyMSX::TinyMSX()
{
    this->cpu = new Z80(readMemory, writeMemory, inPort, outPort, this);
}

TinyMSX::~TinyMSX()
{
    if (this->cpu) delete this->cpu;
    this->cpu = NULL;
}