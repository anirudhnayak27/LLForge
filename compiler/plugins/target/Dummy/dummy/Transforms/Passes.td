#ifndef DUMMY_TRANSFORMS_PASSES_TD_
#define DUMMY_TRANSFORMS_PASSES_TD_

include "mlir/Pass/PassBase.td"

def DummyLowerToLLConv : Pass<"iree-dummy-lower-to-ll-conv"> {
    let summary = "Lowers linalg.generic convolution ops to ll.conv dialect ops";
    let constructor = "mlir::iree_compiler::Dummy::createLowerToLLConvPass()";
    let dependentDialects = [
        "mlir::iree_compiler::Dummy::LL::LLDialect",
        "mlir::linalg::LinalgDialect",
        "mlir::arith::ArithDialect",
        "mlir::tensor::TensorDialect"
    ];
}

#endif
