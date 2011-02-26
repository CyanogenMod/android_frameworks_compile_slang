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

#include <list>

#include "clang/AST/DeclGroup.h"
#include "clang/AST/Expr.h"
#include "clang/AST/OperationKinds.h"
#include "clang/AST/Stmt.h"
#include "clang/AST/StmtVisitor.h"

#include "slang_assert.h"
#include "slang_rs.h"
#include "slang_rs_export_type.h"

namespace slang {

clang::FunctionDecl *RSObjectRefCount::
    RSSetObjectFD[RSExportPrimitiveType::LastRSObjectType -
                  RSExportPrimitiveType::FirstRSObjectType + 1];
clang::FunctionDecl *RSObjectRefCount::
    RSClearObjectFD[RSExportPrimitiveType::LastRSObjectType -
                    RSExportPrimitiveType::FirstRSObjectType + 1];

void RSObjectRefCount::GetRSRefCountingFunctions(clang::ASTContext &C) {
  for (unsigned i = 0;
       i < (sizeof(RSClearObjectFD) / sizeof(clang::FunctionDecl*));
       i++) {
    RSSetObjectFD[i] = NULL;
    RSClearObjectFD[i] = NULL;
  }

  clang::TranslationUnitDecl *TUDecl = C.getTranslationUnitDecl();

  for (clang::DeclContext::decl_iterator I = TUDecl->decls_begin(),
          E = TUDecl->decls_end(); I != E; I++) {
    if ((I->getKind() >= clang::Decl::firstFunction) &&
        (I->getKind() <= clang::Decl::lastFunction)) {
      clang::FunctionDecl *FD = static_cast<clang::FunctionDecl*>(*I);

      // points to RSSetObjectFD or RSClearObjectFD
      clang::FunctionDecl **RSObjectFD;

      if (FD->getName() == "rsSetObject") {
        slangAssert((FD->getNumParams() == 2) &&
                    "Invalid rsSetObject function prototype (# params)");
        RSObjectFD = RSSetObjectFD;
      } else if (FD->getName() == "rsClearObject") {
        slangAssert((FD->getNumParams() == 1) &&
                    "Invalid rsClearObject function prototype (# params)");
        RSObjectFD = RSClearObjectFD;
      } else {
        continue;
      }

      const clang::ParmVarDecl *PVD = FD->getParamDecl(0);
      clang::QualType PVT = PVD->getOriginalType();
      // The first parameter must be a pointer like rs_allocation*
      slangAssert(PVT->isPointerType() &&
          "Invalid rs{Set,Clear}Object function prototype (pointer param)");

      // The rs object type passed to the FD
      clang::QualType RST = PVT->getPointeeType();
      RSExportPrimitiveType::DataType DT =
          RSExportPrimitiveType::GetRSSpecificType(RST.getTypePtr());
      slangAssert(RSExportPrimitiveType::IsRSObjectType(DT)
             && "must be RS object type");

      RSObjectFD[(DT - RSExportPrimitiveType::FirstRSObjectType)] = FD;
    }
  }
}

namespace {

static void AppendToCompoundStatement(clang::ASTContext& C,
                                      clang::CompoundStmt *CS,
                                      std::list<clang::Stmt*> &StmtList,
                                      bool InsertAtEndOfBlock) {
  // Destructor code will be inserted before any return statement.
  // Any subsequent statements in the compound statement are then placed
  // after our new code.
  // TODO(srhines): This should also handle the case of goto/break/continue.

  clang::CompoundStmt::body_iterator bI = CS->body_begin();

  unsigned OldStmtCount = 0;
  for (bI = CS->body_begin(); bI != CS->body_end(); bI++) {
    OldStmtCount++;
  }

  unsigned NewStmtCount = StmtList.size();

  clang::Stmt **UpdatedStmtList;
  UpdatedStmtList = new clang::Stmt*[OldStmtCount+NewStmtCount];

  unsigned UpdatedStmtCount = 0;
  bool FoundReturn = false;
  for (bI = CS->body_begin(); bI != CS->body_end(); bI++) {
    if ((*bI)->getStmtClass() == clang::Stmt::ReturnStmtClass) {
      FoundReturn = true;
      break;
    }
    UpdatedStmtList[UpdatedStmtCount++] = *bI;
  }

  // Always insert before a return that we found, or if we are told
  // to insert at the end of the block
  if (FoundReturn || InsertAtEndOfBlock) {
    std::list<clang::Stmt*>::const_iterator I = StmtList.begin();
    for (std::list<clang::Stmt*>::const_iterator I = StmtList.begin();
         I != StmtList.end();
         I++) {
      UpdatedStmtList[UpdatedStmtCount++] = *I;
    }
  }

  // Pick up anything left over after a return statement
  for ( ; bI != CS->body_end(); bI++) {
    UpdatedStmtList[UpdatedStmtCount++] = *bI;
  }

  CS->setStmts(C, UpdatedStmtList, UpdatedStmtCount);

  delete [] UpdatedStmtList;

  return;
}

static void AppendAfterStmt(clang::ASTContext& C,
                            clang::CompoundStmt *CS,
                            clang::Stmt *OldStmt,
                            clang::Stmt *NewStmt) {
  slangAssert(CS && OldStmt && NewStmt);
  clang::CompoundStmt::body_iterator bI = CS->body_begin();
  unsigned StmtCount = 1;  // Take into account new statement
  for (bI = CS->body_begin(); bI != CS->body_end(); bI++) {
    StmtCount++;
  }

  clang::Stmt **UpdatedStmtList = new clang::Stmt*[StmtCount];

  unsigned UpdatedStmtCount = 0;
  unsigned Once = 0;
  for (bI = CS->body_begin(); bI != CS->body_end(); bI++) {
    UpdatedStmtList[UpdatedStmtCount++] = *bI;
    if (*bI == OldStmt) {
      Once++;
      slangAssert(Once == 1);
      UpdatedStmtList[UpdatedStmtCount++] = NewStmt;
    }
  }

  CS->setStmts(C, UpdatedStmtList, UpdatedStmtCount);

  delete [] UpdatedStmtList;

  return;
}

static void ReplaceInCompoundStmt(clang::ASTContext& C,
                                  clang::CompoundStmt *CS,
                                  clang::Stmt* OldStmt,
                                  clang::Stmt* NewStmt) {
  clang::CompoundStmt::body_iterator bI = CS->body_begin();

  unsigned StmtCount = 0;
  for (bI = CS->body_begin(); bI != CS->body_end(); bI++) {
    StmtCount++;
  }

  clang::Stmt **UpdatedStmtList = new clang::Stmt*[StmtCount];

  unsigned UpdatedStmtCount = 0;
  for (bI = CS->body_begin(); bI != CS->body_end(); bI++) {
    if (*bI == OldStmt) {
      UpdatedStmtList[UpdatedStmtCount++] = NewStmt;
    } else {
      UpdatedStmtList[UpdatedStmtCount++] = *bI;
    }
  }

  CS->setStmts(C, UpdatedStmtList, UpdatedStmtCount);

  delete [] UpdatedStmtList;

  return;
}


// This class visits a compound statement and inserts the StmtList containing
// destructors in proper locations. This includes inserting them before any
// return statement in any sub-block, at the end of the logical enclosing
// scope (compound statement), and/or before any break/continue statement that
// would resume outside the declared scope. We will not handle the case for
// goto statements that leave a local scope.
// TODO(srhines): Make this work properly for break/continue.
class DestructorVisitor : public clang::StmtVisitor<DestructorVisitor> {
 private:
  clang::ASTContext &mC;
  std::list<clang::Stmt*> &mStmtList;
  bool mTopLevel;
 public:
  DestructorVisitor(clang::ASTContext &C, std::list<clang::Stmt*> &StmtList);
  void VisitStmt(clang::Stmt *S);
  void VisitCompoundStmt(clang::CompoundStmt *CS);
};

DestructorVisitor::DestructorVisitor(clang::ASTContext &C,
                                     std::list<clang::Stmt*> &StmtList)
  : mC(C),
    mStmtList(StmtList),
    mTopLevel(true) {
  return;
}

void DestructorVisitor::VisitCompoundStmt(clang::CompoundStmt *CS) {
  if (!CS->body_empty()) {
    AppendToCompoundStatement(mC, CS, mStmtList, mTopLevel);
    mTopLevel = false;
    VisitStmt(CS);
  }
  return;
}

void DestructorVisitor::VisitStmt(clang::Stmt *S) {
  for (clang::Stmt::child_iterator I = S->child_begin(), E = S->child_end();
       I != E;
       I++) {
    if (clang::Stmt *Child = *I) {
      Visit(Child);
    }
  }
  return;
}

clang::Expr *ClearSingleRSObject(clang::ASTContext &C,
                                 clang::Expr *RefRSVar,
                                 clang::SourceLocation Loc) {
  slangAssert(RefRSVar);
  const clang::Type *T = RefRSVar->getType().getTypePtr();
  slangAssert(!T->isArrayType() &&
              "Should not be destroying arrays with this function");

  clang::FunctionDecl *ClearObjectFD = RSObjectRefCount::GetRSClearObjectFD(T);
  slangAssert((ClearObjectFD != NULL) &&
              "rsClearObject doesn't cover all RS object types");

  clang::QualType ClearObjectFDType = ClearObjectFD->getType();
  clang::QualType ClearObjectFDArgType =
      ClearObjectFD->getParamDecl(0)->getOriginalType();

  // Example destructor for "rs_font localFont;"
  //
  // (CallExpr 'void'
  //   (ImplicitCastExpr 'void (*)(rs_font *)' <FunctionToPointerDecay>
  //     (DeclRefExpr 'void (rs_font *)' FunctionDecl='rsClearObject'))
  //   (UnaryOperator 'rs_font *' prefix '&'
  //     (DeclRefExpr 'rs_font':'rs_font' Var='localFont')))

  // Get address of targeted RS object
  clang::Expr *AddrRefRSVar =
      new(C) clang::UnaryOperator(RefRSVar,
                                  clang::UO_AddrOf,
                                  ClearObjectFDArgType,
                                  Loc);

  clang::Expr *RefRSClearObjectFD =
      clang::DeclRefExpr::Create(C,
                                 NULL,
                                 ClearObjectFD->getQualifierRange(),
                                 ClearObjectFD,
                                 ClearObjectFD->getLocation(),
                                 ClearObjectFDType);

  clang::Expr *RSClearObjectFP =
      clang::ImplicitCastExpr::Create(C,
                                      C.getPointerType(ClearObjectFDType),
                                      clang::CK_FunctionToPointerDecay,
                                      RefRSClearObjectFD,
                                      NULL,
                                      clang::VK_RValue);

  clang::CallExpr *RSClearObjectCall =
      new(C) clang::CallExpr(C,
                             RSClearObjectFP,
                             &AddrRefRSVar,
                             1,
                             ClearObjectFD->getCallResultType(),
                             Loc);

  return RSClearObjectCall;
}

static int ArrayDim(const clang::Type *T) {
  if (!T || !T->isArrayType()) {
    return 0;
  }

  const clang::ConstantArrayType *CAT =
    static_cast<const clang::ConstantArrayType *>(T);
  return static_cast<int>(CAT->getSize().getSExtValue());
}

static clang::Stmt *ClearStructRSObject(
    clang::ASTContext &C,
    clang::DeclContext *DC,
    clang::Expr *RefRSStruct,
    clang::SourceRange Range,
    clang::SourceLocation Loc);

static clang::Stmt *ClearArrayRSObject(
    clang::ASTContext &C,
    clang::DeclContext *DC,
    clang::Expr *RefRSArr,
    clang::SourceRange Range,
    clang::SourceLocation Loc) {
  const clang::Type *BaseType = RefRSArr->getType().getTypePtr();
  slangAssert(BaseType->isArrayType());

  int NumArrayElements = ArrayDim(BaseType);
  // Actually extract out the base RS object type for use later
  BaseType = BaseType->getArrayElementTypeNoTypeQual();

  clang::Stmt *StmtArray[2] = {NULL};
  int StmtCtr = 0;

  if (NumArrayElements <= 0) {
    return NULL;
  }

  // Example destructor loop for "rs_font fontArr[10];"
  //
  // (CompoundStmt
  //   (DeclStmt "int rsIntIter")
  //   (ForStmt
  //     (BinaryOperator 'int' '='
  //       (DeclRefExpr 'int' Var='rsIntIter')
  //       (IntegerLiteral 'int' 0))
  //     (BinaryOperator 'int' '<'
  //       (DeclRefExpr 'int' Var='rsIntIter')
  //       (IntegerLiteral 'int' 10)
  //     NULL << CondVar >>
  //     (UnaryOperator 'int' postfix '++'
  //       (DeclRefExpr 'int' Var='rsIntIter'))
  //     (CallExpr 'void'
  //       (ImplicitCastExpr 'void (*)(rs_font *)' <FunctionToPointerDecay>
  //         (DeclRefExpr 'void (rs_font *)' FunctionDecl='rsClearObject'))
  //       (UnaryOperator 'rs_font *' prefix '&'
  //         (ArraySubscriptExpr 'rs_font':'rs_font'
  //           (ImplicitCastExpr 'rs_font *' <ArrayToPointerDecay>
  //             (DeclRefExpr 'rs_font [10]' Var='fontArr'))
  //           (DeclRefExpr 'int' Var='rsIntIter')))))))

  // Create helper variable for iterating through elements
  clang::IdentifierInfo& II = C.Idents.get("rsIntIter");
  clang::VarDecl *IIVD =
      clang::VarDecl::Create(C,
                             DC,
                             Loc,
                             &II,
                             C.IntTy,
                             C.getTrivialTypeSourceInfo(C.IntTy),
                             clang::SC_None,
                             clang::SC_None);
  clang::Decl *IID = (clang::Decl *)IIVD;

  clang::DeclGroupRef DGR = clang::DeclGroupRef::Create(C, &IID, 1);
  StmtArray[StmtCtr++] = new(C) clang::DeclStmt(DGR, Loc, Loc);

  // Form the actual destructor loop
  // for (Init; Cond; Inc)
  //   RSClearObjectCall;

  // Init -> "rsIntIter = 0"
  clang::DeclRefExpr *RefrsIntIter =
      clang::DeclRefExpr::Create(C,
                                 NULL,
                                 Range,
                                 IIVD,
                                 Loc,
                                 C.IntTy);

  clang::Expr *Int0 = clang::IntegerLiteral::Create(C,
      llvm::APInt(C.getTypeSize(C.IntTy), 0), C.IntTy, Loc);

  clang::BinaryOperator *Init =
      new(C) clang::BinaryOperator(RefrsIntIter,
                                   Int0,
                                   clang::BO_Assign,
                                   C.IntTy,
                                   Loc);

  // Cond -> "rsIntIter < NumArrayElements"
  clang::Expr *NumArrayElementsExpr = clang::IntegerLiteral::Create(C,
      llvm::APInt(C.getTypeSize(C.IntTy), NumArrayElements), C.IntTy, Loc);

  clang::BinaryOperator *Cond =
      new(C) clang::BinaryOperator(RefrsIntIter,
                                   NumArrayElementsExpr,
                                   clang::BO_LT,
                                   C.IntTy,
                                   Loc);

  // Inc -> "rsIntIter++"
  clang::UnaryOperator *Inc =
      new(C) clang::UnaryOperator(RefrsIntIter,
                                  clang::UO_PostInc,
                                  C.IntTy,
                                  Loc);

  // Body -> "rsClearObject(&VD[rsIntIter]);"
  // Destructor loop operates on individual array elements

  clang::Expr *RefRSArrPtr =
      clang::ImplicitCastExpr::Create(C,
          C.getPointerType(BaseType->getCanonicalTypeInternal()),
          clang::CK_ArrayToPointerDecay,
          RefRSArr,
          NULL,
          clang::VK_RValue);

  clang::Expr *RefRSArrPtrSubscript =
      new(C) clang::ArraySubscriptExpr(RefRSArrPtr,
                                       RefrsIntIter,
                                       BaseType->getCanonicalTypeInternal(),
                                       Loc);

  RSExportPrimitiveType::DataType DT =
      RSExportPrimitiveType::GetRSSpecificType(BaseType);

  clang::Stmt *RSClearObjectCall = NULL;
  if (BaseType->isArrayType()) {
    RSClearObjectCall =
        ClearArrayRSObject(C, DC, RefRSArrPtrSubscript, Range, Loc);
  } else if (DT == RSExportPrimitiveType::DataTypeUnknown) {
    RSClearObjectCall =
        ClearStructRSObject(C, DC, RefRSArrPtrSubscript, Range, Loc);
  } else {
    RSClearObjectCall = ClearSingleRSObject(C, RefRSArrPtrSubscript, Loc);
  }

  clang::ForStmt *DestructorLoop =
      new(C) clang::ForStmt(C,
                            Init,
                            Cond,
                            NULL,  // no condVar
                            Inc,
                            RSClearObjectCall,
                            Loc,
                            Loc,
                            Loc);

  StmtArray[StmtCtr++] = DestructorLoop;
  slangAssert(StmtCtr == 2);

  clang::CompoundStmt *CS =
      new(C) clang::CompoundStmt(C, StmtArray, StmtCtr, Loc, Loc);

  return CS;
}

static unsigned CountRSObjectTypes(const clang::Type *T) {
  slangAssert(T);
  unsigned RSObjectCount = 0;

  if (T->isArrayType()) {
    return CountRSObjectTypes(T->getArrayElementTypeNoTypeQual());
  }

  RSExportPrimitiveType::DataType DT =
      RSExportPrimitiveType::GetRSSpecificType(T);
  if (DT != RSExportPrimitiveType::DataTypeUnknown) {
    return (RSExportPrimitiveType::IsRSObjectType(DT) ? 1 : 0);
  }

  if (!T->isStructureType()) {
    return 0;
  }

  clang::RecordDecl *RD = T->getAsStructureType()->getDecl();
  RD = RD->getDefinition();
  for (clang::RecordDecl::field_iterator FI = RD->field_begin(),
         FE = RD->field_end();
       FI != FE;
       FI++) {
    const clang::FieldDecl *FD = *FI;
    const clang::Type *FT = RSExportType::GetTypeOfDecl(FD);
    if (CountRSObjectTypes(FT)) {
      // Sub-structs should only count once (as should arrays, etc.)
      RSObjectCount++;
    }
  }

  return RSObjectCount;
}

static clang::Stmt *ClearStructRSObject(
    clang::ASTContext &C,
    clang::DeclContext *DC,
    clang::Expr *RefRSStruct,
    clang::SourceRange Range,
    clang::SourceLocation Loc) {
  const clang::Type *BaseType = RefRSStruct->getType().getTypePtr();

  slangAssert(!BaseType->isArrayType());

  RSExportPrimitiveType::DataType DT =
      RSExportPrimitiveType::GetRSSpecificType(BaseType);

  // Structs should show up as unknown primitive types
  slangAssert(DT == RSExportPrimitiveType::DataTypeUnknown);

  unsigned FieldsToDestroy = CountRSObjectTypes(BaseType);

  unsigned StmtCount = 0;
  clang::Stmt **StmtArray = new clang::Stmt*[FieldsToDestroy];
  for (unsigned i = 0; i < FieldsToDestroy; i++) {
    StmtArray[i] = NULL;
  }

  // Populate StmtArray by creating a destructor for each RS object field
  clang::RecordDecl *RD = BaseType->getAsStructureType()->getDecl();
  RD = RD->getDefinition();
  for (clang::RecordDecl::field_iterator FI = RD->field_begin(),
         FE = RD->field_end();
       FI != FE;
       FI++) {
    // We just look through all field declarations to see if we find a
    // declaration for an RS object type (or an array of one).
    bool IsArrayType = false;
    clang::FieldDecl *FD = *FI;
    const clang::Type *FT = RSExportType::GetTypeOfDecl(FD);
    const clang::Type *OrigType = FT;
    while (FT && FT->isArrayType()) {
      FT = FT->getArrayElementTypeNoTypeQual();
      IsArrayType = true;
    }

    if (RSExportPrimitiveType::IsRSObjectType(FT)) {
      clang::DeclAccessPair FoundDecl =
          clang::DeclAccessPair::make(FD, clang::AS_none);
      clang::MemberExpr *RSObjectMember =
          clang::MemberExpr::Create(C,
                                    RefRSStruct,
                                    false,
                                    NULL,
                                    Range,
                                    FD,
                                    FoundDecl,
                                    clang::DeclarationNameInfo(),
                                    NULL,
                                    OrigType->getCanonicalTypeInternal());

      slangAssert(StmtCount < FieldsToDestroy);

      if (IsArrayType) {
        StmtArray[StmtCount++] = ClearArrayRSObject(C,
                                                    DC,
                                                    RSObjectMember,
                                                    Range,
                                                    Loc);
      } else {
        StmtArray[StmtCount++] = ClearSingleRSObject(C,
                                                     RSObjectMember,
                                                     Loc);
      }
    } else if (FT->isStructureType() && CountRSObjectTypes(FT)) {
      // In this case, we have a nested struct. We may not end up filling all
      // of the spaces in StmtArray (sub-structs should handle themselves
      // with separate compound statements).
      clang::DeclAccessPair FoundDecl =
          clang::DeclAccessPair::make(FD, clang::AS_none);
      clang::MemberExpr *RSObjectMember =
          clang::MemberExpr::Create(C,
                                    RefRSStruct,
                                    false,
                                    NULL,
                                    Range,
                                    FD,
                                    FoundDecl,
                                    clang::DeclarationNameInfo(),
                                    NULL,
                                    OrigType->getCanonicalTypeInternal());

      if (IsArrayType) {
        StmtArray[StmtCount++] = ClearArrayRSObject(C,
                                                    DC,
                                                    RSObjectMember,
                                                    Range,
                                                    Loc);
      } else {
        StmtArray[StmtCount++] = ClearStructRSObject(C,
                                                     DC,
                                                     RSObjectMember,
                                                     Range,
                                                     Loc);
      }
    }
  }

  slangAssert(StmtCount > 0);
  clang::CompoundStmt *CS =
      new(C) clang::CompoundStmt(C, StmtArray, StmtCount, Loc, Loc);

  delete [] StmtArray;

  return CS;
}

static clang::Stmt *CreateSingleRSSetObject(clang::ASTContext &C,
                                            clang::Diagnostic *Diags,
                                            clang::Expr *DstExpr,
                                            clang::Expr *SrcExpr,
                                            clang::SourceLocation Loc) {
  const clang::Type *T = DstExpr->getType().getTypePtr();
  clang::FunctionDecl *SetObjectFD = RSObjectRefCount::GetRSSetObjectFD(T);
  slangAssert((SetObjectFD != NULL) &&
              "rsSetObject doesn't cover all RS object types");

  clang::QualType SetObjectFDType = SetObjectFD->getType();
  clang::QualType SetObjectFDArgType[2];
  SetObjectFDArgType[0] = SetObjectFD->getParamDecl(0)->getOriginalType();
  SetObjectFDArgType[1] = SetObjectFD->getParamDecl(1)->getOriginalType();

  clang::Expr *RefRSSetObjectFD =
      clang::DeclRefExpr::Create(C,
                                 NULL,
                                 SetObjectFD->getQualifierRange(),
                                 SetObjectFD,
                                 Loc,
                                 SetObjectFDType);

  clang::Expr *RSSetObjectFP =
      clang::ImplicitCastExpr::Create(C,
                                      C.getPointerType(SetObjectFDType),
                                      clang::CK_FunctionToPointerDecay,
                                      RefRSSetObjectFD,
                                      NULL,
                                      clang::VK_RValue);

  clang::Expr *ArgList[2];
  ArgList[0] = new(C) clang::UnaryOperator(DstExpr,
                                           clang::UO_AddrOf,
                                           SetObjectFDArgType[0],
                                           Loc);
  ArgList[1] = SrcExpr;

  clang::CallExpr *RSSetObjectCall =
      new(C) clang::CallExpr(C,
                             RSSetObjectFP,
                             ArgList,
                             2,
                             SetObjectFD->getCallResultType(),
                             Loc);

  return RSSetObjectCall;
}

static clang::Stmt *CreateStructRSSetObject(clang::ASTContext &C,
                                            clang::Diagnostic *Diags,
                                            clang::Expr *LHS,
                                            clang::Expr *RHS,
                                            clang::SourceLocation Loc);

static clang::Stmt *CreateArrayRSSetObject(clang::ASTContext &C,
                                           clang::Diagnostic *Diags,
                                           clang::Expr *DstArr,
                                           clang::Expr *SrcArr,
                                           clang::SourceLocation Loc) {
  clang::DeclContext *DC = NULL;
  clang::SourceRange Range;
  const clang::Type *BaseType = DstArr->getType().getTypePtr();
  slangAssert(BaseType->isArrayType());

  int NumArrayElements = ArrayDim(BaseType);
  // Actually extract out the base RS object type for use later
  BaseType = BaseType->getArrayElementTypeNoTypeQual();

  clang::Stmt *StmtArray[2] = {NULL};
  int StmtCtr = 0;

  if (NumArrayElements <= 0) {
    return NULL;
  }

  // Create helper variable for iterating through elements
  clang::IdentifierInfo& II = C.Idents.get("rsIntIter");
  clang::VarDecl *IIVD =
      clang::VarDecl::Create(C,
                             DC,
                             Loc,
                             &II,
                             C.IntTy,
                             C.getTrivialTypeSourceInfo(C.IntTy),
                             clang::SC_None,
                             clang::SC_None);
  clang::Decl *IID = (clang::Decl *)IIVD;

  clang::DeclGroupRef DGR = clang::DeclGroupRef::Create(C, &IID, 1);
  StmtArray[StmtCtr++] = new(C) clang::DeclStmt(DGR, Loc, Loc);

  // Form the actual loop
  // for (Init; Cond; Inc)
  //   RSSetObjectCall;

  // Init -> "rsIntIter = 0"
  clang::DeclRefExpr *RefrsIntIter =
      clang::DeclRefExpr::Create(C,
                                 NULL,
                                 Range,
                                 IIVD,
                                 Loc,
                                 C.IntTy);

  clang::Expr *Int0 = clang::IntegerLiteral::Create(C,
      llvm::APInt(C.getTypeSize(C.IntTy), 0), C.IntTy, Loc);

  clang::BinaryOperator *Init =
      new(C) clang::BinaryOperator(RefrsIntIter,
                                   Int0,
                                   clang::BO_Assign,
                                   C.IntTy,
                                   Loc);

  // Cond -> "rsIntIter < NumArrayElements"
  clang::Expr *NumArrayElementsExpr = clang::IntegerLiteral::Create(C,
      llvm::APInt(C.getTypeSize(C.IntTy), NumArrayElements), C.IntTy, Loc);

  clang::BinaryOperator *Cond =
      new(C) clang::BinaryOperator(RefrsIntIter,
                                   NumArrayElementsExpr,
                                   clang::BO_LT,
                                   C.IntTy,
                                   Loc);

  // Inc -> "rsIntIter++"
  clang::UnaryOperator *Inc =
      new(C) clang::UnaryOperator(RefrsIntIter,
                                  clang::UO_PostInc,
                                  C.IntTy,
                                  Loc);

  // Body -> "rsSetObject(&Dst[rsIntIter], Src[rsIntIter]);"
  // Loop operates on individual array elements

  clang::Expr *DstArrPtr =
      clang::ImplicitCastExpr::Create(C,
          C.getPointerType(BaseType->getCanonicalTypeInternal()),
          clang::CK_ArrayToPointerDecay,
          DstArr,
          NULL,
          clang::VK_RValue);

  clang::Expr *DstArrPtrSubscript =
      new(C) clang::ArraySubscriptExpr(DstArrPtr,
                                       RefrsIntIter,
                                       BaseType->getCanonicalTypeInternal(),
                                       Loc);

  clang::Expr *SrcArrPtr =
      clang::ImplicitCastExpr::Create(C,
          C.getPointerType(BaseType->getCanonicalTypeInternal()),
          clang::CK_ArrayToPointerDecay,
          SrcArr,
          NULL,
          clang::VK_RValue);

  clang::Expr *SrcArrPtrSubscript =
      new(C) clang::ArraySubscriptExpr(SrcArrPtr,
                                       RefrsIntIter,
                                       BaseType->getCanonicalTypeInternal(),
                                       Loc);

  RSExportPrimitiveType::DataType DT =
      RSExportPrimitiveType::GetRSSpecificType(BaseType);

  clang::Stmt *RSSetObjectCall = NULL;
  if (BaseType->isArrayType()) {
    RSSetObjectCall = CreateArrayRSSetObject(C, Diags, DstArrPtrSubscript,
                                             SrcArrPtrSubscript, Loc);
  } else if (DT == RSExportPrimitiveType::DataTypeUnknown) {
    RSSetObjectCall = CreateStructRSSetObject(C, Diags, DstArrPtrSubscript,
                                              SrcArrPtrSubscript, Loc);
  } else {
    RSSetObjectCall = CreateSingleRSSetObject(C, Diags, DstArrPtrSubscript,
                                              SrcArrPtrSubscript, Loc);
  }

  clang::ForStmt *DestructorLoop =
      new(C) clang::ForStmt(C,
                            Init,
                            Cond,
                            NULL,  // no condVar
                            Inc,
                            RSSetObjectCall,
                            Loc,
                            Loc,
                            Loc);

  StmtArray[StmtCtr++] = DestructorLoop;
  slangAssert(StmtCtr == 2);

  clang::CompoundStmt *CS =
      new(C) clang::CompoundStmt(C, StmtArray, StmtCtr, Loc, Loc);

  return CS;
}

static clang::Stmt *CreateStructRSSetObject(clang::ASTContext &C,
                                            clang::Diagnostic *Diags,
                                            clang::Expr *LHS,
                                            clang::Expr *RHS,
                                            clang::SourceLocation Loc) {
  clang::SourceRange Range;
  clang::QualType QT = LHS->getType();
  const clang::Type *T = QT.getTypePtr();
  slangAssert(T->isStructureType());
  slangAssert(!RSExportPrimitiveType::IsRSObjectType(T));

  // Keep an extra slot for the original copy (memcpy)
  unsigned FieldsToSet = CountRSObjectTypes(T) + 1;

  unsigned StmtCount = 0;
  clang::Stmt **StmtArray = new clang::Stmt*[FieldsToSet];
  for (unsigned i = 0; i < FieldsToSet; i++) {
    StmtArray[i] = NULL;
  }

  clang::RecordDecl *RD = T->getAsStructureType()->getDecl();
  RD = RD->getDefinition();
  for (clang::RecordDecl::field_iterator FI = RD->field_begin(),
         FE = RD->field_end();
       FI != FE;
       FI++) {
    bool IsArrayType = false;
    clang::FieldDecl *FD = *FI;
    const clang::Type *FT = RSExportType::GetTypeOfDecl(FD);
    const clang::Type *OrigType = FT;

    if (!CountRSObjectTypes(FT)) {
      // Skip to next if we don't have any viable RS object types
      continue;
    }

    clang::DeclAccessPair FoundDecl =
        clang::DeclAccessPair::make(FD, clang::AS_none);
    clang::MemberExpr *DstMember =
        clang::MemberExpr::Create(C,
                                  LHS,
                                  false,
                                  NULL,
                                  Range,
                                  FD,
                                  FoundDecl,
                                  clang::DeclarationNameInfo(),
                                  NULL,
                                  OrigType->getCanonicalTypeInternal());

    clang::MemberExpr *SrcMember =
        clang::MemberExpr::Create(C,
                                  RHS,
                                  false,
                                  NULL,
                                  Range,
                                  FD,
                                  FoundDecl,
                                  clang::DeclarationNameInfo(),
                                  NULL,
                                  OrigType->getCanonicalTypeInternal());

    if (FT->isArrayType()) {
      FT = FT->getArrayElementTypeNoTypeQual();
      IsArrayType = true;
    }

    RSExportPrimitiveType::DataType DT =
        RSExportPrimitiveType::GetRSSpecificType(FT);

    if (IsArrayType) {
      Diags->Report(clang::FullSourceLoc(Loc, C.getSourceManager()),
          Diags->getCustomDiagID(clang::Diagnostic::Error,
            "Arrays of RS object types within structures cannot be copied"));
      // TODO(srhines): Support setting arrays of RS objects
      // StmtArray[StmtCount++] =
      //    CreateArrayRSSetObject(C, Diags, DstMember, SrcMember, Loc);
    } else if (DT == RSExportPrimitiveType::DataTypeUnknown) {
      StmtArray[StmtCount++] =
          CreateStructRSSetObject(C, Diags, DstMember, SrcMember, Loc);
    } else if (RSExportPrimitiveType::IsRSObjectType(DT)) {
      StmtArray[StmtCount++] =
          CreateSingleRSSetObject(C, Diags, DstMember, SrcMember, Loc);
    } else {
      slangAssert(false);
    }
  }

  slangAssert(StmtCount > 0 && StmtCount < FieldsToSet);

  // We still need to actually do the overall struct copy. For simplicity,
  // we just do a straight-up assignment (which will still preserve all
  // the proper RS object reference counts).
  clang::BinaryOperator *CopyStruct =
      new(C) clang::BinaryOperator(LHS, RHS, clang::BO_Assign, QT, Loc);
  StmtArray[StmtCount++] = CopyStruct;

  clang::CompoundStmt *CS =
      new(C) clang::CompoundStmt(C, StmtArray, StmtCount, Loc, Loc);

  delete [] StmtArray;

  return CS;
}

}  // namespace

void RSObjectRefCount::Scope::ReplaceRSObjectAssignment(
    clang::BinaryOperator *AS,
    clang::Diagnostic *Diags) {

  clang::QualType QT = AS->getType();

  clang::ASTContext &C = RSObjectRefCount::GetRSSetObjectFD(
      RSExportPrimitiveType::DataTypeRSFont)->getASTContext();

  clang::SourceLocation Loc = AS->getExprLoc();
  clang::Stmt *UpdatedStmt = NULL;

  if (!RSExportPrimitiveType::IsRSObjectType(QT.getTypePtr())) {
    // By definition, this is a struct assignment if we get here
    UpdatedStmt =
        CreateStructRSSetObject(C, Diags, AS->getLHS(), AS->getRHS(), Loc);
  } else {
    UpdatedStmt =
        CreateSingleRSSetObject(C, Diags, AS->getLHS(), AS->getRHS(), Loc);
  }

  ReplaceInCompoundStmt(C, mCS, AS, UpdatedStmt);
  return;
}

void RSObjectRefCount::Scope::AppendRSObjectInit(
    clang::Diagnostic *Diags,
    clang::VarDecl *VD,
    clang::DeclStmt *DS,
    RSExportPrimitiveType::DataType DT,
    clang::Expr *InitExpr) {
  slangAssert(VD);

  if (!InitExpr) {
    return;
  }

  clang::ASTContext &C = RSObjectRefCount::GetRSSetObjectFD(
      RSExportPrimitiveType::DataTypeRSFont)->getASTContext();
  clang::SourceLocation Loc = RSObjectRefCount::GetRSSetObjectFD(
      RSExportPrimitiveType::DataTypeRSFont)->getLocation();

  if (DT == RSExportPrimitiveType::DataTypeIsStruct) {
    // TODO(srhines): Skip struct initialization right now
    const clang::Type *T = RSExportType::GetTypeOfDecl(VD);
    clang::DeclRefExpr *RefRSVar =
        clang::DeclRefExpr::Create(C,
                                   NULL,
                                   VD->getQualifierRange(),
                                   VD,
                                   Loc,
                                   T->getCanonicalTypeInternal());

    clang::Stmt *RSSetObjectOps =
        CreateStructRSSetObject(C, Diags, RefRSVar, InitExpr, Loc);

    AppendAfterStmt(C, mCS, DS, RSSetObjectOps);
    return;
  }

  clang::FunctionDecl *SetObjectFD = RSObjectRefCount::GetRSSetObjectFD(DT);
  slangAssert((SetObjectFD != NULL) &&
              "rsSetObject doesn't cover all RS object types");

  clang::QualType SetObjectFDType = SetObjectFD->getType();
  clang::QualType SetObjectFDArgType[2];
  SetObjectFDArgType[0] = SetObjectFD->getParamDecl(0)->getOriginalType();
  SetObjectFDArgType[1] = SetObjectFD->getParamDecl(1)->getOriginalType();

  clang::Expr *RefRSSetObjectFD =
      clang::DeclRefExpr::Create(C,
                                 NULL,
                                 SetObjectFD->getQualifierRange(),
                                 SetObjectFD,
                                 Loc,
                                 SetObjectFDType);

  clang::Expr *RSSetObjectFP =
      clang::ImplicitCastExpr::Create(C,
                                      C.getPointerType(SetObjectFDType),
                                      clang::CK_FunctionToPointerDecay,
                                      RefRSSetObjectFD,
                                      NULL,
                                      clang::VK_RValue);

  const clang::Type *T = RSExportType::GetTypeOfDecl(VD);
  clang::DeclRefExpr *RefRSVar =
      clang::DeclRefExpr::Create(C,
                                 NULL,
                                 VD->getQualifierRange(),
                                 VD,
                                 Loc,
                                 T->getCanonicalTypeInternal());

  clang::Expr *ArgList[2];
  ArgList[0] = new(C) clang::UnaryOperator(RefRSVar,
                                           clang::UO_AddrOf,
                                           SetObjectFDArgType[0],
                                           Loc);
  ArgList[1] = InitExpr;

  clang::CallExpr *RSSetObjectCall =
      new(C) clang::CallExpr(C,
                             RSSetObjectFP,
                             ArgList,
                             2,
                             SetObjectFD->getCallResultType(),
                             Loc);

  AppendAfterStmt(C, mCS, DS, RSSetObjectCall);

  return;
}

void RSObjectRefCount::Scope::InsertLocalVarDestructors() {
  std::list<clang::Stmt*> RSClearObjectCalls;
  for (std::list<clang::VarDecl*>::const_iterator I = mRSO.begin(),
          E = mRSO.end();
        I != E;
        I++) {
    clang::Stmt *S = ClearRSObject(*I);
    if (S) {
      RSClearObjectCalls.push_back(S);
    }
  }
  if (RSClearObjectCalls.size() > 0) {
    DestructorVisitor DV((*mRSO.begin())->getASTContext(), RSClearObjectCalls);
    DV.Visit(mCS);
  }
  return;
}

clang::Stmt *RSObjectRefCount::Scope::ClearRSObject(clang::VarDecl *VD) {
  slangAssert(VD);
  clang::ASTContext &C = VD->getASTContext();
  clang::DeclContext *DC = VD->getDeclContext();
  clang::SourceRange Range = VD->getQualifierRange();
  clang::SourceLocation Loc = VD->getLocation();
  const clang::Type *T = RSExportType::GetTypeOfDecl(VD);

  // Reference expr to target RS object variable
  clang::DeclRefExpr *RefRSVar =
      clang::DeclRefExpr::Create(C,
                                 NULL,
                                 Range,
                                 VD,
                                 Loc,
                                 T->getCanonicalTypeInternal());

  if (T->isArrayType()) {
    return ClearArrayRSObject(C, DC, RefRSVar, Range, Loc);
  }

  RSExportPrimitiveType::DataType DT =
      RSExportPrimitiveType::GetRSSpecificType(T);

  if (DT == RSExportPrimitiveType::DataTypeUnknown ||
      DT == RSExportPrimitiveType::DataTypeIsStruct) {
    return ClearStructRSObject(C, DC, RefRSVar, Range, Loc);
  }

  slangAssert((RSExportPrimitiveType::IsRSObjectType(DT)) &&
              "Should be RS object");

  return ClearSingleRSObject(C, RefRSVar, Loc);
}

bool RSObjectRefCount::InitializeRSObject(clang::VarDecl *VD,
                                          RSExportPrimitiveType::DataType *DT,
                                          clang::Expr **InitExpr) {
  slangAssert(VD && DT && InitExpr);
  const clang::Type *T = RSExportType::GetTypeOfDecl(VD);

  // Loop through array types to get to base type
  while (T && T->isArrayType()) {
    T = T->getArrayElementTypeNoTypeQual();
  }

  bool DataTypeIsStructWithRSObject = false;
  *DT = RSExportPrimitiveType::GetRSSpecificType(T);

  if (*DT == RSExportPrimitiveType::DataTypeUnknown) {
    if (RSExportPrimitiveType::IsStructureTypeWithRSObject(T)) {
      *DT = RSExportPrimitiveType::DataTypeIsStruct;
      DataTypeIsStructWithRSObject = true;
    } else {
      return false;
    }
  }

  bool DataTypeIsRSObject = false;
  if (DataTypeIsStructWithRSObject) {
    DataTypeIsRSObject = true;
  } else {
    DataTypeIsRSObject = RSExportPrimitiveType::IsRSObjectType(*DT);
  }
  *InitExpr = VD->getInit();

  if (!DataTypeIsRSObject && *InitExpr) {
    // If we already have an initializer for a matrix type, we are done.
    return DataTypeIsRSObject;
  }

  clang::Expr *ZeroInitializer =
      CreateZeroInitializerForRSSpecificType(*DT,
                                             VD->getASTContext(),
                                             VD->getLocation());

  if (ZeroInitializer) {
    ZeroInitializer->setType(T->getCanonicalTypeInternal());
    VD->setInit(ZeroInitializer);
  }

  return DataTypeIsRSObject;
}

clang::Expr *RSObjectRefCount::CreateZeroInitializerForRSSpecificType(
    RSExportPrimitiveType::DataType DT,
    clang::ASTContext &C,
    const clang::SourceLocation &Loc) {
  clang::Expr *Res = NULL;
  switch (DT) {
    case RSExportPrimitiveType::DataTypeIsStruct:
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

      Res = new(C) clang::InitListExpr(C, Loc, &CastToNull, 1, Loc);
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
          new(C) clang::InitListExpr(C, Loc, InitVals, N * N, Loc);
      InitExpr->setType(C.getConstantArrayType(FloatTy,
                                               llvm::APInt(32, 4),
                                               clang::ArrayType::Normal,
                                               /* EltTypeQuals = */0));

      Res = new(C) clang::InitListExpr(C, Loc, &InitExpr, 1, Loc);
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
      slangAssert(false && "Not RS object type!");
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
      RSExportPrimitiveType::DataType DT =
          RSExportPrimitiveType::DataTypeUnknown;
      clang::Expr *InitExpr = NULL;
      if (InitializeRSObject(VD, &DT, &InitExpr)) {
        getCurrentScope()->addRSObject(VD);
        getCurrentScope()->AppendRSObjectInit(mDiags, VD, DS, DT, InitExpr);
      }
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
    slangAssert((getCurrentScope() == S) && "Corrupted scope stack!");
    S->InsertLocalVarDestructors();
    mScopeStack.pop();
    delete S;
  }
  return;
}

void RSObjectRefCount::VisitBinAssign(clang::BinaryOperator *AS) {
  clang::QualType QT = AS->getType();

  if (CountRSObjectTypes(QT.getTypePtr())) {
    getCurrentScope()->ReplaceRSObjectAssignment(AS, mDiags);
  }

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

}  // namespace slang
