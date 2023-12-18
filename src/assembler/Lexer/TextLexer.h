#ifndef GNK_TEXTLEXER_H
#define GNK_TEXTLEXER_H

#include <unordered_map>
#include <string>
#include <memory>
#include <stack>
#include <vector>
#include <functional>

namespace gnilk {

    enum class TokenType {
        Unknown,
        Number,
        // These are sub-classes (basically) of number - since we store number definitions as strings from the tokenizer
        NumberHex,
        NumberOctal,
        NumberBinary,

        String,
        Identifier,
        // Registers
        DataReg,
        AddressReg,
        // special registers
        sp,ip,

        // instructions
        Instruction,

        // Operand Size specifier
        OpSize,

        // Grouping and Operators
        BinaryOperator,
        CompareOperator,
        LogicalAndOperator,
        LogicalOrOperator,
        Equals,
        Comma,
        Exclamation,
        Dot,
        Colon,
        Semicolon,
        OpenParen,
        CloseParen,
        OpenBrace,
        CloseBrace,
        OpenBracket,
        CloseBracket,
        LineComment,
        BlockCommentStart,
        BlockCommentEnd,
        CommentedText,
        Quote,
        Null,
        EoL,
        EoF,
    };

    struct Token {
        TokenType type = {};
        std::string value = {}; // Actual token value/string
        size_t idxOrigStr = 0;     // index in the original string
    };

    enum class ParseState {
        Regular,
        BlockComment,
    };

    class Context;
    class Lexer;

    const Context &GetLexerContext();


    class Context {
        friend Lexer;
    public:
        using state_stack_t = std::stack<ParseState>;
        using token_map_t = std::unordered_map<std::string, TokenType>;
    public:
        Context() = default;
        Context(const token_map_t &useKeywords, const token_map_t &useOperators) :
            keywords(useKeywords), operators(useOperators) {
            PushState(startState);
        }

        virtual ~Context() = default;

        state_stack_t &StateStack() { return stateStack; }
        const token_map_t &Keywords() { return keywords; }
        const token_map_t &Operators() { return operators; }
        const std::string &StringEscapeCodes() { return stringEscapeCodes; }
        const std::string &BlockCommentEndOperator() { return blockCommentEndOperator; }

        void SetKeywords(const token_map_t &newKeywords) {
            keywords = newKeywords;
        }
        void SetOperators(const token_map_t &newOperators) {
            operators = newOperators;
        }
        void SetStringEscapeCodes(const std::string &newEscapeCodes) {
            stringEscapeCodes = newEscapeCodes;
        }

        const ParseState State() {
            return stateStack.top();
        }
        __inline void ResetStateStack() {
            while(!stateStack.empty()) {
                stateStack.pop();
            }
            PushState(startState);
        }
    protected:
        // in order to be parallel - we can't use states...
        __inline void PushState(ParseState newState) {
            stateStack.push(newState);
        }
        __inline void PopState() {
            stateStack.pop();
        }

    protected:
        state_stack_t stateStack = {};
        token_map_t keywords = {};
        token_map_t operators = {};
        ParseState startState = ParseState::Regular;

        // Note: these two needs to be re-evaluated how to handle...
        std::string blockCommentEndOperator = {"*/"};
        std::string stringEscapeCodes = {"\"\'\\?abfnrtvox"};
    };


    class Lexer {
    public:
        using Ref = std::shared_ptr<Lexer>;
    public:
        // Move context to CTOR?
        Lexer() = default;
        virtual ~Lexer() = default;

        std::vector<Token> Tokenize(const std::string_view &text);
        bool Tokenize(std::vector<Token> &outTokens, Context &context, const std::string_view &text);

        bool IsInt(int chr);
        bool IsAlpha(int chr);
        bool IsAlphaNum(int chr);
        bool IsSkippable(int chr);
    private:
        // true - continue, false - break
        bool ParseStateRegular(std::vector<Token> &tokens, Context &context, const std::string_view &line);
        bool ParseStateBlockComment(std::vector<Token> &tokens, Context &context, const std::string_view &line);

    private:
//        Token CreateToken(TokenType tokenType, size_t idxIterator, const std::string &str);
        Token CreateToken(TokenType tokenType, size_t idxIterator, const std::string_view &str);
        Token CreateToken(TokenType tokenType, size_t idxIterator, int chr);


        void ParseOperator(std::vector<Token> &outTokens, Context &context, std::string &tokenStr, const std::string_view &line);
        void ParseString(std::vector<Token> &outTokens, Context &context, const std::string_view &line);
        void ParseIdentifier(std::vector<Token> &outTokens, Context &context, const std::string_view &line);
        bool ParseNumber(std::vector<Token> &outTokens, Context &context, const std::string_view &line);
        bool SkipToNewLine(Context &context, const std::string_view &line);
    private:
        std::string_view::const_iterator it;

    };

}

#endif
