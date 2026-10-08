#ifndef DUMMY_ENCODING_LL_ENCODING_EXTERNAL_MODELS_H_
#define DUMMY_ENCODING_LL_ENCODING_EXTERNAL_MODELS_H_

#include "mlir/IR/DialectRegistry.h"

namespace mlir::iree_compiler::Dummy::LL {

/// Attaches the encoding layout interfaces (LayoutResolverAttr,
/// SerializableAttr, LayoutMaterializerAttr, PackedLayoutMaterializerAttr) to
/// `#ll.encoding_resolver`.
void registerLLEncodingExternalModels(DialectRegistry &registry);

}  // namespace mlir::iree_compiler::Dummy::LL

#endif  // DUMMY_ENCODING_LL_ENCODING_EXTERNAL_MODELS_H_
