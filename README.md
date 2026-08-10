# LLForge — Dummy IREE Compiler Plugin

A minimal out-of-tree IREE compiler hardware plugin demonstrating:
- Custom dialect (`ll`) with an `ll.conv` op
- A transform pass that pattern-matches `linalg.generic` convolution ops and lowers them to `ll.conv`
- Plugin registration and HAL target backend integration

## Structure

```
LLForge/
├── iree_compiler_plugin.cmake           # Root plugin entry point
├── compiler/plugins/target/Dummy/
│   ├── CMakeLists.txt                   # Include dirs
│   └── dummy/
│       ├── CMakeLists.txt               # Plugin registration
│       ├── DummyPluginRegistration.cpp  # PluginSession, dialect + pass registration
│       ├── IR/                          # ll dialect + ll.conv op
│       ├── Target/                      # TargetDevice, TargetBackend
│       └── Transforms/                  # DummyLowerToLLConv pass
└── Test/
    └── conv2d_linalg.mlir               # Test input (NCHW 2D conv)
```

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

### Verify plugin is registered

```bash
$IREE_BUILD/tools/iree-compile --help | grep iree-dummy
```

## Variables

- `$IREE_SRC` — path to IREE source tree
- `$IREE_BUILD` — path to IREE build directory
- `$LLFORGE_DIR` — path to this repository
