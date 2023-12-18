#include <iostream>
#include <stdint.h>
#include <chrono>
#include <vector>
#include <string>
#include <filesystem>

#include "fmt/format.h"

#include "VirtualCPU.h"

using namespace gnilk;
using namespace gnilk::vcpu;

bool ProcessFile(std::filesystem::path &pathToBinary);

int main(int argc, char **argv)  {
    std::vector<std::string> filesToRun;
    for(int i=1;i<argc;i++) {
        if (argv[i][0] == '-') {

        } else {
            filesToRun.push_back(argv[i]);
        }
    }

    if (filesToRun.empty()) {
        fmt::println("no files - bailing");
        return 1;
    }
    for(auto &fToRun : filesToRun) {
        std::filesystem::path pathToFile(fToRun);
        if (!exists(pathToFile)) {
            fmt::println(stderr, "No such file or directory, {}", fToRun);
            continue;;
        }
        if (is_directory(pathToFile)) {
            fmt::println(stderr, "Directories not supported");
            continue;;
        }
        if (!is_regular_file(pathToFile)) {
            fmt::println(stderr, "Not a regular file {} - skipping", fToRun);
            continue;;
        }
        if (!ProcessFile(pathToFile)) {
            fmt::println("{} - failed", fToRun);
        } else {
            fmt::println("{} - OK", fToRun);
        }
    }
}

bool ExecuteData(const uint8_t *rawData, size_t szData);

bool ProcessFile(std::filesystem::path &pathToBinary) {
    size_t szFile = file_size(pathToBinary);
    uint8_t *data = (uint8_t *)malloc(szFile + 10);
    if (data == nullptr) {
        return false;
    }
    memset(data, 0, szFile + 10);
    // Read file
    FILE *f = fopen(pathToBinary.c_str(), "r+");
    if (f == nullptr) {
        free(data);
        return false;
    }
    auto nRead = fread(data, 1, szFile, f);
    fclose(f);

    auto res = ExecuteData(data, szFile);
    free(data);
    return res;
}


static void DumpStatus(const VirtualCPU &cpu) {
    auto &status = cpu.GetStatusReg();
    fmt::println("CPU Stat: [{}{}{}{}{}---]",
        status.flags.carry?"C":"-",
        status.flags.overflow?"O":"-",
        status.flags.zero?"Z":"-",
        status.flags.negative?"N":"-",
        status.flags.extend?"E":"-");
}
static void DumpRegs(const VirtualCPU &cpu) {
    auto &regs = cpu.GetRegisters();
    for(int i=0;i<8;i++) {
        fmt::print("d{}=0x{:02x}  ",i,regs.dataRegisters[i].data.byte);
        if ((i & 7) == 7) {
            fmt::println("");
        }
    }
    for(int i=0;i<8;i++) {
        fmt::print("a{}=0x{:02x}  ",i,regs.dataRegisters[i+8].data.byte);
        if ((i & 7) == 7) {
            fmt::println("");
        }
    }
}

static uint8_t cpu_ram_memory[1024*512];    // 512kb of RAM for my CPU...
bool ExecuteData(const uint8_t *rawData, size_t szData) {
    memcpy(cpu_ram_memory, rawData, szData);
    VirtualCPU vcpu;
    vcpu.Begin(cpu_ram_memory, 1024);
    fmt::println("------->> Begin Execution <<--------------");
    // Save ip before we step...
    RegisterValue ip = vcpu.GetInstrPtr();
    while(vcpu.Step()) {
        // generate op-codes for this instruction...
        std::string strOpCodes = "";
        for(auto ofs = ip.data.longword; ofs < vcpu.GetInstrPtr().data.longword;ofs++) {
            strOpCodes += fmt::format("0x{:02x}, ",cpu_ram_memory[ofs]);
        }
        // Retrieve the last decoded instruction and transform to string
        auto lastInstr = vcpu.GetLastDecodedInstr();
        auto str = lastInstr->ToString();

        // now output address (16 bit), instruction and opcodes
        fmt::println("0x{:04x}\t\t{}\t\t; {}", ip.data.word,str, strOpCodes);
        // Dump stats and register changes caused by this operation
        DumpStatus(vcpu);
        DumpRegs(vcpu);

        // save current ip (this is for op-code)
        ip = vcpu.GetInstrPtr();
        fmt::println("");
    }
    fmt::println("------->> Execution Complete <<--------------");
    return true;
}

