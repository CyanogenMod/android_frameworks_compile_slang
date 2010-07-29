#include "slang.hpp"
#include "slang_rs_context.hpp"
#include "slang_rs_reflection.hpp"
#include "slang_rs_export_var.hpp"
#include "slang_rs_export_func.hpp"
#include "slang_rs_export_type.hpp"
#include "slang_rs_pragma_handler.hpp"

#include "clang/AST/Type.h"             /* for class clang::QualType */
#include "clang/AST/Decl.h"             /* for class clang::*Decl */
#include "clang/Index/Utils.h"          /* for class clang::idx::ResolveLocationInAST() */
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
    mLLVMContext(llvm::getGlobalContext()),
    mRSExportVarPragma(NULL),
    mRSExportVarAllPragma(NULL),
    mRSExportFuncPragma(NULL),
    mRSExportFuncAllPragma(NULL),
    mRSExportTypePragma(NULL),
    mRSJavaPackageNamePragma(NULL),
    mRSReflectLicensePragma(NULL),
    mExportAllNonStaticVars(false),
    mExportAllNonStaticFuncs(false)
{
    /* For #pragma rs export_var */
    mRSExportVarPragma = RSPragmaHandler::CreatePragmaExportVarHandler(this);
    if(mRSExportVarPragma != NULL)
        PP->AddPragmaHandler("rs", mRSExportVarPragma);

    /* For #pragma rs export_var_all */
    mRSExportVarAllPragma = RSPragmaHandler::CreatePragmaExportVarAllHandler(this);
    if(mRSExportVarAllPragma != NULL)
        PP->AddPragmaHandler("rs", mRSExportVarAllPragma);

    /* For #pragma rs export_func */
    mRSExportFuncPragma = RSPragmaHandler::CreatePragmaExportFuncHandler(this);
    if(mRSExportFuncPragma != NULL)
        PP->AddPragmaHandler("rs", mRSExportFuncPragma);

    /* For #pragma rs export_func_all */
    mRSExportFuncAllPragma = RSPragmaHandler::CreatePragmaExportFuncAllHandler(this);
    if(mRSExportFuncAllPragma != NULL)
        PP->AddPragmaHandler("rs", mRSExportFuncAllPragma);

    /* For #pragma rs export_type */
    mRSExportTypePragma = RSPragmaHandler::CreatePragmaExportTypeHandler(this);
    if(mRSExportTypePragma != NULL)
        PP->AddPragmaHandler("rs", mRSExportTypePragma);

    /* For #pragma rs java_package_name */
    mRSJavaPackageNamePragma = RSPragmaHandler::CreatePragmaJavaPackageNameHandler(this);
    if(mRSJavaPackageNamePragma != NULL)
        PP->AddPragmaHandler("rs", mRSJavaPackageNamePragma);

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

    if(FD->getStorageClass() != FunctionDecl::None) {
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
    bool Done = false;
    RSExportType* ET = NULL;

    RSExportPointerType::IntegerType = mCtx->IntTy.getTypePtr();

    for(DeclContext::lookup_const_iterator I = R.first;
        I != R.second;
        I++)
    {
        NamedDecl* const ND = *I;
        ASTLocation* LastLoc = new ASTLocation(ND);
        ASTLocation AL = ResolveLocationInAST(*mCtx, ND->getLocStart(), LastLoc);

        delete LastLoc;
        if(AL.isDecl()) {
            Decl* D = AL.dyn_AsDecl();
            const Type* T = NULL;

            switch(D->getKind()) {
                case Decl::Typedef:
                    T = static_cast<const TypedefDecl*>(D)->getCanonicalDecl()->getUnderlyingType().getTypePtr();
                break;

                case Decl::Record:
                    T = static_cast<const RecordDecl*>(D)->getTypeForDecl();
                break;

                default:
                    /* unsupported, skip */
                break;
            }

            if(T != NULL)
                ET = RSExportType::Create(this, T);
        }
    }

    RSExportPointerType::IntegerType = NULL;

    return (ET != NULL);
}

void RSContext::processExport() {
    /* Export variable */
    TranslationUnitDecl* TUDecl = mCtx->getTranslationUnitDecl();
    for(DeclContext::decl_iterator DI = TUDecl->decls_begin();
        DI != TUDecl->decls_end();
        DI++)
    {
        if (DI->getKind() == Decl::Var) {
            VarDecl* VD = (VarDecl*) (*DI);
            if (mExportAllNonStaticVars && VD->getLinkage() == ExternalLinkage) {
                if (!processExportVar(VD)) {
                    printf("RSContext::processExport : failed to export var '%s'\n", VD->getNameAsCString());
                }
            } else {
                NeedExportVarSet::iterator EI = mNeedExportVars.find(VD->getName());
                if(EI != mNeedExportVars.end() && processExportVar(VD))
                    mNeedExportVars.erase(EI);
            }
        } else if (DI->getKind() == Decl::Function) {
            FunctionDecl* FD = (FunctionDecl*) (*DI);
            if (mExportAllNonStaticFuncs && FD->getLinkage() == ExternalLinkage) {
                if (!processExportFunc(FD)) {
                    printf("RSContext::processExport : failed to export func '%s'\n", FD->getNameAsCString());
                }
            } else {
                NeedExportFuncSet::iterator EI = mNeedExportFuncs.find(FD->getName());
                if(EI != mNeedExportFuncs.end() && processExportFunc(FD))
                    mNeedExportFuncs.erase(EI);
            }
        }
    }

    /* Finally, export type forcely set to be exported by user */
    for(NeedExportTypeSet::const_iterator EI = mNeedExportTypes.begin();
        EI != mNeedExportTypes.end();
        EI++)
        if(!processExportType(EI->getKey()))
            printf("RSContext::processExport : failed to export type '%s'\n", EI->getKey().str().c_str());


    return;
}

bool RSContext::insertExportType(const llvm::StringRef& TypeName, RSExportType* ET) {
    ExportTypeMap::value_type* NewItem = ExportTypeMap::value_type::Create(TypeName.begin(), TypeName.end(), mExportTypes.getAllocator(), ET);

    if(mExportTypes.insert(NewItem)) {
        return true;
    } else {
        delete NewItem;
        return false;
    }
}

bool RSContext::reflectToJava(const char* OutputPackageName, const std::string& InputFileName, const std::string& OutputBCFileName) {
    if(OutputPackageName == NULL)
        if(!mReflectJavaPackageName.empty())
            OutputPackageName = mReflectJavaPackageName.c_str();
        else
            return true; /* no package name, just return */

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
    delete mRSExportVarPragma;
    delete mRSExportFuncPragma;
    delete mRSExportTypePragma;
    delete mRSJavaPackageNamePragma;
    delete mRSReflectLicensePragma;

    delete mLicenseNote;
    delete mTargetData;
}

}   /* namespace slang */
