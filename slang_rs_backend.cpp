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

#include <vector>
#include <string>

#include "llvm/Metadata.h"
#include "llvm/Constant.h"
#include "llvm/Constants.h"
#include "llvm/Module.h"
#include "llvm/Function.h"
#include "llvm/DerivedTypes.h"

#include "llvm/System/Path.h"

#include "llvm/Support/IRBuilder.h"

#include "llvm/ADT/Twine.h"
#include "llvm/ADT/StringExtras.h"

#include "clang/AST/DeclGroup.h"

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

void RSBackend::HandleTopLevelDecl(clang::DeclGroupRef D) {
  // Disallow user-defined functions with prefix "rs"
  if (!mAllowRSPrefix) {
    // Iterate all function declarations in the program.
    for (clang::DeclGroupRef::iterator I = D.begin(), E = D.end();
         I != E; I++) {
      clang::FunctionDecl *FD = dyn_cast<clang::FunctionDecl>(*I);
      if (FD == NULL)
        continue;
      if (FD->getName().startswith("rs")) {  // Check prefix
        clang::FullSourceLoc FSL(FD->getLocStart(), mSourceMgr);
        clang::PresumedLoc PLoc = mSourceMgr.getPresumedLoc(FSL);
        llvm::sys::Path HeaderFilename(PLoc.getFilename());

        // Skip if that function declared in the RS default header.
        if (SlangRS::IsRSHeaderFile(HeaderFilename.getLast().data()))
          continue;
        mDiags.Report(FSL, mDiags.getCustomDiagID(clang::Diagnostic::Error,
                      "invalid function name prefix, \"rs\" is reserved: '%0'"))
            << FD->getName();
      }
    }
  }

  Backend::HandleTopLevelDecl(D);
  return;
}

void RSBackend::HandleTranslationUnitEx(clang::ASTContext &Ctx) {
  assert((&Ctx == mContext->getASTContext()) && "Unexpected AST context change"
                                                " during LLVM IR generation");
  mContext->processExport();

  // Dump export variable info
  if (mContext->hasExportVar()) {
    if (mExportVarMetadata == NULL)
      mExportVarMetadata = mpModule->getOrInsertNamedMetadata(RS_EXPORT_VAR_MN);

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
      if (ET->getClass() == RSExportType::ExportClassPrimitive)
        ExportVarInfo.push_back(
            llvm::MDString::get(
                mLLVMContext, llvm::utostr_32(
                    static_cast<const RSExportPrimitiveType*>(ET)->getType())));
      else if (ET->getClass() == RSExportType::ExportClassPointer)
        ExportVarInfo.push_back(
            llvm::MDString::get(
                mLLVMContext, ("*" + static_cast<const RSExportPointerType*>(ET)
                               ->getPointeeType()->getName()).c_str()));
      else
        ExportVarInfo.push_back(
            llvm::MDString::get(mLLVMContext,
                                EV->getType()->getName().c_str()));

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
          mpModule->getOrInsertNamedMetadata(RS_EXPORT_FUNC_MN);

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
        llvm::Function *F = mpModule->getFunction(EF->getName());
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
                                     mpModule);

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
              mpModule->getOrInsertNamedMetadata(RS_EXPORT_TYPE_MN);

        mExportTypeMetadata->addOperand(
            llvm::MDNode::get(mLLVMContext,
                              ExportTypeInfo.data(),
                              ExportTypeInfo.size()));

        // Now, export struct field information to %[struct name]
        std::string StructInfoMetadataName("%");
        StructInfoMetadataName.append(ET->getName());
        llvm::NamedMDNode *StructInfoMetadata =
            mpModule->getOrInsertNamedMetadata(StructInfoMetadataName);
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
