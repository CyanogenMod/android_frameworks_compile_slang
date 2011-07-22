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
#include "clang/AST/TypeLoc.h"

#include "llvm/DerivedTypes.h"
#include "llvm/Target/TargetData.h"

#include "slang_assert.h"
#include "slang_rs_context.h"
#include "slang_rs_export_type.h"

namespace slang {

namespace {

static void ReportNameError(clang::Diagnostic *Diags,
                            const clang::ParmVarDecl *PVD) {
  slangAssert(Diags && PVD);
  const clang::SourceManager &SM = Diags->getSourceManager();

  Diags->Report(clang::FullSourceLoc(PVD->getLocation(), SM),
                Diags->getCustomDiagID(clang::Diagnostic::Error,
                "Duplicate parameter entry (by position/name): '%0'"))
       << PVD->getName();
  return;
}

}  // namespace

// This function takes care of additional validation and construction of
// parameters related to forEach_* reflection.
bool RSExportForEach::validateAndConstructParams(
    RSContext *Context, const clang::FunctionDecl *FD) {
  slangAssert(Context && FD);
  bool valid = true;
  clang::ASTContext &C = Context->getASTContext();
  clang::Diagnostic *Diags = Context->getDiagnostics();

  if (!isRootRSFunc(FD)) {
    slangAssert(false && "must be called on compute root function!");
  }

  numParams = FD->getNumParams();
  slangAssert(numParams > 0);

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
  // TODO(all): Add support for LOD/face when we have them

  for (size_t i = 0; i < numParams; i++) {
    const clang::ParmVarDecl *PVD = FD->getParamDecl(i);
    clang::QualType QT = PVD->getType().getCanonicalType();
    llvm::StringRef ParamName = PVD->getName();

    if (QT->isPointerType()) {
      if (QT->getPointeeType().isConstQualified()) {
        // const T1 *in
        // const T3 *usrData
        if (ParamName.equals("in")) {
          if (mIn) {
            ReportNameError(Diags, PVD);
            valid = false;
          } else {
            mIn = PVD;
          }
        } else if (ParamName.equals("usrData")) {
          if (mUsrData) {
            ReportNameError(Diags, PVD);
            valid = false;
          } else {
            mUsrData = PVD;
          }
        } else {
          // Issue warning about positional parameter usage
          if (!mIn) {
            mIn = PVD;
          } else if (!mUsrData) {
            mUsrData = PVD;
          } else {
            Diags->Report(
                clang::FullSourceLoc(PVD->getLocation(),
                                     Diags->getSourceManager()),
                Diags->getCustomDiagID(clang::Diagnostic::Error,
                                       "Unexpected root() parameter '%0' "
                                       "of type '%1'"))
                << PVD->getName() << PVD->getType().getAsString();
            valid = false;
          }
        }
      } else {
        // T2 *out
        if (ParamName.equals("out")) {
          if (mOut) {
            ReportNameError(Diags, PVD);
            valid = false;
          } else {
            mOut = PVD;
          }
        } else {
          if (!mOut) {
            mOut = PVD;
          } else {
            Diags->Report(
                clang::FullSourceLoc(PVD->getLocation(),
                                     Diags->getSourceManager()),
                Diags->getCustomDiagID(clang::Diagnostic::Error,
                                       "Unexpected root() parameter '%0' "
                                       "of type '%1'"))
                << PVD->getName() << PVD->getType().getAsString();
            valid = false;
          }
        }
      }
    } else if (QT.getUnqualifiedType() == C.UnsignedIntTy) {
      if (ParamName.equals("x")) {
        if (mX) {
          ReportNameError(Diags, PVD);
          valid = false;
        } else {
          mX = PVD;
        }
      } else if (ParamName.equals("y")) {
        if (mY) {
          ReportNameError(Diags, PVD);
          valid = false;
        } else {
          mY = PVD;
        }
      } else if (ParamName.equals("z")) {
        if (mZ) {
          ReportNameError(Diags, PVD);
          valid = false;
        } else {
          mZ = PVD;
        }
      } else if (ParamName.equals("ar")) {
        if (mAr) {
          ReportNameError(Diags, PVD);
          valid = false;
        } else {
          mAr = PVD;
        }
      } else {
        if (!mX) {
          mX = PVD;
        } else if (!mY) {
          mY = PVD;
        } else if (!mZ) {
          mZ = PVD;
        } else if (!mAr) {
          mAr = PVD;
        } else {
          Diags->Report(
              clang::FullSourceLoc(PVD->getLocation(),
                                   Diags->getSourceManager()),
              Diags->getCustomDiagID(clang::Diagnostic::Error,
                                     "Unexpected root() parameter '%0' "
                                     "of type '%1'"))
              << PVD->getName() << PVD->getType().getAsString();
          valid = false;
        }
      }
    } else {
      Diags->Report(
          clang::FullSourceLoc(
              PVD->getTypeSourceInfo()->getTypeLoc().getBeginLoc(),
              Diags->getSourceManager()),
          Diags->getCustomDiagID(clang::Diagnostic::Error,
              "Unexpected root() parameter type '%0'"))
          << PVD->getType().getAsString();
      valid = false;
    }
  }

  if (!mIn && !mOut) {
    Diags->Report(
        clang::FullSourceLoc(FD->getLocation(),
                             Diags->getSourceManager()),
        Diags->getCustomDiagID(clang::Diagnostic::Error,
                               "Compute root() must have at least one "
                               "parameter for in or out"));
    valid = false;
  }

  return valid;
}

RSExportForEach *RSExportForEach::Create(RSContext *Context,
                                         const clang::FunctionDecl *FD) {
  slangAssert(Context && FD);
  llvm::StringRef Name = FD->getName();
  RSExportForEach *FE;

  slangAssert(!Name.empty() && "Function must have a name");

  FE = new RSExportForEach(Context, Name, FD);

  if (!FE->validateAndConstructParams(Context, FD)) {
    return NULL;
  }

  clang::ASTContext &Ctx = Context->getASTContext();

  std::string Id(DUMMY_RS_TYPE_NAME_PREFIX"helper_foreach_param:");
  Id.append(FE->getName()).append(DUMMY_RS_TYPE_NAME_POSTFIX);

  // Extract the usrData parameter (if we have one)
  if (FE->mUsrData) {
    const clang::ParmVarDecl *PVD = FE->mUsrData;
    clang::QualType QT = PVD->getType().getCanonicalType();
    slangAssert(QT->isPointerType() &&
                QT->getPointeeType().isConstQualified());

    const clang::ASTContext &C = Context->getASTContext();
    if (QT->getPointeeType().getCanonicalType().getUnqualifiedType() ==
        C.VoidTy) {
      // In the case of using const void*, we can't reflect an appopriate
      // Java type, so we fall back to just reflecting the ain/aout parameters
      FE->mUsrData = NULL;
    } else {
      clang::RecordDecl *RD =
          clang::RecordDecl::Create(Ctx, clang::TTK_Struct,
                                    Ctx.getTranslationUnitDecl(),
                                    clang::SourceLocation(),
                                    clang::SourceLocation(),
                                    &Ctx.Idents.get(Id));

      llvm::StringRef ParamName = PVD->getName();
      clang::FieldDecl *FD =
          clang::FieldDecl::Create(Ctx,
                                   RD,
                                   clang::SourceLocation(),
                                   clang::SourceLocation(),
                                   PVD->getIdentifier(),
                                   QT->getPointeeType(),
                                   NULL,
                                   /* BitWidth = */ NULL,
                                   /* Mutable = */ false,
                                   /* HasInit = */ false);
      RD->addDecl(FD);
      RD->completeDefinition();

      // Create an export type iff we have a valid usrData type
      clang::QualType T = Ctx.getTagDeclType(RD);
      slangAssert(!T.isNull());

      RSExportType *ET = RSExportType::Create(Context, T.getTypePtr());

      if (ET == NULL) {
        fprintf(stderr, "Failed to export the function %s. There's at least "
                        "one parameter whose type is not supported by the "
                        "reflection\n", FE->getName().c_str());
        return NULL;
      }

      slangAssert((ET->getClass() == RSExportType::ExportClassRecord) &&
                  "Parameter packet must be a record");

      FE->mParamPacketType = static_cast<RSExportRecordType *>(ET);
    }
  }

  if (FE->mIn) {
    const clang::Type *T = FE->mIn->getType().getCanonicalType().getTypePtr();
    FE->mInType = RSExportType::Create(Context, T);
  }

  if (FE->mOut) {
    const clang::Type *T = FE->mOut->getType().getCanonicalType().getTypePtr();
    FE->mOutType = RSExportType::Create(Context, T);
  }

  return FE;
}

bool RSExportForEach::isRSForEachFunc(const clang::FunctionDecl *FD) {
  // We currently support only compute root() being exported via forEach
  if (!isRootRSFunc(FD)) {
    return false;
  }

  if (FD->getNumParams() == 0) {
    // Graphics compute function
    return false;
  }
  return true;
}

bool RSExportForEach::validateSpecialFuncDecl(clang::Diagnostic *Diags,
                                              const clang::FunctionDecl *FD) {
  slangAssert(Diags && FD);
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
      slangAssert(false &&
          "Should not call validateSpecialFuncDecl() on compute root()");
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
