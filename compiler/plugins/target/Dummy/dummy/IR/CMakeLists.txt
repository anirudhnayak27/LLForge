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
        LLAttrsGenInc
    TD_FILE
        "LLAttrs.td"
    OUTS
        --gen-attrdef-decls --attrdefs-dialect=ll LLAttrs.h.inc
        --gen-attrdef-defs --attrdefs-dialect=ll LLAttrs.cpp.inc
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
        "LLAttrs.h"
        "LLDialect.h"
        "LLOps.h"
    TEXTUAL_HDRS
        "LLAttrs.h.inc"
        "LLAttrs.cpp.inc"
        "LLDialect.h.inc"
        "LLDialect.cpp.inc"
        "LLOps.h.inc"
        "LLOps.cpp.inc"
    SRCS
        "LLAttrs.cpp"
        "LLDialect.cpp"
        "LLOps.cpp"
    DEPS
        ::LLAttrsGenInc
        ::LLDialectGenInc
        ::LLOpsGenInc
        MLIRIR
        MLIRParser
        MLIRSupport
    PUBLIC
)
