#include "slang_rs_context.hpp"
#include "slang_rs_export_func.hpp"
#include "slang_rs_export_type.hpp"

#include "llvm/Target/TargetData.h"

#include "clang/AST/Decl.h"

using namespace slang;

RSExportFunc *RSExportFunc::Create(RSContext *Context,
                                   const clang::FunctionDecl *FD) {
  llvm::StringRef Name = FD->getName();
  RSExportFunc *F;

  assert(!Name.empty() && "Function must have a name");

  F = new RSExportFunc(Context, Name);

  // Check whether the parameters passed to the function is exportable
  for(unsigned i = 0;i < FD->getNumParams(); i++) {
    const clang::ParmVarDecl *PVD = FD->getParamDecl(i);
    const llvm::StringRef ParamName = PVD->getName();

    assert(!ParamName.empty() && "Parameter must have a name");

    if (PVD->hasDefaultArg())
      fprintf(stderr,
              "Note: parameter '%s' in function '%s' has default value "
              "will not support\n",
              ParamName.str().c_str(),
              Name.str().c_str());

    // Check type
    RSExportType *PET = RSExportType::CreateFromDecl(Context, PVD);
    if (PET != NULL) {
      F->mParams.push_back(new Parameter(PET, ParamName));
    } else {
      fprintf(stderr, "Note: parameter '%s' in function '%s' uses unsupported "
                      "type\n", ParamName.str().c_str(), Name.str().c_str());
      delete F;
      return NULL;
    }
  }

  return F;
}

const RSExportRecordType *RSExportFunc::getParamPacketType() const {
  // Pack parameters
  if ((mParamPacketType == NULL) && hasParam()) {
    int Index = 0;
    RSExportRecordType *ParamPacketType =
        new RSExportRecordType(mContext,
                               "",
                               /* IsPacked = */false,
                               /* IsArtificial = */true);

    for (const_param_iterator PI = params_begin(),
             PE = params_end();
         PI != PE;
         PI++, Index++) {
      //For-Loop's body should be:
      const RSExportFunc::Parameter* P = *PI;
      std::string nam = P->getName();
      const RSExportType* typ = P->getType();
      std::string typNam = typ->getName();
      // If (type conversion is needed)
      if (typNam.find("rs_") == 0) {
        // P's type set to [1 x i32];
        RSExportConstantArrayType* ECT = new RSExportConstantArrayType
            (mContext, "addObj",
             RSExportPrimitiveType::DataTypeSigned32,
             RSExportPrimitiveType::DataKindUser,
             false,
             1);
        ParamPacketType->mFields.push_back(
            new RSExportRecordType::Field(ECT,
                                          nam,
                                          ParamPacketType,
                                          Index) );
      } else {
        ParamPacketType->mFields.push_back(
            new RSExportRecordType::Field(P->getType(),
                                          nam,
                                          ParamPacketType,
                                          Index) );
      }

      //      ParamPacketType->mFields.push_back(
      //          new RSExportRecordType::Field((*PI)->getType(),
      //                                        (*PI)->getName(),
      //                                        ParamPacketType,
      //                                        Index
      //                                        ));
    }



    ParamPacketType->AllocSize =
        mContext->getTargetData()->getTypeAllocSize(
            ParamPacketType->getLLVMType()
                                                    );

    mParamPacketType = ParamPacketType;
  }

  return mParamPacketType;
}

RSExportFunc::~RSExportFunc() {
  for (const_param_iterator PI = params_begin(),
           PE = params_end();
       PI != params_end();
       PI++)
    delete *PI;

  if(mParamPacketType != NULL)
    delete mParamPacketType;

  return;
}
