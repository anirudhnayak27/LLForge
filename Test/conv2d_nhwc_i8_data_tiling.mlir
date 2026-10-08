// Test input for the Dummy data-tiling pipeline: padded 3x3 i8 conv, NHWC
// activation with HWCF filter, i32 accumulator.
//
// With --iree-dispatch-creation-data-tiling the operands are packed into the
// static 4x8 layouts of #ll.encoding_resolver and the conv is lowered to
// ll.tiled_conv:
//   input  32x32x16  -> [8, 4, 1, 4, 8, 32]  (+1 halo tile on H and W)
//   filter 3x3x16x32 -> [1, 3, 3, 1, 32, 32]
//   output 32x32x32  -> [8, 4, 1, 4, 8, 32]

func.func @conv2d_nhwc_i8(%input: tensor<1x32x32x16xi8>,
                          %filter: tensor<3x3x16x32xi8>)
    -> tensor<1x32x32x32xi32> {
  %c0_i8 = arith.constant 0 : i8
  %c0_i32 = arith.constant 0 : i32
  %padded = tensor.pad %input low[0, 1, 1, 0] high[0, 1, 1, 0] {
  ^bb0(%n: index, %h: index, %w: index, %c: index):
    tensor.yield %c0_i8 : i8
  } : tensor<1x32x32x16xi8> to tensor<1x34x34x16xi8>
  %init = tensor.empty() : tensor<1x32x32x32xi32>
  %fill = linalg.fill ins(%c0_i32 : i32)
      outs(%init : tensor<1x32x32x32xi32>) -> tensor<1x32x32x32xi32>
  %conv = linalg.conv_2d_nhwc_hwcf
      {dilations = dense<1> : tensor<2xi64>, strides = dense<1> : tensor<2xi64>}
      ins(%padded, %filter : tensor<1x34x34x16xi8>, tensor<3x3x16x32xi8>)
      outs(%fill : tensor<1x32x32x32xi32>) -> tensor<1x32x32x32xi32>
  return %conv : tensor<1x32x32x32xi32>
}
