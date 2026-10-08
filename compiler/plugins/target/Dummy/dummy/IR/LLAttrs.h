#ifndef LL_ATTRS_H_
#define LL_ATTRS_H_

#include "dummy/IR/LLDialect.h"
#include "mlir/IR/BuiltinAttributes.h"
#include "mlir/IR/DialectImplementation.h"

#define GET_ATTRDEF_CLASSES
#include "dummy/IR/LLAttrs.h.inc"

namespace mlir::iree_compiler::Dummy::LL {

// Static data-tiling sizes for the Dummy hardware.
inline constexpr int64_t kTileH = 4;        // spatial tile height
inline constexpr int64_t kTileW = 8;        // spatial tile width
inline constexpr int64_t kChannelSet = 32;  // input channels per tile
inline constexpr int64_t kFilterSet = 32;   // output filters per tile

}  // namespace mlir::iree_compiler::Dummy::LL

#endif  // LL_ATTRS_H_
