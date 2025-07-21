include "mlir/IR/OpBase.td"
include "LLForgeDialect.td"

class llforge_dialect_op<string mnemonic, list<Trait> traits = []>
    : Op<llforge_dialect, mnemonic, traits>;

def LLForgeWriteOp : llforge_dialect_op<"write", []> {
  let arguments = (ins I32 : $val);
  let assemblyFormat = "$val attr-dict `:` type($val)";
}