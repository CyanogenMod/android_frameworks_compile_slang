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

#include "slang_rs_export_var.h"

#include "clang/AST/Type.h"

#include "llvm/ADT/APSInt.h"

#include "slang_assert.h"
#include "slang_rs_context.h"
#include "slang_rs_export_type.h"

namespace slang {

RSExportVar::RSExportVar(RSContext *Context,
                         const clang::VarDecl *VD,
                         const RSExportType *ET)
    : RSExportable(Context, RSExportable::EX_VAR),
      mName(VD->getName().data(), VD->getName().size()),
      mET(ET),
      mIsConst(false) {
  // mInit - Evaluate initializer expression
  const clang::Expr *Initializer = VD->getAnyInitializer();
  if (Initializer != NULL) {
    switch (ET->getClass()) {
      case RSExportType::ExportClassPrimitive:
      case RSExportType::ExportClassVector: {
        Initializer->Evaluate(mInit, Context->getASTContext());
        break;
      }
      case RSExportType::ExportClassPointer: {
        if (Initializer->isNullPointerConstant
            (Context->getASTContext(),
             clang::Expr::NPC_ValueDependentIsNotNull)
            )
          mInit.Val = clang::APValue(llvm::APSInt(1));
        else
          Initializer->Evaluate(mInit, Context->getASTContext());
        break;
      }
      case RSExportType::ExportClassRecord: {
        // No action
        fprintf(stderr, "RSExportVar::RSExportVar : Reflection of initializer "
                        "to variable '%s' (of type '%s') is unsupported "
                        "currently.\n",
                mName.c_str(),
                ET->getName().c_str());
        break;
      }
      default: {
        slangAssert(false && "Unknown class of type");
      }
    }
  }

  // mIsConst - Is it a constant?
  clang::QualType QT = VD->getTypeSourceInfo()->getType();
  if (!QT.isNull()) {
    mIsConst = QT.isConstQualified();
  }

  return;
}

}  // namespace slang
