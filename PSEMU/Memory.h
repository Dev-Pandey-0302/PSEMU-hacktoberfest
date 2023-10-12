/*
 *************************************
 *           PSEMU Licence           *
 *************************************

 PSEMU © 2023 by Ronit D'silva is licensed under Attribution-NonCommercial-ShareAlike 4.0 International

*/
#pragma once
#include <cstdint>
#include <vector>
#include "Logging.h"
#include "CPURegisters.h"
#include "GPU.h"

enum class SyncType : uint32_t {
    Manual = 0,
    Request = 1,
    Linked_List = 2
};

enum class DMAChannels : uint32_t {
    MDECin = 0x0,
    MDECout = 0x1,
    GPU = 0x2,
    CDROM = 0x3,
    SPU = 0x4,
    PIO = 0x5,
    OTC = 0x6
};

union DMAIRQReg {
    uint32_t raw;

    struct {
        uint32_t unknown : 6;
        uint32_t not_used : 9;
        uint32_t force : 1;
        uint32_t enable : 7;
        uint32_t master_enable : 1;
        uint32_t flags : 7;
        uint32_t master_flag : 1;
    };
};

union DMAControlReg {
    uint32_t raw;

    struct {
        uint32_t trans_dir : 1;
        uint32_t addr_step : 1;
        uint32_t reserved : 6;
        uint32_t chop_enable : 1;
        SyncType sync_mode : 2;
        uint32_t reserved2 : 5;
        uint32_t chop_dma : 3;
        uint32_t reserved3 : 1;
        uint32_t chop_cpu : 3;
        uint32_t reserved4 : 1;
        uint32_t enable : 1;
        uint32_t reserved5 : 3;
        uint32_t trigger : 1;
        uint32_t unknown : 3;
    };
};

/* Holds info about block size and count. */
union DMABlockReg {
    uint32_t raw;

    struct {
        uint16_t block_size;
        uint16_t block_count; /* Only used in Request sync mode. */
    };
};


typedef uint32_t DMAControl;
typedef uint32_t DMAMemReg;

struct DMAChannel {
    DMAControlReg control;
    DMABlockReg block;
    DMAMemReg base;
};

union ListPacket {
    uint raw;

    struct {
        uint next_addr : 24;
        uint size : 8;
    };
};

class Memory {
public:
    // size = kilobytes
    Memory(size_t size, CPURegisters* rega) : MainRAM((size * 8000) / sizeof(uint8_t)), regs(rega) {};

    // address = bits
    uint8_t& operator[](uint32_t address) {
        if (address < MainRAMEnd) {
            return MainRAM[address - MainRAMStart];
        } else if (address < DMAEnd) {
            //     --FIX--
            return MainRAM[address - DMAStart];
        } else {
            Logging console;
            console.err(54);
            static uint8_t eval = 0;
            return eval;
        }
    }

    uint8_t readByte(uint32_t address) const;

    void writeByte(uint32_t address, uint8_t value);

    uint32_t readWord(uint32_t address) const;

    void writeWord(uint32_t address, uint32_t value);

    void writeHalfword(uint32_t address, uint16_t value);

    uint32_t read32(uint32_t address) const;

    // TODO: Implement memory-mapped I/O for CD-ROM and HDD

    // DMA
    bool is_channel_enabled(DMAChannels channel);
    void transfer_finished(DMAChannels channel);

    void start(DMAChannels channel);
    void block_copy(DMAChannels channel);
    void list_copy(DMAChannels channel);

    uint32_t read(uint32_t address);
    void write(uint32_t address, uint32_t data);

    void tick();
private:
    DMAControl control;
    DMAIRQReg irq;
    DMAChannel channels[7];

    GPU gpu;

    bool irq_pending = false;
    std::vector<uint8_t> MainRAM;
    int MainRAMStart = 0; // bits
    int MainRAMEnd = MainRAM.size(); // bits
    uint32_t DMAStart = 0x1f801080;
    uint32_t DMAEnd = 0x1f801080 + (uint32_t)0x80LL;
    // Main Ram starts at 0 bits and ends at 16384000 bits (divide it by uint8_t to get array size)
    CPURegisters* regs;
};
