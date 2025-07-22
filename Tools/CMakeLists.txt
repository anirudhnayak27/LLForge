get_property(dialect_libs GLOBAL PROPERTY MLIR_DIALECT_LIBS)
get_property(conversion_libs GLOBAL PROPERTY MLIR_CONVERSION_LIBS)

add_llvm_executable(llforge-opt
  LLForgeOpt.cpp
)

llvm_update_compile_flags(llforge-opt)
target_link_libraries(llforge-opt
  PRIVATE
  ${dialect_libs}  
  ${conversion_libs}  
  MLIROptLib
  MLIRLLForgeTransforms
  MLIRLLForge
)