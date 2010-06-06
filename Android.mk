LOCAL_PATH := $(call my-dir)
LLVM_ROOT_PATH := external/llvm

include $(LLVM_ROOT_PATH)/llvm.mk

# Executable for host
# ========================================================
include $(CLEAR_VARS)
include $(CLEAR_TBLGEN_VARS)

LOCAL_MODULE := slang

LOCAL_MODULE_CLASS := EXECUTABLES

TBLGEN_TABLES :=    \
	DiagnosticCommonKinds.inc	\
	DiagnosticFrontendKinds.inc

LOCAL_SRC_FILES :=	\
	slang_driver.cpp	\
	libslang.cpp	\
	slang.cpp	\
	slang_backend.cpp	\
	slang_pragma_recorder.cpp	\
	slang_diagnostic_buffer.cpp	\
	slang_rs_context.cpp	\
	slang_rs_pragma_handler.cpp	\
	slang_rs_backend.cpp	\
	slang_rs_export_type.cpp	\
	slang_rs_export_element.cpp	\
	slang_rs_export_var.cpp	\
	slang_rs_export_func.cpp	\
	slang_rs_reflection.cpp

LOCAL_STATIC_LIBRARIES :=	\
	libLLVMipo	\
	libLLVMBitWriter	\
	libLLVMARMAsmPrinter	\
   	libLLVMX86AsmPrinter	\
	libLLVMAsmPrinter	\
	libLLVMMCParser	\
	libLLVMARMCodeGen	\
	libLLVMARMInfo	\
	libLLVMX86CodeGen	\
	libLLVMX86Info	\
	libLLVMSelectionDAG	\
	libLLVMCodeGen	\
	libLLVMScalarOpts	\
	libLLVMInstCombine	\
	libLLVMTransformUtils	\
	libLLVMipa	\
	libLLVMAnalysis	\
	libLLVMTarget	\
	libLLVMMC	\
	libLLVMCore	\
	libclangIndex	\
	libclangSema	\
	libclangAnalysis	\
	libclangAST	\
	libclangParse	\
	libclangLex	\
	libclangCodeGen	\
	libclangBasic	\
	libLLVMSupport	\
	libLLVMSystem

LOCAL_LDLIBS := -ldl -lpthread

include $(CLANG_HOST_BUILD_MK)
include $(CLANG_TBLGEN_RULES_MK)
include $(BUILD_HOST_EXECUTABLE)
