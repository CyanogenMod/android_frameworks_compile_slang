LOCAL_PATH := $(call my-dir)

# Shared library libslang for host
# ========================================================
include $(CLEAR_VARS)
include $(CLEAR_TBLGEN_VARS)

LLVM_ROOT_PATH := external/llvm
CLANG_ROOT_PATH := external/clang

include $(CLANG_ROOT_PATH)/clang.mk

LOCAL_MODULE := libslang
LOCAL_MODULE_TAGS := optional

LOCAL_MODULE_CLASS := SHARED_LIBRARIES

LOCAL_CFLAGS += -Wno-sign-promo

TBLGEN_TABLES :=    \
	AttrList.inc	\
	Attrs.inc	\
	DeclNodes.inc	\
	DiagnosticCommonKinds.inc	\
	DiagnosticFrontendKinds.inc	\
	DiagnosticSemaKinds.inc	\
	StmtNodes.inc

LOCAL_SRC_FILES :=	\
	slang.cpp	\
	slang_utils.cpp	\
	slang_backend.cpp	\
	slang_pragma_recorder.cpp	\
	slang_diagnostic_buffer.cpp

LOCAL_STATIC_LIBRARIES :=	\
	libLLVMLinker   \
	libLLVMipo	\
	libLLVMBitWriter	\
	libLLVMBitReader	\
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
	libLLVMInstrumentation	\
	libLLVMipa	\
	libLLVMAnalysis	\
	libLLVMTarget	\
	libLLVMMC	\
	libLLVMCore	\
	libclangParse	\
	libclangSema	\
	libclangAnalysis	\
	libclangAST	\
	libclangLex	\
	libclangFrontend	\
	libclangCodeGen	\
	libclangBasic	\
	libLLVMSupport	\
	libLLVMSystem

LOCAL_LDLIBS := -ldl -lpthread

include $(CLANG_HOST_BUILD_MK)
include $(CLANG_TBLGEN_RULES_MK)
include $(LLVM_GEN_INTRINSICS_MK)
include $(BUILD_HOST_SHARED_LIBRARY)

# Executable llvm-rs-link for host
# ========================================================
include $(CLEAR_VARS)
include $(CLEAR_TBLGEN_VARS)

include $(LLVM_ROOT_PATH)/llvm.mk

LOCAL_MODULE := llvm-rs-link

LOCAL_MODULE_CLASS := EXECUTABLES

LOCAL_SRC_FILES :=	\
	llvm-rs-link.cpp

LOCAL_SHARED_LIBRARIES :=	\
	libslang

LOCAL_LDLIBS := -ldl -lpthread

include $(LLVM_HOST_BUILD_MK)
include $(LLVM_GEN_INTRINSICS_MK)
include $(BUILD_HOST_EXECUTABLE)

# Host static library containing rs_types.rsh
# ========================================================
include $(CLEAR_VARS)

input_data_file := frameworks/base/libs/rs/scriptc/rs_types.rsh
slangdata_output_var_name := rs_types_header

LOCAL_IS_HOST_MODULE := true
LOCAL_MODULE := librsheader-types
LOCAL_MODULE_TAGS := optional

include $(LOCAL_PATH)/SlangData.mk
include $(BUILD_HOST_STATIC_LIBRARY)

# Host static library containing rs_cl.rsh
# ========================================================
include $(CLEAR_VARS)

input_data_file := frameworks/base/libs/rs/scriptc/rs_cl.rsh
slangdata_output_var_name := rs_cl_header

LOCAL_IS_HOST_MODULE := true
LOCAL_MODULE := librsheader-cl
LOCAL_MODULE_TAGS := optional

include $(LOCAL_PATH)/SlangData.mk
include $(BUILD_HOST_STATIC_LIBRARY)

# Host static library containing rs_cores.rsh
# ========================================================
include $(CLEAR_VARS)

input_data_file := frameworks/base/libs/rs/scriptc/rs_core.rsh
slangdata_output_var_name := rs_core_header

LOCAL_IS_HOST_MODULE := true
LOCAL_MODULE := librsheader-core
LOCAL_MODULE_TAGS := optional

include $(LOCAL_PATH)/SlangData.mk
include $(BUILD_HOST_STATIC_LIBRARY)

# Host static library containing rs_math.rsh
# ========================================================
include $(CLEAR_VARS)

input_data_file := frameworks/base/libs/rs/scriptc/rs_math.rsh
slangdata_output_var_name := rs_math_header

LOCAL_IS_HOST_MODULE := true
LOCAL_MODULE := librsheader-math
LOCAL_MODULE_TAGS := optional

include $(LOCAL_PATH)/SlangData.mk
include $(BUILD_HOST_STATIC_LIBRARY)

# Host static library containing rs_graphics.rsh
# ========================================================
include $(CLEAR_VARS)

input_data_file := frameworks/base/libs/rs/scriptc/rs_graphics.rsh
slangdata_output_var_name := rs_graphics_header

LOCAL_IS_HOST_MODULE := true
LOCAL_MODULE := librsheader-graphics
LOCAL_MODULE_TAGS := optional

include $(LOCAL_PATH)/SlangData.mk
include $(BUILD_HOST_STATIC_LIBRARY)

# Executable slang for host
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
	StmtNodes.inc	\
	DiagnosticSemaKinds.inc

LOCAL_SRC_FILES :=	\
	slang_driver.cpp	\
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

LOCAL_SHARED_LIBRARIES :=      \
	libslang

LOCAL_STATIC_LIBRARIES :=	\
	librsheader-types	\
	librsheader-cl  \
	librsheader-core	\
	librsheader-math	\
	librsheader-graphics

LOCAL_REQUIRED_MODULES := llvm-rs-link

include $(CLANG_HOST_BUILD_MK)
include $(CLANG_TBLGEN_RULES_MK)
include $(BUILD_HOST_EXECUTABLE)
