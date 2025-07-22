#include "LLForgeDialect.h"

using namespace mlir::LLFORGE;

MLIR_DEFINE_EXPLICIT_TYPE_ID(mlir::LLFORGE::LLForgeDialect);

LLForgeDialect::~LLForgeDialect() = default;

void LLForgeDialect::initialize() {
  addOperations<
#define GET_OP_LIST
#include "Gen_Files/LLForgeOps.cpp.inc"
      >();
}

LLForgeDialect::LLForgeDialect(MLIRContext *ctx)
    : Dialect(getDialectNamespace(), ctx, TypeID::get<LLForgeDialect>()) {
  initialize();
}

#define GET_OP_CLASSES
#include "Gen_Files/LLForgeOps.cpp.inc"