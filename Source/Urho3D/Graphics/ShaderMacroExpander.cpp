#include "./ShaderMacroExpander.h"
#include "../Core/Variant.h"
#include "../Container/Hash.h"
#ifdef URHO3D_DEBUG
#include "../IO/Log.h"
#endif
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
        virtual bool BeginChild() { return false; }
        virtual bool EndChild() { return false; }
        virtual bool IsOperator() { return false; }
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
        virtual bool BeginChild() override {
            return true;
        }
    };
    class IfDefToken : public MacroToken {
    public:
        IfDefToken() : MacroToken() {}
        virtual void Process(unsigned& parentSeek, TokenProcessDesc& processDesc) override {
            ea::shared_ptr<TokenBase> nextToken;

            size_t tokenIdx = desc_.idx_;
            uint8_t loop = 0;
            uint8_t maxSearchLoop = 5;
            // Find next valid token.
            while (loop < maxSearchLoop) {
                ++tokenIdx;
                ++maxSearchLoop;
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
            if (macroIt == processDesc.macros_->end())
                return;
            // Start loop after child index.
            for (unsigned i = nextToken->desc_.childIdx_ + 1; i < children_.size(); ++i)
                children_[i]->Process(i, processDesc);
        }
    };
    class IfToken : public MacroToken {
    public:
        IfToken() : MacroToken() {}
    };
    class ElseToken : public MacroToken {
    public:
        ElseToken() : MacroToken() {}
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
            uint8_t maxSearchLoop = 5;
            // Find along list next break line token
            while (loop < maxSearchLoop) {
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
        // Evaluate Token Operation
        virtual bool Evaluate(const ea::unordered_map<unsigned, ea::string>& macros, ea::shared_ptr<TokenBase> leftOp, ea::shared_ptr<TokenBase> rightOp) { return false; }
    };
    class GreaterThanOpToken : public OperatorToken {
    public:
        GreaterThanOpToken() : OperatorToken(){}
    };
    class LessThanOpToken : public OperatorToken {
    public:
        LessThanOpToken() : OperatorToken() {}
    };
    class GreaterOrEqualOpToken : public OperatorToken {
    public:
        GreaterOrEqualOpToken() : OperatorToken() {}
    };
    class LessOrEqualOpToken : public OperatorToken {
    public:
        LessOrEqualOpToken() : OperatorToken() {}
    };
    class EqualOpToken : public OperatorToken {
    public:
        EqualOpToken() : OperatorToken () {}
    };
    class NotEqualOpToken : public OperatorToken {
    public:
        NotEqualOpToken() : OperatorToken () {}
    };
    class AndOpToken : public OperatorToken {
    public:
        AndOpToken() : OperatorToken () {}
    };
    class OrOpToken : public OperatorToken {
    public:
        OrOpToken() : OperatorToken () {}
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
        for (uint8_t i = 0; i < _countof(sTokenHashes); ++i) {
            if (sTokenHashes[i] == desc.id_)
                return sTokenCtors[i]();
        }
        for (uint8_t i = 0; i < _countof(sTokenOpHashes); ++i) {
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
