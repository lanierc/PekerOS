#ifndef AC97_H
#define AC97_H

#include "common.h"

// AC97 Mixer Registers (NAMBAR)
#define AC97_RESET          0x00
#define AC97_MASTER_VOLUME  0x02
#define AC97_AUX_OUT_VOLUME 0x04
#define AC97_MONO_VOLUME    0x06
#define AC97_PC_BEEP        0x0A
#define AC97_PCM_OUT_VOLUME 0x18
#define AC97_EXT_AUDIO_CTRL 0x2A
#define AC97_EXT_AUDIO_STAT 0x2C
#define AC97_PCM_FRONT_DAC_RATE 0x2C

// AC97 Bus Master Registers (NABMBAR)
#define AC97_PI_BDBAR       0x00 // PCM In Buffer Descriptor list Base Address
#define AC97_PI_CIV         0x04 // PCM In Current Index Value
#define AC97_PI_LVI         0x05 // PCM In Last Valid Index
#define AC97_PI_SR          0x06 // PCM In Status Register
#define AC97_PI_PICB        0x08 // PCM In Position In Current Buffer
#define AC97_PI_PIV         0x0A // PCM In Prefetched Index Value
#define AC97_PI_CR          0x0B // PCM In Control Register

#define AC97_PO_BDBAR       0x10 // PCM Out Buffer Descriptor list Base Address
#define AC97_PO_CIV         0x14 // PCM Out Current Index Value
#define AC97_PO_LVI         0x15 // PCM Out Last Valid Index
#define AC97_PO_SR          0x16 // PCM Out Status Register
#define AC97_PO_PICB        0x18 // PCM Out Position In Current Buffer
#define AC97_PO_PIV         0x1A // PCM Out Prefetched Index Value
#define AC97_PO_CR          0x1B // PCM Out Control Register

#define AC97_MC_BDBAR       0x20 // Mic In Buffer Descriptor list Base Address
#define AC97_MC_CIV         0x24
#define AC97_MC_LVI         0x25
#define AC97_MC_SR          0x26
#define AC97_MC_PICB        0x28
#define AC97_MC_PIV         0x2A
#define AC97_MC_CR          0x2B

// Buffer Descriptor (BD) structure
struct ac97_bd {
    uint32_t buffer_addr; // Physical address of the buffer
    uint16_t length;      // Length in samples (not bytes)
    uint16_t flags;       // Bit 15: IOC (Interrupt On Completion), Bit 14: BUP
} __attribute__((packed));

void init_ac97(unsigned int nambar, unsigned int nabmbar, unsigned char irq);
void ac97_play(uint8_t* pcm_data, uint32_t length);

#endif
