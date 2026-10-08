//===- LLEncodingExternalModels.cpp ---------------------------------------===//
//
// Encoding resolution and materialization for the Dummy backend, implemented
// for `#ll.encoding_resolver`:
//   - IREE::Encoding::LayoutResolverAttr
//   - IREE::Encoding::SerializableAttr
//   - IREE::Encoding::LayoutMaterializerAttr
//   - IREE::Codegen::PackedLayoutMaterializerAttr
//
// Only 2D convolutions are data-tiled, with static tile sizes (see LLAttrs.h):
//   input  [IH, IW, C]    -> [IH/4, IW/8, C/32, 4, 8, 32]
//   filter [KH, KW, C, F] -> [F/32, KH,   KW,   C/32, 32, 32]
//   output [OH, OW, F]    -> [OH/4, OW/8, F/32, 4, 8, 32]
// The H/W/C/F positions are read from the indexing maps, so NHWC and NCHW
// activations with HWCF, FHWC or FCHW filters all map onto the same packed
// layouts. Any other encoding (matmul, pooling, ...) resolves to the identity
// layout and its op is left untiled.
//
//===----------------------------------------------------------------------===//

#include "dummy/Encoding/LLEncodingExternalModels.h"

#include "dummy/IR/LLAttrs.h"
#include "dummy/IR/LLDialect.h"
#include "iree/compiler/Codegen/Dialect/Codegen/IR/IREECodegenTypes.h"
#include "iree/compiler/Codegen/ExternalInterfaces/Utils.h"
#include "iree/compiler/Dialect/Encoding/IR/EncodingTypes.h"
#include "iree/compiler/Dialect/Encoding/Utils/Utils.h"
#include "llvm/ADT/DenseSet.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/Linalg/IR/Linalg.h"
#include "mlir/Dialect/Tensor/IR/Tensor.h"
#include "mlir/Dialect/Utils/StructuredOpsUtils.h"
#include "mlir/IR/BuiltinAttributes.h"

#define DEBUG_TYPE "dummy-materialize-encoding"

namespace mlir::iree_compiler::Dummy::LL {

using IREE::Codegen::MaterializeEncodingInfo;

// Conv encoding operand indices and the attributes SetEncoding puts on a conv
// whose activation was encoded unpadded. These come from patches/iree; they
// are spelled out here so the plugin also builds against an unpatched IREE,
// where no conv encodings are produced and data tiling is simply inactive.
constexpr int64_t kConvLhs = 0;     // IREE::Encoding::CONV_LHS
constexpr int64_t kConvRhs = 1;     // IREE::Encoding::CONV_RHS
constexpr int64_t kConvResult = 2;  // IREE::Encoding::CONV_RESULT
constexpr StringLiteral kConvPaddingAttrName = "iree_encoding.conv_padding";
constexpr StringLiteral kConvInputHWAttrName = "iree_encoding.conv_input_hw";
constexpr StringLiteral kConvInputCAttrName = "iree_encoding.conv_input_c";

namespace {

//===----------------------------------------------------------------------===//
// Convolution indexing-map analysis
//===----------------------------------------------------------------------===//

/// Result positions of each logical dim in the operand indexing maps of a
/// 6-loop 2D convolution, plus its spatial strides.
struct ConvMapInfo {
  int64_t inH, inW, inC;
  int64_t filterF, filterKH, filterKW, filterC;
  int64_t outH, outW, outF;
  int64_t strideH, strideW;
};

/// Returns the coefficient of loop dim `dim` in `expr` (handles d, d*c, c*d and
/// sums of those). Returns 0 if `dim` does not appear.
static int64_t getDimCoefficient(AffineExpr expr, unsigned dim) {
  if (auto d = dyn_cast<AffineDimExpr>(expr)) {
    return d.getPosition() == dim ? 1 : 0;
  }
  auto bin = dyn_cast<AffineBinaryOpExpr>(expr);
  if (!bin) {
    return 0;
  }
  if (bin.getKind() == AffineExprKind::Add) {
    return getDimCoefficient(bin.getLHS(), dim) +
           getDimCoefficient(bin.getRHS(), dim);
  }
  if (bin.getKind() == AffineExprKind::Mul) {
    auto cst = dyn_cast<AffineConstantExpr>(bin.getRHS());
    AffineExpr other = bin.getLHS();
    if (!cst) {
      cst = dyn_cast<AffineConstantExpr>(bin.getLHS());
      other = bin.getRHS();
    }
    if (cst) {
      return cst.getValue() * getDimCoefficient(other, dim);
    }
  }
  return 0;
}

/// Analyzes the [input, filter, (extra inputs...), output] indexing maps of a
/// batch-less 2D convolution. The input map is allowed to have had its kernel
/// offsets stripped (SetEncoding does that when it encodes the unpadded
/// activation), so the analysis only relies on:
///   - spatial input results reference exactly one output-image loop dim,
///   - the channel input result is a bare reduction dim,
///   - filter and output maps are projected permutations.
static std::optional<ConvMapInfo> analyzeConvMaps(ArrayRef<AffineMap> maps) {
  if (maps.size() < 3) {
    return std::nullopt;
  }
  AffineMap inMap = maps[0];
  AffineMap filterMap = maps[1];
  AffineMap outMap = maps.back();
  if (inMap.getNumDims() != 6 || inMap.getNumResults() != 3 ||
      filterMap.getNumResults() != 4 || outMap.getNumResults() != 3 ||
      !filterMap.isProjectedPermutation() ||
      !outMap.isProjectedPermutation()) {
    return std::nullopt;
  }

  auto outPosOf = [&](unsigned dim) -> std::optional<int64_t> {
    for (auto [i, e] : llvm::enumerate(outMap.getResults())) {
      if (cast<AffineDimExpr>(e).getPosition() == dim) {
        return i;
      }
    }
    return std::nullopt;
  };

  SmallVector<int64_t> spatialPos;
  SmallVector<unsigned> spatialDims;
  std::optional<int64_t> chanPos;
  unsigned chanDim = 0;
  for (auto [i, e] : llvm::enumerate(inMap.getResults())) {
    llvm::SmallDenseSet<unsigned> dims;
    e.walk([&](AffineExpr sub) {
      if (auto d = dyn_cast<AffineDimExpr>(sub)) {
        dims.insert(d.getPosition());
      }
    });
    std::optional<unsigned> imageDim;
    for (unsigned d : dims) {
      if (!outPosOf(d)) {
        continue;
      }
      if (imageDim) {
        return std::nullopt;
      }
      imageDim = d;
    }
    if (imageDim) {
      spatialPos.push_back(i);
      spatialDims.push_back(*imageDim);
      continue;
    }
    auto d = dyn_cast<AffineDimExpr>(e);
    if (!d || chanPos) {
      return std::nullopt;
    }
    chanPos = i;
    chanDim = d.getPosition();
  }
  // Depthwise convs (channel also in the output) end up with 3 spatial-like
  // results and are rejected here.
  if (spatialPos.size() != 2 || !chanPos) {
    return std::nullopt;
  }

  ConvMapInfo info;
  info.inH = spatialPos[0];
  info.inW = spatialPos[1];
  info.inC = *chanPos;
  info.strideH = std::max<int64_t>(
      1, getDimCoefficient(inMap.getResult(info.inH), spatialDims[0]));
  info.strideW = std::max<int64_t>(
      1, getDimCoefficient(inMap.getResult(info.inW), spatialDims[1]));
  info.outH = *outPosOf(spatialDims[0]);
  info.outW = *outPosOf(spatialDims[1]);
  info.outF = 3 - info.outH - info.outW;
  unsigned filterDim =
      cast<AffineDimExpr>(outMap.getResult(info.outF)).getPosition();

  SmallVector<int64_t> kernelPos;
  std::optional<int64_t> fPos, cPos;
  for (auto [i, e] : llvm::enumerate(filterMap.getResults())) {
    unsigned d = cast<AffineDimExpr>(e).getPosition();
    if (d == filterDim) {
      fPos = i;
    } else if (d == chanDim) {
      cPos = i;
    } else {
      kernelPos.push_back(i);
    }
  }
  if (!fPos || !cPos || kernelPos.size() != 2) {
    return std::nullopt;
  }
  info.filterF = *fPos;
  info.filterC = *cPos;
  info.filterKH = kernelPos[0];
  info.filterKW = kernelPos[1];
  return info;
}

/// Packed layout for one operand of a data-tiled conv. Identity (empty) when
/// the encoding is not a conv this backend tiles.
static MaterializeEncodingInfo getConvEncodingInfo(RankedTensorType type) {
  MaterializeEncodingInfo info;
  auto encoding =
      dyn_cast_or_null<IREE::Encoding::EncodingAttr>(type.getEncoding());
  if (!encoding ||
      encoding.getOpType().getValue() != IREE::Encoding::EncodingOpType::conv) {
    return info;
  }
  std::optional<ConvMapInfo> conv = analyzeConvMaps(encoding.getRootMaps());
  if (!conv) {
    return info;
  }

  switch (encoding.getOperandIndex().getValue().getZExtValue()) {
  case kConvLhs:
    if (type.getRank() == 3) {
      info.innerDimsPos = {conv->inH, conv->inW, conv->inC};
      info.innerTileSizes = {kTileH, kTileW, kChannelSet};
      info.outerDimsPerm = {conv->inH, conv->inW, conv->inC};
    }
    break;
  case kConvRhs:
    if (type.getRank() == 4) {
      info.innerDimsPos = {conv->filterF, conv->filterC};
      info.innerTileSizes = {kFilterSet, kChannelSet};
      info.outerDimsPerm = {conv->filterF, conv->filterKH, conv->filterKW,
                            conv->filterC};
    }
    break;
  case kConvResult:
    if (type.getRank() == 3) {
      info.innerDimsPos = {conv->outH, conv->outW, conv->outF};
      info.innerTileSizes = {kTileH, kTileW, kFilterSet};
      info.outerDimsPerm = {conv->outH, conv->outW, conv->outF};
    } else if (type.getRank() == 1) {
      // Per-filter bias [F] -> [F/32, 32]. The outer perm must be explicit;
      // an empty one drops the outer dim in lowerGenericOpWithResolvedLayouts.
      info.innerDimsPos = {0};
      info.innerTileSizes = {kFilterSet};
      info.outerDimsPerm = {0};
    }
    break;
  default:
    break;
  }
  return info;
}

//===----------------------------------------------------------------------===//
// Tiled convolution lowering
//===----------------------------------------------------------------------===//

/// Pads the packed activation [IH/tH, IW/tW, C/32, tH, tW, 32] with
/// ceil((k-1)/tile) extra outer H/W tiles so the in-body reads of the bottom /
/// right stencil stay in bounds. The hardware handles the partial edge tiles
/// itself, so the halo is never actually read; zero keeps the accumulation
/// unaffected regardless.
static Value padPackedInputHalo(OpBuilder &b, Location loc, Value packedIn,
                                int64_t kH, int64_t kW) {
  int64_t extraH = (kH + kTileH - 2) / kTileH;
  int64_t extraW = (kW + kTileW - 2) / kTileW;
  if (extraH == 0 && extraW == 0) {
    return packedIn;
  }
  auto inTy = cast<RankedTensorType>(packedIn.getType());
  SmallVector<int64_t> paddedShape(inTy.getShape());
  paddedShape[0] += extraH;
  paddedShape[1] += extraW;
  auto paddedTy = RankedTensorType::get(paddedShape, inTy.getElementType());
  Value zero =
      arith::ConstantOp::create(b, loc, b.getZeroAttr(inTy.getElementType()));
  SmallVector<OpFoldResult> low(6, b.getIndexAttr(0));
  SmallVector<OpFoldResult> high(6, b.getIndexAttr(0));
  high[0] = b.getIndexAttr(extraH);
  high[1] = b.getIndexAttr(extraW);
  return tensor::PadOp::create(b, loc, paddedTy, packedIn, low, high, zero);
}

/// Reads the packed activation inside the tiled conv body. With loop indices
///   iy = ty*(tH*sH) + ly*sH + ky ,  ix = tx*(tW*sW) + lx*sW + kx
/// it extracts [iy/tH, ix/tW, cs, iy%tH, ix%tW, c]. The activation is read via
/// tensor.extract rather than as a mapped operand so it can stay unpadded
/// without tripping the linalg.generic operand-extent verifier.
static Value emitPackedInputExtract(OpBuilder &b, Location loc, Value packedIn,
                                    int64_t strideH, int64_t strideW) {
  auto idx = [&](unsigned dim) -> Value {
    return linalg::IndexOp::create(b, loc, dim);
  };
  auto cst = [&](int64_t v) -> Value {
    return arith::ConstantIndexOp::create(b, loc, v);
  };
  auto rawCoord = [&](Value tile, Value inner, Value kernel, int64_t tileSize,
                      int64_t stride) -> Value {
    Value base = arith::MulIOp::create(b, loc, tile, cst(tileSize * stride));
    Value off = arith::MulIOp::create(b, loc, inner, cst(stride));
    return arith::AddIOp::create(
        b, loc, arith::AddIOp::create(b, loc, base, off), kernel);
  };
  // Loop dims: d0=ty d1=tx d2=fs d3=ky d4=kx d5=cs d6=ly d7=lx d8=f d9=c.
  Value iy = rawCoord(idx(0), idx(6), idx(3), kTileH, strideH);
  Value ix = rawCoord(idx(1), idx(7), idx(4), kTileW, strideW);
  return tensor::ExtractOp::create(
      b, loc, packedIn,
      ValueRange{arith::DivUIOp::create(b, loc, iy, cst(kTileH)),
                 arith::DivUIOp::create(b, loc, ix, cst(kTileW)), idx(5),
                 arith::RemUIOp::create(b, loc, iy, cst(kTileH)),
                 arith::RemUIOp::create(b, loc, ix, cst(kTileW)), idx(9)});
}

/// Returns true if `from` can be widened (or is equal) to `to` within the same
/// int/float family.
static bool canExtendTo(Type from, Type to) {
  if (from == to) {
    return true;
  }
  if (isa<FloatType>(from) != isa<FloatType>(to) ||
      (!from.isIntOrFloat() || !to.isIntOrFloat())) {
    return false;
  }
  return from.getIntOrFloatBitWidth() < to.getIntOrFloatBitWidth();
}

static Value extendTo(OpBuilder &b, Location loc, Value v, Type to) {
  if (v.getType() == to) {
    return v;
  }
  if (isa<FloatType>(to)) {
    return arith::ExtFOp::create(b, loc, to, v);
  }
  return arith::ExtSIOp::create(b, loc, to, v);
}

/// Lowers a conv whose filter carries a CONV_RHS encoding to a 10-D
/// linalg.generic on the packed operands:
///   d0=ty d1=tx d2=fs d3=ky d4=kx d5=cs d6=ly d7=lx d8=f d9=c
///   filter [F/32, KH, KW, C/32, 32, 32] : (d2, d3, d4, d5, d8, d9)
///   output [OH/tH, OW/tW, F/32, tH, tW, 32] : (d0, d1, d2, d6, d7, d8)
/// The op is stamped with `ll.tile_sizes` / `ll.strides` (and the padding /
/// logical input sizes when known) for the ll.tiled_conv lowering.
static FailureOr<Operation *> lowerConvOpWithEncoding(OpBuilder &b,
                                                      linalg::LinalgOp op,
                                                      ValueRange operands) {
  if (!op.hasPureTensorSemantics() || op.getNumDpsInputs() < 2 ||
      op.getNumDpsInits() != 1 || op.getNumLoops() != 6) {
    return failure();
  }
  auto filterEnc = IREE::Encoding::getEncodingAttr(
      cast<RankedTensorType>(op.getDpsInputs()[1].getType()));
  if (!filterEnc ||
      filterEnc.getOperandIndex().getValue() != kConvRhs) {
    return failure();
  }
  std::optional<ConvMapInfo> conv =
      analyzeConvMaps(op.getIndexingMapsArray());
  if (!conv) {
    return failure();
  }

  // Converted operands are [input, filter, extra inputs..., output].
  int64_t numInputs = op.getNumDpsInputs();
  Value packedIn = operands[0];
  Value packedFilter = operands[1];
  Value packedOut = operands[numInputs];
  auto inTy = dyn_cast<RankedTensorType>(packedIn.getType());
  auto filterTy = dyn_cast<RankedTensorType>(packedFilter.getType());
  auto outTy = dyn_cast<RankedTensorType>(packedOut.getType());
  if (!inTy || !filterTy || !outTy || inTy.getRank() != 6 ||
      filterTy.getRank() != 6 || outTy.getRank() != 6 ||
      filterTy.isDynamicDim(1) || filterTy.isDynamicDim(2)) {
    return failure();
  }
  Type accTy = outTy.getElementType();
  if (!canExtendTo(inTy.getElementType(), accTy) ||
      !canExtendTo(filterTy.getElementType(), accTy)) {
    return failure();
  }
  bool isFloat = isa<FloatType>(accTy);

  Location loc = op.getLoc();
  MLIRContext *ctx = b.getContext();
  Value haloIn = padPackedInputHalo(b, loc, packedIn, filterTy.getDimSize(1),
                                    filterTy.getDimSize(2));

  auto d = [&](unsigned i) { return b.getAffineDimExpr(i); };
  SmallVector<AffineMap> maps = {
      AffineMap::get(10, 0, {d(2), d(3), d(4), d(5), d(8), d(9)}, ctx)};
  SmallVector<Value> inputs = {packedFilter};
  // Extra inputs (e.g. quantization zero points) are carried along as scalars
  // but not used in the body; the hardware applies them separately.
  for (int64_t i = 2; i < numInputs; ++i) {
    inputs.push_back(operands[i]);
    maps.push_back(AffineMap::get(10, 0, {}, ctx));
  }
  maps.push_back(
      AffineMap::get(10, 0, {d(0), d(1), d(2), d(6), d(7), d(8)}, ctx));

  using utils::IteratorType;
  SmallVector<IteratorType> iterTypes = {
      IteratorType::parallel,  IteratorType::parallel,
      IteratorType::parallel,  IteratorType::reduction,
      IteratorType::reduction, IteratorType::reduction,
      IteratorType::parallel,  IteratorType::parallel,
      IteratorType::parallel,  IteratorType::reduction};

  int64_t strideH = conv->strideH;
  int64_t strideW = conv->strideW;
  auto genericOp = linalg::GenericOp::create(
      b, loc, outTy, inputs, ValueRange{packedOut}, maps, iterTypes,
      [&](OpBuilder &nb, Location nloc, ValueRange args) {
        Value in = extendTo(
            nb, nloc,
            emitPackedInputExtract(nb, nloc, haloIn, strideH, strideW), accTy);
        Value w = extendTo(nb, nloc, args[0], accTy);
        Value acc = args.back();
        Value sum;
        if (isFloat) {
          sum = arith::AddFOp::create(
              nb, nloc, acc, arith::MulFOp::create(nb, nloc, in, w));
        } else {
          sum = arith::AddIOp::create(
              nb, nloc, acc, arith::MulIOp::create(nb, nloc, in, w));
        }
        linalg::YieldOp::create(nb, nloc, sum);
      });

  genericOp->setAttr("ll.tile_sizes", b.getDenseI64ArrayAttr({kTileH, kTileW}));
  genericOp->setAttr("ll.strides", b.getDenseI64ArrayAttr({strideH, strideW}));
  // SetEncoding records these when it encodes the unpadded activation of a
  // symmetrically padded conv.
  if (auto pad = op->getAttrOfType<DenseI64ArrayAttr>(kConvPaddingAttrName)) {
    genericOp->setAttr("ll.padding", pad);
  }
  if (auto hw = op->getAttrOfType<DenseI64ArrayAttr>(kConvInputHWAttrName)) {
    genericOp->setAttr("ll.input_hw", hw);
  }
  if (auto c = op->getAttrOfType<IntegerAttr>(kConvInputCAttrName)) {
    genericOp->setAttr("ll.input_c", c);
  }
  return genericOp.getOperation();
}

//===----------------------------------------------------------------------===//
// Interface external models
//===----------------------------------------------------------------------===//

struct LLPackedLayoutMaterializerAttr final
    : IREE::PackedLayoutMaterializerAttrExternalModelBase<
          LLPackedLayoutMaterializerAttr, EncodingResolverAttr> {
  DictionaryAttr getConfiguration(Attribute attr) const {
    return cast<EncodingResolverAttr>(attr).getConfiguration();
  }

  MaterializeEncodingInfo getEncodingInfoImpl(Attribute attr,
                                              RankedTensorType type) const {
    return getConvEncodingInfo(type);
  }
};

struct LLLayoutMaterializerAttr final
    : IREE::EncodingLayoutMaterializerAttrExternalModelBase<
          LLLayoutMaterializerAttr, EncodingResolverAttr> {
  Operation *lowerOp(Attribute attr, OpBuilder &b, Operation *op,
                     TypeRange convertedResTypes,
                     ValueRange convertedOperands) const {
    auto linalgOp = dyn_cast<linalg::LinalgOp>(op);
    if (!linalgOp) {
      return nullptr;
    }
    if (auto fillOp = dyn_cast<linalg::FillOp>(op)) {
      return IREE::lowerFillOpWithResolvedLayouts(b, fillOp, convertedResTypes,
                                                  convertedOperands);
    }
    FailureOr<Operation *> convOp =
        lowerConvOpWithEncoding(b, linalgOp, convertedOperands);
    if (succeeded(convOp)) {
      return *convOp;
    }

    // Ops whose encodings all resolved to the identity layout (matmul,
    // pooling, unsupported convs) are kept as-is with plain tensor types.
    bool allIdentity = llvm::all_of(
        llvm::zip_equal(op->getOperandTypes(), convertedOperands.getTypes()),
        [](auto types) {
          auto [orig, converted] = types;
          auto origTensor = dyn_cast<RankedTensorType>(orig);
          return !origTensor || origTensor.dropEncoding() == converted;
        });
    if (allIdentity) {
      return clone(b, op, convertedResTypes, convertedOperands);
    }

    // Elementwise consumers of packed values (e.g. a fused bias add).
    auto genericOp = dyn_cast<linalg::GenericOp>(op);
    if (genericOp && genericOp.getNumLoops() == genericOp.getNumParallelLoops()) {
      return IREE::lowerGenericOpWithResolvedLayouts(
          b, genericOp, convertedResTypes, convertedOperands,
          cast<IREE::Encoding::LayoutMaterializerAttr>(attr));
    }
    return nullptr;
  }
};

struct LLLayoutResolverAttr final
    : IREE::Encoding::LayoutResolverAttr::ExternalModel<LLLayoutResolverAttr,
                                                        EncodingResolverAttr> {
  Attribute cloneWithSimplifiedConfig(Attribute attr,
                                      DictionaryAttr config) const {
    // Tile sizes are static, so nothing from the target config is needed.
    MLIRContext *ctx = attr.getContext();
    return EncodingResolverAttr::get(ctx, DictionaryAttr::get(ctx));
  }

  Attribute getLayout(Attribute attr, RankedTensorType type) const {
    return EncodingResolverAttr::get(
        attr.getContext(),
        IREE::getPackedLayoutImpl(attr, type, /*addEncodingAttr=*/true));
  }
};

struct LLSerializableAttr final
    : IREE::Encoding::SerializableAttr::ExternalModel<LLSerializableAttr,
                                                      EncodingResolverAttr> {
  bool isSerialized(Attribute attr) const {
    DictionaryAttr config = cast<EncodingResolverAttr>(attr).getConfiguration();
    return config && config.contains(IREE::kEncodingInfoAttrName);
  }

  Value calculateStorageSizeInBytes(Attribute attr, Location loc,
                                    OpBuilder &builder, RankedTensorType type,
                                    ValueRange dynamicDims) const {
    return IREE::calculatePackedStorageSizeInBytesImpl(attr, loc, builder, type,
                                                       dynamicDims);
  }
};

}  // namespace

void registerLLEncodingExternalModels(DialectRegistry &registry) {
  registry.addExtension(+[](MLIRContext *ctx, LLDialect *dialect) {
    EncodingResolverAttr::attachInterface<
        LLPackedLayoutMaterializerAttr, LLLayoutMaterializerAttr,
        LLLayoutResolverAttr, LLSerializableAttr>(*ctx);
  });
}

}  // namespace mlir::iree_compiler::Dummy::LL
