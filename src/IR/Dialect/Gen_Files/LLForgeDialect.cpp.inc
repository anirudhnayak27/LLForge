/*===- TableGen'erated file -------------------------------------*- C++ -*-===*\
|*                                                                            *|
|* Dialect Definitions                                                        *|
|*                                                                            *|
|* Automatically generated file, do not edit!                                 *|
|* From: LLForgeDialect.td                                                    *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

MLIR_DEFINE_EXPLICIT_TYPE_ID(::mlir::LLFORGE::llforgedialect)
namespace mlir {
namespace LLFORGE {

llforgedialect::llforgedialect(::mlir::MLIRContext *context)
    : ::mlir::Dialect(getDialectNamespace(), context, ::mlir::TypeID::get<llforgedialect>())
    
     {
  
  initialize();
}

llforgedialect::~llforgedialect() = default;

} // namespace LLFORGE
} // namespace mlir
