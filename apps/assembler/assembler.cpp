//
// Created by gnilk on 14.12.23.
//
#include <stdlib.h>
#include <stdio.h>
#include <vector>
#include <filesystem>

#include "Compiler/Compiler.h"
#include "fmt/format.h"
#include "Parser/Parser.h"

#include "Linker/RawLinker.h"
#include "Linker/ElfLinker.h"
#include "InstructionSetV1/InstructionSetV1.h"
#include "Simd/SIMDInstructionSet.h"
#include "InstructionSet.h"

#include <stack>


static gnilk::assembler::BaseLinker::Ref ptrUseLinker = nullptr;

bool ProcessFile(const std::string &outFilename, std::filesystem::path &pathToSrcFile);

static void Usage() {
    fmt::println("Ruddy Assembler for VCPU project");
    fmt::println("Use:");
    fmt::println("  asm [options] <input file>");
    fmt::println("Options");
    fmt::println("  -o <output>    Output filename (default: a.gnk)");
    fmt::println("  -t <raw | elf> Binary type (default: elf)");
    fmt::println("Ex:");
    fmt::println("  asm -o mybinary.bin -t raw mysource.asm");
    exit(1);
}

int main(int argc, const char **argv) {
    std::string outFilename = "a.gnk";
    std::vector<std::string> filesToAssemble;


    for(int i=1;i<argc;i++) {
        if (argv[i][0] == '-') {
            switch(argv[i][1]) {
                case 'o' :
                    outFilename = argv[++i];
                    break;
                case 't' : {
                    auto linkerName = argv[++i];
                    if (linkerName == std::string("raw")) {
                        ptrUseLinker = std::make_shared<gnilk::assembler::RawLinker>();
                    } else if (linkerName == std::string("elf")) {
                        ptrUseLinker = std::make_shared<gnilk::assembler::ElfLinker>();
                    } else {
                        fmt::println(stderr, "Unknown linker: {}", linkerName);
                        Usage();
                    }
                    break;
                }
                case '?' :
                case 'H' :
                case 'h' :
                    Usage();
                    break;
                default:
                    fmt::println(stderr, "Unknown option '{}'", argv[i]);
                    Usage();
            }
            // parse
        } else {
            filesToAssemble.push_back(argv[i]);
        }
    }

    // Set default linker if not set already...
    if (ptrUseLinker == nullptr) {
        ptrUseLinker = std::make_shared<gnilk::assembler::ElfLinker>();
    }

    if (filesToAssemble.empty()) {
        fmt::println("Nothing to do, bailing...");
        Usage();
        return 0;
    }
    for(auto &fToAsm : filesToAssemble) {
        std::filesystem::path pathToFile(fToAsm);
        if (!exists(pathToFile)) {
            fmt::println(stderr, "No such file or directory, {}", fToAsm);
            continue;;
        }
        if (is_directory(pathToFile)) {
            fmt::println(stderr, "Directories not supported");
            continue;;
        }
        if (!is_regular_file(pathToFile)) {
            fmt::println(stderr, "Not a regular file {} - skipping", fToAsm);
            continue;;
        }
        if (!ProcessFile(outFilename, pathToFile)) {
            fmt::println("{} - failed", fToAsm);
        } else {
            fmt::println("Ok, {} -> {}", fToAsm, outFilename);
        }
    }

    return 0;
}

/// \brief
/// \param pathToFile
/// \return a
bool CompileData(const std::string &outFilename, const std::string_view &srcData);
bool SaveCompiledData(const std::string &outFilename, const std::vector<uint8_t> &data);

static std::stack<std::string> assetStack;


bool LoadAsset(std::string &out, const std::string &assetName, int flags) {
    // Assume absolute
    std::filesystem::path pathToAssetFile(assetName);

    if ((!pathToAssetFile.is_absolute()) && (!assetStack.empty())) {
        printf("Dependent asset, relative path!\n");
        pathToAssetFile = std::filesystem::path(assetStack.top()).parent_path() / assetName;
    }
    printf("Asset Path: %s\n", absolute(pathToAssetFile).c_str());

    if (!exists(pathToAssetFile)) {
        return false;
    }

    size_t szFile = file_size(pathToAssetFile);
    // Ok, so this is really stupid...
    char *data = (char *)malloc(szFile + 10);
    if (data == nullptr) {
        return false;
    }
    memset(data, 0, szFile + 10);

    auto f = fopen(pathToAssetFile.c_str(), "r+");
    if (f == nullptr) {
        return false;
    }
    auto nRead = fread(data, 1, szFile, f);
    out = std::string(data);

    free(data);
    fclose(f);

    assetStack.push(assetName);

    fmt::println("Ok, '{}' loaded", assetName);

    return true;
}

bool ProcessFile(const std::string &outFilename, std::filesystem::path &pathToSrcFile) {

    std::string strData;
    if (!LoadAsset(strData, absolute(pathToSrcFile), 0)) {
        return false;
    }

    return CompileData(outFilename, strData);
}

bool CompileData(const std::string &outFilename, const std::string_view &srcData) {


    if (!gnilk::vcpu::InstructionSetManager::Instance().HaveInstructionSet()) {
        gnilk::vcpu::InstructionSetManager::Instance().SetInstructionSet<gnilk::vcpu::InstructionSetV1>();
    }

    gnilk::assembler::Parser parser;
    gnilk::assembler::Compiler compiler;



    // Use the elf-linker by default...
    compiler.SetLinker(ptrUseLinker);

    parser.SetAssetLoader(LoadAsset);
    auto ast = parser.ProduceAST(srcData);
    if (ast == nullptr) {
        return false;
    }
    if (!compiler.CompileAndLink(ast)) {
        return false;
    }
    fmt::println("Compile took {} milliseconds", compiler.GetCompileDurationMS());
    fmt::println("Link took {} milliseconds", compiler.GetLinkDurationMS());

    return SaveCompiledData(outFilename, compiler.Data());
}

bool SaveCompiledData(const std::string &outFilename, const std::vector<uint8_t> &data) {
    FILE *fOut = fopen(outFilename.c_str(), "w+");
    if (fOut == nullptr) {
        fmt::println(stderr, "Unable to open {} for writing", outFilename);
        return false;
    }
    fwrite(data.data(), 1, data.size(), fOut);
    fclose(fOut);

    return true;
}

