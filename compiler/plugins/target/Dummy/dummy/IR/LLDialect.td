#ifndef LL_DIALECT_TD_
#define LL_DIALECT_TD_

include "mlir/IR/OpBase.td"
include "mlir/IR/DialectBase.td"

def LL_Dialect : Dialect {
    let name = "ll";
    let cppNamespace = "::mlir::iree_compiler::Dummy::LL";

    let description = [{
        Low-level dialect for Dummy hardware operations.
        Captures convolution data lowered from linalg.generic.
    }];

    let extraClassDeclaration = [{
        void initializeLLOps();
    }];
}

#endif
