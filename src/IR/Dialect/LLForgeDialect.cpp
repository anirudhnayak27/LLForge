#include "LLForgeDialect.h"

using namespace mlir;
using namespace mlir::LLFORGE;

MLIR_DEFINE_EXPLICIT_TYPE_ID(mlir::LLFORGE::llforgedialect);

void llforgedialect::initialize() { 
    addOperations<LLForgeWriteOp>();
}

llforgedialect::llforgedialect(MLIRContext *ctx)
    : Dialect(getDialectNamespace(), ctx, TypeID::get<llforgedialect>()) {
  initialize();
}

#define GET_OP_CLASSES
#include "Gen_Files/LLForgeOps.cpp.inc"