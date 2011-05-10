/*
 * Copyright 2011, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "slang_rs_export_foreach.h"

#include <string>

#include "clang/AST/ASTContext.h"
#include "clang/AST/Decl.h"

#include "llvm/DerivedTypes.h"
#include "llvm/Target/TargetData.h"

#include "slang_assert.h"
#include "slang_rs_context.h"
#include "slang_rs_export_type.h"

namespace slang {

RSExportForEach *RSExportForEach::Create(RSContext *Context,
                                         const clang::FunctionDecl *FD) {
  llvm::StringRef Name = FD->getName();
  RSExportForEach *F;

  slangAssert(!Name.empty() && "Function must have a name");

  F = new RSExportForEach(Context, Name, FD);

  F->numParams = FD->getNumParams();

  if (F->numParams == 0) {
    slangAssert(false && "Should have at least one parameter for root");
  }

  clang::ASTContext &Ctx = Context->getASTContext();

  std::string Id(DUMMY_RS_TYPE_NAME_PREFIX"helper_foreach_param:");
  Id.append(F->getName()).append(DUMMY_RS_TYPE_NAME_POSTFIX);

  clang::RecordDecl *RD =
      clang::RecordDecl::Create(Ctx, clang::TTK_Struct,
                                Ctx.getTranslationUnitDecl(),
                                clang::SourceLocation(),
                                clang::SourceLocation(),
                                &Ctx.Idents.get(Id));

  // Extract the usrData parameter (if we have one)
  if (F->numParams >= 3) {
    const clang::ParmVarDecl *PVD = FD->getParamDecl(2);
    clang::QualType QT = PVD->getType().getCanonicalType();
    slangAssert(QT->isPointerType() &&
                QT->getPointeeType().isConstQualified());

    const clang::ASTContext &C = Context->getASTContext();
    if (QT->getPointeeType().getCanonicalType().getUnqualifiedType() ==
        C.VoidTy) {
      // In the case of using const void*, we can't reflect an appopriate
      // Java type, so we fall back to just reflecting the ain/aout parameters
      F->numParams = 2;
    } else {
      llvm::StringRef ParamName = PVD->getName();
      clang::FieldDecl *FD =
          clang::FieldDecl::Create(Ctx,
                                   RD,
                                   clang::SourceLocation(),
                                   clang::SourceLocation(),
                                   PVD->getIdentifier(),
                                   QT->getPointeeType(),
                                   NULL,
                                   /* BitWidth = */NULL,
                                   /* Mutable = */false);
      RD->addDecl(FD);
    }
  }
  RD->completeDefinition();

  if (F->numParams >= 3) {
    // Create an export type iff we have a valid usrData type
    clang::QualType T = Ctx.getTagDeclType(RD);
    slangAssert(!T.isNull());

    RSExportType *ET =
      RSExportType::Create(Context, T.getTypePtr());

    if (ET == NULL) {
      fprintf(stderr, "Failed to export the function %s. There's at least one "
                      "parameter whose type is not supported by the "
                      "reflection\n", F->getName().c_str());
      delete F;
      return NULL;
    }

    slangAssert((ET->getClass() == RSExportType::ExportClassRecord) &&
           "Parameter packet must be a record");

    F->mParamPacketType = static_cast<RSExportRecordType *>(ET);
  }

  return F;
}

bool RSExportForEach::isRSForEachFunc(const clang::FunctionDecl *FD) {
  // We currently support only compute root() being exported via forEach
  if (!isRootRSFunc(FD)) {
    return false;
  }

  const clang::ASTContext &C = FD->getASTContext();
  if (FD->getNumParams() == 0 &&
      FD->getResultType().getCanonicalType() == C.IntTy) {
    // Graphics compute function
    return false;
  }
  return true;
}

bool RSExportForEach::validateSpecialFuncDecl(clang::Diagnostic *Diags,
                                              const clang::FunctionDecl *FD) {
  if (!FD) {
    return false;
  }

  bool valid = true;
  const clang::ASTContext &C = FD->getASTContext();

  if (isRootRSFunc(FD)) {
    unsigned int numParams = FD->getNumParams();
    if (numParams == 0) {
      // Graphics root function, so verify that it returns an int
      if (FD->getResultType().getCanonicalType() != C.IntTy) {
        Diags->Report(
            clang::FullSourceLoc(FD->getLocation(), Diags->getSourceManager()),
            Diags->getCustomDiagID(clang::Diagnostic::Error,
                                   "root(void) is required to return "
                                   "an int for graphics usage"));
        valid = false;
      }
    } else {
      // Compute root functions are required to return a void type for now
      if (FD->getResultType().getCanonicalType() != C.VoidTy) {
        Diags->Report(
            clang::FullSourceLoc(FD->getLocation(), Diags->getSourceManager()),
            Diags->getCustomDiagID(clang::Diagnostic::Error,
                                   "compute root() is required to return a "
                                   "void type"));
        valid = false;
      }

      // Validate remaining parameter types
      const clang::ParmVarDecl *tooManyParams = NULL;
      for (unsigned int i = 0; i < numParams; i++) {
        const clang::ParmVarDecl *PVD = FD->getParamDecl(i);
        clang::QualType QT = PVD->getType().getCanonicalType();
        switch (i) {
          case 0:     // const T1 *ain
          case 2: {   // const T3 *usrData
            if (!QT->isPointerType() ||
                !QT->getPointeeType().isConstQualified()) {
              Diags->Report(
                  clang::FullSourceLoc(PVD->getLocation(),
                                       Diags->getSourceManager()),
                  Diags->getCustomDiagID(clang::Diagnostic::Error,
                                         "compute root() parameter must be a "
                                         "const pointer type"));
              valid = false;
            }
            break;
          }
          case 1: {   // T2 *aout
            if (!QT->isPointerType()) {
              Diags->Report(
                  clang::FullSourceLoc(PVD->getLocation(),
                                       Diags->getSourceManager()),
                  Diags->getCustomDiagID(clang::Diagnostic::Error,
                                         "compute root() parameter must be a "
                                         "pointer type"));
              valid = false;
            }
            break;
          }
          case 3:     // unsigned int x
          case 4:     // unsigned int y
          case 5:     // unsigned int z
          case 6: {   // unsigned int ar
            if (QT.getUnqualifiedType() != C.UnsignedIntTy) {
              Diags->Report(
                  clang::FullSourceLoc(PVD->getLocation(),
                                       Diags->getSourceManager()),
                  Diags->getCustomDiagID(clang::Diagnostic::Error,
                                         "compute root() parameter must be a "
                                         "uint32_t type"));
              valid = false;
            }
            break;
          }
          default: {
            if (!tooManyParams) {
              tooManyParams = PVD;
            }
            break;
          }
        }
      }
      if (tooManyParams) {
        Diags->Report(
            clang::FullSourceLoc(tooManyParams->getLocation(),
                                 Diags->getSourceManager()),
            Diags->getCustomDiagID(clang::Diagnostic::Error,
                                   "too many compute root() parameters "
                                   "specified"));
        valid = false;
      }
    }
  } else if (isInitRSFunc(FD)) {
    if (FD->getNumParams() != 0) {
      Diags->Report(
          clang::FullSourceLoc(FD->getLocation(), Diags->getSourceManager()),
          Diags->getCustomDiagID(clang::Diagnostic::Error,
                                 "init(void) is required to have no "
                                 "parameters"));
      valid = false;
    }

    if (FD->getResultType().getCanonicalType() != C.VoidTy) {
      Diags->Report(
          clang::FullSourceLoc(FD->getLocation(), Diags->getSourceManager()),
          Diags->getCustomDiagID(clang::Diagnostic::Error,
                                 "init(void) is required to have a void "
                                 "return type"));
      valid = false;
    }
  } else {
    slangAssert(false && "must be called on init or root function!");
  }

  return valid;
}

}  // namespace slang
