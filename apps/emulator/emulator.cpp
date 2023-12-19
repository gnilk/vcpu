#include <iostream>
#include <stdint.h>
#include <chrono>
#include <vector>
#include <string>
#include <filesystem>
#include <functional>

#include "fmt/format.h"
#include "StrUtil.h"
#include "VirtualCPU.h"

using namespace gnilk;
using namespace gnilk::vcpu;

static uint64_t loadToAddress = 0;
static uint64_t startAddress = 0;

std::optional<uint64_t> ParseNumber(const std::string_view &line);

bool ProcessFile(std::filesystem::path &pathToBinary);

int main(int argc, char **argv)  {
    std::vector<std::string> filesToRun;
    for(int i=1;i<argc;i++) {
        if (argv[i][0] == '-') {
            switch(argv[i][1]) {
                case 'd' : {
                        auto tmp = ParseNumber(argv[++i]);
                        if (!tmp.has_value()) {
                            fmt::println(stderr, "Invalid number {} as argument, use: -d <number>", argv[i]);
                            return 1;
                        }
                        loadToAddress = *tmp;
                        startAddress = *tmp;
                    }
                    break;
            }
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
    VirtualCPU vcpu;

    // Test syscall dispatch...
    vcpu.RegisterSysCall(0x01, "writeline",[](Registers &regs, CPUBase *cpu) {
       fmt::println("wefwef - from syscall");
    });


    fmt::println("Copy firmware to address: {:#x}", loadToAddress);
    memcpy(&cpu_ram_memory[loadToAddress], rawData, szData);
    vcpu.Begin(cpu_ram_memory, 1024 * 512);


    fmt::println("Set Initial IP to: {:#x}", startAddress);
    vcpu.SetInstrPtr(startAddress);

    // Save ip before we step...
    RegisterValue ip = vcpu.GetInstrPtr();

    fmt::println("------->> Begin Execution <<--------------");
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


///////// Helpers
std::optional<uint64_t> ParseNumber(const std::string_view &line) {

    std::string num;
    auto it = line.begin();

    std::function<bool(const int chr)> isnumber = [](const int chr) -> bool {
        return std::isdigit(chr);
    };

    //
    // We could enhance this with more features normally found in assemblers
    // $<hex> - for address
    // #$<hex> - alt. syntax for hex numbers
    // #<dec>  - alt. syntax for dec numbers
    //
    // '#' is a common denominator for numerical values
    if (*it == '#') {
        ++it;
    }

    enum class TNum {
        Number,
        NumberHex,
        NumberBinary,
        NumberOctal,
    };

    auto numberType = TNum::Number;
    if (*it == '0') {
        num += *it;
        it++;
        // Convert number here or during parsing???
        switch(tolower(*it)) {
            case 'x' : // hex
                num += *it;
                ++it;
                numberType = TNum::NumberHex;
                isnumber = [](const int chr) -> bool {
                    static std::string hexnum = {"abcdef"};
                    return (std::isdigit(chr) || (hexnum.find(tolower(chr)) != std::string::npos));
                };
                break;
            case 'b' : // binary
                num += *it;
                ++it;
                numberType = TNum::NumberBinary;
                isnumber = [](const int chr) -> bool {
                    return (chr=='1' || chr=='0');
                };

                break;
            case 'o' : // octal
                num += *it;
                ++it;
                numberType = TNum::NumberOctal;
                isnumber = [](const int chr) -> bool {
                    static std::string hexnum = {"01234567"};
                    return (hexnum.find(tolower(chr)) != std::string::npos);
                };
                break;
            default :
                if (std::isdigit(*it)) {
                    fprintf(stderr,"WARNING: Numerical tokens shouldn't start with zero!");
                }
                break;
        }
    }

    while(it != line.end() && isnumber(*it)) {
        num += *it;
        ++it;
    }
    if (numberType == TNum::Number) {
        return {strutil::to_int32(num)};
    } else if (numberType == TNum::NumberHex) {
        return {uint64_t(strutil::hex2dec(num))};
    }

    return {};
}
