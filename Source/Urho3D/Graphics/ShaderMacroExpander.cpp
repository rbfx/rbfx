#include "./ShaderMacroExpander.h"
#include "../Core/Variant.h"
#include "../Container/Hash.h"
#ifdef URHO3D_DEBUG
#include "../IO/Log.h"
#endif
#define MAX_SEARCH_LOOP 5
namespace Urho3D
{
    static unsigned sNewLineTokenHash = MakeHash('\n');
    static unsigned sSpacementTokenHash = MakeHash(' ');

    struct TokenDesc {
        ea::string value_;
        unsigned id_{M_MAX_UNSIGNED};
        unsigned idx_;
        unsigned childIdx_;
        size_t line_;
        size_t col_;
    };

    class TokenBase;
    struct TokenProcessDesc {
        const ea::unordered_map<unsigned, ea::string>* macros_;
        // List of all Tokens
        const ea::vector<ea::shared_ptr<TokenBase>>* tokenList_;
        // Root of Token Tree, this is where algorithm will run.
        const ea::vector<ea::shared_ptr<TokenBase>>* tokenRootTree_;
        const ea::string* sourceCode_;
        ea::string* outputCode_;
    };
    class TokenBase {
    public:
        TokenBase(){}
        virtual void Process(unsigned& parentSeek, TokenProcessDesc& processDesc) {
            for (unsigned i = 0; i < children_.size(); ++i) {
                children_[i]->Process(i, processDesc);
            }
        }
        virtual bool Evaluate(size_t firstValue, size_t secondValue) { return false; }
        virtual bool BeginChild() { return false; }
        virtual bool EndChild() { return false; }
        virtual bool IsOperator() { return false; }
        virtual bool IsMacro() { return false; }
#ifdef URHO3D_DEBUG
        void PrintDbgTree(unsigned identationLevel = 0) {
            ea::string identationSymbol = "";
            for (unsigned i = 0; i < identationLevel; ++i)
                identationSymbol.append("-");
            if (identationLevel > 0)
                identationSymbol.append(" ");
            URHO3D_LOGDEBUG(Format("{}{}", identationSymbol, desc_.value_));
            for (auto childIt = children_.end(); childIt != children_.end(); ++childIt) {
                (*childIt)->PrintDbgTree(identationLevel + 1);
            }
        }
#endif
        // Children Tokens
        ea::vector<ea::shared_ptr<TokenBase>> children_;
        ea::weak_ptr<TokenBase> parent_;

        TokenDesc desc_{};
    };
    class IdentityToken : public TokenBase {
    public:
        IdentityToken() : TokenBase() {}
         virtual void Process(unsigned& parentSeek, TokenProcessDesc& processDesc) override {
            processDesc.outputCode_->append(desc_.value_);
            TokenBase::Process(parentSeek,processDesc);
        }
    };
    class MacroToken : public TokenBase {
    public:
        MacroToken() : TokenBase() {}
        virtual bool IsMacro() override {
            return true;
        }
        virtual bool BeginChild() override {
            return true;
        }
        bool enabled_{ false };
    };
    class IfDefToken : public MacroToken {
    public:
        IfDefToken() : MacroToken() {}
        virtual void Process(unsigned& parentSeek, TokenProcessDesc& processDesc) override {
            ea::shared_ptr<TokenBase> nextToken;

            size_t tokenIdx = desc_.idx_;
            uint8_t loop = 0;
            // Find next valid token.
            while (loop < MAX_SEARCH_LOOP) {
                ++tokenIdx;
                ++loop;
                nextToken = processDesc.tokenList_->at(tokenIdx);
                if (nextToken->IsOperator()) {
                    throw std::runtime_error(
                        Format("Expected a identifier. ({}, {})",
                            nextToken->desc_.col_,
                            nextToken->desc_.line_
                        ).c_str()
                    );
                }
                if (nextToken->desc_.id_ == sSpacementTokenHash)
                    continue;
                break;
            }

            auto macroIt = processDesc.macros_->find(nextToken->desc_.id_);
            enabled_ = macroIt != processDesc.macros_->end();
            if (!enabled_)
                return;
            // Start loop after child index.
            for (unsigned i = nextToken->desc_.childIdx_ + 1; i < children_.size(); ++i)
                children_[i]->Process(i, processDesc);
        }
    };
    class IfToken : public MacroToken {
    public:
        IfToken() : MacroToken() {}
        virtual void Process(unsigned& parentSeek, TokenProcessDesc& processDesc) override {
            ea::vector<size_t> tokenValues;
            ea::vector<ea::shared_ptr<TokenBase>> opTokens;

            unsigned currIdx = desc_.idx_;
            ea::shared_ptr<TokenBase> currToken;

            // Evaluate macro values and get all operators
            {
                uint8_t loop = 0;
                while (loop < children_.size()) {
                    ++loop;
                    ++currIdx;

                    currToken = processDesc.tokenList_->at(currIdx);
                    if (currToken->desc_.id_ == sSpacementTokenHash)
                        continue;
                    if (currToken->desc_.id_ == sNewLineTokenHash && tokenValues.size() - 1 == opTokens.size())
                        break;
                    // WRONG Case: #if || or  #if && COMPILE_PS
                    if (currToken->IsOperator() && tokenValues.size() == opTokens.size()) {
                        throw std::runtime_error(
                            Format(
                        "Expected evaluator. ({:d}, {:d})",
                            currToken->desc_.line_,
                            currToken->desc_.col_
                            ).c_str()
                        );
                    }
                    if (currToken->IsOperator()) {
                        opTokens.push_back(currToken);
                    }
                    else {
                        ea::string value = currToken->desc_.value_;
                        // Remove defined() syntax and search for macro value
                        if (value.starts_with("defined(")) {
                            if (!value.ends_with(")")) {
                                throw std::runtime_error(
                                    Format(
                                        "Expected a ')'. ({:d}, {:d})",
                                        currToken->desc_.line_,
                                        currToken->desc_.col_+value.size()
                                    ).c_str()
                                );
                            }

                            value.replace("defined(", "");
                            value.pop_back();

                            // Find macro and add to token values value 1
                            tokenValues.push_back(processDesc.macros_->contains(StringHash(value).ToHash()) ? 1 : 0);
                        }
                    }
                }

            }

            if (tokenValues.size() == 0) {
                throw std::runtime_error(
                    Format("Expected an evaluator. ({}, {})", desc_.line_, desc_.col_).c_str()
                );
            }

            if (tokenValues.size() - 1 != opTokens.size()) {
                throw std::runtime_error(
                    Format("Invalid Syntax at ({}, {})", desc_.line_, desc_.col_).c_str()
                );
            }

            // Evaluate macros with operators
            size_t firstValue = tokenValues[0];
            for (unsigned i = 0; i < opTokens.size(); ++i) {
                firstValue = opTokens[i]->Evaluate(firstValue, tokenValues[i + 1]);
                if (opTokens[i]->desc_.value_ == "&&")
                    break;
            }

            enabled_ = firstValue > 0;
            if (!enabled_)
                return;
            // Start loop at last current token
            for (unsigned i = currToken->desc_.childIdx_ + 1; i < children_.size(); ++i)
                children_[i]->Process(i, processDesc);
        }
    };
    class ElseToken : public MacroToken {
    public:
        ElseToken() : MacroToken() {}
        virtual void Process(unsigned& parentSeek, TokenProcessDesc& processDesc) {
            auto parentList = !parent_.expired() ? parent_.lock()->children_ : *processDesc.tokenRootTree_;

            // Find sibling macro token and check if else token must enable
            ea::shared_ptr<MacroToken> siblingMacroToken;
            int currIdx = desc_.childIdx_;

            while (currIdx > 0) {
                --currIdx;
                if (parentList.at(currIdx)->IsMacro()) {
                    siblingMacroToken = ea::static_pointer_cast<MacroToken>(parentList.at(currIdx));
                    break;
                }
            }

            if (siblingMacroToken->enabled_)
                return;
            TokenBase::Process(parentSeek, processDesc);
        }
        virtual bool EndChild() override {
            return true;
        }
    };
    class ElseIfToken : public MacroToken {
    public:
        ElseIfToken() : MacroToken() {}
    };
    class EndifToken : public TokenBase {
    public:
        EndifToken() : TokenBase() {}
        virtual bool EndChild() override {
            return true;
        }
    };
    class LineToken : public TokenBase {
    public:
        LineToken() : TokenBase() {}
        virtual void Process(unsigned& parentSeek, TokenProcessDesc& processDesc) override {
            // When diligent goes to convert HLSL code to GLSL
            // #line directive does not work, we must remove from code unfortunally.
            ea::shared_ptr<TokenBase> nextToken;

            unsigned currIdx = desc_.idx_;
            uint8_t loop = 0;
            // Find along list next break line token
            while (loop < MAX_SEARCH_LOOP) {
                ++currIdx;
                ++loop;
                nextToken = processDesc.tokenList_->at(currIdx);
                if (nextToken->desc_.id_ == sNewLineTokenHash)
                    break;
            }
            // Change Parent Seek to break line
            parentSeek = nextToken->desc_.childIdx_;
        }
    };
    // @{ Begin Operator Token Classes
    class OperatorToken : public IdentityToken {
    public:
        OperatorToken() : IdentityToken() {}
        virtual bool IsOperator() override { return true; }
    };
    class GreaterThanOpToken : public OperatorToken {
    public:
        GreaterThanOpToken() : OperatorToken(){}
        virtual bool Evaluate(size_t first, size_t second) override {
            return first > second;
        }
    };
    class LessThanOpToken : public OperatorToken {
    public:
        LessThanOpToken() : OperatorToken() {}
        virtual bool Evaluate(size_t first, size_t second) override {
            return first < second;
        }
    };
    class GreaterOrEqualOpToken : public OperatorToken {
    public:
        GreaterOrEqualOpToken() : OperatorToken() {}
        virtual bool Evaluate(size_t first, size_t second) override {
            return first >= second;
        }
    };
    class LessOrEqualOpToken : public OperatorToken {
    public:
        LessOrEqualOpToken() : OperatorToken() {}
        virtual bool Evaluate(size_t first, size_t second) override {
            return first <= second;
        }
    };
    class EqualOpToken : public OperatorToken {
    public:
        EqualOpToken() : OperatorToken () {}
        virtual bool Evaluate(size_t first, size_t second) override {
            return first == second;
        }
    };
    class NotEqualOpToken : public OperatorToken {
    public:
        NotEqualOpToken() : OperatorToken () {}
        virtual bool Evaluate(size_t first, size_t second) override {
            return first != second;
        }
    };
    class AndOpToken : public OperatorToken {
    public:
        AndOpToken() : OperatorToken () {}
        virtual bool Evaluate(size_t first, size_t second) override {
            return first && second;
        }
    };
    class OrOpToken : public OperatorToken {
    public:
        OrOpToken() : OperatorToken () {}
        virtual bool Evaluate(size_t first, size_t second) override {
            return first || second;
        }
    };
    // @} End Operator Token Classes

    typedef ea::shared_ptr<TokenBase>(*TokenCtor)();

    static unsigned sTokenHashes[] = {
       StringHash("#ifdef").ToHash(),
       StringHash("#if").ToHash(),
       StringHash("#else").ToHash(),
       StringHash("#elif").ToHash(),
       StringHash("#endif").ToHash(),
       StringHash("#line").ToHash()
    };
    static TokenCtor sTokenCtors[] = {
        []() -> ea::shared_ptr<TokenBase> { return ea::make_shared<IfDefToken>(); },
        []() -> ea::shared_ptr<TokenBase> { return ea::make_shared<IfToken>(); },
        []() -> ea::shared_ptr<TokenBase> { return ea::make_shared<ElseToken>(); },
        []() -> ea::shared_ptr<TokenBase> { return ea::make_shared<ElseIfToken>(); },
        []() -> ea::shared_ptr<TokenBase> { return ea::make_shared<EndifToken>(); },
        []() -> ea::shared_ptr<TokenBase> { return ea::make_shared<LineToken>(); }
    };

    static unsigned sTokenOpHashes[] = {
        StringHash(">").ToHash(),
        StringHash("<").ToHash(),
        StringHash(">=").ToHash(),
        StringHash("<=").ToHash(),
        StringHash("==").ToHash(),
        StringHash("!=").ToHash(),
        StringHash("&&").ToHash(),
        StringHash("||").ToHash()
    };
    static TokenCtor sTokenOpCtors[] = {
        []() -> ea::shared_ptr<TokenBase> { return ea::make_shared<GreaterThanOpToken>(); },
        []() -> ea::shared_ptr<TokenBase> { return ea::make_shared<LessThanOpToken>(); },
        []() -> ea::shared_ptr<TokenBase> { return ea::make_shared<GreaterOrEqualOpToken>(); },
        []() -> ea::shared_ptr<TokenBase> { return ea::make_shared<LessOrEqualOpToken>(); },
        []() -> ea::shared_ptr<TokenBase> { return ea::make_shared<EqualOpToken>(); },
        []() -> ea::shared_ptr<TokenBase> { return ea::make_shared<NotEqualOpToken>(); },
        []() -> ea::shared_ptr<TokenBase> { return ea::make_shared<AndOpToken>(); },
        []() -> ea::shared_ptr<TokenBase> { return ea::make_shared<OrOpToken>(); },
    };

    static ea::shared_ptr<TokenBase> sBuildToken(const TokenDesc& desc) {
        for (uint8_t i = 0; i < std::size(sTokenHashes); ++i) {
            if (sTokenHashes[i] == desc.id_)
                return sTokenCtors[i]();
        }
        for (uint8_t i = 0; i < std::size(sTokenOpHashes); ++i) {
            if (sTokenOpHashes[i] == desc.id_)
                return sTokenOpCtors[i]();
        }

        return ea::make_shared<IdentityToken>();
    }

    ShaderMacroExpander::ShaderMacroExpander(ShaderMacroExpanderCreationDesc& desc) :
        desc_(desc){
        assert(desc.shaderCode_.size() > 0);
    }

    bool ShaderMacroExpander::Process(ea::string& outputShaderCode) {
        outputShaderCode = "";
        ea::vector<ea::shared_ptr<TokenBase>> tokenList;
        ea::vector<ea::shared_ptr<TokenBase>> tokenRootTree_;
        ea::unordered_map<unsigned, ea::string> macros;

        // Collect Macros to Map
        {
            for (auto macroIt = desc_.macros_.defines_.begin(); macroIt != desc_.macros_.defines_.end(); ++macroIt)
                macros[StringHash(macroIt->first).ToHash()] = macroIt->second;
        }

        // Collect Token Descriptions and Build Token Objects
        {
            TokenDesc currDesc = {};
            size_t currLine = 1;
            size_t currCol = 1;
            unsigned idx = 0;
            do {
                char c = desc_.shaderCode_[idx];
                ++idx;

                if (c == ' ' || c == '\n') {
                    currDesc.id_ = StringHash(currDesc.value_).ToHash();
                    currDesc.col_ = currCol - currDesc.value_.size();
                    currDesc.line_ = currLine;
                    currDesc.idx_ = tokenList.size();

                    ea::shared_ptr<TokenBase> token = sBuildToken(currDesc);
                    token->desc_ = currDesc;


                    tokenList.push_back(token);

                    currDesc.value_ = "";
                    currDesc.value_.push_back(c);
                    currDesc.idx_ = tokenList.size();
                    ++currCol;
                    token = ea::make_shared<IdentityToken>();
                    if (c == '\n') {
                        ++currLine;
                        currDesc.col_ = currCol - currDesc.value_.size();
                        currDesc.id_ = sNewLineTokenHash;
                        currCol = 1;
                    }
                    else {
                        currDesc.col_ = currCol - currDesc.value_.size();
                        currDesc.id_ = sSpacementTokenHash;
                    }

                    // Add New Line or Spacement Token too
                    token->desc_ = currDesc;
                    tokenList.push_back(token);

                    currDesc.value_ = "";
                    continue;
                }
                ++currCol;
                currDesc.value_ += c;
            } while (idx < desc_.shaderCode_.size());
        }

        // Build Macro Tree
        {
            ea::shared_ptr<TokenBase> parent;
            for (auto tokenIt = tokenList.begin(); tokenIt != tokenList.end(); ++tokenIt) {
                ea::shared_ptr<TokenBase> token = *tokenIt;

                if (token->EndChild() && parent) {
                    parent = parent->parent_.lock();
                }

                if (parent) {
                    token->desc_.childIdx_ = parent->children_.size();
                    parent->children_.push_back(token);
                }
                else {
                    token->desc_.childIdx_ = tokenRootTree_.size();
                    tokenRootTree_.push_back(token);
                }

                if (token->BeginChild()) {
                    token->parent_ = parent;
                    parent = token;
                }
            }
        }


#ifdef URHO3D_DEBUG
        // Preview Token Tree
        for (auto tokenIt = tokenRootTree_.begin(); tokenIt != tokenRootTree_.end(); ++tokenIt) {
            (*tokenIt)->PrintDbgTree();
        }
#endif
        // Execute Macro Process
        {
            TokenProcessDesc processDesc = {};
            processDesc.tokenList_ = &tokenList;
            processDesc.sourceCode_ = &desc_.shaderCode_;
            processDesc.outputCode_ = &outputShaderCode;
            processDesc.macros_ = &macros;
            processDesc.tokenRootTree_ = &tokenRootTree_;

            for (unsigned i = 0; i < tokenRootTree_.size(); ++i) {
                ea::shared_ptr<TokenBase> currToken = tokenRootTree_[i];

                try {
                    currToken->Process(i, processDesc);
                }
                catch (std::runtime_error& e) {
                    errorOutput_ = ea::string(e.what());
                    return false;
                }
            }
        }
       
        return true;
    }
}
