#include "dummy/Transforms/Passes.h"

#include "dummy/IR/LLOps.h"
#include "llvm/Support/Debug.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/Linalg/IR/Linalg.h"
#include "mlir/Dialect/Linalg/IR/LinalgInterfaces.h"
#include "mlir/Dialect/Linalg/Utils/Utils.h"
#include "mlir/Dialect/Tensor/IR/Tensor.h"
#include "mlir/Dialect/Utils/StructuredOpsUtils.h"
#include "mlir/IR/AffineExpr.h"
#include "mlir/IR/AffineMap.h"
#include "mlir/IR/PatternMatch.h"
#include "mlir/Transforms/DialectConversion.h"
#include "mlir/Transforms/GreedyPatternRewriteDriver.h"

#define DEBUG_TYPE "dummy-lower-to-ll"

namespace mlir::iree_compiler::Dummy {
namespace {

// Attributes stamped on the tiled conv generic by #ll.encoding_resolver
// (see dummy/Encoding/LLEncodingExternalModels.cpp).
constexpr StringLiteral kTileSizesAttrName = "ll.tile_sizes";
constexpr StringLiteral kStridesAttrName = "ll.strides";
constexpr StringLiteral kPaddingAttrName = "ll.padding";
constexpr StringLiteral kInputHwAttrName = "ll.input_hw";
constexpr StringLiteral kInputCAttrName = "ll.input_c";

SmallVector<int64_t> extractConvStrides(linalg::LinalgOp linalgOp) {
  if (auto dims = linalg::inferConvolutionDims(linalgOp);
      succeeded(dims) && !dims->strides.empty()) {
    return llvm::to_vector(dims->strides);
  }
  return {1, 1};
}

SmallVector<int64_t> extractConvDilations(linalg::LinalgOp linalgOp) {
  if (auto dims = linalg::inferConvolutionDims(linalgOp);
      succeeded(dims) && !dims->dilations.empty()) {
    return llvm::to_vector(dims->dilations);
  }
  return {1, 1};
}

SmallVector<int64_t> extractConvPaddingLow(Value input) {
  auto padOp = input.getDefiningOp<tensor::PadOp>();
  if (!padOp) return {0, 0};
  ArrayRef<int64_t> low = padOp.getStaticLow();
  if (low.empty()) return {0, 0};
  return {low[low.size() > 1 ? 1 : 0], low.back()};
}

SmallVector<int64_t> extractConvPaddingHigh(Value input) {
  auto padOp = input.getDefiningOp<tensor::PadOp>();
  if (!padOp) return {0, 0};
  ArrayRef<int64_t> high = padOp.getStaticHigh();
  if (high.empty()) return {0, 0};
  return {high[high.size() > 1 ? 1 : 0], high.back()};
}

bool isConv2D(linalg::LinalgOp linalgOp) {
  if (linalgOp.getNumDpsInputs() < 2) return false;
  // Data-tiled convs are handled by LowerTiledConvGenericToLLTiledConv.
  if (linalgOp->hasAttr(kTileSizesAttrName)) return false;

  SmallVector<AffineMap> maps = linalgOp.getIndexingMapsArray();
  if (maps.size() < 3) return false;

  ArrayRef<AffineMap> mapsRef(maps);
  if (!mapsRef.back().isProjectedPermutation()) return false;

  SmallVector<utils::IteratorType> iterTypes = linalgOp.getIteratorTypesArray();
  int64_t numParallel = 0, numReduction = 0;
  for (auto type : iterTypes) {
    if (type == utils::IteratorType::reduction) numReduction++;
    if (type == utils::IteratorType::parallel) numParallel++;
  }
  return numParallel >= 2 && numReduction >= 2;
}

class LowerLinalgGenericToLLConv
    : public OpRewritePattern<linalg::GenericOp> {
 public:
  using OpRewritePattern<linalg::GenericOp>::OpRewritePattern;

  LogicalResult matchAndRewrite(linalg::GenericOp genericOp,
                                PatternRewriter &rewriter) const override {
    linalg::LinalgOp linalgOp = cast<linalg::LinalgOp>(genericOp.getOperation());
    if (!isConv2D(linalgOp)) return failure();

    Value input = linalgOp.getDpsInputs()[0];
    Value filter = linalgOp.getDpsInputs()[1];
    Value output = linalgOp.getDpsInits()[0];

    SmallVector<int64_t> strides = extractConvStrides(linalgOp);
    SmallVector<int64_t> dilations = extractConvDilations(linalgOp);
    SmallVector<int64_t> padLow = extractConvPaddingLow(input);
    SmallVector<int64_t> padHigh = extractConvPaddingHigh(input);

    auto convOp = rewriter.create<Dummy::LL::ConvOp>(
        genericOp.getLoc(), output.getType(), input, filter, output,
        rewriter.getDenseI64ArrayAttr(strides),
        rewriter.getDenseI64ArrayAttr(dilations),
        rewriter.getDenseI64ArrayAttr(padLow),
        rewriter.getDenseI64ArrayAttr(padHigh));

    LLVM_DEBUG(llvm::dbgs()
               << "Lowered conv to ll.conv: " << convOp << "\n");
    rewriter.replaceOp(genericOp, convOp.getResult());
    return success();
  }
};

/// Lowers the 10-D tiled conv generic produced by data tiling to
/// ll.tiled_conv. The generic reads the packed activation off its operand list
/// through a tensor.extract in the body; its operands are the packed filter
/// (plus unused extra inputs) and the packed output.
class LowerTiledConvGenericToLLTiledConv
    : public OpRewritePattern<linalg::GenericOp> {
 public:
  using OpRewritePattern<linalg::GenericOp>::OpRewritePattern;

  LogicalResult matchAndRewrite(linalg::GenericOp genericOp,
                                PatternRewriter &rewriter) const override {
    auto tileSizes =
        genericOp->getAttrOfType<DenseI64ArrayAttr>(kTileSizesAttrName);
    auto strides = genericOp->getAttrOfType<DenseI64ArrayAttr>(kStridesAttrName);
    if (!tileSizes || !strides) {
      return rewriter.notifyMatchFailure(genericOp, "not a tiled conv");
    }
    if (genericOp.getNumDpsInputs() < 1 || genericOp.getNumDpsInits() != 1) {
      return rewriter.notifyMatchFailure(genericOp, "unexpected operands");
    }

    Value input;
    genericOp.getBody()->walk([&](tensor::ExtractOp extractOp) {
      Value tensor = extractOp.getTensor();
      if (!genericOp.getRegion().isAncestor(tensor.getParentRegion())) {
        input = tensor;
      }
    });
    if (!input) {
      return rewriter.notifyMatchFailure(genericOp,
                                         "no packed activation read in body");
    }

    Value filter = genericOp.getDpsInputs()[0];
    Value output = genericOp.getDpsInits()[0];
    auto inputC = genericOp->getAttrOfType<IntegerAttr>(kInputCAttrName);
    auto tiledConv = Dummy::LL::TiledConvOp::create(
        rewriter, genericOp.getLoc(), output.getType(), input, filter, output, tileSizes,
        strides, genericOp->getAttrOfType<DenseI64ArrayAttr>(kPaddingAttrName),
        genericOp->getAttrOfType<DenseI64ArrayAttr>(kInputHwAttrName),
        inputC);
    LLVM_DEBUG(llvm::dbgs() << "Lowered tiled conv to ll.tiled_conv: "
                            << tiledConv << "\n");
    rewriter.replaceOp(genericOp, tiledConv.getResult());
    return success();
  }
};

struct DummyLowerToLLConvPass
    : public impl::DummyLowerToLLConvBase<DummyLowerToLLConvPass> {
  using DummyLowerToLLConvBase::DummyLowerToLLConvBase;

  void runOnOperation() override {
    MLIRContext *ctx = &getContext();
    RewritePatternSet patterns(ctx);
    patterns.add<LowerLinalgGenericToLLConv, LowerTiledConvGenericToLLTiledConv>(
        ctx);
    if (failed(applyPatternsGreedily(getOperation(), std::move(patterns)))) {
      signalPassFailure();
    }
  }
};

}  // namespace

std::unique_ptr<Pass> createLowerToLLConvPass() {
  return std::make_unique<DummyLowerToLLConvPass>();
}

}  // namespace mlir::iree_compiler::Dummy
