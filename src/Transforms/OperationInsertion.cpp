#include "IR/Dialect/LLForgeDialect.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/PatternMatch.h"
#include "mlir/IR/SymbolTable.h"
#include <memory>

#include "Passes.h"

namespace mlir::LLFORGE {
#define GEN_PASS_DEF_OPERATIONINSERTION
#include "Transforms/Passes.h.inc"

namespace {
class OperationInsertion
    : public impl::OperationInsertionBase<OperationInsertion> {
public:
  void runOnOperation() override;
};
} // namespace

LogicalResult maybeAndRewrite(func::FuncOp funcOp, IRRewriter &rewriter) {
  auto funcName = funcOp.getSymName();
  auto newFuncName = (funcName + "_converted").str();

  funcOp.setVisibility(SymbolTable::Visibility::Private);

  MLIRContext *ctx = funcOp->getContext();
  ctx->loadDialect<mlir::LLFORGE::LLForgeDialect>();
  FunctionType newFuncType = FunctionType::get(ctx, {}, {});

  rewriter.setInsertionPoint(funcOp);
  auto newFuncOp =
      rewriter.create<func::FuncOp>(funcOp.getLoc(), newFuncName, newFuncType);

  if (!newFuncOp) {
    llvm::errs() << "unable to create new func-op\n";
    return failure();
  }

  auto entryBlock = newFuncOp.addEntryBlock();
  rewriter.setInsertionPointToStart(entryBlock);

  auto i32Type = rewriter.getI32Type();
  auto valueAttr = rewriter.getI32IntegerAttr(42);
  auto constantOp = rewriter.create<arith::ConstantOp>(newFuncOp.getLoc(),
                                                       i32Type, valueAttr);
  rewriter.create<LLForgeWriteOp>(newFuncOp.getLoc(), constantOp);

  rewriter.create<func::ReturnOp>(newFuncOp.getLoc());

  return success();
}

void OperationInsertion::runOnOperation() {
  auto moduleOp = getOperation();

  MLIRContext *ctx = &getContext();
  IRRewriter rewriter(ctx);

  auto walkResult = moduleOp->walk([&](func::FuncOp funcOp) {
    if (failed(maybeAndRewrite(funcOp, rewriter))) {
      funcOp.emitError("failed to convert function operation");
      return WalkResult::interrupt();
    }
    return WalkResult::advance();
  });

  if (walkResult.wasInterrupted()) {
    return signalPassFailure();
  }
}

std::unique_ptr<Pass> createOperationInsertion() {
  return std::make_unique<OperationInsertion>();
}

} // namespace mlir::LLFORGE