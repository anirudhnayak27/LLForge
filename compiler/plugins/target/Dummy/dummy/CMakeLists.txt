set(IREE_PACKAGE_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}")
set(IREE_PACKAGE_ROOT_PREFIX "iree::target::dummy")

iree_cc_library(
    NAME
        registration
    SRCS
        "DummyPluginRegistration.cpp"
    DEPS
        iree::compiler::Dialect::HAL::Target
        iree::compiler::PluginAPI
        iree::target::dummy::IR::LLDialect
        iree::target::dummy::Target
        iree::target::dummy::Transforms
        iree::base::core_headers
    PUBLIC
)

iree_compiler_register_plugin(
    PLUGIN_ID
        hal_target_dummy
    TARGET
        ::registration
)

add_subdirectory(IR)
add_subdirectory(Target)
add_subdirectory(Transforms)
