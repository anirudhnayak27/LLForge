#include "dummy/IR/LLOps.h"

#include "dummy/IR/LLDialect.h"
#include "mlir/IR/OpDefinition.h"

namespace mlir::iree_compiler::Dummy::LL {

void LLDialect::initializeLLOps() {
  addOperations<
#define GET_OP_LIST
#include "dummy/IR/LLOps.cpp.inc"
      >();
}

}  // namespace mlir::iree_compiler::Dummy::LL

#define GET_OP_CLASSES
#include "dummy/IR/LLOps.cpp.inc"
