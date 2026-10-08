set(IREE_PACKAGE_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}")
set(IREE_PACKAGE_ROOT_PREFIX "iree::target::dummy")
iree_add_all_subdirs()

iree_cc_library(
    NAME
        Target
    HDRS
        "DummyTarget.h"
    SRCS
        "DummyTarget.cpp"
    DEPS
        iree::compiler::Dialect::Encoding::IR
        iree::compiler::Dialect::HAL::Target
        iree::target::dummy::IR::LLDialect
        iree::target::dummy::Transforms
    PUBLIC
)
