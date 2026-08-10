set(IREE_PACKAGE_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}")
set(IREE_PACKAGE_ROOT_PREFIX "iree::target::dummy::Transforms")
iree_add_all_subdirs()

iree_tablegen_library(
    NAME
        PassesGenInc
    TD_FILE
        "Passes.td"
    OUTS
        --gen-pass-decls Passes.h.inc
)

iree_cc_library(
    NAME
        PassHeaders
    HDRS
        "PassDetail.h"
        "Passes.h"
        "Passes.h.inc"
    DEPS
        ::PassesGenInc
        iree::target::dummy::IR::LLDialect
        MLIRPass
        MLIRIR
        MLIRLinalgDialect
        MLIRAffineDialect
        MLIRFuncDialect
        MLIRTensorDialect
        MLIRArithDialect
        MLIRMathDialect
    PUBLIC
)

iree_cc_library(
    NAME
        Transforms
    HDRS
        "Passes.h"
    SRCS
        "Passes.cpp"
        "LowerToLLConv.cpp"
    DEPS
        ::PassHeaders
        ::PassesGenInc
        iree::target::dummy::IR::LLDialect
        MLIRPass
        MLIRIR
        MLIRSupport
        MLIRLinalgDialect
        MLIRLinalgUtils
        MLIRAffineDialect
        MLIRFuncDialect
        MLIRTensorDialect
        MLIRArithDialect
        MLIRMathDialect
        MLIRTransformUtils
    PUBLIC
)
