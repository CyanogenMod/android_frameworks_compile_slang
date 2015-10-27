/*
 * Copyright 2015, The Android Open Source Project
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

#include "slang_rs_foreach_lowering.h"

#include "clang/AST/ASTContext.h"
#include "llvm/Support/raw_ostream.h"
#include "slang_rs_context.h"
#include "slang_rs_export_foreach.h"

namespace slang {

namespace {

const char KERNEL_LAUNCH_FUNCTION_NAME[] = "rsForEach";
const char KERNEL_LAUNCH_FUNCTION_NAME_WITH_OPTIONS[] = "rsForEachWithOptions";
const char INTERNAL_LAUNCH_FUNCTION_NAME[] =
    "_Z17rsForEachInternaliP14rs_script_calliiz";

}  // anonymous namespace

RSForEachLowering::RSForEachLowering(RSContext* ctxt)
    : mCtxt(ctxt), mASTCtxt(ctxt->getASTContext()) {}

// Check if the passed-in expr references a kernel function in the following
// pattern in the AST.
//
// ImplicitCastExpr 'void *' <BitCast>
//  `-ImplicitCastExpr 'int (*)(int)' <FunctionToPointerDecay>
//    `-DeclRefExpr 'int (int)' Function 'foo' 'int (int)'
const clang::FunctionDecl* RSForEachLowering::matchFunctionDesignator(
    clang::Expr* expr) {
  clang::ImplicitCastExpr* ToVoidPtr =
      clang::dyn_cast<clang::ImplicitCastExpr>(expr);
  if (ToVoidPtr == nullptr) {
    return nullptr;
  }

  clang::ImplicitCastExpr* Decay =
      clang::dyn_cast<clang::ImplicitCastExpr>(ToVoidPtr->getSubExpr());

  if (Decay == nullptr) {
    return nullptr;
  }

  clang::DeclRefExpr* DRE =
      clang::dyn_cast<clang::DeclRefExpr>(Decay->getSubExpr());

  if (DRE == nullptr) {
    return nullptr;
  }

  const clang::FunctionDecl* FD =
      clang::dyn_cast<clang::FunctionDecl>(DRE->getDecl());

  if (FD == nullptr) {
    return nullptr;
  }

  return FD;
}

// Checks if the call expression is a legal rsForEach call by looking for the
// following pattern in the AST. On success, returns the first argument that is
// a FunctionDecl of a kernel function.
//
// CallExpr 'void'
// |
// |-ImplicitCastExpr 'void (*)(void *, ...)' <FunctionToPointerDecay>
// | `-DeclRefExpr  'void (void *, ...)'  'rsForEach' 'void (void *, ...)'
// |
// |-ImplicitCastExpr 'void *' <BitCast>
// | `-ImplicitCastExpr 'int (*)(int)' <FunctionToPointerDecay>
// |   `-DeclRefExpr 'int (int)' Function 'foo' 'int (int)'
// |
// |-ImplicitCastExpr 'rs_allocation':'rs_allocation' <LValueToRValue>
// | `-DeclRefExpr 'rs_allocation':'rs_allocation' lvalue ParmVar 'in' 'rs_allocation':'rs_allocation'
// |
// `-ImplicitCastExpr 'rs_allocation':'rs_allocation' <LValueToRValue>
//   `-DeclRefExpr  'rs_allocation':'rs_allocation' lvalue ParmVar 'out' 'rs_allocation':'rs_allocation'
const clang::FunctionDecl* RSForEachLowering::matchKernelLaunchCall(
    clang::CallExpr* CE, int* slot, bool* hasOptions) {
  const clang::Decl* D = CE->getCalleeDecl();
  const clang::FunctionDecl* FD = clang::dyn_cast<clang::FunctionDecl>(D);

  if (FD == nullptr) {
    return nullptr;
  }

  const clang::StringRef& funcName = FD->getName();

  if (funcName.equals(KERNEL_LAUNCH_FUNCTION_NAME)) {
    *hasOptions = false;
  } else if (funcName.equals(KERNEL_LAUNCH_FUNCTION_NAME_WITH_OPTIONS)) {
    *hasOptions = true;
  } else {
    return nullptr;
  }

  clang::Expr* arg0 = CE->getArg(0);
  const clang::FunctionDecl* kernel = matchFunctionDesignator(arg0);

  if (kernel == nullptr) {
    mCtxt->ReportError(arg0->getExprLoc(),
                       "Invalid kernel launch call. "
                       "Expects a function designator for the first argument.");
    return nullptr;
  }

  // Verifies that kernel is indeed a "kernel" function.
  *slot = mCtxt->getForEachSlotNumber(kernel);
  if (*slot == -1) {
    mCtxt->ReportError(CE->getExprLoc(), "%0 applied to non kernel function %1")
            << funcName << kernel->getName();
    return nullptr;
  }

  return kernel;
}

// Create an AST node for the declaration of rsForEachInternal
clang::FunctionDecl* RSForEachLowering::CreateForEachInternalFunctionDecl() {
  clang::DeclContext* DC = mASTCtxt.getTranslationUnitDecl();
  clang::SourceLocation Loc;

  llvm::StringRef SR(INTERNAL_LAUNCH_FUNCTION_NAME);
  clang::IdentifierInfo& II = mASTCtxt.Idents.get(SR);
  clang::DeclarationName N(&II);

  clang::FunctionProtoType::ExtProtoInfo EPI;
  EPI.Variadic = true;  // varargs

  clang::QualType T = mASTCtxt.getFunctionType(
      mASTCtxt.VoidTy,       // Return type
                             // Argument types:
      { mASTCtxt.IntTy,      // int slot
        mASTCtxt.VoidPtrTy,  // rs_script_call_t* launch_options
        mASTCtxt.IntTy,      // int numOutput
        mASTCtxt.IntTy       // int numInputs
      },
      EPI);

  clang::FunctionDecl* FD = clang::FunctionDecl::Create(
      mASTCtxt, DC, Loc, Loc, N, T, nullptr, clang::SC_Extern);

  return FD;
}

// Create an expression like the following that references the rsForEachInternal to
// replace the callee in the original call expression that references rsForEach.
//
// ImplicitCastExpr 'void (*)(int, rs_allocation, rs_allocation)' <FunctionToPointerDecay>
// `-DeclRefExpr 'void' Function '_Z17rsForEachInternali13rs_allocationS_' 'void (int, rs_allocation, rs_allocation)'
clang::Expr* RSForEachLowering::CreateCalleeExprForInternalForEach() {
  clang::FunctionDecl* FDNew = CreateForEachInternalFunctionDecl();

  clang::DeclRefExpr* refExpr = clang::DeclRefExpr::Create(
      mASTCtxt, clang::NestedNameSpecifierLoc(), clang::SourceLocation(), FDNew,
      false, clang::SourceLocation(), mASTCtxt.VoidTy, clang::VK_RValue);

  const clang::QualType FDNewType = FDNew->getType();

  clang::Expr* calleeNew = clang::ImplicitCastExpr::Create(
      mASTCtxt, mASTCtxt.getPointerType(FDNewType),
      clang::CK_FunctionToPointerDecay, refExpr, nullptr, clang::VK_RValue);

  return calleeNew;
}

// This visit method checks (via pattern matching) if the call expression is to
// rsForEach, and the arguments satisfy the restrictions on the
// rsForEach API. If so, replace the call with a rsForEachInternal call
// with the first argument replaced by the slot number of the kernel function
// referenced in the original first argument.
//
// See comments to the helper methods defined above for details.
void RSForEachLowering::VisitCallExpr(clang::CallExpr* CE) {
  int slot;
  bool hasOptions;
  const clang::FunctionDecl* kernel = matchKernelLaunchCall(CE, &slot, &hasOptions);
  if (kernel == nullptr) {
    return;
  }

  slangAssert(slot >= 0);

  const unsigned numArgsOrig = CE->getNumArgs();

  clang::QualType resultType = kernel->getReturnType().getCanonicalType();
  int numOutput = resultType->isVoidType() ? 0 : 1;

  unsigned numInputs = RSExportForEach::getNumInputs(mCtxt->getTargetAPI(), kernel);

  // Verifies that rsForEach takes the right number of input and output allocations.
  // TODO: Check input/output allocation types match kernel function expectation.
  const unsigned expectedNumAllocations = numArgsOrig - (hasOptions ? 2 : 1);
  if (numInputs + numOutput != expectedNumAllocations) {
    mCtxt->ReportError(
      CE->getExprLoc(),
      "Number of input and output allocations unexpected for kernel function %0")
    << kernel->getName();
    return;
  }

  clang::Expr* calleeNew = CreateCalleeExprForInternalForEach();
  CE->setCallee(calleeNew);

  const clang::CanQualType IntTy = mASTCtxt.IntTy;
  const unsigned IntTySize = mASTCtxt.getTypeSize(IntTy);
  const llvm::APInt APIntSlot(IntTySize, slot);
  const clang::Expr* arg0 = CE->getArg(0);
  const clang::SourceLocation Loc(arg0->getLocStart());
  clang::Expr* IntSlotNum =
      clang::IntegerLiteral::Create(mASTCtxt, APIntSlot, IntTy, Loc);
  CE->setArg(0, IntSlotNum);

  const unsigned numAdditionalArgs =
    hasOptions ?
    2 :  // numOutputs and numInputs
    3;   // launchOptions, numOutputs and numInputs

  const unsigned numArgs = numArgsOrig + numAdditionalArgs;

  // Makes extra room in the argument list for new arguments to add at position
  // 1, 2, and 3. The last (numInputs + numOutput) arguments, i.e., the input
  // and output allocations, are moved up numAdditionalArgs (2 or 3) positions
  // in the argument list.
  CE->setNumArgs(mASTCtxt, numArgs);
  for (unsigned i = numArgs - 1; i >= 4; i--) {
    CE->setArg(i, CE->getArg(i - numAdditionalArgs));
  }

  // Sets the new arguments for NULL launch option (if the user does not set one),
  // the number of outputs, and the number of inputs.

  if (!hasOptions) {
    const llvm::APInt APIntZero(IntTySize, 0);
    clang::Expr* IntNull =
        clang::IntegerLiteral::Create(mASTCtxt, APIntZero, IntTy, Loc);
    CE->setArg(1, IntNull);
  }

  const llvm::APInt APIntNumOutput(IntTySize, numOutput);
  clang::Expr* IntNumOutput =
      clang::IntegerLiteral::Create(mASTCtxt, APIntNumOutput, IntTy, Loc);
  CE->setArg(2, IntNumOutput);

  const llvm::APInt APIntNumInputs(IntTySize, numInputs);
  clang::Expr* IntNumInputs =
      clang::IntegerLiteral::Create(mASTCtxt, APIntNumInputs, IntTy, Loc);
  CE->setArg(3, IntNumInputs);
}

void RSForEachLowering::VisitStmt(clang::Stmt* S) {
  for (clang::Stmt* Child : S->children()) {
    if (Child) {
      Visit(Child);
    }
  }
}

}  // namespace slang
