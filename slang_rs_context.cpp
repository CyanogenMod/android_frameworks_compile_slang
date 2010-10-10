#include "slang_rs_context.h"

#include "llvm/LLVMContext.h"
#include "llvm/Target/TargetData.h"

#include "clang/Basic/TargetInfo.h"
#include "clang/Basic/Linkage.h"
#include "clang/AST/Type.h"
#include "clang/AST/Decl.h"
#include "clang/AST/DeclBase.h"
#include "clang/AST/ASTContext.h"
#include "clang/Index/ASTLocation.h"

#include "slang.h"
#include "slang_rs_reflection.h"
#include "slang_rs_exportable.h"
#include "slang_rs_export_var.h"
#include "slang_rs_export_func.h"
#include "slang_rs_export_type.h"
#include "slang_rs_pragma_handler.h"

using namespace slang;

RSContext::RSContext(clang::Preprocessor *PP,
                     clang::ASTContext *Ctx,
                     const clang::TargetInfo *Target)
    : mPP(PP),
      mCtx(Ctx),
      mTarget(Target),
      mTargetData(NULL),
      mLLVMContext(llvm::getGlobalContext()),
      mExportAllNonStaticVars(true),
      mExportAllNonStaticFuncs(false),
      mLicenseNote(NULL) {
  // For #pragma rs export_var
  PP->AddPragmaHandler(
      "rs", RSPragmaHandler::CreatePragmaExportVarHandler(this));

  // For #pragma rs export_var_all
  PP->AddPragmaHandler(
      "rs", RSPragmaHandler::CreatePragmaExportVarAllHandler(this));

  // For #pragma rs export_func
  PP->AddPragmaHandler(
      "rs", RSPragmaHandler::CreatePragmaExportFuncHandler(this));

  // For #pragma rs export_func_all
  PP->AddPragmaHandler(
      "rs", RSPragmaHandler::CreatePragmaExportFuncAllHandler(this));

  // For #pragma rs export_type
  PP->AddPragmaHandler(
      "rs", RSPragmaHandler::CreatePragmaExportTypeHandler(this));

  // For #pragma rs java_package_name
  PP->AddPragmaHandler(
      "rs", RSPragmaHandler::CreatePragmaJavaPackageNameHandler(this));

  // For #pragma rs set_reflect_license
  PP->AddPragmaHandler(
      "rs", RSPragmaHandler::CreatePragmaReflectLicenseHandler(this));

  // Prepare target data
  mTargetData = new llvm::TargetData(Slang::TargetDescription);

  return;
}

bool RSContext::processExportVar(const clang::VarDecl *VD) {
  assert(!VD->getName().empty() && "Variable name should not be empty");

  // TODO(zonr): some check on variable

  RSExportType *ET = RSExportType::CreateFromDecl(this, VD);
  if (!ET)
    return false;

  RSExportVar *EV = new RSExportVar(this, VD, ET);
  if (EV == NULL)
    return false;
  else
    mExportVars.push_back(EV);

  return true;
}

bool RSContext::processExportFunc(const clang::FunctionDecl *FD) {
  assert(!FD->getName().empty() && "Function name should not be empty");

  if (!FD->isThisDeclarationADefinition())
    return false;

  if (FD->getStorageClass() != clang::SC_None) {
    fprintf(stderr, "RSContext::processExportFunc : cannot export extern or "
                    "static function '%s'\n", FD->getName().str().c_str());
    return false;
  }

  RSExportFunc *EF = RSExportFunc::Create(this, FD);
  if (EF == NULL)
    return false;
  else
    mExportFuncs.push_back(EF);

  return true;
}


bool RSContext::processExportType(const llvm::StringRef &Name) {
  clang::TranslationUnitDecl *TUDecl = mCtx->getTranslationUnitDecl();

  assert(TUDecl != NULL && "Translation unit declaration (top-level "
                           "declaration) is null object");

  const clang::IdentifierInfo *II = mPP->getIdentifierInfo(Name);
  if (II == NULL)
    // TODO(zonr): alert identifier @Name mark as an exportable type cannot be
    //             found
    return false;

  clang::DeclContext::lookup_const_result R = TUDecl->lookup(II);
  RSExportType *ET = NULL;

  RSExportPointerType::IntegerType = mCtx->IntTy.getTypePtr();

  for (clang::DeclContext::lookup_const_iterator I = R.first, E = R.second;
       I != E;
       I++) {
    clang::NamedDecl *const ND = *I;
    const clang::Type *T = NULL;

    switch (ND->getKind()) {
      case clang::Decl::Typedef: {
        T = static_cast<const clang::TypedefDecl*>(
            ND)->getCanonicalDecl()->getUnderlyingType().getTypePtr();
        break;
      }
      case clang::Decl::Record: {
        T = static_cast<const clang::RecordDecl*>(ND)->getTypeForDecl();
        break;
      }
      default: {
        // unsupported, skip
        break;
      }
    }

    if (T != NULL)
      ET = RSExportType::Create(this, T);
  }

  RSExportPointerType::IntegerType = NULL;

  return (ET != NULL);
}

void RSContext::processExport() {
  if (mNeedExportVars.empty() &&
      mNeedExportFuncs.empty() &&
      !mExportAllNonStaticVars && !mExportAllNonStaticFuncs) {
    fprintf(stderr, "note: No reflection because there are no #pragma "
                    "rs export_var(...), #pragma rs export_func(...), "
                    "#pragma rs export_var_all, or #pragma rs "
                    "export_func_all\n");
  }

  // Export variable
  clang::TranslationUnitDecl *TUDecl = mCtx->getTranslationUnitDecl();
  for (clang::DeclContext::decl_iterator DI = TUDecl->decls_begin(),
           DE = TUDecl->decls_end();
       DI != TUDecl->decls_end();
       DI++) {
    if (DI->getKind() == clang::Decl::Var) {
      clang::VarDecl *VD = (clang::VarDecl*) (*DI);
      if (mExportAllNonStaticVars &&
          (VD->getLinkage() == clang::ExternalLinkage)) {
        if (!processExportVar(VD)) {
          fprintf(stderr, "RSContext::processExport : failed to export var "
                          "'%s'\n", VD->getNameAsString().c_str());
        }
      } else {
        NeedExportVarSet::iterator EI = mNeedExportVars.find(VD->getName());
        if (EI != mNeedExportVars.end() && processExportVar(VD)) {
          mNeedExportVars.erase(EI);
        }
      }
    } else if (DI->getKind() == clang::Decl::Function) {
      // Export functions
      clang::FunctionDecl *FD = (clang::FunctionDecl*) (*DI);
      if (mExportAllNonStaticFuncs &&
          (FD->getLinkage() == clang::ExternalLinkage)) {
        if (!processExportFunc(FD)) {
          fprintf(stderr, "RSContext::processExport : failed to export func "
                          "'%s'\n", FD->getNameAsString().c_str());
        }
      } else {
        NeedExportFuncSet::iterator EI = mNeedExportFuncs.find(FD->getName());
        if (EI != mNeedExportFuncs.end() && processExportFunc(FD))
          mNeedExportFuncs.erase(EI);
      }
    }
  }

  // Finally, export type forcely set to be exported by user
  for (NeedExportTypeSet::const_iterator EI = mNeedExportTypes.begin(),
           EE = mNeedExportTypes.end();
       EI != EE;
       EI++) {
    if (!processExportType(EI->getKey())) {
      fprintf(stderr, "RSContext::processExport : failed to export type "
              "'%s'\n", EI->getKey().str().c_str());
    }
  }

  return;
}

bool RSContext::insertExportType(const llvm::StringRef &TypeName,
                                 RSExportType *ET) {
  ExportTypeMap::value_type *NewItem =
      ExportTypeMap::value_type::Create(TypeName.begin(),
                                        TypeName.end(),
                                        mExportTypes.getAllocator(),
                                        ET);

  if (mExportTypes.insert(NewItem)) {
    return true;
  } else {
    free(NewItem);
    return false;
  }
}

bool RSContext::reflectToJava(const char *OutputPackageName,
                              const std::string &InputFileName,
                              const std::string &OutputBCFileName,
                              char realPackageName[],
                              int bSize) {
  if (realPackageName != NULL) {
    *realPackageName = 0;
  }

  if (OutputPackageName == NULL) {
    if (!mReflectJavaPackageName.empty()) {
      OutputPackageName = mReflectJavaPackageName.c_str();
    } else {
      // no package name, just return
      return true;
    }
  }

  // Copy back the really applied package name
  if (realPackageName != NULL) {
    strncpy(realPackageName, OutputPackageName, bSize - 1);
  }

  RSReflection *R = new RSReflection(this);
  bool ret = R->reflect(OutputPackageName, InputFileName, OutputBCFileName);
  if (!ret)
    fprintf(stderr, "RSContext::reflectToJava : failed to do reflection "
                    "(%s)\n", R->getLastError());
  delete R;
  return ret;
}

bool RSContext::reflectToJavaPath(const char *OutputPathName) {
  if (OutputPathName == NULL)
    // no path name, just return
    return true;

  setReflectJavaPathName(std::string(OutputPathName));
  return true;
}

RSContext::~RSContext() {
  delete mLicenseNote;
  delete mTargetData;
  for (ExportableList::iterator I = mExportables.begin(),
          E = mExportables.end();
       I != E;
       I++) {
    delete *I;
  }
}
