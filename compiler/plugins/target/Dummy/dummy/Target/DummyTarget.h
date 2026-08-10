#ifndef IREE_TARGET_DUMMY_TARGET_H_
#define IREE_TARGET_DUMMY_TARGET_H_

#include <memory>
#include <string>

#include "iree/compiler/Dialect/HAL/Target/TargetBackend.h"
#include "iree/compiler/Dialect/HAL/Target/TargetDevice.h"
#include "iree/compiler/Utils/OptionUtils.h"

namespace mlir::iree_compiler::Dummy {

struct DummyOptions {
  bool enableDebug = false;

  void bindOptions(OptionsBinder &binder) {
    static llvm::cl::OptionCategory category("dummy-options");
    binder.opt<bool>("iree-dummy-enable-debug", enableDebug,
                     llvm::cl::desc("Enables debug output for dummy backend"),
                     llvm::cl::init(false), llvm::cl::cat(category));
  }
};

std::shared_ptr<IREE::HAL::TargetDevice> createTarget(
    const DummyOptions &options);

std::shared_ptr<IREE::HAL::TargetBackend> createBackend(
    const DummyOptions &options);

}  // namespace mlir::iree_compiler::Dummy

#endif  // IREE_TARGET_DUMMY_TARGET_H_
