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

#include "slang_rs_backend.h"

#include <stack>
#include <vector>
#include <string>

#include "llvm/Metadata.h"
#include "llvm/Constant.h"
#include "llvm/Constants.h"
#include "llvm/Module.h"
#include "llvm/Function.h"
#include "llvm/DerivedTypes.h"

#include "llvm/Support/IRBuilder.h"

#include "llvm/ADT/Twine.h"
#include "llvm/ADT/StringExtras.h"

#include "clang/AST/DeclGroup.h"
#include "clang/AST/Expr.h"
#include "clang/AST/OperationKinds.h"
#include "clang/AST/Stmt.h"
#include "clang/AST/StmtVisitor.h"

#include "slang_rs.h"
#include "slang_rs_context.h"
#include "slang_rs_metadata.h"
#include "slang_rs_export_var.h"
#include "slang_rs_export_func.h"
#include "slang_rs_export_type.h"

using namespace slang;

RSBackend::RSBackend(RSContext *Context,
                     clang::Diagnostic &Diags,
                     const clang::CodeGenOptions &CodeGenOpts,
                     const clang::TargetOptions &TargetOpts,
                     const PragmaList &Pragmas,
                     llvm::raw_ostream *OS,
                     Slang::OutputType OT,
                     clang::SourceManager &SourceMgr,
                     bool AllowRSPrefix)
    : Backend(Diags,
              CodeGenOpts,
              TargetOpts,
              Pragmas,
              OS,
              OT),
      mContext(Context),
      mSourceMgr(SourceMgr),
      mAllowRSPrefix(AllowRSPrefix),
      mExportVarMetadata(NULL),
      mExportFuncMetadata(NULL),
      mExportTypeMetadata(NULL) {
  return;
}

///////////////////////////////////////////////////////////////////////////////

namespace {

  class RSObjectRefCounting : public clang::StmtVisitor<RSObjectRefCounting> {
   private:
    class Scope {
     private:
      clang::CompoundStmt *mCS;      // Associated compound statement ({ ... })
      std::list<clang::Decl*> mRSO;  // Declared RS object in this scope

     public:
      Scope(clang::CompoundStmt *CS) : mCS(CS) {
        return;
      }

      inline void addRSObject(clang::Decl* D) { mRSO.push_back(D); }
    };
    std::stack<Scope*> mScopeStack;

    inline Scope *getCurrentScope() { return mScopeStack.top(); }

    // Return false if the type of variable declared in VD is not an RS object
    // type.
    static bool InitializeRSObject(clang::VarDecl *VD);
    // Return an zero-initializer expr of the type DT. This processes both
    // RS matrix type and RS object type.
    static clang::Expr *CreateZeroInitializerForRSSpecificType(
        RSExportPrimitiveType::DataType DT,
        clang::ASTContext &C,
        const clang::SourceLocation &Loc);

   public:
    void VisitChildren(clang::Stmt *S) {
      for (clang::Stmt::child_iterator I = S->child_begin(), E = S->child_end();
           I != E;
           I++)
        if (clang::Stmt *Child = *I)
          Visit(Child);
    }
    void VisitStmt(clang::Stmt *S) { VisitChildren(S); }

    void VisitDeclStmt(clang::DeclStmt *DS);
    void VisitCompoundStmt(clang::CompoundStmt *CS);
    void VisitBinAssign(clang::BinaryOperator *AS);

    // We believe that RS objects never are involved in CompoundAssignOperator.
    // I.e., rs_allocation foo; foo += bar;
  };
}

bool RSObjectRefCounting::InitializeRSObject(clang::VarDecl *VD) {
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

clang::Expr *RSObjectRefCounting::CreateZeroInitializerForRSSpecificType(
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

void RSObjectRefCounting::VisitDeclStmt(clang::DeclStmt *DS) {
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

void RSObjectRefCounting::VisitCompoundStmt(clang::CompoundStmt *CS) {
  if (!CS->body_empty()) {
    // Push a new scope
    Scope *S = new Scope(CS);
    mScopeStack.push(S);

    VisitChildren(CS);

    // Destroy the scope
    // TODO: Update reference count of the RS object refenced by the
    //       getCurrentScope().
    assert((getCurrentScope() == S) && "Corrupted scope stack!");
    mScopeStack.pop();
    delete S;
  }
  return;
}

void RSObjectRefCounting::VisitBinAssign(clang::BinaryOperator *AS) {
  // TODO: Update reference count
  return;
}

// 1) Add zero initialization of local RS object types
void RSBackend::AnnotateFunction(clang::FunctionDecl *FD) {
  if (FD &&
      FD->hasBody() &&
      !SlangRS::IsFunctionInRSHeaderFile(FD, mSourceMgr)) {
    RSObjectRefCounting RSObjectRefCounter;
    RSObjectRefCounter.Visit(FD->getBody());
  }
  return;
}

void RSBackend::HandleTopLevelDecl(clang::DeclGroupRef D) {
  // Disallow user-defined functions with prefix "rs"
  if (!mAllowRSPrefix) {
    // Iterate all function declarations in the program.
    for (clang::DeclGroupRef::iterator I = D.begin(), E = D.end();
         I != E; I++) {
      clang::FunctionDecl *FD = dyn_cast<clang::FunctionDecl>(*I);
      if (FD == NULL)
        continue;
      if (!FD->getName().startswith("rs"))  // Check prefix
        continue;
      if (!SlangRS::IsFunctionInRSHeaderFile(FD, mSourceMgr))
        mDiags.Report(clang::FullSourceLoc(FD->getLocation(), mSourceMgr),
                      mDiags.getCustomDiagID(clang::Diagnostic::Error,
                                             "invalid function name prefix, "
                                             "\"rs\" is reserved: '%0'"))
            << FD->getName();
    }
  }

  // Process any non-static function declarations
  for (clang::DeclGroupRef::iterator I = D.begin(), E = D.end(); I != E; I++) {
    AnnotateFunction(dyn_cast<clang::FunctionDecl>(*I));
  }

  Backend::HandleTopLevelDecl(D);
  return;
}

void RSBackend::HandleTranslationUnitPre(clang::ASTContext& C) {
  clang::TranslationUnitDecl *TUDecl = C.getTranslationUnitDecl();

  // Process any static function declarations
  for (clang::DeclContext::decl_iterator I = TUDecl->decls_begin(),
          E = TUDecl->decls_end(); I != E; I++) {
    if ((I->getKind() >= clang::Decl::firstFunction) &&
        (I->getKind() <= clang::Decl::lastFunction)) {
      AnnotateFunction(static_cast<clang::FunctionDecl*>(*I));
    }
  }

  return;
}

///////////////////////////////////////////////////////////////////////////////
void RSBackend::HandleTranslationUnitPost(llvm::Module *M) {
  mContext->processExport();

  // Dump export variable info
  if (mContext->hasExportVar()) {
    if (mExportVarMetadata == NULL)
      mExportVarMetadata = M->getOrInsertNamedMetadata(RS_EXPORT_VAR_MN);

    llvm::SmallVector<llvm::Value*, 2> ExportVarInfo;

    for (RSContext::const_export_var_iterator I = mContext->export_vars_begin(),
            E = mContext->export_vars_end();
         I != E;
         I++) {
      const RSExportVar *EV = *I;
      const RSExportType *ET = EV->getType();

      // Variable name
      ExportVarInfo.push_back(
          llvm::MDString::get(mLLVMContext, EV->getName().c_str()));

      // Type name
      switch (ET->getClass()) {
        case RSExportType::ExportClassPrimitive: {
          ExportVarInfo.push_back(
              llvm::MDString::get(
                mLLVMContext, llvm::utostr_32(
                  static_cast<const RSExportPrimitiveType*>(ET)->getType())));
          break;
        }
        case RSExportType::ExportClassPointer: {
          ExportVarInfo.push_back(
              llvm::MDString::get(
                mLLVMContext, ("*" + static_cast<const RSExportPointerType*>(ET)
                  ->getPointeeType()->getName()).c_str()));
          break;
        }
        case RSExportType::ExportClassMatrix: {
          ExportVarInfo.push_back(
              llvm::MDString::get(
                mLLVMContext, llvm::utostr_32(
                  RSExportPrimitiveType::DataTypeRSMatrix2x2 +
                  static_cast<const RSExportMatrixType*>(ET)->getDim() - 2)));
          break;
        }
        case RSExportType::ExportClassVector:
        case RSExportType::ExportClassConstantArray:
        case RSExportType::ExportClassRecord: {
          ExportVarInfo.push_back(
              llvm::MDString::get(mLLVMContext,
                EV->getType()->getName().c_str()));
          break;
        }
      }

      mExportVarMetadata->addOperand(
          llvm::MDNode::get(mLLVMContext,
                            ExportVarInfo.data(),
                            ExportVarInfo.size()) );

      ExportVarInfo.clear();
    }
  }

  // Dump export function info
  if (mContext->hasExportFunc()) {
    if (mExportFuncMetadata == NULL)
      mExportFuncMetadata =
          M->getOrInsertNamedMetadata(RS_EXPORT_FUNC_MN);

    llvm::SmallVector<llvm::Value*, 1> ExportFuncInfo;

    for (RSContext::const_export_func_iterator
            I = mContext->export_funcs_begin(),
            E = mContext->export_funcs_end();
         I != E;
         I++) {
      const RSExportFunc *EF = *I;

      // Function name
      if (!EF->hasParam()) {
        ExportFuncInfo.push_back(llvm::MDString::get(mLLVMContext,
                                                     EF->getName().c_str()));
      } else {
        llvm::Function *F = M->getFunction(EF->getName());
        llvm::Function *HelperFunction;
        const std::string HelperFunctionName(".helper_" + EF->getName());

        assert(F && "Function marked as exported disappeared in Bitcode");

        // Create helper function
        {
          llvm::StructType *HelperFunctionParameterTy = NULL;

          if (!F->getArgumentList().empty()) {
            std::vector<const llvm::Type*> HelperFunctionParameterTys;
            for (llvm::Function::arg_iterator AI = F->arg_begin(),
                 AE = F->arg_end(); AI != AE; AI++)
              HelperFunctionParameterTys.push_back(AI->getType());

            HelperFunctionParameterTy =
                llvm::StructType::get(mLLVMContext, HelperFunctionParameterTys);
          }

          if (!EF->checkParameterPacketType(HelperFunctionParameterTy)) {
            fprintf(stderr, "Failed to export function %s: parameter type "
                            "mismatch during creation of helper function.\n",
                    EF->getName().c_str());

            const RSExportRecordType *Expected = EF->getParamPacketType();
            if (Expected) {
              fprintf(stderr, "Expected:\n");
              Expected->getLLVMType()->dump();
            }
            if (HelperFunctionParameterTy) {
              fprintf(stderr, "Got:\n");
              HelperFunctionParameterTy->dump();
            }
          }

          std::vector<const llvm::Type*> Params;
          if (HelperFunctionParameterTy) {
            llvm::PointerType *HelperFunctionParameterTyP =
                llvm::PointerType::getUnqual(HelperFunctionParameterTy);
            Params.push_back(HelperFunctionParameterTyP);
          }

          llvm::FunctionType * HelperFunctionType =
              llvm::FunctionType::get(F->getReturnType(),
                                      Params,
                                      /* IsVarArgs = */false);

          HelperFunction =
              llvm::Function::Create(HelperFunctionType,
                                     llvm::GlobalValue::ExternalLinkage,
                                     HelperFunctionName,
                                     M);

          HelperFunction->addFnAttr(llvm::Attribute::NoInline);
          HelperFunction->setCallingConv(F->getCallingConv());

          // Create helper function body
          {
            llvm::Argument *HelperFunctionParameter =
                &(*HelperFunction->arg_begin());
            llvm::BasicBlock *BB =
                llvm::BasicBlock::Create(mLLVMContext, "entry", HelperFunction);
            llvm::IRBuilder<> *IB = new llvm::IRBuilder<>(BB);
            llvm::SmallVector<llvm::Value*, 6> Params;
            llvm::Value *Idx[2];

            Idx[0] =
                llvm::ConstantInt::get(llvm::Type::getInt32Ty(mLLVMContext), 0);

            // getelementptr and load instruction for all elements in
            // parameter .p
            for (size_t i = 0; i < EF->getNumParameters(); i++) {
              // getelementptr
              Idx[1] =
                  llvm::ConstantInt::get(
                      llvm::Type::getInt32Ty(mLLVMContext), i);
              llvm::Value *Ptr = IB->CreateInBoundsGEP(HelperFunctionParameter,
                                                       Idx,
                                                       Idx + 2);

              // load
              llvm::Value *V = IB->CreateLoad(Ptr);
              Params.push_back(V);
            }

            // Call and pass the all elements as paramter to F
            llvm::CallInst *CI = IB->CreateCall(F,
                                                Params.data(),
                                                Params.data() + Params.size());

            CI->setCallingConv(F->getCallingConv());

            if (F->getReturnType() == llvm::Type::getVoidTy(mLLVMContext))
              IB->CreateRetVoid();
            else
              IB->CreateRet(CI);

            delete IB;
          }
        }

        ExportFuncInfo.push_back(
            llvm::MDString::get(mLLVMContext, HelperFunctionName.c_str()));
      }

      mExportFuncMetadata->addOperand(
          llvm::MDNode::get(mLLVMContext,
                            ExportFuncInfo.data(),
                            ExportFuncInfo.size()));

      ExportFuncInfo.clear();
    }
  }

  // Dump export type info
  if (mContext->hasExportType()) {
    llvm::SmallVector<llvm::Value*, 1> ExportTypeInfo;

    for (RSContext::const_export_type_iterator
            I = mContext->export_types_begin(),
            E = mContext->export_types_end();
         I != E;
         I++) {
      // First, dump type name list to export
      const RSExportType *ET = I->getValue();

      ExportTypeInfo.clear();
      // Type name
      ExportTypeInfo.push_back(
          llvm::MDString::get(mLLVMContext, ET->getName().c_str()));

      if (ET->getClass() == RSExportType::ExportClassRecord) {
        const RSExportRecordType *ERT =
            static_cast<const RSExportRecordType*>(ET);

        if (mExportTypeMetadata == NULL)
          mExportTypeMetadata =
              M->getOrInsertNamedMetadata(RS_EXPORT_TYPE_MN);

        mExportTypeMetadata->addOperand(
            llvm::MDNode::get(mLLVMContext,
                              ExportTypeInfo.data(),
                              ExportTypeInfo.size()));

        // Now, export struct field information to %[struct name]
        std::string StructInfoMetadataName("%");
        StructInfoMetadataName.append(ET->getName());
        llvm::NamedMDNode *StructInfoMetadata =
            M->getOrInsertNamedMetadata(StructInfoMetadataName);
        llvm::SmallVector<llvm::Value*, 3> FieldInfo;

        assert(StructInfoMetadata->getNumOperands() == 0 &&
               "Metadata with same name was created before");
        for (RSExportRecordType::const_field_iterator FI = ERT->fields_begin(),
                FE = ERT->fields_end();
             FI != FE;
             FI++) {
          const RSExportRecordType::Field *F = *FI;

          // 1. field name
          FieldInfo.push_back(llvm::MDString::get(mLLVMContext,
                                                  F->getName().c_str()));

          // 2. field type name
          FieldInfo.push_back(
              llvm::MDString::get(mLLVMContext,
                                  F->getType()->getName().c_str()));

          // 3. field kind
          switch (F->getType()->getClass()) {
            case RSExportType::ExportClassPrimitive:
            case RSExportType::ExportClassVector: {
              const RSExportPrimitiveType *EPT =
                  static_cast<const RSExportPrimitiveType*>(F->getType());
              FieldInfo.push_back(
                  llvm::MDString::get(mLLVMContext,
                                      llvm::itostr(EPT->getKind())));
              break;
            }

            default: {
              FieldInfo.push_back(
                  llvm::MDString::get(mLLVMContext,
                                      llvm::itostr(
                                        RSExportPrimitiveType::DataKindUser)));
              break;
            }
          }

          StructInfoMetadata->addOperand(llvm::MDNode::get(mLLVMContext,
                                                           FieldInfo.data(),
                                                           FieldInfo.size()));

          FieldInfo.clear();
        }
      }   // ET->getClass() == RSExportType::ExportClassRecord
    }
  }

  return;
}

RSBackend::~RSBackend() {
  return;
}
