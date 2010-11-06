/*
 * Copyright 2010, The Android Open Source Project
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

#include "slang_rs_object_ref_count.h"

#include "clang/AST/DeclGroup.h"
#include "clang/AST/Expr.h"
#include "clang/AST/OperationKinds.h"
#include "clang/AST/Stmt.h"
#include "clang/AST/StmtVisitor.h"

#include "slang_rs.h"
#include "slang_rs_export_type.h"

using namespace slang;

bool RSObjectRefCount::InitializeRSObject(clang::VarDecl *VD) {
  const clang::Type *T = RSExportType::GetTypeOfDecl(VD);
  RSExportPrimitiveType::DataType DT =
      RSExportPrimitiveType::GetRSSpecificType(T);

  if (DT == RSExportPrimitiveType::DataTypeUnknown)
    return false;

  if (VD->hasInit()) {
    // TODO: Update the reference count of RS object in initializer.
    // This can potentially be done as part of the assignment pass.
  } else {
    clang::Expr *ZeroInitializer =
        CreateZeroInitializerForRSSpecificType(DT,
                                               VD->getASTContext(),
                                               VD->getLocation());

    if (ZeroInitializer) {
      ZeroInitializer->setType(T->getCanonicalTypeInternal());
      VD->setInit(ZeroInitializer);
    }
  }

  return RSExportPrimitiveType::IsRSObjectType(DT);
}

clang::Expr *RSObjectRefCount::CreateZeroInitializerForRSSpecificType(
    RSExportPrimitiveType::DataType DT,
    clang::ASTContext &C,
    const clang::SourceLocation &Loc) {
  clang::Expr *Res = NULL;
  switch (DT) {
    case RSExportPrimitiveType::DataTypeRSElement:
    case RSExportPrimitiveType::DataTypeRSType:
    case RSExportPrimitiveType::DataTypeRSAllocation:
    case RSExportPrimitiveType::DataTypeRSSampler:
    case RSExportPrimitiveType::DataTypeRSScript:
    case RSExportPrimitiveType::DataTypeRSMesh:
    case RSExportPrimitiveType::DataTypeRSProgramFragment:
    case RSExportPrimitiveType::DataTypeRSProgramVertex:
    case RSExportPrimitiveType::DataTypeRSProgramRaster:
    case RSExportPrimitiveType::DataTypeRSProgramStore:
    case RSExportPrimitiveType::DataTypeRSFont: {
      //    (ImplicitCastExpr 'nullptr_t'
      //      (IntegerLiteral 0)))
      llvm::APInt Zero(C.getTypeSize(C.IntTy), 0);
      clang::Expr *Int0 = clang::IntegerLiteral::Create(C, Zero, C.IntTy, Loc);
      clang::Expr *CastToNull =
          clang::ImplicitCastExpr::Create(C,
                                          C.NullPtrTy,
                                          clang::CK_IntegralToPointer,
                                          Int0,
                                          NULL,
                                          clang::VK_RValue);

      Res = new (C) clang::InitListExpr(C, Loc, &CastToNull, 1, Loc);
      break;
    }
    case RSExportPrimitiveType::DataTypeRSMatrix2x2:
    case RSExportPrimitiveType::DataTypeRSMatrix3x3:
    case RSExportPrimitiveType::DataTypeRSMatrix4x4: {
      // RS matrix is not completely an RS object. They hold data by themselves.
      // (InitListExpr rs_matrix2x2
      //   (InitListExpr float[4]
      //     (FloatingLiteral 0)
      //     (FloatingLiteral 0)
      //     (FloatingLiteral 0)
      //     (FloatingLiteral 0)))
      clang::QualType FloatTy = C.FloatTy;
      // Constructor sets value to 0.0f by default
      llvm::APFloat Val(C.getFloatTypeSemantics(FloatTy));
      clang::FloatingLiteral *Float0Val =
          clang::FloatingLiteral::Create(C,
                                         Val,
                                         /* isExact = */true,
                                         FloatTy,
                                         Loc);

      unsigned N = 0;
      if (DT == RSExportPrimitiveType::DataTypeRSMatrix2x2)
        N = 2;
      else if (DT == RSExportPrimitiveType::DataTypeRSMatrix3x3)
        N = 3;
      else if (DT == RSExportPrimitiveType::DataTypeRSMatrix4x4)
        N = 4;

      // Directly allocate 16 elements instead of dynamically allocate N*N
      clang::Expr *InitVals[16];
      for (unsigned i = 0; i < sizeof(InitVals) / sizeof(InitVals[0]); i++)
        InitVals[i] = Float0Val;
      clang::Expr *InitExpr =
          new (C) clang::InitListExpr(C, Loc, InitVals, N * N, Loc);
      InitExpr->setType(C.getConstantArrayType(FloatTy,
                                               llvm::APInt(32, 4),
                                               clang::ArrayType::Normal,
                                               /* EltTypeQuals = */0));

      Res = new (C) clang::InitListExpr(C, Loc, &InitExpr, 1, Loc);
      break;
    }
    case RSExportPrimitiveType::DataTypeUnknown:
    case RSExportPrimitiveType::DataTypeFloat16:
    case RSExportPrimitiveType::DataTypeFloat32:
    case RSExportPrimitiveType::DataTypeFloat64:
    case RSExportPrimitiveType::DataTypeSigned8:
    case RSExportPrimitiveType::DataTypeSigned16:
    case RSExportPrimitiveType::DataTypeSigned32:
    case RSExportPrimitiveType::DataTypeSigned64:
    case RSExportPrimitiveType::DataTypeUnsigned8:
    case RSExportPrimitiveType::DataTypeUnsigned16:
    case RSExportPrimitiveType::DataTypeUnsigned32:
    case RSExportPrimitiveType::DataTypeUnsigned64:
    case RSExportPrimitiveType::DataTypeBoolean:
    case RSExportPrimitiveType::DataTypeUnsigned565:
    case RSExportPrimitiveType::DataTypeUnsigned5551:
    case RSExportPrimitiveType::DataTypeUnsigned4444:
    case RSExportPrimitiveType::DataTypeMax: {
      assert(false && "Not RS object type!");
    }
    // No default case will enable compiler detecting the missing cases
  }

  return Res;
}

void RSObjectRefCount::VisitDeclStmt(clang::DeclStmt *DS) {
  for (clang::DeclStmt::decl_iterator I = DS->decl_begin(), E = DS->decl_end();
       I != E;
       I++) {
    clang::Decl *D = *I;
    if (D->getKind() == clang::Decl::Var) {
      clang::VarDecl *VD = static_cast<clang::VarDecl*>(D);
      if (InitializeRSObject(VD))
        getCurrentScope()->addRSObject(VD);
    }
  }
  return;
}

void RSObjectRefCount::VisitCompoundStmt(clang::CompoundStmt *CS) {
  if (!CS->body_empty()) {
    // Push a new scope
    Scope *S = new Scope(CS);
    mScopeStack.push(S);

    VisitStmt(CS);

    // Destroy the scope
    // TODO: Update reference count of the RS object refenced by
    //       getCurrentScope().
    assert((getCurrentScope() == S) && "Corrupted scope stack!");
    mScopeStack.pop();
    delete S;
  }
  return;
}

void RSObjectRefCount::VisitBinAssign(clang::BinaryOperator *AS) {
  // TODO: Update reference count
  return;
}

void RSObjectRefCount::VisitStmt(clang::Stmt *S) {
  for (clang::Stmt::child_iterator I = S->child_begin(), E = S->child_end();
       I != E;
       I++) {
    if (clang::Stmt *Child = *I) {
      Visit(Child);
    }
  }
  return;
}

