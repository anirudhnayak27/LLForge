// Test input for Dummy plugin: 2D convolution with NCHW layout.
// This linalg.generic represents a valid 2D convolution that should be
// lowered to ll.conv by the DummyLowerToLLConv pass.

func.func @conv2d_nchw(%input: tensor<1x3x32x32xf32>, %filter: tensor<6x3x3x3xf32>) -> tensor<1x6x30x30xf32> {
  %c0 = arith.constant 0.0 : f32
  %init = tensor.empty() : tensor<1x6x30x30xf32>
  %fill = linalg.fill ins(%c0 : f32) outs(%init : tensor<1x6x30x30xf32>) -> tensor<1x6x30x30xf32>
  %0 = linalg.generic {
    indexing_maps = [
      affine_map<(n, oh, ow, oc, kh, kw, ic) -> (n, ic, oh + kh, ow + kw)>,
      affine_map<(n, oh, ow, oc, kh, kw, ic) -> (oc, ic, kh, kw)>,
      affine_map<(n, oh, ow, oc, kh, kw, ic) -> (n, oc, oh, ow)>
    ],
    iterator_types = ["parallel", "parallel", "parallel", "parallel",
                      "reduction", "reduction", "reduction"]
  } ins(%input, %filter : tensor<1x3x32x32xf32>, tensor<6x3x3x3xf32>)
    outs(%fill : tensor<1x6x30x30xf32>) {
  ^bb0(%in: f32, %fil: f32, %out: f32):
    %mul = arith.mulf %in, %fil : f32
    %add = arith.addf %out, %mul : f32
    linalg.yield %add : f32
  } -> tensor<1x6x30x30xf32>
  return %0 : tensor<1x6x30x30xf32>
}
