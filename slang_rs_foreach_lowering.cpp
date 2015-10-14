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

const char KERNEL_LAUNCH_FUNCTION_NAME[] = "rsParallelFor";
const char INTERNAL_LAUNCH_FUNCTION_NAME[] =
    "_Z17rsForEachInternali13rs_allocationS_";

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

  // TODO: Verify the launch has the expected number of input allocations

  return FD;
}

// Checks if the call expression is a legal rsParallelFor call by looking for the
// following pattern in the AST. On success, returns the first argument that is
// a FunctionDecl of a kernel function.
//
// CallExpr 'void'
// |
// |-ImplicitCastExpr 'void (*)(void *, ...)' <FunctionToPointerDecay>
// | `-DeclRefExpr  'void (void *, ...)'  'rsParallelFor' 'void (void *, ...)'
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
    clang::CallExpr* CE) {
  const clang::Decl* D = CE->getCalleeDecl();
  const clang::FunctionDecl* FD = clang::dyn_cast<clang::FunctionDecl>(D);

  if (FD == nullptr) {
    return nullptr;
  }

  const clang::StringRef& funcName = FD->getName();

  if (!funcName.equals(KERNEL_LAUNCH_FUNCTION_NAME)) {
    return nullptr;
  }

  const clang::FunctionDecl* kernel = matchFunctionDesignator(CE->getArg(0));

  if (kernel == nullptr ||
      CE->getNumArgs() < 3) {  // TODO: Make argument check more accurate
    mCtxt->ReportError(CE->getExprLoc(), "Invalid kernel launch call.");
  }

  return kernel;
}

// Create an AST node for the declaration of rsForEachInternal
clang::FunctionDecl* RSForEachLowering::CreateForEachInternalFunctionDecl() {
  const clang::QualType& AllocTy = mCtxt->getAllocationType();
  clang::DeclContext* DC = mASTCtxt.getTranslationUnitDecl();
  clang::SourceLocation Loc;

  llvm::StringRef SR(INTERNAL_LAUNCH_FUNCTION_NAME);
  clang::IdentifierInfo& II = mASTCtxt.Idents.get(SR);
  clang::DeclarationName N(&II);

  clang::FunctionProtoType::ExtProtoInfo EPI;

  clang::QualType T = mASTCtxt.getFunctionType(
      mASTCtxt.VoidTy,                     // Return type
      {mASTCtxt.IntTy, AllocTy, AllocTy},  // Argument types
      EPI);

  clang::FunctionDecl* FD = clang::FunctionDecl::Create(
      mASTCtxt, DC, Loc, Loc, N, T, nullptr, clang::SC_Extern);
  return FD;
}

// Create an expression like the following that references the rsForEachInternal to
// replace the callee in the original call expression that references rsParallelFor.
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
// rsParallelFor, and the arguments satisfy the restrictions on the
// rsParallelFor API. If so, replace the call with a rsForEachInternal call
// with the first argument replaced by the slot number of the kernel function
// referenced in the original first argument.
//
// See comments to the helper methods defined above for details.
void RSForEachLowering::VisitCallExpr(clang::CallExpr* CE) {
  const clang::FunctionDecl* kernel = matchKernelLaunchCall(CE);
  if (kernel == nullptr) {
    return;
  }

  clang::Expr* calleeNew = CreateCalleeExprForInternalForEach();
  CE->setCallee(calleeNew);

  const int slot = mCtxt->getForEachSlotNumber(kernel);
  const llvm::APInt APIntSlot(mASTCtxt.getTypeSize(mASTCtxt.IntTy), slot);
  const clang::Expr* arg0 = CE->getArg(0);
  const clang::SourceLocation Loc(arg0->getLocStart());
  clang::Expr* IntSlotNum =
      clang::IntegerLiteral::Create(mASTCtxt, APIntSlot, mASTCtxt.IntTy, Loc);
  CE->setArg(0, IntSlotNum);
}

void RSForEachLowering::VisitStmt(clang::Stmt* S) {
  for (clang::Stmt* Child : S->children()) {
    if (Child) {
      Visit(Child);
    }
  }
}

}  // namespace slang
