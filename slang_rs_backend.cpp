#include <vector>
#include <string>

#include "slang_rs_backend.hpp"
#include "slang_rs_context.hpp"
#include "slang_rs_export_var.hpp"
#include "slang_rs_export_func.hpp"
#include "slang_rs_export_type.hpp"

#include "llvm/Metadata.h"          /* for class llvm::NamedMDNode */
#include "llvm/ADT/Twine.h"         /* for class llvm::Twine */

#include "clang/AST/DeclGroup.h"    /* for class clang::DeclGroup */
#include "llvm/ADT/StringExtras.h"  /* for function llvm::utostr_32() and llvm::itostr() */

#include "llvm/Support/IRBuilder.h" /* for class llvm::IRBuilder */
#include "llvm/Constant.h"          /* for class llvm::Constant */
#include "llvm/Constants.h"          /* for class llvm::Constant* */
#include "llvm/Module.h"            /* for class llvm::Module */
#include "llvm/Function.h"          /* for class llvm::Function */
#include "llvm/DerivedTypes.h"      /* for class llvm::*Type */

#define MAKE

namespace slang {

RSBackend::RSBackend(RSContext* Context,
                     Diagnostic &Diags,
                     const CodeGenOptions& CodeGenOpts,
                     const TargetOptions& TargetOpts,
                     const PragmaList& Pragmas,
                     llvm::raw_ostream* OS,
                     SlangCompilerOutputTy OutputType,
                     SourceManager& SourceMgr) :
    mContext(Context),
    Backend(Diags,
            CodeGenOpts,
            TargetOpts,
            Pragmas,
            OS,
            OutputType,
            SourceMgr),
    mExportVarMetadata(NULL)
{
    return;
}

void RSBackend::HandleTopLevelDecl(DeclGroupRef D) {
    Backend::HandleTopLevelDecl(D);
    return;
}

void RSBackend::HandleTranslationUnitEx(ASTContext& Ctx) {
    assert((&Ctx == mContext->getASTContext()) && "Unexpected AST context change during LLVM IR generation");
    mContext->processExport();

    /* Dump export variable info */
    if(mContext->hasExportVar()) {
        if(mExportVarMetadata == NULL)
            mExportVarMetadata = llvm::NamedMDNode::Create(mLLVMContext, "#rs_export_var", NULL, 0, mpModule);

        llvm::SmallVector<llvm::Value*, 2> ExportVarInfo;

        for(RSContext::const_export_var_iterator I = mContext->export_vars_begin();
            I != mContext->export_vars_end();
            I++)
        {
            const RSExportVar* EV = *I;
            const RSExportType* ET = EV->getType();

            /* variable name */
            ExportVarInfo.push_back( llvm::MDString::get(mLLVMContext, EV->getName().c_str()) );

            /* type name */
            if(ET->getClass() == RSExportType::ExportClassPrimitive) 
                ExportVarInfo.push_back( llvm::MDString::get(mLLVMContext, llvm::utostr_32(static_cast<const RSExportPrimitiveType*>(ET)->getType())) );
            else if(ET->getClass() == RSExportType::ExportClassPointer)
                ExportVarInfo.push_back( llvm::MDString::get(mLLVMContext, ("*" + static_cast<const RSExportPointerType*>(ET)->getPointeeType()->getName()).c_str()) );
            else
                ExportVarInfo.push_back( llvm::MDString::get(mLLVMContext, EV->getType()->getName().c_str()) );

            mExportVarMetadata->addOperand( llvm::MDNode::get(mLLVMContext, ExportVarInfo.data(), ExportVarInfo.size()) );

            ExportVarInfo.clear();
        }
    }

    /* Dump export function info */
    if(mContext->hasExportFunc()) {
        if(mExportFuncMetadata == NULL)
            mExportFuncMetadata = llvm::NamedMDNode::Create(mLLVMContext, "#rs_export_func", NULL, 0, mpModule);

        llvm::SmallVector<llvm::Value*, 1> ExportFuncInfo;

        for(RSContext::const_export_func_iterator I = mContext->export_funcs_begin();
            I != mContext->export_funcs_end();
            I++)
        {
            const RSExportFunc* EF = *I;

            /* function name */
            if(!EF->hasParam()) 
                ExportFuncInfo.push_back( llvm::MDString::get(mLLVMContext, EF->getName().c_str()) );
            else {
                llvm::Function* F = mpModule->getFunction(EF->getName());
                llvm::Function* HelperFunction;
                const std::string HelperFunctionName = ".helper_" + EF->getName();

                assert(F && "Function marked as exported disappeared in Bitcode");

                /* Create helper function */
                {
                    llvm::PointerType* HelperFunctionParameterTypeP = llvm::PointerType::getUnqual(EF->getParamPacketType()->getLLVMType());
                    llvm::FunctionType* HelperFunctionType;
                    std::vector<const llvm::Type*> Params;

                    Params.push_back(HelperFunctionParameterTypeP);
                    HelperFunctionType = llvm::FunctionType::get(F->getReturnType(), Params, false);

                    HelperFunction = llvm::Function::Create(HelperFunctionType, llvm::GlobalValue::ExternalLinkage, HelperFunctionName, mpModule);

                    HelperFunction->addFnAttr(llvm::Attribute::NoInline);
                    HelperFunction->setCallingConv(F->getCallingConv());

                    /* Create helper function body */
                    {
                        llvm::Argument* HelperFunctionParameter = &(*HelperFunction->arg_begin());
                        llvm::BasicBlock* BB = llvm::BasicBlock::Create(mLLVMContext, "entry", HelperFunction);
                        llvm::IRBuilder<>* IB = new llvm::IRBuilder<>(BB);
                        llvm::SmallVector<llvm::Value*, 6> Params;
                        llvm::Value* Idx[2];

                        Idx[0] = llvm::ConstantInt::get(llvm::Type::getInt32Ty(mLLVMContext), 0);

                        /* getelementptr and load instruction for all elements in parameter .p */
                        for(int i=0;i<EF->getNumParameters();i++) {
                            /* getelementptr */
                            Idx[1] = llvm::ConstantInt::get(llvm::Type::getInt32Ty(mLLVMContext), i);
                            llvm::Value* Ptr = IB->CreateInBoundsGEP(HelperFunctionParameter, Idx, Idx + 2); 
                            /* load */
                            llvm::Value* V = IB->CreateLoad(Ptr);
                            Params.push_back(V);
                        }

                        /* call and pass the all elements as paramter to F */
                        llvm::CallInst* CI = IB->CreateCall(F, Params.data(), Params.data() + Params.size());
                        CI->setCallingConv(F->getCallingConv());

                        if(F->getReturnType() == llvm::Type::getVoidTy(mLLVMContext))
                            IB->CreateRetVoid();
                        else
                            IB->CreateRet(CI);

                        delete IB;
                    }
                }

                ExportFuncInfo.push_back( llvm::MDString::get(mLLVMContext, HelperFunctionName.c_str()) );
            }

            mExportFuncMetadata->addOperand( llvm::MDNode::get(mLLVMContext, ExportFuncInfo.data(), ExportFuncInfo.size()) );

            ExportFuncInfo.clear();
        }
    }

    /* Dump export type info */
    if(mContext->hasExportType()) {
        llvm::SmallVector<llvm::Value*, 1> ExportTypeInfo;

        for(RSContext::const_export_type_iterator I = mContext->export_types_begin();
            I != mContext->export_types_end();
            I++)
        {
            /* First, dump type name list to export */
            const RSExportType* ET = I->getValue();

            ExportTypeInfo.clear();
            /* type name */
            ExportTypeInfo.push_back( llvm::MDString::get(mLLVMContext, ET->getName().c_str()) );

            if(ET->getClass() == RSExportType::ExportClassRecord) {
                const RSExportRecordType* ERT = static_cast<const RSExportRecordType*>(ET);

                if(mExportTypeMetadata == NULL)
                    mExportTypeMetadata = llvm::NamedMDNode::Create(mLLVMContext, "#rs_export_type", NULL, 0, mpModule);

                mExportTypeMetadata->addOperand( llvm::MDNode::get(mLLVMContext, ExportTypeInfo.data(), ExportTypeInfo.size()) );

                /* Now, export struct field information to %[struct name] */
                std::string StructInfoMetadataName = "%" + ET->getName();
                llvm::NamedMDNode* StructInfoMetadata = llvm::NamedMDNode::Create(mLLVMContext, StructInfoMetadataName.c_str(), NULL, 0, mpModule);
                llvm::SmallVector<llvm::Value*, 3> FieldInfo;

                assert(StructInfoMetadata->getNumOperands() == 0 && "Metadata with same name was created before");
                for(RSExportRecordType::const_field_iterator FI = ERT->fields_begin();
                    FI != ERT->fields_end();
                    FI++)
                {
                    const RSExportRecordType::Field* F = *FI;

                    /* 1. field name */
                    FieldInfo.push_back( llvm::MDString::get(mLLVMContext, F->getName().c_str()) );
                    /* 2. field type name */
                    FieldInfo.push_back( llvm::MDString::get(mLLVMContext, F->getType()->getName().c_str()) );

                    /* 3. field kind */
                    switch(F->getType()->getClass()) {
                        case RSExportType::ExportClassPrimitive:
                        case RSExportType::ExportClassVector:
                        {
                            RSExportPrimitiveType* EPT = (RSExportPrimitiveType*) F->getType();
                            FieldInfo.push_back( llvm::MDString::get(mLLVMContext, llvm::itostr(EPT->getKind())) );
                        }
                        break;

                        default:
                            FieldInfo.push_back( llvm::MDString::get(mLLVMContext, llvm::itostr(RSExportPrimitiveType::DataKindUser)) );
                        break;
                    }

                    StructInfoMetadata->addOperand( llvm::MDNode::get(mLLVMContext, FieldInfo.data(), FieldInfo.size()) );

                    FieldInfo.clear();
                }
            }   /* ET->getClass() == RSExportType::ExportClassRecord */
        }
    }

    return;
}

RSBackend::~RSBackend() {
    return;
}

}   /* namespace slang */
