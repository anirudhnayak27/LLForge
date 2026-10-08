#include "dummy/IR/LLAttrs.h"

#include "dummy/IR/LLDialect.h"
#include "llvm/ADT/TypeSwitch.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/DialectImplementation.h"

#define GET_ATTRDEF_CLASSES
#include "dummy/IR/LLAttrs.cpp.inc"

namespace mlir::iree_compiler::Dummy::LL {

void LLDialect::initializeLLAttrs() {
  addAttributes<
#define GET_ATTRDEF_LIST
#include "dummy/IR/LLAttrs.cpp.inc"
      >();
}

}  // namespace mlir::iree_compiler::Dummy::LL
