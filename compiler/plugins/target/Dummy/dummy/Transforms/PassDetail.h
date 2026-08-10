#ifndef DUMMY_TRANSFORMS_PASS_DETAIL_H_
#define DUMMY_TRANSFORMS_PASS_DETAIL_H_

#include "dummy/IR/LLDialect.h"
#include "dummy/IR/LLOps.h"
#include "mlir/Dialect/Affine/IR/AffineOps.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/Linalg/IR/Linalg.h"
#include "mlir/Dialect/Math/IR/Math.h"
#include "mlir/Dialect/Tensor/IR/Tensor.h"
#include "mlir/IR/Operation.h"
#include "mlir/Interfaces/FunctionInterfaces.h"
#include "mlir/Pass/Pass.h"

namespace mlir::iree_compiler::Dummy {
#define GEN_PASS_DECL
#define GEN_PASS_DEF_DUMMYLOWERTOLLCONV
#include "dummy/Transforms/Passes.h.inc"
}  // namespace mlir::iree_compiler::Dummy

#endif  // DUMMY_TRANSFORMS_PASS_DETAIL_H_
