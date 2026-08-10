#include "dummy/Transforms/Passes.h"

#include "mlir/Pass/PassManager.h"
#include "mlir/Transforms/Passes.h"

namespace mlir::iree_compiler::Dummy {

void buildDummyTransformPassPipeline(OpPassManager &passManager) {
  passManager.addPass(createLowerToLLConvPass());
  passManager.addPass(createCanonicalizerPass());
}

namespace {
#define GEN_PASS_REGISTRATION
#include "dummy/Transforms/Passes.h.inc"
}  // namespace

void registerDummyPasses() { registerPasses(); }

}  // namespace mlir::iree_compiler::Dummy
