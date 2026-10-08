set(IREE_PACKAGE_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}")
set(IREE_PACKAGE_ROOT_PREFIX "iree::target::dummy::Encoding")
iree_add_all_subdirs()

iree_cc_library(
    NAME
        ExternalModels
    HDRS
        "LLEncodingExternalModels.h"
    SRCS
        "LLEncodingExternalModels.cpp"
    DEPS
        iree::compiler::Codegen::Dialect::Codegen::IR::IREECodegenDialect
        iree::compiler::Codegen::ExternalInterfaces::ExternalModels
        iree::compiler::Dialect::Encoding::IR
        iree::compiler::Dialect::Encoding::Utils
        iree::target::dummy::IR::LLDialect
        MLIRArithDialect
        MLIRIR
        MLIRLinalgDialect
        MLIRTensorDialect
    PUBLIC
)
