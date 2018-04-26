#define __STDC_FORMAT_MACROS
#include <algorithm>
#include <cstring>
#include <cinttypes>
#include <iostream>
#include <cassert>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <queue>
#include <vector>
#include <random>

#include <iostream>

struct RAM
{
    RAM()
    {
    }

    char
    operator()(uint64_t address) const
    {
        if (!data.count(address)) {
            std::random_device rd;
            std::mt19937_64 gen(rd());
            std::uniform_int_distribution<char> dis;

            data[address] = dis(gen);
        }
        return data[address];
    }

    char &
    operator()(uint64_t address)
    {
        if (!data.count(address)) {
            std::random_device rd;
            std::mt19937_64 gen(rd());
            std::uniform_int_distribution<char> dis;

            data[address] = dis(gen);
        }
        return data[address];
    }

    void
    print(uint64_t fromAddr, uint64_t toAddr) const
    {
        for (uint64_t from=fromAddr;
             from<toAddr;
             (from+40>from) ? from+=40 : from=toAddr)
        {
            uint64_t to = (from+40>from) ? std::min(from+40, toAddr)
                                         : toAddr;

            for (uint64_t i=from; i<=to && (i-1<i || from==0); ++i) {
                std::printf("---");
            }
            std::printf("\n");
            for (uint64_t i=from; i<=to && (i-1<i || from==0); ++i) {
                std::printf("%02x ", 0xFF & operator()(i));
            }
            std::printf("\n");
            for (uint64_t i=from; i<=to && (i-1<i || from==0); ++i) {
                std::printf("---");
            }
            std::printf("\n");
            for (uint64_t i=from; i<=to && (i-1<i || from==0); ++i) {
                if (i % 8 == 0) {
                    std::printf("|%-23" PRIu64, i);
                }
            }
            std::printf("\n");
            for (uint64_t i=from; i<=to && (i-1<i || from==0); ++i) {
                if (i % 8 == 0) {
                    std::printf("|0x%-21" PRIx64, i);
                }
            }
            std::printf("\n\n");
        }
    }

    mutable std::unordered_map<uint64_t,char>  data;
};

struct CpuRegister
{
    CpuRegister()
        : zero(0)
    {
        std::random_device rd;
        std::mt19937_64 gen(rd());
        std::uniform_int_distribution<uint64_t> dis;

        for (int i=0; i<256; ++i) {

            r[i] = dis(gen);
        }
    }


    const uint64_t &
    operator()(unsigned char id) const
    {
        zero = 0;
        if (id==0) {
            return zero;
        }
        return r[id];
    }

    uint64_t &
    operator()(unsigned char id)
    {
        zero = 0;
        return (id!=0) ? r[id] : zero;
    }

    void
    print(unsigned char     from,
          unsigned char     to) const
    {

        for (unsigned char i=from; i<=to; ++i) {
            std::printf("Reg%-2d   0x%016" PRIx64, i, operator()(i));
            if (i==0) {
                std::printf(" (Zero register)");
            }
            std::printf("\n");
        }
        std::printf("\n\n");
    }

    uint64_t  r[256];
    mutable uint64_t  zero;
};

struct IO
{
    char
    get()
    {
        std::printf("Input one character: ");
        char c = getchar();
        getchar();
        return c;
    }

    void
    put(char c)
    {
        output.append(1, c);
    }

    void
    print()
    {
        std::printf("Terminal:\n");
        if (output.length()!=0) {
            std::printf("%s\n\n", output.c_str());
        }
    }

    std::string         output;
};

struct DataBus
{
    DataBus(RAM &ram, CpuRegister &cpuRegister, IO &io)
        : ram(ram), cpuRegister(cpuRegister), io(io)
    {
    }

    template <unsigned int bytes, bool asSigned=true>
    void
    fetch(uint64_t address, unsigned char reg)
    {
        assert(bytes==1 || bytes==2 || bytes==4 || bytes==8);
        assert(address % bytes == 0);

        cpuRegister(reg) = 0;
        if (asSigned && (0x80 & ram(address))) {
            cpuRegister(reg) = -1;
        }
        for (unsigned int i=0; i<bytes; ++i) {
            cpuRegister(reg) = cpuRegister(reg) << 8;
            cpuRegister(reg) = cpuRegister(reg) | (0xFF & ram(address+i));
        }
    }

    std::uint32_t
    fetchInstr(uint64_t address)
    {
        assert(address % 4 == 0);

        std::uint32_t ir = 0;
        for (unsigned int i=0; i<4; ++i) {
            ir = ir  << 8;
            ir = ir | (0xFF & ram(address+i));
        }
        return ir;
    }

    template <unsigned int bytes>
    void
    store(unsigned char reg, uint64_t address)
    {
        assert(bytes==1 || bytes==2 || bytes==4 || bytes==8);
        assert(address % bytes == 0);

        uint64_t value = cpuRegister(reg);
        for (unsigned int i=0; i<bytes; ++i) {
            char byte      = value & 0xFF;
            ram(address+bytes-i-1) = byte;
            value = value >> 8;
        }
    }

    template <unsigned int bytes>
    void
    store(uint64_t value, uint64_t address)
    {
        assert(bytes==1 || bytes==2 || bytes==4 || bytes==8);
        assert(address % bytes == 0);

        for (unsigned int i=0; i<bytes; ++i) {
            char byte      = value & 0xFF;
            ram(address+bytes-i-1) = byte;
            value = value >> 8;
        }
    }

    void
    get(unsigned char reg)
    {
        unsigned char c = io.get();
        cpuRegister(reg) = c;
    }

    void
    put(unsigned char reg)
    {
        unsigned char c = (0xFF & cpuRegister(reg));
        io.put(c);
    }

    void
    putc(unsigned char c)
    {
        io.put(c);
    }


    RAM         &ram;
    CpuRegister &cpuRegister;
    IO          &io;
};


struct ALU
{
    ALU(CpuRegister &cpuRegister)
        : cpuRegister(cpuRegister), zf(0), of(0), sf(0), cf(0)
    {
    }

    void
    setWord(uint16_t valueXY, int regZ)
    {
        cpuRegister(regZ) = valueXY;
    }

    void
    bitwiseOR(unsigned char regX, unsigned char regY, unsigned char regZ)
    {
        uint64_t valueX = cpuRegister(regX);
        uint64_t valueY = cpuRegister(regY);

        cpuRegister(regZ) = valueX | valueY;
        zf = cpuRegister(regZ)==0 ? 1 : 0;
    }

    void
    bitwiseOR(uint64_t valueX, unsigned char regY, unsigned char regZ)
    {
        uint64_t valueY = cpuRegister(regY);

        cpuRegister(regZ) = valueX | valueY;
        zf = cpuRegister(regZ)==0 ? 1 : 0;
    }

    void
    bitwiseAND(unsigned char regX, unsigned char regY, unsigned char regZ)
    {
        uint64_t valueX = cpuRegister(regX);
        uint64_t valueY = cpuRegister(regY);

        cpuRegister(regZ) = valueX & valueY;
        zf = cpuRegister(regZ)==0 ? 1 : 0;
    }

    void
    bitwiseAND(uint64_t valueX, unsigned char regY, unsigned char regZ)
    {
        uint64_t valueY = cpuRegister(regY);

        cpuRegister(regZ) = valueX & valueY;
        zf = cpuRegister(regZ)==0 ? 1 : 0;
    }

    void
    bitwiseSHL(unsigned char regX, unsigned char regY, unsigned char regZ)
    {
        uint64_t valueX = cpuRegister(regX);
        uint64_t valueY = cpuRegister(regY);

        cpuRegister(regZ) = (valueY << valueX);
    }

    void
    bitwiseSHL(uint64_t valueX, unsigned char regY, unsigned char regZ)
    {
        uint64_t valueY = cpuRegister(regY);

        cpuRegister(regZ) = (valueY << valueX);
    }

    void
    bitwiseSHR(unsigned char regX, unsigned char regY, unsigned char regZ)
    {
        uint64_t valueX = cpuRegister(regX);
        uint64_t valueY = cpuRegister(regY);

        cpuRegister(regZ) = (valueY >> valueX);
    }

    void
    bitwiseSHR(uint64_t valueX, unsigned char regY, unsigned char regZ)
    {
        uint64_t valueY = cpuRegister(regY);

        cpuRegister(regZ) = (valueY >> valueX);
    }

    void
    add(unsigned char regX, unsigned char regY, unsigned char regZ)
    {
        uint64_t valueX = cpuRegister(regX);
        uint64_t valueY = cpuRegister(regY);
        uint64_t valueZ = cpuRegister(regZ);

        zf = of = sf = cf = 0;

        valueZ = valueX + valueY;
        if ((valueZ < valueX) || (valueZ < valueY)) {
            cf = 1;
        }
        if (valueZ==0) {
            zf = 1;
        }

        bool signX = (valueX>>63);
        bool signY = (valueY>>63);
        bool signZ = (valueZ>>63);

        if ((signX==signY) && (signX!=signZ)) {
            of = 1;
        }

        sf = signZ;
        cpuRegister(regZ) = valueZ;
    }

    void
    add(uint64_t valueX, unsigned char regY, unsigned char regZ)
    {
        uint64_t valueY = cpuRegister(regY);
        uint64_t valueZ = cpuRegister(regZ);

        zf = of = sf = cf = 0;

        valueZ = valueX + valueY;
        if ((valueZ < valueX) || (valueZ < valueY)) {
            cf = 1;
        }
        if (valueZ==0) {
            zf = 1;
        }

        bool signX = (valueX>>63);
        bool signY = (valueY>>63);
        bool signZ = (valueZ>>63);

        if ((signX==signY) && (signX!=signZ)) {
            of = 1;
        }

        sf = signZ;
        cpuRegister(regZ) = valueZ;
    }

    void
    sub(unsigned char regX, unsigned char regY, unsigned char regZ)
    {
        add(-cpuRegister(regX), regY, regZ);

        uint64_t valueX = cpuRegister(regX);
        uint64_t valueY = cpuRegister(regY);
        uint64_t valueZ = cpuRegister(regZ);

        valueZ = valueY - valueX;

        if (valueZ>valueY) {
            cf = 1;
        } else {
            cf = 0;
        }
    }

    void
    sub(uint64_t valueX, unsigned char regY, unsigned char regZ)
    {
        add(-valueX, regY, regZ);

        uint64_t valueY = cpuRegister(regY);
        uint64_t valueZ = cpuRegister(regZ);

        valueZ = valueY - valueX;

        if (valueZ>valueY) {
            cf = 1;
        } else {
            cf = 0;
        }
    }

    void
    unsignedMul(unsigned char regX, unsigned char regY, int regZ)
    {
        cpuRegister(regZ) = cpuRegister(regX) * cpuRegister(regY);
    }

    void
    unsignedMul(uint64_t valueX, unsigned char regY, int regZ)
    {
        cpuRegister(regZ) = valueX * cpuRegister(regY);
    }

    void
    print()
    {
        std::printf("ZF = %1d, OF = %1d, SF = %1d, CF = %1d\n", zf, of, sf, cf);
    }

    CpuRegister &cpuRegister;

    int         zf, of, sf, cf;
};

struct CPU
{
    CPU(RAM &ram, IO &io)
        : alu(cpuRegister), dataBus(ram, cpuRegister, io), exit(0)
    {
        ip     = 0;         // Instruction Pointer (IP)
        ir     = 0;         // Instruction Register (IR)
        IR_asm = "halt";
    }

    void
    print()
    {
        std::printf("IP      0x%016" PRIx64 " (= %" PRIu64 ")\n", ip, ip);
        std::printf("IR:     0x%08" PRIx32 "         (ASM: %-s)\n\n",
                    ir, IR_asm.c_str());
        cpuRegister.print(0,15);
        alu.print();
    }

    bool
    cycle()
    {
        char asmBuffer[101];
        bool run = true;


        // (A) FETCH
        ir = dataBus.fetchInstr(ip);

        // (B) DECODE
        uint32_t instruction = ir;

        unsigned char     Z           = instruction       & 0xFF;
        unsigned char     Y           = (instruction>> 8) & 0xFF;
        unsigned char     X           = (instruction>>16) & 0xFF;
        unsigned char     op          = (instruction>>24) & 0xFF;

        signed char Xs = X;
        signed char Ys = Y;
        signed char Zs = Z;

        // (C) EXECUTE
        uint64_t IP_old = ip;

        uint16_t XY = X;
        XY = (XY<<8) | Y;

        uint32_t XYZ = XY;
        XYZ = (XYZ<<8) | Z;

        int32_t XYZs = XY;
        XYZs = (XYZs<<8) | Z;

        XYZs = Zs;

        switch (op) {
            // halt
            case 0x00:
                std::snprintf(asmBuffer, 100, "halt %%%d", X);
                run = false;
                exit = cpuRegister(X);
                break;

            // get
            case 0x01:
                std::snprintf(asmBuffer, 100,
                              "get %%%d", X);
                dataBus.get(X);
                break;

            // put
            case 0x02:
                std::snprintf(asmBuffer, 100,
                              "put %%%d", X);
                dataBus.put(X);
                break;

            // put
            case 0x03:
                std::snprintf(asmBuffer, 100,
                              "put $%d, $%d, $%d", X, Y, Z);
                dataBus.putc(X);
                dataBus.putc(Y);
                dataBus.putc(Z);
                break;

            // NOP / illegal instruction
            case 0xFF:
                std::snprintf(asmBuffer, 100, "nop");
                break;

            // load $XY, %Z
            case 0xF6:
                std::snprintf(asmBuffer, 100,
                              "load $%d, %%%d", XY, Z);
                alu.setWord(XY, Z);
                break;


            //
            // Data Bus:  Fetch
            //

            // movq (%X,%Y), %Z
            case 0x10:
                std::snprintf(asmBuffer, 100,
                              "movq (%%%d, %%%d), %%%d", X, Y, Z);
                dataBus.fetch<8>(cpuRegister(X)+cpuRegister(Y), Z);
                break;

            // movq Xs(%Y), %Z
            case 0x11:
                std::snprintf(asmBuffer, 100,
                              "movq %d(%%%d), %%%d", Xs, Y, Z);
                dataBus.fetch<8>(Xs+cpuRegister(Y), Z);
                break;

            // movzlq (%X,%Y), %Z
            case 0x12:
                std::snprintf(asmBuffer, 100,
                              "movzlq (%%%d, %%%d), %%%d", X, Y, Z);
                dataBus.fetch<4, false>(cpuRegister(X)+cpuRegister(Y), Z);
                break;

            // movslq (%X,%Y), %Z
            case 0x13:
                std::snprintf(asmBuffer, 100,
                              "movslq (%%%d, %%%d), %%%d", X, Y, Z);
                dataBus.fetch<4, true>(cpuRegister(X)+cpuRegister(Y), Z);
                break;

            // movzlq Y(%X), %Z
            case 0x14:
                std::snprintf(asmBuffer, 100,
                              "movzlq %d(%%%d), %%%d", Ys, X, Z);
                dataBus.fetch<4, false>(cpuRegister(X)+Ys, Z);
                break;

            // movslq Y(%X), %Z
            case 0x15:
                std::snprintf(asmBuffer, 100,
                              "movslq %d(%%%d), %%%d", Ys, X, Z);
                dataBus.fetch<4, true>(cpuRegister(X)+Ys, Z);
                break;

            // movzwq (%X,%Y), %Z
            case 0x16:
                std::snprintf(asmBuffer, 100,
                              "movzwq (%%%d, %%%d), %%%d", X, Y, Z);
                dataBus.fetch<2, false>(cpuRegister(X)+cpuRegister(Y), Z);
                break;

            // movswq (%X,%Y), %Z
            case 0x17:
                std::snprintf(asmBuffer, 100,
                              "movswq (%%%d, %%%d), %%%d", X, Y, Z);
                dataBus.fetch<2, true>(cpuRegister(X)+cpuRegister(Y), Z);
                break;

            // movzwq Y(%X), %Z
            case 0x18:
                std::snprintf(asmBuffer, 100,
                              "movzwq %d(%%%d), %%%d", Ys, X, Z);
                dataBus.fetch<2, false>(cpuRegister(X)+Ys, Z);
                break;

            // movswq Y(%X), %Z
            case 0x19:
                std::snprintf(asmBuffer, 100,
                              "movswq %d(%%%d), %%%d", Ys, X, Z);
                dataBus.fetch<2, true>(cpuRegister(X)+Ys, Z);
                break;

            // movzbq (%X,%Y), %Z
            case 0x1A:
                std::snprintf(asmBuffer, 100,
                              "movzbq (%%%d, %%%d), %%%d", X, Y, Z);
                dataBus.fetch<1, false>(cpuRegister(X)+cpuRegister(Y), Z);
                break;

            // movsbq (%X,%Y), %Z
            case 0x1B:
                std::snprintf(asmBuffer, 100,
                              "movsbq (%%%d, %%%d), %%%d", X, Y, Z);
                dataBus.fetch<1, true>(cpuRegister(X)+cpuRegister(Y), Z);
                break;

            // movzbq Y(%X), %Z
            case 0x1C:
                std::snprintf(asmBuffer, 100,
                              "movzbq %d(%%%d), %%%d", Ys, X, Z);
                dataBus.fetch<1, false>(cpuRegister(X)+Ys, Z);
                break;

            // movsbq Y(%X), %Z
            case 0x1D:
                std::snprintf(asmBuffer, 100,
                              "movsbq %d(%%%d), %%%d", Ys, X, Z);
                dataBus.fetch<1, true>(cpuRegister(X)+Ys, Z);
                break;


            //
            // Data Bus:  Store
            //

            // movq %X, (%Y,%Z)
            case 0x40:
                std::snprintf(asmBuffer, 100,
                              "movq %%%d, (%%%d, %%%d)", X, Y, Z);
                dataBus.store<8>(X, cpuRegister(Y)+cpuRegister(Z));
                break;

            // movq %X, Z(%Y)
            case 0x41:
                std::snprintf(asmBuffer, 100,
                              "movq %%%d, %d(%%%d)", X, Zs, Y);
                dataBus.store<8>(X, cpuRegister(Y)+Zs);
                break;

            // movl %X, (%Y,%Z)
            case 0x42:
                std::snprintf(asmBuffer, 100,
                              "movl %%%d, (%%%d, %%%d)", X, Y, Z);
                dataBus.store<4>(X, cpuRegister(Y)+cpuRegister(Z));
                break;

            // movl %X, Z(%Y)
            case 0x43:
                std::snprintf(asmBuffer, 100,
                              "movl %%%d, %d(%%%d)", X, Zs, Y);
                dataBus.store<4>(X, cpuRegister(Y)+Zs);
                break;

            // movw %X, (%Y,%Z)
            case 0x44:
                std::snprintf(asmBuffer, 100,
                              "movw %%%d, (%%%d, %%%d)", X, Y, Z);
                dataBus.store<2>(X, cpuRegister(Y)+cpuRegister(Z));
                break;

            // movw %X, Z(%Y)
            case 0x45:
                std::snprintf(asmBuffer, 100,
                              "movw %%%d, %d(%%%d)", X, Zs, Y);
                dataBus.store<2>(X, cpuRegister(Y)+Zs);
                break;


            // movb %X, (%Y,%Z)
            case 0x46:
                std::snprintf(asmBuffer, 100,
                              "movb %%%d, (%%%d, %%%d)", X, Y, Z);
                dataBus.store<1>(X, cpuRegister(Y)+cpuRegister(Z));
                break;

            // movb %X, Z(%Y)
            case 0x47:
                std::snprintf(asmBuffer, 100,
                              "movb %%%d, %d(%%%d)", X, Zs, Y);
                dataBus.store<1>(X, cpuRegister(Y)+Zs);
                break;


            //
            //  Integer-Arithmetic
            //

            // addq %X, %Y, %Z
            case 0x60:
                std::snprintf(asmBuffer, 100,
                              "addq  %%%d, %%%d, %%%d", X, Y, Z);
                alu.add(X, Y, Z);
                break;

            // addq $X, %Y, %Z
            case 0x61:
                std::snprintf(asmBuffer, 100,
                              "addq  $%d, %%%d, %%%d", Xs, Y, Z);
                alu.add(uint64_t(Xs), Y, Z);
                break;

            // subq %X, %Y, %Z
            case 0x62:
                std::snprintf(asmBuffer, 100,
                              "subq  %%%d, %%%d, %%%d", X, Y, Z);
                alu.sub(X, Y, Z);
                break;

            // subq $X, %Y, %Z
            case 0x63:
                std::snprintf(asmBuffer, 100,
                              "subq  $%d, %%%d, %%%d", Xs, Y, Z);
                alu.sub(uint64_t(Xs), Y, Z);
                break;

            // mulq %X, %Y, %Z
            case 0x70:
                std::snprintf(asmBuffer, 100,
                              "mulq  %%%d, %%%d, %%%d", X, Y, Z);
                alu.unsignedMul(X, Y, Z);
                break;


            // mulq $X, %Y, %Z
            case 0x71:
                std::snprintf(asmBuffer, 100,
                              "mulq  $%d, %%%d, %%%d", X, Y, Z);
                alu.unsignedMul(uint64_t(X), Y, Z);
                break;

            //
            // Bit-Operations
            //

            // orq %X, %Y, %Z
            case 0x80:
                std::snprintf(asmBuffer, 100,
                              "orq   %%%d, %%%d, %%%d", X, Y, Z);
                alu.bitwiseOR(X, Y, Z);
                break;

            // orq $X, %Y, %Z
            case 0x81:
                std::snprintf(asmBuffer, 100,
                              "orq   $%d, %%%d, %%%d", X, Y, Z);
                alu.bitwiseOR(uint64_t(X), Y, Z);
                break;

            // andq %X, %Y, %Z
            case 0x82:
                std::snprintf(asmBuffer, 100,
                              "andq  %%%d, %%%d, %%%d", X, Y, Z);
                alu.bitwiseAND(X, Y, Z);
                break;

            // andq $X, %Y, %Z
            case 0x83:
                std::snprintf(asmBuffer, 100,
                              "andq  $%d, %%%d, %%%d", X, Y, Z);
                alu.bitwiseAND(uint64_t(X), Y, Z);
                break;

            // shlq %X, %Y, %Z
            case 0x84:
                std::snprintf(asmBuffer, 100,
                              "shlq  %%%d, %%%d, %%%d", X, Y, Z);
                alu.bitwiseSHL(X, Y, Z);
                break;

            // shlq $X, %Y, %Z
            case 0x85:
                std::snprintf(asmBuffer, 100,
                              "shlq  $%d, %%%d, %%%d", X, Y, Z);
                alu.bitwiseSHL(uint64_t(X), Y, Z);
                break;

            // shrq %X, %Y, %Z
            case 0x86:
                std::snprintf(asmBuffer, 100,
                              "shrq  %%%d, %%%d, %%%d", X, Y, Z);
                alu.bitwiseSHR(X, Y, Z);
                break;

            // shrq $X, %Y, %Z
            case 0x87:
                std::snprintf(asmBuffer, 100,
                              "shrq  $%d, %%%d, %%%d", X, Y, Z);
                alu.bitwiseSHR(uint64_t(X), Y, Z);
                break;

            // jmp $XYZ
            case 0x9A:
                std::snprintf(asmBuffer, 100,
                              "jmp   @+$%d", XYZs);
                ip = int64_t(ip)+ 4*int64_t(XYZs);
                break;

            // jmp %X, %Y
            case 0x9B:
                std::snprintf(asmBuffer, 100,
                              "jmp   %%%d, %%%d", X, Y);
                cpuRegister(Y) = ip + 4;
                ip             = cpuRegister(X);
                break;

            // jmp %X
            case 0x9C:
                std::snprintf(asmBuffer, 100,
                              "jmp   %%%d", X);
                --cpuRegister(Y);
                ip = cpuRegister(X+cpuRegister(Y));
                break;

            // jz $XYZ
            case 0x90:
                std::snprintf(asmBuffer, 100,
                              "jz    @+$%d (false)", XYZs);
                if (alu.zf) {
                    std::snprintf(asmBuffer, 100,
                                  "jz    @+$%d (true)", XYZs);
                    ip = int64_t(ip)+ 4*int64_t(XYZs);
                }
                break;

            // jnz $XYZ
            case 0x91:
                std::snprintf(asmBuffer, 100,
                              "jnz   @+$%d (false)", XYZs);
                if (!alu.zf) {
                    std::snprintf(asmBuffer, 100,
                                  "jnz   @+$%d (true)", XYZs);
                    ip = int64_t(ip)+ 4*int64_t(XYZs);
                }
                break;

            // jl $XYZ
            case 0x92:
                std::snprintf(asmBuffer, 100,
                              "jl   @+$%d (false)", XYZs);
                if (alu.sf != alu.of) {
                    std::snprintf(asmBuffer, 100,
                                  "jl   @+$%d (true)", XYZs);
                    ip = int64_t(ip)+ 4*int64_t(XYZs);
                }
                break;

            // jge $XYZ
            case 0x93:
                std::snprintf(asmBuffer, 100,
                              "jge   @+$%d (false)", XYZs);
                if (alu.sf == alu.of) {
                    std::snprintf(asmBuffer, 100,
                                  "jge   @+$%d (true)", XYZs);
                    ip = int64_t(ip)+ 4*int64_t(XYZs);
                }
                break;

 
            // jle $XYZ
            case 0x94:
                std::snprintf(asmBuffer, 100,
                              "jle   @+$%d (false)", XYZs);
                if (alu.zf || (alu.sf != alu.of)) {
                    std::snprintf(asmBuffer, 100,
                                  "jle   @+$%d (true)", XYZs);
                    ip = int64_t(ip)+ 4*int64_t(XYZs);
                }
                break;

            // jg $XYZ
            case 0x95:
                std::snprintf(asmBuffer, 100,
                              "jg   @+$%d (false)", XYZs);
                if ((alu.zf==0) && (alu.sf == alu.of)) {
                    std::snprintf(asmBuffer, 100,
                                  "jg   @+$%d (true)", XYZs);
                    ip = int64_t(ip)+ 4*int64_t(XYZs);
                }
                break;

            // jb $XYZ
            case 0x96:
                std::snprintf(asmBuffer, 100,
                              "jle   @+$%d (false)", XYZs);
                if (alu.cf) {
                    std::snprintf(asmBuffer, 100,
                                  "jle   @+$%d (true)", XYZs);
                    ip = int64_t(ip)+ 4*int64_t(XYZs);
                }
                break;

            // jnb $XYZ
            // jae $XYZ
            case 0x97:
                std::snprintf(asmBuffer, 100,
                              "jnb   @+$%d (false)", XYZs);
                if (!alu.cf) {
                    std::snprintf(asmBuffer, 100,
                                  "jnb   @+$%d (true)", XYZs);
                    ip = int64_t(ip)+ 4*int64_t(XYZs);
                }
                break;

            // jbe $XYZ
            case 0x98:
                std::snprintf(asmBuffer, 100,
                              "jbe   @+$%d (false)", XYZs);
                std::printf("%d %d\n", alu.cf, alu.zf);
                if (alu.cf || alu.zf) {
                    std::snprintf(asmBuffer, 100,
                                  "jbe   @+$%d (true)", XYZs);
                    ip = int64_t(ip)+ 4*int64_t(XYZs);
                }
                break;

            // ja $XYZ
            case 0x99:
                std::snprintf(asmBuffer, 100,
                              "ja   @+$%d (false)", XYZs);
                if (!alu.cf && !alu.zf) {
                    std::snprintf(asmBuffer, 100,
                                  "ja   @+$%d (true)", XYZs);
                    ip = int64_t(ip)+ 4*int64_t(XYZs);
                }
                break;

            default:
                std::fprintf(stderr, "Illegal instruction or instruction not "
                                     "implemented\n");
                assert(0);
        }

        if (IP_old==ip) {
            ip += 4;
        }

        // (D) STORE


        IR_asm = std::string(asmBuffer);
        return run;
    }

    CpuRegister cpuRegister;
    uint32_t    ir;
    uint64_t    ip;
    ALU         alu;
    DataBus     dataBus;
    int         exit;
    std::string IR_asm;
};

struct Computer
{
    Computer(const char *filename, bool interactive=true)
        : cpu(ram, io), cycle(0), exit(0), interactive(interactive)
    {
        if (!load(filename)) {
            std::fprintf(stderr, "Code in %s corrupted\n", filename);
            assert(0);
        }
    }

    void
    run()
    {
        do {
            print();
            ++cycle;

            if (interactive) {
                std::getchar();
            }
        } while (cpu.cycle());
        print();
    }

    bool
    load(const char *filename)
    {
        std::ifstream   infile(filename);
        std::string     line;
        std::uint64_t   code;
        std::uint64_t   addr = 0;

        while (std::getline(infile, line))
        {
            line.erase(find(line.begin(), line.end(), '#'),
                       line.end());
            line.erase(remove_if(line.begin(), line.end(), isspace),
                       line.end());
            std::istringstream in(line);

            size_t len = in.str().length();

            if (len==0) {
                continue;
            }

            if (len % 2) {
                return false;
            }

            if (! (in >> std::hex >> code)) {
                return false;
            }

            addr += len/2;
            for (size_t i=0; i<len/2; ++i) {
                ram(addr-i-1) = 0xFF & (code >> i*8);
            }

        }
        return true;
    }

    void
    print()
    {
        std::printf("CPU cycles done %4d\n\n", cycle);
        ram.print(0,160);
        std::printf("....\n\n");
        ram.print(uint64_t(-1)-79,uint64_t(-1));
        cpu.print();
        io.print();
        std::printf("\n\n");
    }

    RAM         ram;
    IO          io;
    CPU         cpu;
    int         cycle;
    int         exit;
    bool        interactive;
};

int
main(int argc, const char **argv)
{
    const char *code;
    bool       interactive = true;

    if (argc<2) {
        std::fprintf(stderr, "usage: [-r] %s code-file\n", argv[0]);
        return 1;
    }
    for (int i=1; i<argc; ++i) {
        if (!strcmp(argv[i], "-r")) {
            interactive = false;
        } else {
            code = argv[i];
        }
    }

    Computer    computer(code, interactive);

    computer.run();
    std::printf("ULM was halted with exit code %d (=0x%02X)\n",
                computer.cpu.exit, computer.cpu.exit);
    return computer.cpu.exit;
}
