#include "dummy/IR/LLOps.h"

#include "dummy/IR/LLAttrs.h"
#include "dummy/IR/LLDialect.h"
#include "mlir/IR/OpDefinition.h"

namespace mlir::iree_compiler::Dummy::LL {

void LLDialect::initializeLLOps() {
  addOperations<
#define GET_OP_LIST
#include "dummy/IR/LLOps.cpp.inc"
      >();
}

LogicalResult TiledConvOp::verify() {
  auto inTy = cast<RankedTensorType>(getInput().getType());
  auto filterTy = cast<RankedTensorType>(getFilter().getType());
  auto outTy = cast<RankedTensorType>(getOutput().getType());
  if (inTy.getRank() != 6 || filterTy.getRank() != 6 || outTy.getRank() != 6) {
    return emitOpError("expected rank-6 packed input, filter and output");
  }
  if (getResult().getType() != outTy) {
    return emitOpError("expected result type to match output type");
  }
  if (getTileSizes().size() != 2 || getStrides().size() != 2) {
    return emitOpError("expected 2-element tile_sizes and strides");
  }
  if (getPadding() && getPadding()->size() != 2) {
    return emitOpError("expected 2-element padding");
  }
  if (getInputHw() && getInputHw()->size() != 2) {
    return emitOpError("expected 2-element input_hw");
  }

  // Static dims must agree with the packed layouts documented on the op.
  auto mismatch = [](int64_t a, int64_t b) {
    return !ShapedType::isDynamic(a) && !ShapedType::isDynamic(b) && a != b;
  };
  ArrayRef<int64_t> in = inTy.getShape();
  ArrayRef<int64_t> filter = filterTy.getShape();
  ArrayRef<int64_t> out = outTy.getShape();
  ArrayRef<int64_t> tile = getTileSizes();
  if (mismatch(in[3], tile[0]) || mismatch(in[4], tile[1]) ||
      mismatch(out[3], tile[0]) || mismatch(out[4], tile[1])) {
    return emitOpError("input/output inner spatial tiles must match "
                       "tile_sizes");
  }
  if (mismatch(in[5], kChannelSet) || mismatch(filter[5], kChannelSet) ||
      mismatch(filter[4], kFilterSet) || mismatch(out[5], kFilterSet)) {
    return emitOpError("expected inner channel/filter tiles of ")
           << kChannelSet;
  }
  if (mismatch(in[2], filter[3])) {
    return emitOpError("input and filter channel-set counts differ");
  }
  if (mismatch(filter[0], out[2])) {
    return emitOpError("filter and output filter-set counts differ");
  }
  return success();
}

}  // namespace mlir::iree_compiler::Dummy::LL

#define GET_OP_CLASSES
#include "dummy/IR/LLOps.cpp.inc"
