#ifndef LLFORGE_TRANSFORMS_PASSES
#define LLFORGE_TRANSFORMS_PASSES

#include "mlir/Pass/Pass.h"

namespace mlir::LLFORGE {
std::unique_ptr<mlir::Pass> createOperationInsertion();

#define GEN_PASS_DECL
#include "Transforms/Passes.h.inc"

#define GEN_PASS_REGISTRATION
#include "Transforms/Passes.h.inc"
} // namespace mlir::LLFORGE

#endif