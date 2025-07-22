#include "IR/Dialect/LLForgeDialect.h"
#include "Transforms/Passes.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/Linalg/IR/Linalg.h"
#include "mlir/Dialect/MemRef/IR/MemRef.h"
#include "mlir/Dialect/SCF/IR/SCF.h"
#include "mlir/Dialect/Tosa/IR/TosaOps.h"
#include "mlir/IR/DialectRegistry.h"
#include "mlir/IR/MLIRContext.h"
#include "mlir/InitAllDialects.h"
#include "mlir/Pass/Pass.h"
#include "mlir/Pass/PassManager.h"
#include "mlir/Tools/mlir-opt/MlirOptMain.h"
#include "mlir/Transforms/Passes.h"

void LLForgePassPipeline(mlir::OpPassManager &pm) {
  pm.addPass(mlir::LLFORGE::createOperationInsertion());
  pm.addPass(mlir::createCanonicalizerPass());
  pm.addPass(mlir::createSCCPPass());
}

int main(int argc, char **argv) {

  mlir::DialectRegistry registry;


  registry.insert<mlir::func::FuncDialect, mlir::arith::ArithDialect,
                  mlir::BuiltinDialect, mlir::linalg::LinalgDialect,
                  mlir::scf::SCFDialect, mlir::tosa::TosaDialect,
                  mlir::memref::MemRefDialect, mlir::LLFORGE::LLForgeDialect>();

  mlir::registerAllDialects(registry);

  mlir::PassPipelineRegistration<> LLForgeOpPassPipeline(
      "llforge-op", "inserts custom op", LLForgePassPipeline);

  return mlir::asMainReturnCode(
      mlir::MlirOptMain(argc, argv, "llforge-opt", registry));
}