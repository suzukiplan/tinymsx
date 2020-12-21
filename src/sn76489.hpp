#ifndef INCLUDE_SN76489_HPP
#define INCLUDE_SN76489_HPP

#include <string.h>
#define SAMPLE_RATE 44100.0
#define PSG_SHIFT 16

class SN76489
{
  private:
    unsigned int clock;
    unsigned char levels[16];

  public:
    struct Context {
        int b;
        int i;
        unsigned int r[8];
        unsigned int c[4];
        unsigned int e[4];
        unsigned int np;
        unsigned int ns;
        unsigned int nx;
    } ctx;

    void reset(int cpuRate)
    {
        memset(&ctx, 0, sizeof(ctx));
        this->clock = cpuRate / SAMPLE_RATE * (1 << PSG_SHIFT);
        unsigned char levels[16] = {255, 180, 127, 90, 63, 44, 31, 22, 15, 10, 7, 5, 3, 2, 1, 0};
        memcpy(this->levels, &levels, sizeof(levels));
    }

    inline void write(unsigned char value)
    {
        if (value & 0x80) {
            this->ctx.i = (value >> 4) & 7;
            this->ctx.r[this->ctx.i] = value & 0x0f;
        } else {
            this->ctx.r[this->ctx.i] |= (value & 0x3f) << 4;
        }
        switch (this->ctx.r[6] & 3) {
            case 0: this->ctx.np = 1; break;
            case 1: this->ctx.np = 2; break;
            case 2: this->ctx.np = 4; break;
            case 3: this->ctx.np = this->ctx.r[4]; break;
        }
        this->ctx.nx = (this->ctx.r[6] & 0x04) ? 0x12000 : 0x08000;
    }

    inline void execute(short* left, short* right)
    {
        for (int i = 0; i < 3; i++) {
            int regidx = i << 1;
            if (this->ctx.r[regidx]) {
                unsigned int cc = this->clock + this->ctx.c[i];
                while ((cc & 0x80000000) == 0) {
                    cc -= (this->ctx.r[regidx] << (PSG_SHIFT + 4));
                    this->ctx.e[i] ^= 1;
                }
                this->ctx.c[i] = cc;
            } else {
                this->ctx.e[i] = 1;
            }
        }
        if (this->ctx.np) {
            unsigned int cc = this->clock + this->ctx.c[3];
            while ((cc & 0x80000000) == 0) {
                cc -= (this->ctx.np << (PSG_SHIFT + 4));
                this->ctx.ns >>= 1;
                if (this->ctx.ns & 1) {
                    this->ctx.ns = this->ctx.ns ^ this->ctx.nx;
                    this->ctx.e[3] = 1;
                } else {
                    this->ctx.e[3] = 0;
                }
            }
            this->ctx.c[3] = cc;
        }
        int w = 0;
        if (this->ctx.e[0]) w += this->levels[this->ctx.r[1]];
        if (this->ctx.e[1]) w += this->levels[this->ctx.r[3]];
        if (this->ctx.e[2]) w += this->levels[this->ctx.r[5]];
        if (this->ctx.e[3]) w += this->levels[this->ctx.r[7]];
        w <<= 4;
        w = (short)w;
        w *= 45;
        w /= 10;
        if (32767 < w) {
            w = 32767;
        } else if (w < -32768) {
            w = -32768;
        }
        *left = (short)w;
        *right = (short)w;
    }
};

#endif // INCLUDE_SN76489_HPP
