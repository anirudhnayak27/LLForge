#include "dummy/Transforms/Passes.h"

#include "iree/compiler/Codegen/Common/Passes.h"
#include "iree/compiler/Utils/PassUtils.h"
#include "mlir/Pass/PassManager.h"
#include "mlir/Transforms/Passes.h"

namespace mlir::iree_compiler::Dummy {

void buildDummyTransformPassPipeline(OpPassManager &variantPassManager) {
  OpPassManager &modulePassManager = variantPassManager.nest<ModuleOp>();
  {
    FunctionLikeNest funcPassManager(modulePassManager);
    addCommonTargetExecutablePreprocessingPasses(funcPassManager);
  }
  // Data tiling: resolve the iree_encoding layouts of the dispatch through
  // #ll.encoding_resolver. Encoded convolutions become 10-D tiled generics on
  // packed operands; without --iree-dispatch-creation-data-tiling there are no
  // encodings and this is a no-op.
  {
    FunctionLikeNest funcPassManager(modulePassManager);
    funcPassManager.addPass(
        []() { return createMaterializeDeviceEncodingPass(); });
  }
  modulePassManager.addPass(createLowerToLLConvPass());
  modulePassManager.addPass(createCanonicalizerPass());
}

namespace {
#define GEN_PASS_REGISTRATION
#include "dummy/Transforms/Passes.h.inc"
}  // namespace

void registerDummyPasses() { registerPasses(); }

}  // namespace mlir::iree_compiler::Dummy
