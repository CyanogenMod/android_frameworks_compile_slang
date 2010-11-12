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

#ifndef _FRAMEWORKS_COMPILE_SLANG_SLANG_RS_OBJECT_REF_COUNT_H_  // NOLINT
#define _FRAMEWORKS_COMPILE_SLANG_SLANG_RS_OBJECT_REF_COUNT_H_

#include <list>
#include <stack>

#include "clang/AST/StmtVisitor.h"

#include "slang_rs_export_type.h"

namespace clang {
  class Expr;
}

namespace slang {

class RSObjectRefCount : public clang::StmtVisitor<RSObjectRefCount> {
 private:
  class Scope {
   private:
    clang::CompoundStmt *mCS;      // Associated compound statement ({ ... })
    std::list<clang::VarDecl*> mRSO;  // Declared RS objects in this scope

    // RSSetObjectFD and RSClearObjectFD holds FunctionDecl of rsSetObject()
    // and rsClearObject() in the current ASTContext.
    static clang::FunctionDecl *RSSetObjectFD[];
    static clang::FunctionDecl *RSClearObjectFD[];

   public:
    explicit Scope(clang::CompoundStmt *CS) : mCS(CS) {
      return;
    }

    inline void addRSObject(clang::VarDecl* VD) {
      mRSO.push_back(VD);
      return;
    }

    // Initialize RSSetObjectFD and RSClearObjectFD.
    static void GetRSRefCountingFunctions(clang::ASTContext &C);

    void InsertLocalVarDestructors();

    static clang::Expr *ClearRSObject(clang::VarDecl *VD);
  };

  std::stack<Scope*> mScopeStack;
  bool RSInitFD;

  inline Scope *getCurrentScope() {
    return mScopeStack.top();
  }

  // TODO(srhines): Composite types and arrays based on RS object types need
  // to be handled for both zero-initialization + clearing.

  // Return false if the type of variable declared in VD is not an RS object
  // type.
  static bool InitializeRSObject(clang::VarDecl *VD);

  // Return a zero-initializer expr of the type DT. This processes both
  // RS matrix type and RS object type.
  static clang::Expr *CreateZeroInitializerForRSSpecificType(
      RSExportPrimitiveType::DataType DT,
      clang::ASTContext &C,
      const clang::SourceLocation &Loc);

 public:
  RSObjectRefCount()
      : RSInitFD(false) {
    return;
  }

  void Init(clang::ASTContext &C) {
    if (!RSInitFD) {
      Scope::GetRSRefCountingFunctions(C);
      RSInitFD = true;
    }
    return;
  }

  void VisitStmt(clang::Stmt *S);
  void VisitDeclStmt(clang::DeclStmt *DS);
  void VisitCompoundStmt(clang::CompoundStmt *CS);
  void VisitBinAssign(clang::BinaryOperator *AS);

  // We believe that RS objects are never involved in CompoundAssignOperator.
  // I.e., rs_allocation foo; foo += bar;
};

}  // namespace slang

#endif  // _FRAMEWORKS_COMPILE_SLANG_SLANG_RS_OBJECT_REF_COUNT_H_  NOLINT
