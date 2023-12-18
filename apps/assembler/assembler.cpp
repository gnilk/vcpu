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

bool ProcessFile(const std::string &outFilename, std::filesystem::path &pathToSrcFile);

int main(int argc, const char **argv) {
    std::string outFilename = "a.gnk";
    std::vector<std::string> filesToAssemble;
    for(int i=1;i<argc;i++) {
        if (argv[i][0] == '-') {
            // parse
        } else {
            filesToAssemble.push_back(argv[i]);
        }
    }
    if (filesToAssemble.empty()) {
        fmt::println("Nothing to do, bailing...");
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
            fmt::println("{} - OK", fToAsm);
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

    auto ast = parser.ProduceAST(srcData);
    if (ast == nullptr) {
        return false;
    }
    if (!compiler.GenerateCode(ast)) {
        return false;
    }

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

