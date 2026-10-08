#include "dummy/IR/LLDialect.h"

#include "dummy/IR/LLDialect.cpp.inc"
#include "dummy/IR/LLOps.h"

namespace mlir::iree_compiler::Dummy::LL {

void LLDialect::initialize() {
  initializeLLAttrs();
  initializeLLOps();
}

}  // namespace mlir::iree_compiler::Dummy::LL
