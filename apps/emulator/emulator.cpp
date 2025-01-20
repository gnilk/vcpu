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
#include "elfio/elfio.hpp"
#include "InstructionSet.h"
#include "InstructionSetV1/InstructionSetV1.h"

using namespace gnilk;
using namespace gnilk::vcpu;

using namespace ELFIO;

static gnilk::vcpu::VirtualCPU cpuemu;

static uint64_t rawLoadToAddress = 0;
static uint64_t rawStartAddress = 0;

// 1024 pages is more than enough for simple testing...
static uint8_t cpu_ram_memory[1024*VCPU_MMU_PAGE_SIZE] = {};    // 512kb of RAM for my CPU...


std::optional<uint64_t> ParseNumber(const std::string_view &line);

bool ProcessFile(std::filesystem::path &pathToBinary);
bool ProcessElf(std::filesystem::path &pathToBinary);
bool ProcessRaw(std::filesystem::path &pathToBinary);

static void Usage() {
    fmt::println("Lousy emulator for VCPU project");
    fmt::println("Use:");
    fmt::println("  emu [options] <binary>");
    fmt::println("Options:");
    fmt::println("  -d <num>     Load and Start to this address (default=0)");
    fmt::println("  -s <num>     Start at this address (default=0)");
    fmt::println("Example (load binary to address 0 but start from address 0x2000):");
    fmt::println("  emu -s 0x2000 mybinary.bin");
}

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
                        rawLoadToAddress = *tmp;
                        rawStartAddress = *tmp;
                    }
                    break;
                case 's' : {
                        auto tmp = ParseNumber(argv[++i]);
                        if (!tmp.has_value()) {
                            fmt::println(stderr, "Invalid number {} as argument, use: -d <number>", argv[i]);
                            return 1;
                        }
                        rawStartAddress = *tmp;
                    }
                    break;
                case 'h' :
                case '?' :
                    Usage();
                    exit(1);
                    break;
                default:
                    fmt::println(stderr,"Unknown option {}", argv[i]);
                    Usage();
                    exit(1);
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

    // Set the root instruction set
    InstructionSetManager::Instance().SetInstructionSet<InstructionSetV1>();

    // Now, initialize the CPU
    cpuemu.Begin(cpu_ram_memory, 1024 * VCPU_MMU_PAGE_SIZE);

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

bool ExecuteData(uint64_t startAddress, size_t szCode);


enum class kEmuFileType {
    Unknown,
    RawBinary,
    ElfBinary,
};
static const uint8_t signatureElf[]={0x7f,0x45,0x4c,0x46};

static kEmuFileType FileType(std::filesystem::path &pathToBinary) {
    size_t szFile = file_size(pathToBinary);
    if (szFile < 4) {
        fmt::println("file size too small {}, can't detect file type", szFile);
        return kEmuFileType::Unknown;
    }
    char signature[8] = {};
    FILE *f = fopen(pathToBinary.c_str(), "r+");
    fread(signature, 1, 4, f);
    fclose(f);

    if (!memcmp(signature,signatureElf, sizeof(signatureElf))) {
        return kEmuFileType::ElfBinary;
    }
    // Should probably add a signature here as well...
    return kEmuFileType::RawBinary;
}

bool ProcessFile(std::filesystem::path &pathToBinary) {

    auto sig = FileType(pathToBinary);
    switch(sig) {
        case kEmuFileType::ElfBinary :
            fmt::println("ELF binary detected");
            return ProcessElf(pathToBinary);
        case kEmuFileType::RawBinary :
            return ProcessRaw(pathToBinary);
        default:
            fmt::println("Unknown file type check: {}",pathToBinary.c_str());
            break;
    }
    return false;

}
bool ProcessElf(std::filesystem::path &pathToBinary) {
    elfio reader;

    if (!reader.load(pathToBinary.c_str())) {
        return false;
    }

    size_t szExec = 0;

    auto nSections = reader.sections.size();
    fmt::println("Mapping sections");
    for(size_t idxSection = 0; idxSection < nSections; idxSection++) {
        auto section = reader.sections[idxSection];
        auto address = section->get_address();
        auto data = section->get_data();
        auto szData = section->get_size();
        auto name = section->get_name();
        fmt::println("  Name={}  Address={:#x}  Size={:#x}", name, address, szData);
        if (section->get_type() == PT_LOAD) {
            fmt::println("    -> Mapping to CPU RAM");
            cpuemu.LoadDataToRam(address, data, szData);
        }

        // FIXME: this is not correct..  =)
        // Note - doesn't matter much - the 'szExec' is only used for disassembling the code before executing
        if (name == ".text") {
            szExec = szData;
        }
    }

    auto entryAddress = reader.get_entry();
    ExecuteData(entryAddress, szExec);

    return true;
}
bool ProcessRaw(std::filesystem::path &pathToBinary) {
    size_t szFile = file_size(pathToBinary);
    // Read file
    FILE *f = fopen(pathToBinary.c_str(), "r+");
    if (f == nullptr) {
        return false;
    }

    auto data = malloc(szFile + 1);

    //auto nRead = fread(&cpuemu.GetRamPtr()[rawLoadToAddress], 1, szFile, f);
    auto nRead = fread(data, 1, szFile, f);
    if ((nRead == 0) & !feof(f)) {
        fmt::println(stderr, "ERR: ProcessRaw failed to read file '{}'", pathToBinary.c_str());
        free(data);
        return false;
    }
    fclose(f);

    cpuemu.memoryUnit.CopyToRamFromExt(rawLoadToAddress,data, szFile);

    free(data);

    auto res = ExecuteData(rawStartAddress, szFile);
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

bool ExecuteData(uint64_t startAddress, size_t szCode) {

    // Test syscall dispatch...
    cpuemu.RegisterSysCall(0x01, "writeline",[](Registers &regs, CPUBase *cpu) {
        auto addrString = regs.addressRegisters[0].data.longword;
        auto ptrString = cpu->GetRawPtrToRAM(addrString);
        fmt::println("syscall - writeline - a0 = {}",(char *)ptrString);
    });
/*
    // --> Break out to own function
    fmt::println("Disasm firmware from address: {:#x}", startAddress);
    // This is pretty lousy, as we have no clue where code/data ends...
    uint64_t disasmPtr = startAddress;
    while(disasmPtr < (startAddress + szCode)) {
        auto instrDec = InstructionDecoder::Create(disasmPtr);
        if (!instrDec->Decode(cpuemu)) {
            break;
        }
        std::string strOpCodes = "";
        for(auto ofs = instrDec->GetInstrStartOfs(); ofs < instrDec->GetInstrEndOfs(); ofs++) {
            strOpCodes += fmt::format("0x{:02x}, ",cpu_ram_memory[ofs]);
        }
        // Retrieve the last decoded instruction and transform to string
        fmt::println("0x{:04x}\t\t{}\t\t; {}", disasmPtr, instrDec->ToString(), strOpCodes);
        disasmPtr += instrDec->GetInstrSizeInBytes();
    }
    // <-- end of disassembly..
*/

    fmt::println("Set Initial IP to: {:#x}", startAddress);
    cpuemu.SetInstrPtr(startAddress);

    // Save ip before we step...
    RegisterValue ip = cpuemu.GetInstrPtr();

    fmt::println("------->> Begin Execution <<--------------");
    while(cpuemu.Step()) {
        // generate op-codes for this instruction...
        std::string strOpCodes = "";

        auto lastDecoded = cpuemu.GetLastDecodedInstr();

        for(auto ofs = lastDecoded->GetInstrStartOfs(); ofs < lastDecoded->GetInstrEndOfs(); ofs++) {
            strOpCodes += fmt::format("0x{:02x}, ",cpu_ram_memory[ofs]);
        }
        // Retrieve the last decoded instruction and transform to string
        auto lastInstr = cpuemu.GetLastDecodedInstr();
        auto str = lastInstr->ToString();

        // now output address (16 bit), instruction and opcodes
        fmt::println("0x{:04x}\t\t{}\t\t; {}", ip.data.word,str, strOpCodes);
        // Dump stats and register changes caused by this operation
        DumpStatus(cpuemu);
        DumpRegs(cpuemu);

        // save current ip (this is for op-code)
        ip = cpuemu.GetInstrPtr();
        fmt::println("");

        // Did we trigger 'halt'
        if (cpuemu.IsHalted()) {
            break;
        }
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
