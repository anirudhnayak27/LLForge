include "mlir/Pass/PassBase.td"

def OperationInsertion : Pass<"operation-insertion"> {
    let constructor = "mlir::LLFORGE::createOperationInsertion()";
}