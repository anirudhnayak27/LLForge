#ifndef DUMMY_TRANSFORMS_PASSES_H_
#define DUMMY_TRANSFORMS_PASSES_H_

#include "dummy/Transforms/PassDetail.h"
#include "mlir/Pass/Pass.h"
#include "mlir/Pass/PassManager.h"

namespace mlir::iree_compiler::Dummy {

std::unique_ptr<Pass> createLowerToLLConvPass();

void buildDummyTransformPassPipeline(OpPassManager &passManager);

void registerDummyPasses();

}  // namespace mlir::iree_compiler::Dummy

#endif  // DUMMY_TRANSFORMS_PASSES_H_
