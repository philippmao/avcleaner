//
// Created by vladimir on 28.09.19.
//

#include "MatchHandler.h"
#include <random>
#include <regex>
#include <utility>
#include <vector>
#include <string>
#include <sstream>
#include "clang/Lex/Preprocessor.h"
#include "clang/Tooling/Inclusions/HeaderIncludes.h"
#include "clang/Tooling/Inclusions/IncludeStyle.h"
#include "Globals.h"
#include "Utils.h"

using namespace clang;

MatchHandler::MatchHandler(clang::Rewriter *rewriter) {
    this->ASTRewriter = rewriter;
}

std::vector<std::string>
MatchHandler::getNodeParents(const StringLiteral &NodeString, clang::ast_type_traits::DynTypedNode Node,
                             clang::ASTContext *const Context, std::vector<std::string> &CurrentParents,
                             uint64_t Iterations) {

    if (Iterations > Globs::CLIMB_PARENTS_MAX_ITER) {
        return CurrentParents;
    }

    ASTContext::DynTypedNodeList parents = Context->getParents(NodeString);

    if (Iterations > 0) {
        parents = Context->getParents(Node);
    }

    for (const auto &parent : parents) {

        StringRef ParentNodeKind = parent.getNodeKind().asStringRef();

        if (ParentNodeKind.find("Cast") != std::string::npos) {

            return getNodeParents(NodeString, parent, Context, CurrentParents, ++Iterations);
        }

        CurrentParents.push_back(ParentNodeKind);
        return getNodeParents(NodeString, parent, Context, CurrentParents, ++Iterations);
    }

    return CurrentParents;
}

std::vector<clang::ast_type_traits::DynTypedNode>
MatchHandler::getNodeParentsAsNodes(clang::ast_type_traits::DynTypedNode Node,
                             clang::ASTContext *const Context, std::vector<clang::ast_type_traits::DynTypedNode> &CurrentParents,
                             uint64_t Iterations) {

    if (Iterations > Globs::CLIMB_PARENTS_MAX_ITER) {
        return CurrentParents;
    }

    ASTContext::DynTypedNodeList parents = Context->getParents(Node);

    for (const auto &parent : parents) {

        StringRef ParentNodeKind = parent.getNodeKind().asStringRef();

        if (ParentNodeKind.find("Cast") != std::string::npos) {

            return getNodeParentsAsNodes(parent, Context, CurrentParents, ++Iterations);
        }

        CurrentParents.push_back(parent);
        return getNodeParentsAsNodes(parent, Context, CurrentParents, ++Iterations);
    }

    return CurrentParents;
}

std::string
MatchHandler::findStringType(const StringLiteral &NodeString, clang::ASTContext *const pContext) {

    ASTContext::DynTypedNodeList parents = pContext->getParents(NodeString);;
    std::string StringType;
    for (const auto &parent : parents) {

        StringRef ParentNodeKind = parent.getNodeKind().asStringRef();

        if (ParentNodeKind.find("Cast") != std::string::npos) {

            StringType = parent.get<clang::ImplicitCastExpr>()->getType().getAsString();
            llvm::outs() << "StringType is " << StringType  << "\n";
        }

        llvm::outs() << "getParent, Node kind ot^^o: " << parent.getNodeKind().asStringRef() << "\n";
    }
    
    //f** const remove it!
    auto keyword = std::string("const");
    auto pos = StringType.find(keyword);
    if (pos != std::string::npos){
        StringType.erase(pos, keyword.length());
    }

    return StringType;
}

bool
MatchHandler::climbParentsIgnoreCast(const StringLiteral &NodeString, clang::ast_type_traits::DynTypedNode node,
                                     clang::ASTContext *const pContext, uint64_t iterations, std::string StringType) {

    ASTContext::DynTypedNodeList parents = pContext->getParents(NodeString);;

    if (iterations > 0) {
        parents = pContext->getParents(node);
    }

    for (const auto &parent : parents) {

        StringRef ParentNodeKind = parent.getNodeKind().asStringRef();

        if (ParentNodeKind.find("Cast") != std::string::npos) {

            return climbParentsIgnoreCast(NodeString, parent, pContext, ++iterations, StringType);
        }

        handleStringInContext(&NodeString, pContext, parent, StringType);
    }

    return false;
}

void MatchHandler::run(const MatchResult &Result) {
    const auto *Decl = Result.Nodes.getNodeAs<clang::StringLiteral>("decl");
    clang::SourceManager &SM = ASTRewriter->getSourceMgr();

    // skip function declaration in included headers
    if (!SM.isInMainFile(Decl->getBeginLoc()))
        return;

    if (!Decl->getBytes().str().size() > 4) {
        return;
    }

    llvm::outs() << "String Literal detected " << Decl->getBytes() << "\n";

    auto StringType = findStringType(*Decl, Result.Context);

    climbParentsIgnoreCast(*Decl, clang::ast_type_traits::DynTypedNode(), Result.Context, 0, StringType);

}

void MatchHandler::handleStringInContext(const clang::StringLiteral *pLiteral, clang::ASTContext *const pContext,
                                         const clang::ast_type_traits::DynTypedNode node, std::string StringType) {

    StringRef ParentNodeKind = node.getNodeKind().asStringRef();

    llvm::outs() << "Context: " << ParentNodeKind << "\n";

    if (ParentNodeKind.compare("CallExpr") == 0) {
        handleCallExpr(pLiteral, pContext, node, StringType);
    } else if (ParentNodeKind.compare("InitListExpr") == 0) {
        handleInitListExpr(pLiteral, pContext, node, StringType);
    } else if(ParentNodeKind.compare("VarDecl") == 0) {
        handleVarDeclExpr(pLiteral, pContext, node, StringType);
    } else if(ParentNodeKind.compare("CXXConstructExpr") == 0){
        handleCXXConstructExpr(pLiteral, pContext, node, StringType);
    } else {
        llvm::outs() << "Unhandled context " << ParentNodeKind << " for string " << pLiteral->getBytes() << "\n";
    }
}

bool MatchHandler::handleExpr(const clang::StringLiteral *pLiteral, clang::ASTContext *const pContext,
                                  const clang::ast_type_traits::DynTypedNode node, std::string StringType, std::string NewType) {

    clang::SourceRange LiteralRange = clang::SourceRange(
            ASTRewriter->getSourceMgr().getFileLoc(pLiteral->getBeginLoc()),
            ASTRewriter->getSourceMgr().getFileLoc(pLiteral->getEndLoc())
    );

    if(shouldAbort(pLiteral, pContext, LiteralRange))
        return false;

    std::string Replacement = Utils::translateStringToIdentifier(pLiteral->getBytes().str());

    if(!insertVariableDeclaration(pLiteral, pContext, LiteralRange, Replacement, StringType)){
        llvm::outs() << "Failed to insert the variable declaration\n";
        return false ;
    }

    if(!StringType.empty() && !NewType.empty())
        Replacement = "(" + NewType + ")" + Replacement;

    Globs::PatchedSourceLocation.push_back(LiteralRange);

    return replaceStringLiteral(pLiteral, pContext, LiteralRange, Replacement);
}

void MatchHandler::handleCallExpr(const clang::StringLiteral *pLiteral, clang::ASTContext *const pContext,
                                  const clang::ast_type_traits::DynTypedNode node, std::string StringType) {

    // below is an attempt to guess the correct string type
    const auto *FunctionCall = node.get<clang::CallExpr>();
    const FunctionDecl *FnDeclaration = FunctionCall->getDirectCallee();

    //abort if invalid call
    if (FnDeclaration == nullptr)
        return;

    IdentifierInfo *II = FnDeclaration->getIdentifier();

    if (II == nullptr) {
        return;
    }

    llvm::outs() << "Function is " << II->getName() << "\n";
    clang::LangOptions LangOpts;
    LangOpts.CPlusPlus = true;
    auto MacroName = clang::Lexer::getImmediateMacroName(FunctionCall->getSourceRange().getBegin(), pContext->getSourceManager(), LangOpts);

    if(!MacroName.empty() && MacroName.compare(II->getName())){
        llvm::outs() << "Macro detected " << II-> getName();
        //HACK TODO, actually handle these macros
        std::vector<std::string> exclude_macros { "_wassert" }; 
        if(std::find(exclude_macros.begin(), exclude_macros.end(), II->getName()) != exclude_macros.end()){
            // the macro is in the list of macros to be excluded
            llvm::outs() << "Macro is excluded, ignoring the node\n";
            return;
        }
    }

    for(auto i = 0 ; i < FunctionCall->getDirectCallee()->getNumParams() ; i++) {

        auto ArgStart = pContext->getSourceManager().getSpellingColumnNumber(FunctionCall->getArg(i)->getBeginLoc());
        auto StringStart = pContext->getSourceManager().getSpellingColumnNumber(pLiteral->getBeginLoc());

        if(ArgStart == StringStart) {

            auto DeclType = FunctionCall->getDirectCallee()->getParamDecl(i)->getType().getAsString();
            auto Type = FunctionCall->getDirectCallee()->getParamDecl(i)->getType();

            // isConstQualified API returns incorrect result for LPCSTR or LPCWSTR, so the heuristic below is used.
            // WTF is this  !?!?
            
            if(DeclType.find("const") == std::string::npos && DeclType.find("LPC") == std::string::npos) {

                auto keyword = std::string("const");
                auto pos = StringType.find(keyword);
                if (pos != std::string::npos){
                    StringType.erase(pos, keyword.length());
                }
            }
            break;
        }
    };

    if (isBlacklistedFunction(FunctionCall)) {
        return; // TODO: exclude printf-like functions when the replacement is not constant anymore.
    }

    handleExpr(pLiteral, pContext, node, StringType);
}

// TODO : search includes for "common.h" or add it
void MatchHandler::handleInitListExpr(const clang::StringLiteral *pLiteral, clang::ASTContext *const pContext,
                                      const clang::ast_type_traits::DynTypedNode node, std::string StringType) {


    handleExpr(pLiteral, pContext, node, StringType);
}

void MatchHandler::handleVarDeclExpr(const clang::StringLiteral *pLiteral, clang::ASTContext *const pContext,
                                      const clang::ast_type_traits::DynTypedNode node, std::string StringType) {

    auto Identifier = node.get<clang::VarDecl>()->getIdentifier()->getName();
    auto TypeLoc =  node.get<clang::VarDecl>()->getTypeSourceInfo()->getTypeLoc();
    auto Type = TypeLoc.getType().getAsString();
    auto Loc = TypeLoc.getSourceRange();

    std::string NewType;

    if(Type.find("const") != std::string::npos){
        llvm::outs() << "Type " << Type << "\n";
        Type = Type.erase(Type.find("const "), 6);
    }

    llvm::outs() << "Type after removing const" << Type <<"\n";

    if(Type.find(" []") != std::string::npos){
        // if the string was defined as a character array with [] and curly braces, the [] need to be replaced with 
        // the equivalent * notation
        NewType = Type.replace(Type.find(" []"),3," * ");
        std::string LHSReplacement = NewType + Identifier.str(); 
        ASTRewriter->ReplaceText(Loc, LHSReplacement);}
    else{
        NewType = Type+" ";
    }

    llvm::outs() << "Type of " << Identifier << " is " << NewType << "\n";
    
    handleExpr(pLiteral, pContext, node, NewType);
}


void MatchHandler::handleCXXConstructExpr(const clang::StringLiteral *pLiteral, clang::ASTContext *const pContext,
                                      const clang::ast_type_traits::DynTypedNode node, std::string StringType) {

    //need to retrieve the variable decleration
    //if this is a variable declaration of the type std::string = "asdf"; The parent nodes of the string literal migth be as following
    //CXXConstructExpr, CXXBindTemporaryExpr, MaterializeTemporaryExpr, CXXConstructExpr, ExprWithCleanups, VarDecl
    //Currently we simply look for the Variable Declaration such that it shows up before either FunctionDecl, CXXMethodDecl or CXXConstructorDecl

    isStringLiteralInGlobal(pContext, *pLiteral);

    llvm::outs() << "After searching stringliteralglobal\n";

    std::vector<clang::ast_type_traits::DynTypedNode> Parents;
    getNodeParentsAsNodes(node, pContext, Parents, 0);

    llvm::outs() << Parents.size() << "After getting the partents\n";

    const clang::VarDecl *VarDecl;
    clang::ast_type_traits::DynTypedNode newNode;

    for (auto &CurrentParent : Parents) {

        llvm::outs() << "LOOP START \n";

        StringRef ParentNodeKind = CurrentParent.getNodeKind().asStringRef();

        llvm::outs() << "Searching for VarDecl " << ParentNodeKind << "\n";

        if (ParentNodeKind == "FunctionDecl") {
            break;
        }
        llvm::outs() << "WTFFFFF " << ParentNodeKind << "\n";
        if (ParentNodeKind == "CXXMethodDecl") {
            break;
        }
        llvm::outs() << "WTFFFFF " << ParentNodeKind << "\n";
        if (ParentNodeKind == "CXXConstructorDecl") {
            break;
        }
        llvm::outs() << "WTFFFFF " << ParentNodeKind << "\n";
        if (ParentNodeKind == "VarDecl") {
            newNode = CurrentParent;
            VarDecl = CurrentParent.get<clang::VarDecl>();
        }
        llvm::outs() << "WTFFFFF " << ParentNodeKind << "\n";
    }

    if(VarDecl == NULL){
        llvm::outs() << "No Variable decleration for CXXConstructExpr, aborting!";
        return;
    }

    auto Identifier = VarDecl->getIdentifier()->getName();
    auto TypeLoc =  VarDecl->getTypeSourceInfo()->getTypeLoc();
    auto Type = TypeLoc.getType().getAsString();
    auto Loc = TypeLoc.getSourceRange();

    std::string NewType;

    if(Type.find("const") != std::string::npos){
        llvm::outs() << "Type " << Type << "\n";
        Type = Type.erase(Type.find("const "), 6);
    }

    llvm::outs() << "Type after removing const" << Type <<"\n";

    if(Type.find("std::string") != std::string::npos){
        llvm::outs() <<  Type <<"\n";
        Type = Type.replace(Type.find("std::string"), sizeof("std::string")-1, "char");
    }

    if(Type.find("std::wstring") != std::string::npos){
        Type = Type.replace(Type.find("std::wstring"), sizeof("std::wstring")-1, "wchar_t");
    }

    if(Type.find(" []") != std::string::npos){
        // if the string was defined as a character array with [] and curly braces, the [] need to be replaced with 
        // the equivalent * notation
        NewType = Type.replace(Type.find(" []"),3," * ");
        std::string LHSReplacement = NewType + Identifier.str(); 
        ASTRewriter->ReplaceText(Loc, LHSReplacement);}
    else{
        NewType = Type+" ";
    }

    llvm::outs() << "Type of " << Identifier << " is " << NewType << "\n";
    
    handleExpr(pLiteral, pContext, newNode, NewType);
}


bool MatchHandler::insertVariableDeclaration(const clang::StringLiteral *pLiteral, clang::ASTContext *const pContext,
                                             clang::SourceRange range, const std::string& Replacement, std::string StringType) {

    // should only be using the bytes for the encoding TODO
    std::string StringLiteralContent = pLiteral->getBytes().str();

    bool IsInGlobalContext = isStringLiteralInGlobal(pContext, *pLiteral);

    // inject code to declare the string in an encrypted fashion
    SourceRange FreeSpace = findInjectionSpot(pContext, clang::ast_type_traits::DynTypedNode(), *pLiteral,
                                              IsInGlobalContext, 0);

    std::string StringVariableDeclaration = Utils::generateVariableDeclaration(Replacement, StringLiteralContent, StringType);

    if (!IsInGlobalContext) {
        //StringVariableDeclaration += "\tdprintf(\"" + Replacement + "\");\n";
        StringVariableDeclaration.insert(0, 1, '\t');
    }

    StringVariableDeclaration.insert(0, 1, '\n');

    bool InsertResult = ASTRewriter->InsertText(FreeSpace.getBegin(), StringVariableDeclaration);

    if (InsertResult) {
        llvm::errs()<<" Could not finish to patch the string literal, the location to be written to is a Macro.\n";
        Globs::PatchedSourceLocation.push_back(range);
    }

    return !InsertResult;
}

bool MatchHandler::replaceStringLiteral(const clang::StringLiteral *pLiteral, clang::ASTContext *const pContext,
                                        clang::SourceRange LiteralRange,
                                        const std::string& Replacement) {

    // handle "TEXT" macro argument, for instance LoadLibrary(TEXT("ntdll"));
    bool isMacro = ASTRewriter->getSourceMgr().isMacroBodyExpansion(pLiteral->getBeginLoc());

    if (isMacro) {
        StringRef OrigText = clang::Lexer::getSourceText(CharSourceRange(pLiteral->getSourceRange(), true),
                                                         pContext->getSourceManager(), pContext->getLangOpts());

        // weird bug with TEXT Macro / other macros...there must be a proper way to do this.
        if (OrigText.find("TEXT") != std::string::npos) {

            ASTRewriter->RemoveText(LiteralRange);
            LiteralRange.setEnd(ASTRewriter->getSourceMgr().getFileLoc(pLiteral->getEndLoc().getLocWithOffset(-1)));
            //LiteralRange.setEnd(ASTRewriter->getSourceMgr().getFileLoc(pLiteral->getEndLoc()));
        }
        if (OrigText.find("OLESTR") != std::string::npos) {

            ASTRewriter->RemoveText(LiteralRange);
            LiteralRange.setEnd(ASTRewriter->getSourceMgr().getFileLoc(pLiteral->getEndLoc().getLocWithOffset(-1)));
            //LiteralRange.setEnd(ASTRewriter->getSourceMgr().getFileLoc(pLiteral->getEndLoc()));
        }
    }

    return ASTRewriter->ReplaceText(LiteralRange, Replacement);
}

SourceRange
MatchHandler::findInjectionSpot(clang::ASTContext *const Context, clang::ast_type_traits::DynTypedNode Parent,
                                const clang::StringLiteral &Literal, bool IsGlobal, uint64_t Iterations) {

    if (Iterations > Globs::CLIMB_PARENTS_MAX_ITER){
        throw std::runtime_error("Reached max iterations when trying to find a function declaration");
        llvm::outs() << "Reached max iterations when trying to find a function declaration\n";
    }

    ASTContext::DynTypedNodeList parents = Context->getParents(Literal);;

    if (Iterations > 0) {
        parents = Context->getParents(Parent);
    }

    for (const auto &parent : parents) {

        StringRef ParentNodeKind = parent.getNodeKind().asStringRef();

        // There's a bug preventing the rewriting if the first child whose location is to be used, is a macro.

        if (ParentNodeKind.find("FunctionDecl") != std::string::npos) {
            auto FunDecl = parent.get<clang::FunctionDecl>();
            auto *Statement = FunDecl->getBody();
            auto *FirstChild = *Statement->child_begin();
            return {FirstChild->getBeginLoc(), FunDecl->getEndLoc()};

        } else if (ParentNodeKind.find("CXXMethodDecl") != std::string::npos){
            auto FunDecl = parent.get<clang::CXXMethodDecl>();
            auto *Statement = FunDecl->getBody();
            auto *FirstChild = *Statement->child_begin();
            return {FirstChild->getBeginLoc(), FunDecl->getEndLoc()};
        } else if (ParentNodeKind.find("CXXConstructorDecl") != std::string::npos){
            auto FunDecl = parent.get<clang::CXXConstructorDecl>();
            auto *Statement = FunDecl->getBody();
            auto *FirstChild = *Statement->child_begin();
            return {FirstChild->getBeginLoc(), FunDecl->getEndLoc()};
        } else if (ParentNodeKind.find("VarDecl") != std::string::npos) {

            if (IsGlobal) {
                return parent.get<clang::VarDecl>()->getSourceRange();
            }
        }

        return findInjectionSpot(Context, parent, Literal, IsGlobal, ++Iterations);
    }
}

bool MatchHandler::isBlacklistedFunction(const CallExpr *FunctionCall) {

    const FunctionDecl *FnDeclaration = FunctionCall->getDirectCallee();

    //abort if invalid call
    if (FnDeclaration == nullptr)
        return true;

    IdentifierInfo *II = FnDeclaration->getIdentifier();

    if (II == nullptr) {
        return true;
    }

    std::string ApiName = II->getName();

    return ApiName.find("dprintf") != std::string::npos;
}

bool MatchHandler::isStringLiteralInGlobal(clang::ASTContext *const Context, const clang::StringLiteral &Literal) {

    std::vector<std::string> Parents;
    getNodeParents(Literal, clang::ast_type_traits::DynTypedNode(), Context, Parents, 0);

    for (auto &CurrentParent : Parents) {

        llvm::outs() << "Checking if String Literal is Global " << CurrentParent << "\n";
        if (CurrentParent == "FunctionDecl") {
            return false;
        }
        if (CurrentParent == "CXXMethodDecl") {
            return false;
        }
        if (CurrentParent == "CXXConstructorDecl") {
            return false;
        }
    }

    return true;
}

bool
MatchHandler::shouldAbort(const clang::StringLiteral *pLiteral, clang::ASTContext *const pContext, SourceRange string) {

    std::string StringLiteralContent = pLiteral->getBytes().str();

    if (StringLiteralContent.size() < 6) {
        return true;
    }

    auto ShouldSkip = false;

    // does it overlap the source location of an already patched string?
    for(auto &Range : Globs::PatchedSourceLocation)  {

        if(pContext->getSourceManager().isPointWithin(string.getBegin(), Range.getBegin(), Range.getEnd())) {
            ShouldSkip = true;
        }
        else if(pContext->getSourceManager().isPointWithin(string.getEnd(), Range.getBegin(), Range.getEnd())){
            ShouldSkip = true;
        }

        if(ShouldSkip)  {
            llvm::outs() << "Ignoring " << pLiteral->getBytes() << " because it was already patched\n";
            return true;
        }
    }

    return false;
}

