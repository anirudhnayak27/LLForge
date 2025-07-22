include "mlir/IR/OpBase.td"
include "mlir/IR/DialectBase.td"

def LLForgeDialect: Dialect {
    let name = "llforge";
    let cppNamespace = "::mlir::LLFORGE";
}