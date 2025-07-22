/*===- TableGen'erated file -------------------------------------*- C++ -*-===*\
|*                                                                            *|
|* Dialect Definitions                                                        *|
|*                                                                            *|
|* Automatically generated file, do not edit!                                 *|
|* From: LLForgeDialect.td                                                    *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

MLIR_DEFINE_EXPLICIT_TYPE_ID(::mlir::LLFORGE::LLForgeDialect)
namespace mlir {
namespace LLFORGE {

LLForgeDialect::LLForgeDialect(::mlir::MLIRContext *context)
    : ::mlir::Dialect(getDialectNamespace(), context, ::mlir::TypeID::get<LLForgeDialect>())
    
     {
  
  initialize();
}

LLForgeDialect::~LLForgeDialect() = default;

} // namespace LLFORGE
} // namespace mlir
