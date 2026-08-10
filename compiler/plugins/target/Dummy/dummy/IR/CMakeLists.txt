set(IREE_PACKAGE_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}")
set(IREE_PACKAGE_ROOT_PREFIX "iree::target::dummy::IR")
iree_add_all_subdirs()

iree_tablegen_library(
    NAME
        LLDialectGenInc
    TD_FILE
        "LLDialect.td"
    OUTS
        --gen-dialect-decls LLDialect.h.inc
        --gen-dialect-defs LLDialect.cpp.inc
)

iree_tablegen_library(
    NAME
        LLOpsGenInc
    TD_FILE
        "LLOps.td"
    OUTS
        --gen-op-decls LLOps.h.inc
        --gen-op-defs LLOps.cpp.inc
)

iree_cc_library(
    NAME
        LLDialect
    HDRS
        "LLDialect.h"
        "LLOps.h"
    TEXTUAL_HDRS
        "LLDialect.h.inc"
        "LLDialect.cpp.inc"
        "LLOps.h.inc"
        "LLOps.cpp.inc"
    SRCS
        "LLDialect.cpp"
        "LLOps.cpp"
    DEPS
        ::LLDialectGenInc
        ::LLOpsGenInc
        MLIRIR
        MLIRParser
        MLIRSupport
    PUBLIC
)
