#ifndef LL_OPS_TD_
#define LL_OPS_TD_

include "LLDialect.td"
include "mlir/IR/OpBase.td"
include "mlir/Interfaces/SideEffectInterfaces.td"

class LL_Op<string mnemonic, list<Trait> traits = []> :
    Op<LL_Dialect, mnemonic, traits>;

def LL_ConvOp : LL_Op<"conv", [Pure]> {
    let summary = "Low-level convolution operation with full linalg.generic data";
    let description = [{
        Captures all data from a linalg.generic 2D convolution operation,
        including operand tensors, convolution parameters (strides, dilations,
        padding), and the original indexing maps and iterator types for
        reference. This op represents a hardware convolution for the Dummy
        backend.

        Example:
        ```mlir
        %result = ll.conv %input, %filter, %output
          strides = [1, 1] dilations = [1, 1]
          pad_low = [0, 0] pad_high = [0, 0]
        : tensor<1x3x32x32xf32>, tensor<6x3x3x3xf32>, tensor<1x6x30x30xf32>
          -> tensor<1x6x30x30xf32>
        ```
    }];

    let arguments = (ins
        AnyTensor:$input,
        AnyTensor:$filter,
        AnyTensor:$output,
        OptionalAttr<DenseI64ArrayAttr>:$strides,
        OptionalAttr<DenseI64ArrayAttr>:$dilations,
        OptionalAttr<DenseI64ArrayAttr>:$pad_low,
        OptionalAttr<DenseI64ArrayAttr>:$pad_high
    );

    let results = (outs
        AnyTensor:$result
    );

    let assemblyFormat = [{
        $input `,` $filter `,` $output
        (`strides` `=` $strides^)?
        (`dilations` `=` $dilations^)?
        (`pad_low` `=` $pad_low^)?
        (`pad_high` `=` $pad_high^)?
        attr-dict `:` `(` type($input) `,` type($filter) `,` type($output) `)` `->` type($result)
    }];
}

def LL_TiledConvOp : LL_Op<"tiled_conv", [Pure]> {
    let summary = "Convolution on data-tiled (packed) operands";
    let description = [{
        A 2D convolution whose operands are already in the packed layouts
        produced by data tiling (`#ll.encoding_resolver`):

          input  : [IH/tH, IW/tW, C/32, tH, tW, 32]
          filter : [F/32,  KH,    KW,   C/32, 32, 32]
          output : [OH/tH, OW/tW, F/32, tH, tW, 32]

        `tile_sizes` is the spatial tile [tH, tW]. `padding` is the symmetric
        spatial padding that was dropped from the activation when it was
        encoded; the hardware applies it as a halo. `input_hw` and `input_c`
        carry the logical (unpadded) input sizes, which the packed shapes only
        give rounded up to the tile.

        Example:
        ```mlir
        %r = ll.tiled_conv %in, %filter, %out
          tile_sizes = [4, 8] strides = [1, 1] padding = [1, 1]
          input_hw = [32, 32] input_c = 3
        : (tensor<9x5x1x4x8x32xi8>, tensor<1x3x3x1x32x32xi8>,
           tensor<8x4x1x4x8x32xi32>) -> tensor<8x4x1x4x8x32xi32>
        ```
    }];

    let arguments = (ins
        AnyRankedTensor:$input,
        AnyRankedTensor:$filter,
        AnyRankedTensor:$output,
        DenseI64ArrayAttr:$tile_sizes,
        DenseI64ArrayAttr:$strides,
        OptionalAttr<DenseI64ArrayAttr>:$padding,
        OptionalAttr<DenseI64ArrayAttr>:$input_hw,
        OptionalAttr<I64Attr>:$input_c
    );

    let results = (outs
        AnyRankedTensor:$result
    );

    let assemblyFormat = [{
        $input `,` $filter `,` $output
        `tile_sizes` `=` $tile_sizes
        `strides` `=` $strides
        (`padding` `=` $padding^)?
        (`input_hw` `=` $input_hw^)?
        (`input_c` `=` $input_c^)?
        attr-dict `:` `(` type($input) `,` type($filter) `,` type($output) `)` `->` type($result)
    }];

    let hasVerifier = 1;
}

#endif
