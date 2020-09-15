//
// Created by Vladimir on 20.06.20.
//

#include "ApiMatchHandler.h"

#include <string>
#include <sstream>
#include <algorithm>
#include "clang/Lex/Preprocessor.h"
#include "clang/Tooling/Inclusions/IncludeStyle.h"
#include "Globals.h"
#include "Utils.h"

using namespace clang;

std::string ApiMatchHandler::getFunctionIdentifier(const CallExpr *CallExpression) {

    const FunctionDecl *FnDeclaration = CallExpression->getDirectCallee();

    //abort if invalid call
    if (FnDeclaration == nullptr)
        return nullptr;

    IdentifierInfo *II = FnDeclaration->getIdentifier();

    if (II == nullptr) {
        return nullptr;
    }

    return II->getName();
}

bool ApiMatchHandler::replaceIdentifier(const CallExpr *CallExpression, const std::string &ApiName,
                                        const std::string &NewIdentifier, clang::ASTContext *const pContext) {
    llvm::outs() << "replacing the identifier " << ApiName <<  " with newIdentifier " << NewIdentifier << "\n";

    if(this->ASTRewriter->ReplaceText(CallExpression->getBeginLoc(), ApiName.length(), NewIdentifier)){
        return true;
    }else{
        return false;
    }
}

bool ApiMatchHandler::handleCallExpr(const CallExpr *CallExpression, clang::ASTContext *const pContext) {

    clang::SourceLocation Begin = CallExpression->getBeginLoc();

    std::string Identifier = getFunctionIdentifier(CallExpression);

    //if the function is a macro like MessageBox with a A/W version, the replacement stays the same 
    //and the source range of the current injection point is added to a vector
    //the reason for this is because the function identifier cannot be overwritten in this case

    if(!Begin.isFileID()){
        llvm::outs() << "Begin of Callexpression is not rewritable -> First node might be a macro\n";  
        if (!(Identifier.back() == 'A' || Identifier.back() == 'W')){
            llvm::outs() << "Unwritable function which is not A/W Exiting!\n";
            return false;
        }
        //The Identifier stays the same and the replacement need the A/W removed
        std::string Replacement = Identifier;
        Replacement.pop_back();
        //get the current context
        clang::SourceRange ContextLocation = (findInjectionSpot(pContext, clang::ast_type_traits::DynTypedNode(),
                                                           *CallExpression, 0));
        std::string SourceRangeText = ContextLocation.printToString(ASTRewriter->getSourceMgr());
        //check if macro is already in a list
        if(std::find(MacroAdded.begin(), MacroAdded.end(), SourceRangeText) != MacroAdded.end()) {
            llvm::outs() << "Function definition already in this context!\n";
            return true;
        } else{
            llvm::outs() << "Adding function definition for new function!\n";
            MacroAdded.push_back(SourceRangeText);
            return addGetProcAddress(CallExpression, pContext, Replacement, Identifier);
        }
    }

    std::string Replacement = Utils::translateStringToIdentifier(Identifier);

    if (!addGetProcAddress(CallExpression, pContext, Replacement, Identifier)){
        llvm::outs() << "Failed to insert getProcAdress \n";
        return false;
    }
        
    return replaceIdentifier(CallExpression, Identifier, Replacement, pContext);
}

void ApiMatchHandler::run(const MatchResult &Result) {

    llvm::outs() << "Found " << _ApiName << "\n";

    const auto *CallExpression = Result.Nodes.getNodeAs<clang::CallExpr>("callExpr");
    handleCallExpr(CallExpression, Result.Context);
}


bool ApiMatchHandler::addGetProcAddress(const clang::CallExpr *pCallExpression, clang::ASTContext *const pContext,
                                        const std::string &NewIdentifier, std::string &ApiName) {

    std::stringstream Result;
    
    clang::SourceRange EnclosingFunctionRange = findInjectionSpot(pContext, clang::ast_type_traits::DynTypedNode(),
                                                           *pCallExpression, 0);

    std::string SourceRangeString = EnclosingFunctionRange.printToString(ASTRewriter->getSourceMgr());
        
    // add function prototype if not already added
    if(std::find(TypedefAdded.begin(), TypedefAdded.end(), SourceRangeString) != TypedefAdded.end()) {
        llvm::outs() << "Typedef already defined!\n";
    } else{
        Result << "\t" << _TypeDef << "\n";
        TypedefAdded.push_back(SourceRangeString);
    }

    // add LoadLibrary with obfuscated strings
    std::string LoadLibraryVariable = Utils::translateStringToIdentifier(_Library);
    std::string LoadLibraryString = Utils::generateVariableDeclaration(LoadLibraryVariable, _Library, "TCHAR ");
    std::string LoadLibraryHandleIdentifier = Utils::translateStringToIdentifier("hHandle_"+_Library);
    Result << "\t" << LoadLibraryString << std::endl;
    Result << "\tHMODULE " << LoadLibraryHandleIdentifier << " = LoadLibrary(" << LoadLibraryVariable << ");\n";

    // add GetProcAddress with obfuscated string: TypeDef NewIdentifier = (TypeDef) GetProcAddress(handleIdentifier, ApiName)
    std::string ApiNameIdentifier = Utils::translateStringToIdentifier(ApiName);
    std::string getprocchar = "char ";
    std::string ApiNameDecl = Utils::generateVariableDeclaration(ApiNameIdentifier, ApiName, getprocchar);
    Result << "\t" << ApiNameDecl << "\n";
    Result << "\t_"<< ApiName << " " << NewIdentifier << " = (_" << ApiName << ") GetProcAddress("
           << LoadLibraryHandleIdentifier << ", " << ApiNameIdentifier << ");\n";

    // add everything at the beginning of the function.
    return !(ASTRewriter->InsertText(EnclosingFunctionRange.getBegin(), Result.str()));
}

SourceRange
ApiMatchHandler::findInjectionSpot(clang::ASTContext *const Context, clang::ast_type_traits::DynTypedNode Parent,
                                   const clang::CallExpr &Literal, uint64_t Iterations) {

    if (Iterations > Globs::CLIMB_PARENTS_MAX_ITER)
        throw std::runtime_error("Reached max iterations when trying to find a function declaration");

    ASTContext::DynTypedNodeList parents = Context->getParents(Literal);

    if (Iterations > 0) {
        parents = Context->getParents(Parent);
    }

    for (const auto &parent : parents) {

        StringRef ParentNodeKind = parent.getNodeKind().asStringRef();

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
        }
        return findInjectionSpot(Context, parent, Literal, ++Iterations);
    }
}