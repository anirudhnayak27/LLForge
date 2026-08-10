#include "dummy/IR/LLDialect.h"
#include "dummy/IR/LLOps.h"
#include "dummy/Target/DummyTarget.h"
#include "dummy/Transforms/Passes.h"
#include "iree/compiler/Dialect/HAL/Target/TargetRegistry.h"
#include "iree/compiler/PluginAPI/Client.h"
#include "iree/compiler/Utils/OptionUtils.h"

IREE_DEFINE_COMPILER_OPTION_FLAGS(::mlir::iree_compiler::Dummy::DummyOptions);

namespace mlir::iree_compiler {
namespace {

struct DummySession
    : public PluginSession<DummySession, Dummy::DummyOptions,
                           PluginActivationPolicy::DefaultActivated> {
  static void registerPasses() { Dummy::registerDummyPasses(); }

  void onRegisterDialects(DialectRegistry& registry) override {
    registry.insert<Dummy::LL::LLDialect>();
  }

  void populateHALTargetDevices(
      IREE::HAL::TargetDeviceList& targets) override {
    targets.add("dummy",
                [=]() { return Dummy::createTarget(options); });
  }

  void populateHALTargetBackends(
      IREE::HAL::TargetBackendList& targets) override {
    targets.add("dummy",
                [=]() { return Dummy::createBackend(options); });
  }
};

}  // namespace
}  // namespace mlir::iree_compiler

extern "C" bool iree_register_compiler_plugin_hal_target_dummy(
    mlir::iree_compiler::PluginRegistrar* registrar) {
  registrar->registerPlugin<::mlir::iree_compiler::DummySession>(
      "hal_target_dummy");
  return true;
}
