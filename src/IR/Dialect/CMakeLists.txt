set(CMAKE_CURRENT_BINARY_DIR ${CMAKE_CURRENT_SOURCE_DIR})

set(dialect llforgedialect)
set(dialect_namespace llforge)

# Dialect definition generation
set(LLVM_TARGET_DEFINITIONS LLForgeDialect.td)
mlir_tablegen(./Gen_Files/LLForgeDialect.h.inc -gen-dialect-decls -dialect=${dialect_namespace})
mlir_tablegen(./Gen_Files/LLForgeDialect.cpp.inc -gen-dialect-defs -dialect=${dialect_namespace})

set(LLVM_TARGET_DEFINITIONS LLForgeOps.td)
mlir_tablegen(./Gen_Files/LLForgeOps.h.inc -gen-op-decls)
mlir_tablegen(./Gen_Files/LLForgeOps.cpp.inc -gen-op-defs)

add_public_tablegen_target(MLIR${dialect}IncGen)

add_mlir_dialect_library(MLIRLLForge
        LLForgeDialect.cpp
        LLForgeOps.cpp
        PARTIAL_SOURCES_INTENDED

        DEPENDS
        MLIR${dialect}IncGen

        LINK_LIBS PUBLIC

        MLIRAnalysis
        MLIRCallInterfaces
        MLIRCastInterfaces
        MLIRExecutionEngine
        MLIRIR
        MLIRTosaDialect
        MLIRLLVMCommonConversion
        MLIRLLVMDialect
        MLIRLLVMToLLVMIRTranslation
        MLIRMemRefDialect
        MLIRParser
        MLIRPass
        MLIRSideEffectInterfaces
        MLIRSupport
        MLIRTargetLLVMIRExport
        MLIRTransforms
)
