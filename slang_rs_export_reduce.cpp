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

#include "slang_rs_export_reduce.h"

#include <algorithm>
#include <string>

#include "clang/AST/Attr.h"
#include "clang/AST/ASTContext.h"

#include "slang_assert.h"
#include "slang_rs_context.h"
#include "slang_rs_export_type.h"
#include "slang_version.h"


namespace {

bool haveReduceInTargetAPI(unsigned int TargetAPI) {
  return TargetAPI == RS_DEVELOPMENT_API;
}

} // end anonymous namespace


namespace slang {

// Validate the parameters to a reduce kernel, and set up the
// exportable object if the kernel is valid.
//
// This checks that the passed function declaration of a reduce kernel is
// a function which satisfies all the requirements for a reduce
// kernel. Namely, we check for:
//  - correct target API
//  - correct parameter count
//  - non void return type
//  - return type and parameter types match
//  - no pointer types in signature.
//
// We try to report useful errors to the user.
//
// On success, this function returns true and sets the fields mIns and
// mType to point to the arguments and to the kernel type.
//
// If an error was detected, this function returns false.
bool RSExportReduce::validateAndConstructParams(
    RSContext *Context, const clang::FunctionDecl *FD) {
  slangAssert(Context && FD);
  bool Valid = true;

  clang::ASTContext &ASTCtx = FD->getASTContext();

  // Validate API version.
  if (!haveReduceInTargetAPI(Context->getTargetAPI())) {
    Context->ReportError(FD->getLocation(),
                         "Reduce-style kernel %0() unsupported in SDK level %1")
      << FD->getName() << Context->getTargetAPI();
    Valid = false;
  }

  // Validate parameter count.
  if (FD->getNumParams() != 2) {
    Context->ReportError(FD->getLocation(),
                         "Reduce-style kernel %0() must take 2 parameters "
                         "(found %1).")
      << FD->getName() << FD->getNumParams();
    Valid = false;
  }

  // Validate return type.
  const clang::QualType ReturnTy = FD->getReturnType().getCanonicalType();

  if (ReturnTy->isVoidType()) {
    Context->ReportError(FD->getLocation(),
                         "Reduce-style kernel %0() cannot return void")
      << FD->getName();
    Valid = false;
  } else if (ReturnTy->isPointerType()) {
    Context->ReportError(FD->getLocation(),
                         "Reduce-style kernel %0() cannot return a pointer "
                         "type: %1")
      << FD->getName() << ReturnTy.getAsString();
    Valid = false;
  }

  // Validate parameter types.
  if (FD->getNumParams() == 0) {
    return false;
  }

  const clang::ParmVarDecl &FirstParam = *FD->getParamDecl(0);
  const clang::QualType FirstParamTy = FirstParam.getType().getCanonicalType();

  for (auto PVD = FD->param_begin(), PE = FD->param_end(); PVD != PE; ++PVD) {
    const clang::ParmVarDecl &Param = **PVD;
    const clang::QualType ParamTy = Param.getType().getCanonicalType();

    // Check that the parameter is not a pointer.
    if (ParamTy->isPointerType()) {
      Context->ReportError(Param.getLocation(),
                           "Reduce-style kernel %0() cannot have "
                           "parameter '%1' of pointer type: '%2'")
        << FD->getName() << Param.getName() << ParamTy.getAsString();
      Valid = false;
    }

    // Check for type mismatch between this parameter and the return type.
    if (!ASTCtx.hasSameUnqualifiedType(ReturnTy, ParamTy)) {
      Context->ReportError(FD->getLocation(),
                           "Reduce-style kernel %0() return type '%1' is not "
                           "the same type as parameter '%2' (type '%3')")
        << FD->getName() << ReturnTy.getAsString() << Param.getName()
        << ParamTy.getAsString();
      Valid = false;
    }

    // Check for type mismatch between parameters. It is sufficient to check
    // for a mismatch with the type of the first argument.
    if (ParamTy != FirstParamTy) {
      Context->ReportError(FirstParam.getLocation(),
                           "In reduce-style kernel %0(): parameter '%1' "
                           "(type '%2') does not have the same type as "
                           "parameter '%3' (type '%4')")
        << FD->getName() << FirstParam.getName() << FirstParamTy.getAsString()
        << Param.getName() << ParamTy.getAsString();
      Valid = false;
    }
  }

  if (Valid) {
    // If the validation was successful, then populate the fields of
    // the exportable.
    if (!(mType = RSExportType::Create(Context, ReturnTy.getTypePtr()))) {
      // There was an error exporting the type for the reduce kernel.
      return false;
    }

    slangAssert(mIns.size() == 2 && FD->param_end() - FD->param_begin() == 2);
    std::copy(FD->param_begin(), FD->param_end(), mIns.begin());
  }

  return Valid;
}

RSExportReduce *RSExportReduce::Create(RSContext *Context,
                                       const clang::FunctionDecl *FD) {
  slangAssert(Context && FD);
  llvm::StringRef Name = FD->getName();

  slangAssert(!Name.empty() && "Function must have a name");

  RSExportReduce *RE = new RSExportReduce(Context, Name);

  if (!RE->validateAndConstructParams(Context, FD)) {
    // Don't delete RE here - owned by Context.
    return nullptr;
  }

  return RE;
}

bool RSExportReduce::isRSReduceFunc(unsigned int /* targetAPI */,
                                    const clang::FunctionDecl *FD) {
  slangAssert(FD);
  clang::KernelAttr *KernelAttrOrNull = FD->getAttr<clang::KernelAttr>();
  return KernelAttrOrNull && KernelAttrOrNull->getKernelKind().equals("reduce");
}

}  // namespace slang
