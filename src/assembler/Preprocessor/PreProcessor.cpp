//
// Created by gnilk on 20.01.25.
//

#include <ranges>
#include <algorithm>
#include "PreProcessor.h"
#include "StrUtil.h"
#include <fmt/format.h>

using namespace gnilk::assembler;

bool PreProcessor::Process(std::vector<Token> &tokens, LoadAssetDelegate assetLoader) {
    for(size_t i=0;i<tokens.size();i++) {

        // Skip any non-preprocessor token
        if (tokens[i].type != TokenType::PreProcessor) {
            continue;
        }
        // Verify the directive has a valid structure
        if (!IsValidDirectiveAt(i, tokens)) {
            fmt::println(stderr, "ERR: Invalid preprocessor directive");
            return false;
        }

        // Just one directive supported right now...
        if (tokens[i].value == "include") {
            if (!ProcessIncludeDirective(i, tokens, assetLoader)) {
                return false;
            }
        } else {
            fmt::println(stderr, "ERR: Invalid preprocessor directive {}", tokens[i].value);
            return false;
        }
    }
    return true;
}

//
// Checks that the preprocessor directive is valid
//
bool PreProcessor::IsValidDirectiveAt(size_t idx, const std::vector<Token> &tokens) {
    // preprocessor must have prefix (dot) and post-fix string
    if ((idx < 0) || (idx == tokens.size() - 1)) {
        return false;
    }
    if (tokens[idx - 1].type != TokenType::Dot) {
        // pre-processor directive must have '.' first
        return false;
    }
    if (tokens[idx + 1].type != TokenType::String) {
        return false;
    }
    return true;
}

// Note: Will change the index (-1) and insert asset-tokens in the tokens vector
bool PreProcessor::ProcessIncludeDirective(size_t &idxAt, std::vector<Token> &tokens, LoadAssetDelegate assetLoader) {

    // We know idxAt is proper and can be referenced like this - IsValidDirectiveAt checks this for us
    auto includeName = tokens[idxAt + 1].value;

    // Skip already loaded assets
    if (IsAssetAlreadyLoaded(includeName)) {
        fmt::println("Warning, include '{}' already loaded, skipping - circular dependency?", includeName);
        return true;
    }

    // Load asset data
    std::string assetData;
    if (!assetLoader(assetData, includeName, 0)) {
        fmt::println(stderr, "Err: include '{}' failed to load asset", includeName);
        return false;
    }

    // Parse asset data and replace the '.include' directive with the asset tokens

    Lexer lexer;
    auto assetTokens = lexer.Tokenize(assetData);
    // Erase include directive from tokens
    tokens.erase(tokens.begin()+idxAt-1, tokens.begin()+idxAt+2);
    // insert the tokens from the asset instead
    tokens.insert(tokens.begin()+idxAt-1, assetTokens.begin(), assetTokens.end()-1);

    // rewind counter to the first token of the loaded assets
    // this ensures parsing continues on the included file
    // . | include | <asset>        ; will be replaced with everything from the asset, the '.' is current - 1
    //
    idxAt = idxAt - 1;

    return true;
}

bool PreProcessor::IsAssetAlreadyLoaded(const std::string &assetName) {
    return (std::find(assetsLoaded.begin(), assetsLoaded.end(), assetName) != assetsLoaded.end());
}