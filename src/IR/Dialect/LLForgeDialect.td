include "mlir/IR/OpBase.td"
include "mlir/IR/DialectBase.td"

def llforge_dialect: Dialect {
    let name = "llforge";
    let cppNamespace = "::mlir::LLFORGE";
}