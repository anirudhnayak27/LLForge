include(AddMLIR)

set(dialect LLForgePass)

set(LLVM_TARGET_DEFINITIONS Passes.td)
mlir_tablegen(Passes.h.inc --gen-pass-decls -name LLForge)
add_public_tablegen_target(MLIR${dialect}IncGen)


get_property(dialect_libs GLOBAL PROPERTY MLIR_DIALECT_LIBS)

add_mlir_library(MLIRLLForgeTransforms
  OperationInsertion.cpp

  ADDITIONAL_HEADER_DIRS
  ${MLIR_MAIN_INCLUDE_DIR}
  
  DEPENDS
  MLIR${dialect}IncGen
  
  LINK_LIBS PUBLIC
  MLIRIR
  MLIRPass
  MLIRTransformUtils
  ${dialect_libs}
)
