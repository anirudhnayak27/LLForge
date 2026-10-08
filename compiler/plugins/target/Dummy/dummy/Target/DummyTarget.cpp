#include "dummy/Target/DummyTarget.h"

#include "dummy/IR/LLAttrs.h"
#include "dummy/IR/LLDialect.h"
#include "dummy/Transforms/Passes.h"
#include "iree/compiler/Dialect/Encoding/IR/EncodingTypes.h"
#include "iree/compiler/Dialect/HAL/Target/TargetRegistry.h"
#include "llvm/Support/Debug.h"
#include "mlir/IR/Builders.h"
#include "mlir/Pass/PassManager.h"

#define DEBUG_TYPE "dummy-target"

namespace mlir::iree_compiler::Dummy {

class DummyTargetDevice final : public IREE::HAL::TargetDevice {
 public:
  explicit DummyTargetDevice(DummyOptions options)
      : options(std::move(options)) {}

  IREE::HAL::DeviceTargetAttr getDefaultDeviceTarget(
      MLIRContext *context,
      const IREE::HAL::TargetRegistry &targetRegistry) const override {
    Builder b(context);
    SmallVector<NamedAttribute> configItems;
    auto configAttr = b.getDictionaryAttr(configItems);

    SmallVector<IREE::HAL::ExecutableTargetAttr> executableTargetAttrs;
    targetRegistry.getTargetBackend("dummy")
        ->getDefaultExecutableTargets(context, "dummy", configAttr,
                                      executableTargetAttrs);

    return IREE::HAL::DeviceTargetAttr::get(
        context, b.getStringAttr("dummy"), configAttr, executableTargetAttrs);
  }

 private:
  DummyOptions options;
};

class DummyTargetBackend final : public IREE::HAL::TargetBackend {
 public:
  explicit DummyTargetBackend(DummyOptions options)
      : options(std::move(options)) {}

  std::string getLegacyDefaultDeviceID() const override { return "dummy"; }

  void getDefaultExecutableTargets(
      MLIRContext *context, StringRef deviceID, DictionaryAttr deviceConfigAttr,
      SmallVectorImpl<IREE::HAL::ExecutableTargetAttr> &executableTargetAttrs)
      const override {
    Builder b(context);
    SmallVector<NamedAttribute> configItems;
    // Resolves data-tiling encodings to the static packed layouts.
    configItems.emplace_back(
        b.getStringAttr(IREE::Encoding::kEncodingResolverAttrName),
        LL::EncodingResolverAttr::get(context, {}));
    auto configAttr = b.getDictionaryAttr(configItems);

    executableTargetAttrs.push_back(IREE::HAL::ExecutableTargetAttr::get(
        context, b.getStringAttr("dummy"),
        b.getStringAttr("dummy-bytecode-fb"), configAttr));
  }

  void buildTranslationPassPipeline(IREE::HAL::ExecutableTargetAttr targetAttr,
                                    OpPassManager &passManager) override {
    buildDummyTransformPassPipeline(passManager);
    LLVM_DEBUG(llvm::dbgs() << "DummyTargetBackend: buildTranslationPassPipeline\n");
  }

  LogicalResult serializeExecutable(const SerializationOptions &serOptions,
                                    IREE::HAL::ExecutableVariantOp variantOp,
                                    OpBuilder &executableBuilder) override {
    LLVM_DEBUG(llvm::dbgs() << "DummyTargetBackend: serializing executable '"
                            << variantOp.getName() << "'\n");

    SmallVector<char> data = {'D', 'U', 'M', 'M', 'Y', '\0'};

    auto bufferType = VectorType::get(
        {static_cast<int64_t>(data.size())},
        IntegerType::get(executableBuilder.getContext(), 8));
    auto bufferAttr = DenseIntElementsAttr::get(bufferType, data);

    auto binaryOp = IREE::HAL::ExecutableBinaryOp::create(
        executableBuilder, variantOp.getLoc(), variantOp.getSymName(),
        variantOp.getTarget().getFormat(), bufferAttr);
    binaryOp.setMimeTypeAttr(
        executableBuilder.getStringAttr("application/x-dummy"));

    return success();
  }

  void getDependentDialects(DialectRegistry &registry) const override {
    registry.insert<LL::LLDialect>();
  }

 private:
  DummyOptions options;
};

std::shared_ptr<IREE::HAL::TargetDevice> createTarget(
    const DummyOptions &options) {
  return std::make_shared<DummyTargetDevice>(options);
}

std::shared_ptr<IREE::HAL::TargetBackend> createBackend(
    const DummyOptions &options) {
  return std::make_shared<DummyTargetBackend>(options);
}

}  // namespace mlir::iree_compiler::Dummy
