#ifndef INCLUDE_AY8910_HPP
#define INCLUDE_AY8910_HPP

#include <string.h>
#define SAMPLE_RATE 44100.0
#define PSG_SHIFT 16

class AY8910
{
  private:
    unsigned int clock;
    unsigned char levels[16];

  public:
    struct Context {
        int clocks;
        unsigned char latch;
        unsigned char tmask[3];
        unsigned char nmask[3];
        unsigned char reserved[5];
        unsigned char reg[0x20];
        unsigned int count[3];
        unsigned int volume[3];
        unsigned int freq[3];
        unsigned int edge[3];
        struct Envelope {
            unsigned int volume;
            unsigned int ptr;
            unsigned int face;
            unsigned int cont;
            unsigned int attack;
            unsigned int alternate;
            unsigned int hold;
            unsigned int pause;
            unsigned int reset;
            unsigned int freq;
            unsigned int count;
            unsigned int reserved;
        } env;
        struct Noise {
            unsigned int seed;
            unsigned int count;
            unsigned int freq;
            unsigned int reserved;
        } noise;
        short ch_out[4];
    } ctx;

    void reset()
    {
        unsigned char levels[16] = {0, 1, 2, 3, 5, 7, 11, 15, 22, 31, 44, 63, 90, 127, 180, 255};
        memcpy(this->levels, &levels, sizeof(levels));
        for (int i = 0; i < 3; i++) this->ctx.count[i] = 0x1000;
        this->ctx.noise.seed = 0xffff;
        this->ctx.noise.count = 0x40;
        this->ctx.env.pause = 1;
    }

    inline void latch(unsigned char value) { this->ctx.latch = value & 0x1F; }
    inline unsigned char read() { return this->ctx.reg[this->ctx.latch]; }
    inline unsigned char getPad1() { return this->ctx.reg[0x0E]; }
    inline unsigned char getPad2() { return this->ctx.reg[0x0F]; }

    inline void setPads(unsigned char pad1, unsigned char pad2)
    {
        this->ctx.reg[0x0E] = ~pad1;
        this->ctx.reg[0x0F] = ~pad2;
    }

    inline void write(unsigned char value)
    {
        int reg = this->ctx.latch;
        this->ctx.reg[reg] = value;
        if (reg < 6) {
            value &= reg & 1 ? 0x0F : 0xFF;
            int c = reg >> 1;
            this->ctx.freq[c] = ((this->ctx.reg[c * 2 + 1] & 15) << 8) + this->ctx.reg[c * 2];
        } else if (6 == reg) {
            this->ctx.noise.freq = (value & 0x1F) << 1;
        } else if (7 == reg) {
            this->ctx.tmask[0] = value & 1;
            this->ctx.tmask[1] = value & 2;
            this->ctx.tmask[2] = value & 4;
            this->ctx.nmask[0] = value & 8;
            this->ctx.nmask[1] = value & 16;
            this->ctx.nmask[2] = value & 32;
        } else if (reg < 11) {
            this->ctx.volume[reg - 8] = (value & 0x1F) << 1;
        } else if (reg < 13) {
            this->ctx.env.freq = (this->ctx.reg[12] << 8) + this->ctx.reg[11];
        } else if (13 == reg) {
            value &= 0x0F;
            this->ctx.env.cont = (value >> 3) & 1;
            this->ctx.env.attack = (value >> 2) & 1;
            this->ctx.env.alternate = (value >> 1) & 1;
            this->ctx.env.hold = value & 1;
            this->ctx.env.face = this->ctx.env.attack;
            this->ctx.env.pause = 0;
            this->ctx.env.count = 0x10000 - this->ctx.env.freq;
            this->ctx.env.ptr = this->ctx.env.face ? 0 : 0x1F;
        }
    }

    inline void execute(short* left, short* right)
    {
        // Envelope
        this->ctx.env.count += 5;
        while (this->ctx.env.count >= 0x10000 && this->ctx.env.freq != 0) {
            if (!this->ctx.env.pause) {
                if (this->ctx.env.face) {
                    this->ctx.env.ptr = (this->ctx.env.ptr + 1) & 0x3f;
                } else {
                    this->ctx.env.ptr = (this->ctx.env.ptr + 0x3f) & 0x3f;
                }
            }
            if (this->ctx.env.ptr & 0x20) {
                if (this->ctx.env.cont) {
                    if (this->ctx.env.alternate ^ this->ctx.env.hold) this->ctx.env.face ^= 1;
                    if (this->ctx.env.hold) this->ctx.env.pause = 1;
                    this->ctx.env.ptr = this->ctx.env.face ? 0 : 0x1f;
                } else {
                    this->ctx.env.pause = 1;
                    this->ctx.env.ptr = 0;
                }
            }
            this->ctx.env.count -= this->ctx.env.freq;
        }

        // Noise
        this->ctx.noise.count += 5;
        if (this->ctx.noise.count & 0x40) {
            if (this->ctx.noise.seed & 1) {
                this->ctx.noise.seed ^= 0x24000;
            }
            this->ctx.noise.seed >>= 1;
            this->ctx.noise.count -= this->ctx.noise.freq ? this->ctx.noise.freq : (1 << 1);
        }
        int noise = this->ctx.noise.seed & 1;

        // Tone
        for (int i = 0; i < 3; i++) {
            this->ctx.count[i] += 5;
            if (this->ctx.count[i] & 0x1000) {
                if (this->ctx.freq[i] > 1) {
                    this->ctx.edge[i] = !this->ctx.edge[i];
                    this->ctx.count[i] -= this->ctx.freq[i];
                } else {
                    this->ctx.edge[i] = 1;
                }
            }
            if ((this->ctx.tmask[i] || this->ctx.edge[i]) && (this->ctx.nmask[i] || noise)) {
                this->ctx.ch_out[i] += (this->levels[(this->ctx.volume[i] & 0x1F) >> 1] << 4);
            }
            this->ctx.ch_out[i] >>= 1;
        }
        int w = this->ctx.ch_out[0];
        w += this->ctx.ch_out[1];
        w += this->ctx.ch_out[2];
        if (32767 < w) w = 32767;
        if (w < -32768) w = -32768;
        *left = (short)w;
        *right = *left;
    }
};

#endif // INCLUDE_AY8910_HPP
