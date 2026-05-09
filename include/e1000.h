#ifndef E1000_H
#define E1000_H

#include "common.h"

// E1000 Register Offsets
#define E1000_CTRL          0x0000  // Device Control
#define E1000_STATUS        0x0008  // Device Status
#define E1000_EECD          0x0010  // EEPROM Control
#define E1000_EERD          0x0014  // EEPROM Read
#define E1000_ICR           0x00C0  // Interrupt Cause Read
#define E1000_IMS           0x00D0  // Interrupt Mask Set
#define E1000_IMC           0x00D8  // Interrupt Mask Clear
#define E1000_RCTL          0x0100  // Receive Control
#define E1000_TCTL          0x0400  // Transmit Control
#define E1000_RDBAL         0x2800  // Receive Descriptor Base Address Low
#define E1000_RDBAH         0x2804  // Receive Descriptor Base Address High
#define E1000_RDLEN         0x2808  // Receive Descriptor Length
#define E1000_RDH           0x2810  // Receive Descriptor Head
#define E1000_RDT           0x2818  // Receive Descriptor Tail
#define E1000_TDBAL         0x3800  // Transmit Descriptor Base Address Low
#define E1000_TDBAH         0x3804  // Transmit Descriptor Base Address High
#define E1000_TDLEN         0x3808  // Transmit Descriptor Length
#define E1000_TDH           0x3810  // Transmit Descriptor Head
#define E1000_TDT           0x3818  // Transmit Descriptor Tail
#define E1000_RAL           0x5400  // Receive Address Low
#define E1000_RAH           0x5404  // Receive Address High

// Control Register Bits
#define E1000_CTRL_SLU      (1 << 6)   // Set Link Up
#define E1000_CTRL_ASDE     (1 << 5)   // Auto-Speed Detection Enable
#define E1000_CTRL_RST      (1 << 26)  // Device Reset

// Receive Control Bits
#define E1000_RCTL_EN       (1 << 1)   // Receiver Enable
#define E1000_RCTL_SBP      (1 << 2)   // Store Bad Packets
#define E1000_RCTL_UPE      (1 << 3)   // Unicast Promiscuous Enable
#define E1000_RCTL_MPE      (1 << 4)   // Multicast Promiscuous Enable
#define E1000_RCTL_LPE      (1 << 5)   // Long Packet Reception Enable
#define E1000_RCTL_BAM      (1 << 15)  // Broadcast Accept Mode
#define E1000_RCTL_BSIZE_2048 0x00000000
#define E1000_RCTL_SECRC    (1 << 26)  // Strip Ethernet CRC

// Transmit Control Bits
#define E1000_TCTL_EN       (1 << 1)   // Transmit Enable
#define E1000_TCTL_PSP      (1 << 3)   // Pad Short Packets
#define E1000_TCTL_CT       0x000000F0 // Collision Threshold
#define E1000_TCTL_COLD     0x0003F000 // Collision Distance

// Descriptors
struct e1000_rx_desc {
    uint64_t addr;
    uint16_t length;
    uint16_t checksum;
    uint8_t status;
    uint8_t errors;
    uint16_t special;
} __attribute__((packed));

struct e1000_tx_desc {
    uint64_t addr;
    uint16_t length;
    uint8_t cso;
    uint8_t cmd;
    uint8_t status;
    uint8_t css;
    uint16_t special;
} __attribute__((packed));

#define E1000_NUM_RX_DESC 128
#define E1000_NUM_TX_DESC 128

void init_e1000(unsigned int mmio_base, unsigned char irq);
void e1000_send_packet(void *data, int len);

#endif