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

#endif
