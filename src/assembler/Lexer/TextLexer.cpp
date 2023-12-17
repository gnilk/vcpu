/*
 * The lexer is responsible to split the incoming raw text to a well defined tokens.
 * Tokens can either belong to a group or be defined explicitly.
 *
 * A token can be a keyword (if/while/return/etc..) or an operator (-,*,-, etc..) - EACH character will be processed
 * and classified. The result is an array/list of tokens.
 *
 * Basically this transforms the stream of characters to a stream of tokens which are sent to the parser.
 *
 * While tokens can be shared by multiple languages they are generally defined together with the grammar in for the
 * parser.
 *
 * Note: This lexer is fast - but has a quirk - any multichar token; like '==' or '>=' (in C) MUST have a single
 * char token with same start ('=' or '>') - if they are not used - you should map to 'TokenType::Unknown'.
 *
 * In the built-in token set I have defined '&' and '|' as such tokens since I don't support C bitwise operators.
 * But I support the C logical operators ('&&' and '||').
 *
 * The reason behind this quirk is an optimization - the 'current token under classification' is a string which
 * get's concatenated for each new char. For each concatenation a pre-classification is done on the first char.
 * As such - functions can't start with numbers nor symbols. And operators can't be 'names'.
 *
 * However - the lexer is very fast..  =)
 *
 */
#include "TextLexer.h"
#include "StrUtil.h"

using namespace gnilk;

//
// TOOD:
//   - fix direct access to iterator
//   - helper for calculating the distance (may need one with an offset as well)
//   - function to peek the next char, this will speed up a few things in a few places -> string escape stuff
//   - add pre-processor stuff to either keywords or special '#' operator
//   - add more operators
//   - generalize the operator and key-word tables, use a struct { TokenType, TokenFamily, ActionLambda?? }
//

// Note quite sure about this:
//  https://en.cppreference.com/w/cpp/language/operator_alternative
// Should do this:
//   https://en.cppreference.com/w/cpp/language/escape
//



const Context &gnilk::GetLexerContext() {
    static std::unordered_map<std::string, TokenType> keywords = {
        // Instructions
        {"move", TokenType::Instruction},
        {"add", TokenType::Instruction},
        {"sub", TokenType::Instruction},
        {"mul", TokenType::Instruction},
        {"div", TokenType::Instruction},
        {"call", TokenType::Instruction},
        {"ret",TokenType::Instruction},
        {"nop",TokenType::Instruction},
        {"brk",TokenType::Instruction},
        {"null", TokenType::Null},
        // Data Registers
        {"d0", TokenType::DataReg},
        {"d1", TokenType::DataReg},
        {"d2", TokenType::DataReg},
        {"d3", TokenType::DataReg},
        {"d4", TokenType::DataReg},
        {"d5", TokenType::DataReg},
        {"d6", TokenType::DataReg},
        {"d7", TokenType::DataReg},
        // Address Registers
        {"a0", TokenType::AddressReg},
        {"a1", TokenType::AddressReg},
        {"a2", TokenType::AddressReg},
        {"a3", TokenType::AddressReg},
        {"a4", TokenType::AddressReg},
        {"a5", TokenType::AddressReg},
        {"a6", TokenType::AddressReg},
        {"a7", TokenType::AddressReg},

};

    static std::unordered_map<std::string, TokenType> operators = {
        // Size operators
        {".b", TokenType::OpSize},
        {".w", TokenType::OpSize},
        {".d", TokenType::OpSize},
        {".l", TokenType::OpSize},

        // compare operators
        {"==", TokenType::CompareOperator},
        {"!=", TokenType::CompareOperator},
        {">=", TokenType::CompareOperator},
        {"<=", TokenType::CompareOperator},
        {">", TokenType::CompareOperator},
        {"<", TokenType::CompareOperator},

        {"(", TokenType::OpenParen},
        {")", TokenType::CloseParen},
        {"{", TokenType::OpenBrace},
        {"}", TokenType::CloseBrace},
        {"{", TokenType::OpenBracket},
        {"}", TokenType::CloseBracket},
        {";", TokenType::Semicolon},
        {":", TokenType::Colon},
        {",", TokenType::Comma},
        {".", TokenType::Dot},
        {"=", TokenType::Equals},
        {"!", TokenType::Exclamation},
        {"\"", TokenType::Quote},
        {"//", TokenType::LineComment},

        // binary operators
        {"+", TokenType::BinaryOperator},
        {"-", TokenType::BinaryOperator},
        {"*", TokenType::BinaryOperator},
        {"/", TokenType::BinaryOperator},
        {"%", TokenType::BinaryOperator},

        // logical operators
        {"&&", TokenType::LogicalAndOperator},
        {"||", TokenType::LogicalOrOperator},

        // Remove if/when we support this - the tokenizer requires at least a single char of something that might
        // be part of a multi-char operator for the initial match...
        {"&", TokenType::Unknown},
        {"|", TokenType::Unknown},

    };

// Add these...
//    {"//", TokenType::LineComment},
//            {"/*", TokenType::BlockCommentStart},
//            {"*/", TokenType::BlockCommentEnd},

/*
    // Supported operators
    static std::unordered_map<std::string, TokenType> operators = {
        {"\n", TokenType::EoL},
        {"(", TokenType::OpenParen},
        {")", TokenType::CloseParen},
        {"{", TokenType::OpenBrace},
        {"}", TokenType::CloseBrace},
        {"[", TokenType::OpenBracket},
        {"]", TokenType::CloseBracket},
        {";", TokenType::Semicolon},
        {":", TokenType::Colon},
        {",", TokenType::Comma},
        {".", TokenType::Dot},
        {"#", TokenType::HashTag},
        {"=", TokenType::Equals},
        {"\"", TokenType::Quote},
        // binary operators
        {"+", TokenType::BinaryOperator},
        {"-", TokenType::BinaryOperator},
        {"*", TokenType::BinaryOperator},
        {"/", TokenType::BinaryOperator},
        {"%", TokenType::BinaryOperator},   // modulos
        {"|", TokenType::BinaryOperator},   // or
        {"^", TokenType::BinaryOperator},   // xor
        {"&", TokenType::BinaryOperator},   // and
        // Post operators -> token type is bogus
        {"--", TokenType::BinaryOperator},
        {"++", TokenType::BinaryOperator},
        // Compare
        {"==", TokenType::BinaryOperator},
        // Assign + operator
        {"+=", TokenType::BinaryOperator},
        {"-=", TokenType::BinaryOperator},
        {"*=", TokenType::BinaryOperator},
        {"/=", TokenType::BinaryOperator},
        {"&=", TokenType::BinaryOperator},
        {"|=", TokenType::BinaryOperator},

    };
  */
    static Context context(keywords, operators);
    return context;
} // 'GetCppContext

std::vector<Token> Lexer::Tokenize(const std::string &text) {
    auto context = GetLexerContext();
    std::vector<Token> outTokens;
    if (!Tokenize(outTokens, context, text)) {
        return {};
    }
    return outTokens;
}
bool Lexer::Tokenize(std::vector<Token> &outTokens, Context &context, const std::string_view &text) {


    it = text.begin();
    outTokens.clear();
    while(it != text.end()) {
        if (context.State() == ParseState::Regular) {
            if (!ParseStateRegular(outTokens, context, text)) {
                goto leave;
            }
        } else if (context.State() == ParseState::BlockComment) {
            if (!ParseStateBlockComment(outTokens, context, text)) {
                goto leave;
            }
        }
    }
leave:
    outTokens.push_back(CreateToken(TokenType::EoF, 0, "EndOfFile"));
    return true;
}

bool Lexer::ParseStateRegular(std::vector<Token> &tokens, Context &context, const std::string_view &line) {
    std::string tokenStr(1,*it);
    auto &operators = context.Operators();
    if (operators.contains(tokenStr)) {
        // Handle string literal...  FIXME: should be separate function
        if (operators.at(tokenStr) == TokenType::Quote) {
            ParseString(tokens, context, line);
        } else {
            ParseOperator(tokens, context, tokenStr, line);
            if (tokens.back().type == TokenType::BlockCommentStart) {
                context.PushState(ParseState::BlockComment);
            } else if (tokens.back().type == TokenType::LineComment) {
                auto idxStartComment = std::distance(line.begin(), it+1);
                if (idxStartComment > line.length()) {
                    int breakme = 1;
                    return false;
                }
                SkipToNewLine(context, line);
                auto idxCommentEnd = std::distance(line.begin(), it+1);
                auto commentLength = idxCommentEnd - idxStartComment;
                tokens.push_back(CreateToken(TokenType::CommentedText, idxStartComment, line.substr(idxStartComment, commentLength)));
                if (it != line.end()) {
                    ++it;
                } else {
                    //
                    tokens.push_back(CreateToken(TokenType::EoL, 0, 0));
                }
                return true;
            }
        }
        // FIXME: Not needed when multiline strings are supported
        if (it != line.end()) {
            ++it;
        }
    } else {
        if (IsInt(*it)) {
            if (!ParseNumber(tokens, context, line)) {
                return false;
            }
        } else if (IsAlpha(*it)) {
            ParseIdentifier(tokens, context, line);
        } else if (IsSkippable(*it)) {
            ++it;
        } else {
            // Unknown char - skip it...
            ++it;
            //fprintf(stderr, "Unknown character in source: 0x%.2x (%c)", *it, *it);
            //exit(1);
        }
    }
    return true;
}

// make this more robust - handle multiple types of CR/LN combinations
bool Lexer::SkipToNewLine(Context &context, const std::string_view &line) {
    std::string tokenStr(1,*it);
    auto &operators = context.Operators();
    while(it != line.end()) {
        if (operators.contains(tokenStr) && (operators.at(tokenStr) == TokenType::EoL)) {
            // We should not eat this char..
            --it;
            return true;
        }
        if (it != line.end()) {
            ++it;
        }
        tokenStr = *it;
    }
    return true;
}


// This is more or less a full parse-loop we are just hunting for the block-end-operator
bool Lexer::ParseStateBlockComment(std::vector<Token> &tokens, Context &context, const std::string_view &line) {
    std::string commentText(1,*it);
    std::string tokenStr = std::string(1,*it);
    auto idxCommentStart = std::distance(line.begin(), it);
    auto idxTokenStart = std::distance(line.begin(), it);

    //
    // Note: I don't think we can handle 'blockCommentEndOperators' larger > 2 chars -> test & verify!!
    //
    while(it != line.end()) {
        if (strutil::startsWith(tokenStr, context.BlockCommentEndOperator())) {
            // Remove the 'block end operator' from the commented text
            commentText.erase(commentText.length() - tokenStr.length());
            if (!commentText.empty()) {
                tokens.push_back(CreateToken(TokenType::CommentedText, idxCommentStart, commentText));
            }
            tokens.push_back(CreateToken(TokenType::BlockCommentEnd,  idxTokenStart, tokenStr));
            context.PopState();
            ++it;
            return true;
        } else {
            // We didn't match - reset the tokenstr at the current char plus the index for it...
            tokenStr = std::string(1,*it);
            idxTokenStart = std::distance(line.begin(), it);
        }
        // advance to next and append...
        // this makes the tokenstr a sliding window over two chars...
        ++it;
        commentText += *it;
        tokenStr += *it;
    }
    // We ran the whole string - so this is just all commented text...
    tokens.push_back(CreateToken(TokenType::CommentedText, idxCommentStart, commentText));
    return false;
}


// Parse an operator - single or multi-char
void Lexer::ParseOperator(std::vector<Token> &outTokens, Context &context, std::string &tokenStr, const std::string_view &line) {
    const auto idxTokenStart = it - line.begin(); //std::distance(srcCode.begin(), it);
    // As long has we have operator matches - continue to read data...

    auto &operators = context.Operators();
    while(it != line.end() && operators.contains(tokenStr)) {
        ++it;
        tokenStr += *it;
    }
    // We will always read plus one - i.e. until we don't have a match any more - pop-one off again
    --it;
    tokenStr.pop_back();
    outTokens.push_back(CreateToken(operators.at(tokenStr), idxTokenStart, tokenStr));
}

// Parse a string
void Lexer::ParseString(std::vector<Token> &outTokens, Context &context, const std::string_view &line) {
    // FIXME: better string parsing
    ++it;  // remove \"
    std::string str;
    const auto idxTokenStart = std::distance(line.begin(), it);
    while((it != line.end()) && (*it != '\"')) {
        // TODO: Handle in-string operators - like escape characters...
        // https://en.cppreference.com/w/cpp/language/escape
        if (*it=='\\') {
            ++it;
            // Do we have a valid escape code?
            if (context.StringEscapeCodes().find(*it) != std::string::npos) {
                ++it;
            }
        }
        str += *it;
        ++it;
    }
    outTokens.push_back(CreateToken(TokenType::String,  idxTokenStart, str));
}

// Parse an identifier - identifiers can not start with numbers (but may contain them)
void Lexer::ParseIdentifier(std::vector<Token> &tokens, Context &context, const std::string_view &line) {
    std::string ident;

    auto &keywords = context.Keywords();
    const auto idxTokenStart = std::distance(line.begin(), it);
    while(it != line.end() && IsAlphaNum(*it)) {        // <- DO NOT use 'IsAlpha' here
        ident += *it;
        ++it;
    }
    // Keyword??
    if (keywords.contains(ident)) {
        tokens.push_back(CreateToken(keywords.at(ident), idxTokenStart, ident));
    } else {
        tokens.push_back(CreateToken(TokenType::Identifier, idxTokenStart, ident));
    }
}

// Parse a number - numbers are integers: '1234' or floating point numbers '123.4345'..
bool Lexer::ParseNumber(std::vector<Token> &tokens, Context &context, const std::string_view &line) {

    std::string num;
    const auto idxTokenStart = it - line.begin(); //std::distance(srcCode.begin(), it);

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

    auto numberType = TokenType::Number;
    if (*it == '0') {
        num += *it;
        it++;
        // Convert number here or during parsing???
        switch(tolower(*it)) {
            case 'x' : // hex
                num += *it;
                ++it;
                numberType = TokenType::NumberHex;
                isnumber = [](const int chr) -> bool {
                    static std::string hexnum = {"abcdef"};
                    return (std::isdigit(chr) || (hexnum.find(tolower(chr)) != std::string::npos));
                };
                break;
            case 'b' : // binary
                num += *it;
                ++it;
                numberType = TokenType::NumberBinary;
                isnumber = [](const int chr) -> bool {
                    return (chr=='1' || chr=='0');
                };

                break;
            case 'o' : // octal
                num += *it;
                ++it;
                numberType = TokenType::NumberOctal;
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


    tokens.push_back(CreateToken(numberType, idxTokenStart, num));
    return true;
}

// short cuts for creating tokens..
//Token Lexer::CreateToken(TokenType tokenType, size_t idx, const std::string &str) {
//    return Token {str, idx,  tokenType };
//}
Token Lexer::CreateToken(TokenType tokenType, size_t idx, const std::string_view &str) {
    return Token {tokenType, std::string(str), idx};
}
Token Lexer::CreateToken(TokenType tokenType, size_t idx, int chr) {
    return Token { tokenType , std::string(1, chr), idx};
}

bool Lexer::IsInt(int chr) {
    return std::isdigit(chr);
}

bool Lexer::IsAlpha(int chr) {
    return std::isalpha(chr);
}

bool Lexer::IsAlphaNum(int chr) {
    return std::isalnum(chr);
}

bool Lexer::IsSkippable(int chr) {
    return (chr == ' ' || chr=='\n' || chr=='\t');
}

