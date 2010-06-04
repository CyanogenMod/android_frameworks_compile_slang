#ifndef _SLANG_COMPILER_BACKEND_HPP
#   define _SLANG_COMPILER_BACKEND_HPP

#include "libslang.h"
#include "slang_pragma_recorder.hpp"

#include "llvm/PassManager.h"               /* for class llvm::PassManager and llvm::FunctionPassManager */

#include "llvm/Target/TargetData.h"         /* for class llvm::TargetData */

#include "llvm/Support/StandardPasses.h"    /* for function llvm::createStandardFunctionPasses() and llvm::createStandardModulePasses() */
#include "llvm/Support/FormattedStream.h"   /* for class llvm::formatted_raw_ostream */

#include "clang/AST/ASTConsumer.h"          /* for class clang::ASTConsumer */
#include "clang/CodeGen/CodeGenOptions.h"   /* for class clang::CodeGenOptions */

namespace llvm {

class LLVMContext;
class NamedMDNode;
class raw_ostream;
class Module;

}   /* namespace llvm */

namespace clang {

class ASTConsumer;
class Diagnostic;
class TargetOptions;
class PragmaList;
class CodeGenerator;
class ASTContext;
class DeclGroupRef;
class TagDecl;
class VarDecl;

}   /* namespace clang */

namespace slang {

using namespace clang;

class Backend : public ASTConsumer {
private:
    const CodeGenOptions& mCodeGenOpts;
    const TargetOptions& mTargetOpts;

    /* Output stream */
    llvm::raw_ostream* mpOS;
    SlangCompilerOutputTy mOutputType;

    llvm::TargetData* mpTargetData;

    /* The @Gen here help us to translate AST using in Clang to LLVM IR */
    CodeGenerator* mGen;

    /* Passes */
    llvm::FunctionPassManager* mPerFunctionPasses;  /* passes apply on function scope in a translation unit */
    llvm::PassManager* mPerModulePasses;            /* passes apply on module scope */
    llvm::FunctionPassManager* mCodeGenPasses;      /* passes for code emission */

    llvm::formatted_raw_ostream FormattedOutStream;

    inline void CreateFunctionPasses() {
        if(!mPerFunctionPasses) {
            mPerFunctionPasses = new llvm::FunctionPassManager(mpModule);
            mPerFunctionPasses->add(new llvm::TargetData(*mpTargetData));

            llvm::createStandardFunctionPasses(mPerFunctionPasses, mCodeGenOpts.OptimizationLevel);
        }
        return;
    }

    inline void CreateModulePasses() {
        if(!mPerModulePasses) {
            /* inline passes */
            mPerModulePasses = new llvm::PassManager();
            mPerModulePasses->add(new llvm::TargetData(*mpTargetData));

            llvm::createStandardModulePasses(mPerModulePasses,
                                             mCodeGenOpts.OptimizationLevel,
                                             mCodeGenOpts.OptimizeSize,
                                             mCodeGenOpts.UnitAtATime,
                                             mCodeGenOpts.UnrollLoops,
                                             /* SimplifyLibCalls */true,    /* Some libc functions will be replaced
                                                                             *  by the LLVM built-in optimized function (e.g. strcmp)
                                                                             */
                                             /* HaveExceptions */false,
                                             /* InliningPass */NULL);
        }

        /*
         * llvm::createStandardFunctionPasses and llvm::createStandardModulePasses insert lots of optimization passes for
         *  the code generator. For the conventional desktop PC which memory resources and computation power is relative
         *  large, doing lots optimization as possible is reasonible and feasible. However, on the mobile device or embedded
         *  system, this may cause some problem due to the hardware resources limitation. So they need further refine.
         */
        return;
    }

    bool CreateCodeGenPasses();

protected:
    llvm::Module* mpModule;

    llvm::LLVMContext& mLLVMContext;
    const PragmaList& mPragmas;
    Diagnostic &mDiags;

    /* Extra handler for subclass to handle translation unit before emission */
    virtual void HandleTranslationUnitEx(ASTContext& Ctx) { return; }

public:
    Backend(Diagnostic &Diags,
            const CodeGenOptions& CodeGenOpts,
            const TargetOptions& TargetOpts,
            const PragmaList& Pragmas,
            llvm::raw_ostream* OS,
            SlangCompilerOutputTy OutputType);

    /*
     * Initialize - This is called to initialize the consumer, providing the
     *  ASTContext.
     */
    virtual void Initialize(ASTContext &Ctx);

    /*
     * HandleTopLevelDecl - Handle the specified top-level declaration.  This is
     *  called by the parser to process every top-level Decl*. Note that D can be
     *  the head of a chain of Decls (e.g. for `int a, b` the chain will have two
     *  elements). Use Decl::getNextDeclarator() to walk the chain.
     */
    virtual void HandleTopLevelDecl(DeclGroupRef D);

    /*
     * HandleTranslationUnit - This method is called when the ASTs for entire
     *  translation unit have been parsed.
     */
    virtual void HandleTranslationUnit(ASTContext& Ctx);

    /*
     * HandleTagDeclDefinition - This callback is invoked each time a TagDecl
     *  (e.g. struct, union, enum, class) is completed.  This allows the client to
     *  hack on the type, which can occur at any point in the file (because these
     *  can be defined in declspecs).
     */
    virtual void HandleTagDeclDefinition(TagDecl* D);

    /*
     * CompleteTentativeDefinition - Callback invoked at the end of a translation
     *  unit to notify the consumer that the given tentative definition should be
     *  completed.
     */
    virtual void CompleteTentativeDefinition(VarDecl* D);

    virtual ~Backend();
};

}   /* namespace slang */

#endif  /* _SLANG_COMPILER_BACKEND_HPP */
