#include "slang_rs_context.hpp"
#include "slang_rs_export_func.hpp"
#include "slang_rs_export_type.hpp"

#include "llvm/Target/TargetData.h" /* for class llvm::TargetData */

#include "clang/AST/Decl.h"         /* for clang::*Decl */

namespace slang {

RSExportFunc* RSExportFunc::Create(RSContext* Context, const FunctionDecl* FD) {
    llvm::StringRef Name = FD->getName();
    RSExportFunc* F;

    assert(!Name.empty() && "Function must have a name");

    F = new RSExportFunc(Context, Name);

    /* Check whether the parameters passed to the function is exportable */
    for(int i=0;i<FD->getNumParams();i++) {
        const ParmVarDecl* PVD = FD->getParamDecl(i);
        const llvm::StringRef ParamName = PVD->getName();

        assert(!ParamName.empty() && "Parameter must have a name");

        if(PVD->hasDefaultArg())
            printf("Note: parameter '%s' in function '%s' has default value will not support\n", ParamName.str().c_str(), Name.str().c_str());

        /* Check type */
        RSExportType* PET = RSExportType::CreateFromDecl(Context, PVD);
        if(PET != NULL) {
            F->mParams.push_back(new Parameter(PET, ParamName));
        } else {
            printf("Note: parameter '%s' in function '%s' uses unsupported type\n", ParamName.str().c_str(), Name.str().c_str());
            delete F;
            return NULL;
        }
    }

    return F;
}

const RSExportRecordType* RSExportFunc::getParamPacketType() const {
    /* Pack parameters */
    if((mParamPacketType == NULL) && hasParam()) {
        RSExportRecordType* ParamPacketType = new RSExportRecordType(mContext, "", false /* IsPacked */, true /* IsArtificial */);
        int Index = 0;

        for(const_param_iterator PI = params_begin();
            PI != params_end();
            PI++, Index++) {
          const RSExportFunc::Parameter* P = *PI;
          std::string nam = P->getName();
          const RSExportType* typ = P->getType();
          std::string typNam = typ->getName();
          /* If (type conversion is needed) */
          if (typNam.find("rs_") == 0) {
            /* P's type set to [1 x i32]; */
            RSExportConstantArrayType* ECT = new RSExportConstantArrayType(mContext, "addObj",
                                                                           RSExportPrimitiveType::DataTypeSigned32,
                                                                           RSExportPrimitiveType::DataKindUser,
                                                                           false,
                                                                           1);
            //printf("datatype = %d\n", ECT->getType());
            ParamPacketType->mFields.push_back( new RSExportRecordType::Field(ECT, nam, ParamPacketType, Index) );
          } else {
            ParamPacketType->mFields.push_back( new RSExportRecordType::Field(P->getType(), nam, ParamPacketType, Index) );
          }
        }

        ParamPacketType->AllocSize = mContext->getTargetData()->getTypeAllocSize(ParamPacketType->getLLVMType());

        mParamPacketType = ParamPacketType;
    }

    return mParamPacketType;
}

RSExportFunc::~RSExportFunc() {
    for(const_param_iterator PI = params_begin();
        PI != params_end();
        PI++)
        delete *PI;

    if(mParamPacketType != NULL)
        delete mParamPacketType;

    return;
}

}   /* namespace slang */
