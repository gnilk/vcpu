//
// Created by gnilk on 20.01.25.
//

#ifndef VCPU_PREPROCESSOR_H
#define VCPU_PREPROCESSOR_H

#include <string>
#include <string_view>
#include <functional>
#include <vector>

#include "Lexer/TextLexer.h"

using namespace gnilk;

namespace gnilk {
    namespace assembler {
        class PreProcessor {
        public:
            using LoadAssetDelegate = std::function<bool(std::string &out, const std::string &assetName, int flags)>;
        public:
            PreProcessor() = default;
            virtual ~PreProcessor() = default;

            bool Process(std::vector <Token> &tokens, LoadAssetDelegate assetLoader);
        protected:
            bool IsValidDirectiveAt(size_t idx, const std::vector<Token> &tokens);
            bool ProcessIncludeDirective(size_t &idxAt, std::vector<Token> &tokens, LoadAssetDelegate assetLoader);
            bool IsAssetAlreadyLoaded(const std::string &assetName);
        private:
            std::vector<std::string> assetsLoaded;
        };
    }
}


#endif //VCPU_PREPROCESSOR_H
