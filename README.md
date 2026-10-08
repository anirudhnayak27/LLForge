# LLForge — Dummy IREE Compiler Plugin

A minimal out-of-tree IREE compiler hardware plugin demonstrating:
- Custom dialect (`ll`) with an `ll.conv` op
- A transform pass that pattern-matches `linalg.generic` convolution ops and lowers them to `ll.conv`
- Plugin registration and HAL target backend integration
- Data tiling of convolutions with static 4x8 tiles (`#ll.encoding_resolver`,
  lowered to `ll.tiled_conv`)

## Structure

```
LLForge/
├── iree_compiler_plugin.cmake           # Root plugin entry point
├── compiler/plugins/target/Dummy/
│   ├── CMakeLists.txt                   # Include dirs
│   └── dummy/
│       ├── CMakeLists.txt               # Plugin registration
│       ├── DummyPluginRegistration.cpp  # PluginSession, dialect + pass registration
│       ├── Encoding/                    # #ll.encoding_resolver layout/materialization models
│       ├── IR/                          # ll dialect: ll.conv, ll.tiled_conv, #ll.encoding_resolver
│       ├── Target/                      # TargetDevice, TargetBackend
│       └── Transforms/                  # DummyLowerToLLConv pass, translation pipeline
├── patches/iree/                        # Optional IREE patch enabling conv data tiling
└── Test/
    ├── conv2d_linalg.mlir               # Test input (NCHW f32 2D conv)
    └── conv2d_nhwc_i8_data_tiling.mlir  # Test input (padded NHWC i8 conv)
```

## Data tiling

Data tiling packs conv operands into static 4x8 tiles, following the same flow
as `exsl-sw` but without a tile selector. Stock IREE only data-tiles matmuls,
so convolutions need the patch in `patches/iree/`. The plugin builds and works
without it, but convs then stay untiled.

1. `--iree-dispatch-creation-data-tiling --iree-dispatch-creation-data-tiling-convs`
   (patched IREE): convs get data-tiling hints and their operands are wrapped
   in `iree_encoding.set_encoding` with `op_type = conv` encodings.
2. The `dummy` executable target sets
   `iree.encoding.resolver = #ll.encoding_resolver<>`, so stream resolves the
   encoded buffer sizes, and `iree-codegen-materialize-device-encoding` packs
   the operands:

   | operand | logical            | packed                          |
   |---------|--------------------|---------------------------------|
   | input   | `[IH, IW, C]`      | `[IH/4, IW/8, C/32, 4, 8, 32]`  |
   | filter  | `[KH, KW, C, F]`   | `[F/32, KH, KW, C/32, 32, 32]`  |
   | output  | `[OH, OW, F]`      | `[OH/4, OW/8, F/32, 4, 8, 32]`  |

   H/W/C/F are found from the indexing maps, so NCHW activations and
   HWCF/FHWC/FCHW filters are supported. The conv becomes a 10-D tiled
   `linalg.generic`, int or float.
3. `iree-dummy-lower-to-ll-conv` turns that generic into `ll.tiled_conv`.

The tile sizes are `kTileH`/`kTileW`/`kChannelSet`/`kFilterSet` in
`dummy/IR/LLAttrs.h`. Only 2D, non-depthwise, batch-1 convs are tiled. Matmul
encodings resolve to the identity layout and stay untiled.

### IREE patch

`patches/iree/0001-data-tiling-for-2d-convolutions.patch` applies to IREE
commit `44ef72e7b9` (`iree-3.10.0rc20260113-639-g44ef72e7b9`). It adds:

- conv encodings (`CONV_LHS/RHS/RESULT`) in `getEncodingProperties`;
- conv data-tiling hints, opt-in with `--iree-dispatch-creation-data-tiling-convs`;
- SetEncoding support for padded activations: the unpadded tensor is encoded
  and the padding is recorded as `iree_encoding.conv_padding`,
  `conv_input_hw` and `conv_input_c` (shown on `ll.tiled_conv` as `padding`,
  `input_hw`, `input_c`);
- `tensor.pad` handling in encoding hoisting/fusion, and no sinking of
  `unset_encoding` through transposes;
- `--iree-dispatch-creation-disable-collapse-dimensions`;
- a null check when a resolver cannot lower an op during materialization.

Without the conv flag, patched IREE behaves like stock IREE apart from the
`tensor.pad` handling in encoding hoisting/fusion. The patch is a cleaned-up
subset of the data-tiling changes in the sandlogic IREE fork used by `exsl-sw`.

```bash
git clone https://github.com/iree-org/iree.git $IREE_SRC
cd $IREE_SRC
git checkout 44ef72e7b9
git submodule update --init --recursive --depth 1
git apply $LLFORGE_DIR/patches/iree/0001-data-tiling-for-2d-convolutions.patch
```

To move to a newer IREE, rebase the patch: check out the new commit,
`git apply -3` the patch, fix conflicts, and regenerate it with
`git format-patch -1`.

## Build

Point CMake at this repo via `IREE_CMAKE_PLUGIN_PATHS` when configuring IREE:

```bash
# Configure IREE build (append to your existing config)
cmake $IREE_SRC \
  -DIREE_CMAKE_PLUGIN_PATHS="$LLFORGE_DIR" \
  ...your other flags...

# Rebuild
cmake --build $IREE_BUILD --target tools/iree-compile
```

If you already have a configured build, add the path and re-run cmake:

```bash
cmake $IREE_BUILD -DIREE_CMAKE_PLUGIN_PATHS="$LLFORGE_DIR"
cmake --build $IREE_BUILD --target tools/iree-compile
```

## Test

### Compile to HAL (see ll.conv after the pass)

```bash
$IREE_BUILD/tools/iree-compile \
  --iree-hal-target-backends=dummy \
  $LLFORGE_DIR/Test/conv2d_linalg.mlir \
  --mlir-print-ir-after=iree-dummy-lower-to-ll-conv \
  --compile-to=hal
```

### Compile to vmfb (full serialization)

```bash
$IREE_BUILD/tools/iree-compile \
  --iree-hal-target-backends=dummy \
  $LLFORGE_DIR/Test/conv2d_linalg.mlir \
  -o /tmp/dummy.vmfb
```

### Data tiling (see ll.tiled_conv)

Requires an IREE build with `patches/iree` applied.

```bash
# Data tiling: should print ll.tiled_conv with tile_sizes = [4, 8]
$IREE_BUILD/tools/iree-compile $LLFORGE_DIR/Test/conv2d_nhwc_i8_data_tiling.mlir \
  --iree-hal-target-backends=dummy \
  --iree-dispatch-creation-data-tiling \
  --iree-dispatch-creation-data-tiling-convs \
  --iree-dispatch-creation-enable-fuse-padding-into-linalg-consumer-ops \
  --mlir-print-ir-after=iree-dummy-lower-to-ll-conv \
  --mlir-disable-threading \
  --compile-to=executable-targets -o /dev/null 2> tiled.log
grep -n "ll.tiled_conv\|error" tiled.log

# Same for the NCHW f32 test
$IREE_BUILD/tools/iree-compile $LLFORGE_DIR/Test/conv2d_linalg.mlir \
  --iree-hal-target-backends=dummy \
  --iree-dispatch-creation-data-tiling \
  --iree-dispatch-creation-data-tiling-convs \
  --mlir-print-ir-after=iree-dummy-lower-to-ll-conv \
  --mlir-disable-threading \
  --compile-to=executable-targets -o /dev/null 2> nchw.log
grep -n "ll.tiled_conv\|error" nchw.log
```

For the NHWC test, the expected `ll.tiled_conv` operands are input
`9x5x1x4x8x32xi8` (8x4 tiles plus one halo tile), filter `1x3x3x1x32x32xi8`
and output `8x4x1x4x8x32xi32`.

`--iree-dispatch-creation-enable-fuse-padding-into-linalg-consumer-ops` keeps
the activation `tensor.pad` next to the conv. SetEncoding then encodes the
unpadded activation and records the padding, which shows up on
`ll.tiled_conv` as `padding`, `input_hw` and `input_c`.

### Verify plugin is registered

```bash
$IREE_BUILD/tools/iree-compile --help | grep iree-dummy
```

## Variables

- `$IREE_SRC` — path to IREE source tree
- `$IREE_BUILD` — path to IREE build directory
- `$LLFORGE_DIR` — path to this repository
