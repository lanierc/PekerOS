#include "ac97.h"
#include "kheap.h"
#include "pmm.h"

static unsigned int ac97_nambar = 0;
static unsigned int ac97_nabmbar = 0;
static struct ac97_bd *bdl = 0; // Buffer Descriptor List

// AC97 Mixer Out/In helpers
static void ac97_mixer_outw(unsigned short reg, unsigned short val) {
    outw(ac97_nambar + reg, val);
}

static unsigned short ac97_mixer_inw(unsigned short reg) {
    return inw(ac97_nambar + reg);
}

// AC97 Bus Master Out/In helpers
static void ac97_bm_outb(unsigned short reg, unsigned char val) {
    outb(ac97_nabmbar + reg, val);
}

static void ac97_bm_outw(unsigned short reg, unsigned short val) {
    outw(ac97_nabmbar + reg, val);
}

static void ac97_bm_outl(unsigned short reg, unsigned int val) {
    outl(ac97_nabmbar + reg, val);
}

static unsigned char ac97_bm_inb(unsigned short reg) {
    return inb(ac97_nabmbar + reg);
}

void init_ac97(unsigned int nambar, unsigned int nabmbar, unsigned char irq) {
    put_str("[AC97] Baslatiliyor... NAMBAR: ");
    put_hex(nambar); put_str(" NABMBAR: "); put_hex(nabmbar);
    put_str("\n");
    
    ac97_nambar = nambar;
    ac97_nabmbar = nabmbar;
    
    // 1. Reset
    ac97_mixer_outw(AC97_RESET, 1);
    
    // 2. Set Volumes (0 is max volume usually, or 0x0000 for Master and PCM)
    // Bits 0-5 are volume (0=loud, 0x3F=mute). Bit 15 is Mute.
    ac97_mixer_outw(AC97_MASTER_VOLUME, 0x0000); 
    ac97_mixer_outw(AC97_PCM_OUT_VOLUME, 0x0000);
    
    // 3. Allocate Buffer Descriptor List (must be aligned, we'll allocate 1 frame)
    unsigned int bdl_phys = alloc_frame();
    bdl = (struct ac97_bd*)(bdl_phys * 4096); 
    // Clear BDL
    for(int i=0; i<32; i++) {
        bdl[i].buffer_addr = 0;
        bdl[i].length = 0;
        bdl[i].flags = 0;
    }
    
    // 4. Reset Bus Master Control Register
    ac97_bm_outb(AC97_PO_CR, 0x02); // Reset (RR bit)
    // Wait until it clears or just small delay
    for(volatile int i=0; i<10000; i++);
    
    // 5. Set BDBAR
    ac97_bm_outl(AC97_PO_BDBAR, (unsigned int)bdl);
    
    put_str("[AC97] Hazir!\n");
}

void ac97_play(uint8_t* pcm_data, uint32_t length) {
    if (!ac97_nabmbar) return; // Driver not initialized
    
    // length is in bytes. Length in BD is in samples. 
    // 1 sample = 2 bytes. 
    uint16_t num_samples = length / 2;
    
    // Setup first BD
    bdl[0].buffer_addr = (uint32_t)pcm_data; // Note: must be physical addr. In our kernel, identity mapped.
    bdl[0].length = num_samples;
    bdl[0].flags = 0x8000; // IOC
    
    // Set LVI (Last Valid Index)
    ac97_bm_outb(AC97_PO_LVI, 0);
    
    // Start playback (Run/Pause bit = 1)
    ac97_bm_outb(AC97_PO_CR, 0x01);
}
