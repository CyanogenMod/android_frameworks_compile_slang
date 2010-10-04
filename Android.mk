LOCAL_PATH := $(call my-dir)

LLVM_ROOT_PATH := external/llvm
include $(LLVM_ROOT_PATH)/llvm.mk

CLANG_ROOT_PATH := external/clang
include $(CLANG_ROOT_PATH)/clang.mk

# Executable for host
# ========================================================
include $(CLEAR_VARS)
include $(CLEAR_TBLGEN_VARS)

LOCAL_MODULE := llvm-rs-link

LOCAL_MODULE_CLASS := EXECUTABLES

LOCAL_SRC_FILES :=	\
	llvm-rs-link.cpp

LOCAL_STATIC_LIBRARIES :=	\
	libLLVMLinker   \
	libLLVMipo	\
	libLLVMBitWriter	\
	libLLVMBitReader        \
	libLLVMScalarOpts	\
	libLLVMInstCombine	\
	libLLVMTransformUtils	\
	libLLVMipa	\
	libLLVMAnalysis	\
	libLLVMTarget	\
	libLLVMCore	\
	libLLVMSupport	\
	libLLVMSystem

LOCAL_LDLIBS := -ldl -lpthread

include $(LLVM_HOST_BUILD_MK)
include $(LLVM_GEN_INTRINSICS_MK)
include $(BUILD_HOST_EXECUTABLE)

# Executable for host
# ========================================================
include $(CLEAR_VARS)
include $(CLEAR_TBLGEN_VARS)

LOCAL_MODULE := slang

LOCAL_MODULE_CLASS := EXECUTABLES

LOCAL_CFLAGS += -Wno-sign-promo

TBLGEN_TABLES :=    \
	AttrList.inc    \
	Attrs.inc    \
	DeclNodes.inc    \
	DiagnosticCommonKinds.inc   \
	DiagnosticFrontendKinds.inc \
	StmtNodes.inc	\
	DiagnosticSemaKinds.inc

LOCAL_SRC_FILES :=	\
	slang_driver.cpp	\
	slang.cpp	\
	slang_backend.cpp	\
	slang_pragma_recorder.cpp	\
	slang_diagnostic_buffer.cpp	\
	slang_rs.cpp	\
	slang_rs_context.cpp	\
	slang_rs_pragma_handler.cpp	\
	slang_rs_backend.cpp	\
	slang_rs_export_type.cpp	\
	slang_rs_export_element.cpp	\
	slang_rs_export_var.cpp	\
	slang_rs_export_func.cpp	\
	slang_rs_reflection.cpp \
	slang_rs_reflect_utils.cpp

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
	libclangParse   \
	libclangSema	\
	libclangAnalysis	\
	libclangAST	\
	libclangLex	\
	libclangCodeGen	\
	libclangBasic	\
	libclangFrontend	\
	libLLVMSupport	\
	libLLVMSystem

LOCAL_LDLIBS := -ldl -lpthread

LOCAL_REQUIRED_MODULES := llvm-rs-link

include $(CLANG_HOST_BUILD_MK)
include $(CLANG_TBLGEN_RULES_MK)
include $(BUILD_HOST_EXECUTABLE)
