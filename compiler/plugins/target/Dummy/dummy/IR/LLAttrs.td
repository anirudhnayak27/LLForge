#ifndef LL_ATTRS_TD_
#define LL_ATTRS_TD_

include "LLDialect.td"
include "mlir/IR/AttrTypeBase.td"
include "mlir/IR/BuiltinAttributes.td"

def LL_EncodingResolverAttr :
    AttrDef<LL_Dialect, "EncodingResolver", []> {
  let mnemonic = "encoding_resolver";
  let summary = "Encoding resolver for data tiling on the Dummy backend";
  let description = [{
    Placed in the executable target configuration under
    `iree.encoding.resolver`. It resolves the `iree_encoding.encoding`
    attributes that `--iree-dispatch-creation-data-tiling` attaches to
    convolution operands into static packed layouts, and lowers the encoded
    convolution to a tiled `linalg.generic`.

    Tile sizes are fixed (4x8 spatial, 32 channels/filters):
      input  -> [H/4, W/8, C/32, 4, 8, 32]
      filter -> [F/32, KH, KW, C/32, 32, 32]
      output -> [OH/4, OW/8, F/32, 4, 8, 32]

    The interface implementations are attached as external models in
    `dummy/Encoding/LLEncodingExternalModels.cpp`.

    Example: `#ll.encoding_resolver<>`
  }];

  let parameters = (ins
    OptionalParameter<"DictionaryAttr", "Executable target configuration. It "
    "is expected to be used in a pass scope, but not the final IR output.">
        :$configuration
  );

  let assemblyFormat = "`<` struct(params) `>`";
}

#endif
