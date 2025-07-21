/*===- TableGen'erated file -------------------------------------*- C++ -*-===*\
|*                                                                            *|
|* Dialect Declarations                                                       *|
|*                                                                            *|
|* Automatically generated file, do not edit!                                 *|
|* From: LLForgeDialect.td                                                    *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

namespace mlir {
namespace LLFORGE {

class llforgedialect : public ::mlir::Dialect {
  explicit llforgedialect(::mlir::MLIRContext *context);

  void initialize();
  friend class ::mlir::MLIRContext;
public:
  ~llforgedialect() override;
  static constexpr ::llvm::StringLiteral getDialectNamespace() {
    return ::llvm::StringLiteral("llforge");
  }
};
} // namespace LLFORGE
} // namespace mlir
MLIR_DECLARE_EXPLICIT_TYPE_ID(::mlir::LLFORGE::llforgedialect)
