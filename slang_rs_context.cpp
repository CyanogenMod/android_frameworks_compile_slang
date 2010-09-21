#include "slang.hpp"
#include "slang_rs_context.hpp"
#include "slang_rs_reflection.hpp"
#include "slang_rs_export_var.hpp"
#include "slang_rs_export_func.hpp"
#include "slang_rs_export_type.hpp"
#include "slang_rs_pragma_handler.hpp"

#include "clang/AST/Type.h"             /* for class clang::QualType */
#include "clang/AST/Decl.h"             /* for class clang::*Decl */
#include "clang/AST/DeclBase.h"         /* for class clang::Decl and clang::DeclContext */
#include "clang/AST/ASTContext.h"       /* for class clang::ASTContext */
#include "clang/Basic/TargetInfo.h"     /* for class clang::TargetInfo */
#include "clang/Basic/Linkage.h"        /* for class clang::Linkage */
#include "clang/Index/ASTLocation.h"    /* for class clang::idx::ASTLocation */

#include "llvm/LLVMContext.h"           /* for function llvm::getGlobalContext() */
#include "llvm/Target/TargetData.h"     /* for class llvm::TargetData */

using namespace clang::idx;

namespace slang {

RSContext::RSContext(Preprocessor* PP, ASTContext* Ctx, const TargetInfo* Target) :
    mPP(PP),
    mCtx(Ctx),
    mTarget(Target),
    mTargetData(NULL),
    mRSExportVarAllPragma(NULL),
    mRSExportFuncAllPragma(NULL),
    mRSReflectLicensePragma(NULL),
    mExportAllNonStaticVars(false),
    mExportAllNonStaticFuncs(false),
    mLicenseNote(NULL),
    mLLVMContext(llvm::getGlobalContext())
{
    /* For #pragma rs export_var */

    /* For #pragma rs export_var_all */
    mRSExportVarAllPragma = RSPragmaHandler::CreatePragmaExportVarAllHandler(this);
    if(mRSExportVarAllPragma != NULL)
        PP->AddPragmaHandler("rs", mRSExportVarAllPragma);

    /* For #pragma rs export_func_all */
    mRSExportFuncAllPragma = RSPragmaHandler::CreatePragmaExportFuncAllHandler(this);
    if(mRSExportFuncAllPragma != NULL)
        PP->AddPragmaHandler("rs", mRSExportFuncAllPragma);

    PP->AddPragmaHandler("rs", RSPragmaHandler::CreatePragmaExportVarHandler(this));
    PP->AddPragmaHandler("rs", RSPragmaHandler::CreatePragmaExportFuncHandler(this));
    PP->AddPragmaHandler("rs", RSPragmaHandler::CreatePragmaExportTypeHandler(this));
    PP->AddPragmaHandler("rs", RSPragmaHandler::CreatePragmaJavaPackageNameHandler(this));

    /* For #pragma rs set_reflect_license */
    mRSReflectLicensePragma = RSPragmaHandler::RSPragmaHandler::CreatePragmaReflectLicenseHandler(this);
    if (mRSReflectLicensePragma != NULL)
        PP->AddPragmaHandler("rs", mRSReflectLicensePragma);

    /* Prepare target data */
    mTargetData = new llvm::TargetData(Slang::TargetDescription);

    return;
}

bool RSContext::processExportVar(const VarDecl* VD) {
    assert(!VD->getName().empty() && "Variable name should not be empty");

    /* TODO: some check on variable */

    RSExportType* ET = RSExportType::CreateFromDecl(this, VD);
    if(!ET)
        return false;

    RSExportVar* EV = new RSExportVar(this, VD, ET);
    if(EV == NULL)
        return false;
    else
        mExportVars.push_back(EV);

    return true;
}

bool RSContext::processExportFunc(const FunctionDecl* FD) {
    assert(!FD->getName().empty() && "Function name should not be empty");

    if(!FD->isThisDeclarationADefinition())
        return false;

    if(FD->getStorageClass() != clang::SC_None) {
        printf("RSContext::processExportFunc : cannot export extern or static function '%s'\n", FD->getName().str().c_str());
        return false;
    }

    RSExportFunc* EF = RSExportFunc::Create(this, FD);
    if(EF == NULL)
        return false;
    else
        mExportFuncs.push_back(EF);

    return true;
}


bool RSContext::processExportType(const llvm::StringRef& Name) {
    TranslationUnitDecl* TUDecl = mCtx->getTranslationUnitDecl();

    assert(TUDecl != NULL && "Translation unit declaration (top-level declaration) is null object");

    const IdentifierInfo* II = mPP->getIdentifierInfo(Name);
    if(II == NULL)
        /* TODO: error: identifier @Name mark as an exportable type cannot be found */
        return false;

    DeclContext::lookup_const_result R = TUDecl->lookup(II);
    RSExportType* ET = NULL;

    RSExportPointerType::IntegerType = mCtx->IntTy.getTypePtr();

    for(DeclContext::lookup_const_iterator I = R.first;
        I != R.second;
        I++)
    {
        NamedDecl* const ND = *I;
        const Type* T = NULL;

        switch(ND->getKind()) {
            case Decl::Typedef:
                T = static_cast<const TypedefDecl*>(ND)->getCanonicalDecl()->getUnderlyingType().getTypePtr();
            break;

            case Decl::Record:
                T = static_cast<const RecordDecl*>(ND)->getTypeForDecl();
            break;

            default:
                /* unsupported, skip */
           break;
        }

        if(T != NULL)
            ET = RSExportType::Create(this, T);
    }

    RSExportPointerType::IntegerType = NULL;

    return (ET != NULL);
}

void RSContext::processExport() {
    if (mNeedExportVars.empty() && mNeedExportFuncs.empty() && !mExportAllNonStaticVars && !mExportAllNonStaticFuncs) {
        printf("note: No reflection because there are no #pragma rs export_var(...), #pragma rs export_func(...), #pragma rs export_var_all, or #pragma rs export_func_all\n");
        //mExportAllNonStaticVars = true;
        //mExportAllNonStaticFuncs = true;
    }
    TranslationUnitDecl* TUDecl = mCtx->getTranslationUnitDecl();
    for (DeclContext::decl_iterator DI = TUDecl->decls_begin();
        DI != TUDecl->decls_end();
        DI++) {
        if (DI->getKind() == Decl::Var) {
            /* Export variables */
            VarDecl* VD = (VarDecl*) (*DI);
            if (mExportAllNonStaticVars && VD->getLinkage() == ExternalLinkage) {
                if (!processExportVar(VD)) {
                  printf("RSContext::processExport : failed to export var '%s'\n", VD->getNameAsString().c_str());
                }
            } else {
                NeedExportVarSet::iterator EI = mNeedExportVars.find(VD->getName());
                if (EI != mNeedExportVars.end() && processExportVar(VD)) {
                    mNeedExportVars.erase(EI);
                }
            }
        } else if (DI->getKind() == Decl::Function) {
            /* Export functions */
            FunctionDecl* FD = (FunctionDecl*) (*DI);
            if (mExportAllNonStaticFuncs && FD->getLinkage() == ExternalLinkage) {
                if (!processExportFunc(FD)) {
                  printf("RSContext::processExport : failed to export func '%s'\n", FD->getNameAsString().c_str());
                }
            } else {
                NeedExportFuncSet::iterator EI = mNeedExportFuncs.find(FD->getName());
                if (EI != mNeedExportFuncs.end() && processExportFunc(FD))
                    mNeedExportFuncs.erase(EI);
            }
        }
    }

    /* Finally, export type forcely set to be exported by user */
    for (NeedExportTypeSet::const_iterator EI = mNeedExportTypes.begin();
         EI != mNeedExportTypes.end();
         EI++) {
      if (!processExportType(EI->getKey())) {
        printf("RSContext::processExport : failed to export type '%s'\n", EI->getKey().str().c_str());

      }
    }

    return;
}

bool RSContext::insertExportType(const llvm::StringRef& TypeName, RSExportType* ET) {
    ExportTypeMap::value_type* NewItem = ExportTypeMap::value_type::Create(TypeName.begin(), TypeName.end(), mExportTypes.getAllocator(), ET);

    if(mExportTypes.insert(NewItem)) {
        return true;
    } else {
        free(NewItem);
        return false;
    }
}

bool RSContext::reflectToJava(const char* OutputPackageName, const std::string& InputFileName, const std::string& OutputBCFileName, char realPackageName[], int bSize) {
    if (realPackageName != NULL) {
        *realPackageName = 0;
    }

    if(OutputPackageName == NULL) {
        if(!mReflectJavaPackageName.empty()) {
            OutputPackageName = mReflectJavaPackageName.c_str();
        } else {
            return true; /* no package name, just return */
        }
    }

    // Copy back the really applied package name
    if (realPackageName != NULL) {
        strncpy(realPackageName, OutputPackageName, bSize - 1);
    }

    RSReflection* R = new RSReflection(this);
    bool ret = R->reflect(OutputPackageName, InputFileName, OutputBCFileName);
    if(!ret)
        printf("RSContext::reflectToJava : failed to do reflection (%s)\n", R->getLastError());
    delete R;
    return ret;
}

bool RSContext::reflectToJavaPath(const char* OutputPathName) {
    if(OutputPathName == NULL)
        return true; /* no path name, just return */

    setReflectJavaPathName(std::string(OutputPathName));
    return true;
}

RSContext::~RSContext() {
  /*    delete mRSExportVarPragma;  // These are deleted by mPP.reset() that was invoked at the end of Slang::compile(). Doing it again in ~RSContext here will cause seg-faults
    delete mRSExportFuncPragma;
    delete mRSExportTypePragma;
    delete mRSJavaPackageNamePragma;
    delete mRSReflectLicensePragma;
  */
    delete mLicenseNote;
    delete mTargetData;
}

}   /* namespace slang */
