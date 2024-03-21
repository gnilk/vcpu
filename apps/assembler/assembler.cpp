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

#include "Linker/DummyLinker.h"
#include "Linker/ElfLinker.h"

static gnilk::assembler::DummyLinker dummyLinker;
static gnilk::assembler::ElfLinker elfLinker;

static gnilk::assembler::BaseLinker *ptrUseLinker = &elfLinker;

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
                        ptrUseLinker = &dummyLinker;
                    } else if (linkerName == std::string("elf")) {
                        ptrUseLinker = &elfLinker;
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

bool ProcessFile(const std::string &outFilename, std::filesystem::path &pathToSrcFile) {
    size_t szFile = file_size(pathToSrcFile);
    char *data = (char *)malloc(szFile + 10);
    if (data == nullptr) {
        return false;
    }
    memset(data, 0, szFile + 10);
    std::string_view strData(data, szFile);

    // Read file
    FILE *f = fopen(pathToSrcFile.c_str(), "r+");
    if (f == nullptr) {
        free(data);
        return false;
    }
    auto nRead = fread(data, 1, szFile, f);
    fclose(f);

    return CompileData(outFilename, strData);
}

bool CompileData(const std::string &outFilename, const std::string_view &srcData) {

    gnilk::assembler::Parser parser;
    gnilk::assembler::Compiler compiler;

    // Use the elf-linker by default...
    compiler.SetLinker(ptrUseLinker);

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

