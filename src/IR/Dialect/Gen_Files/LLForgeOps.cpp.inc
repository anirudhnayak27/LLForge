/*===- TableGen'erated file -------------------------------------*- C++ -*-===*\
|*                                                                            *|
|* Op Definitions                                                             *|
|*                                                                            *|
|* Automatically generated file, do not edit!                                 *|
|* From: LLForgeOps.td                                                        *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#ifdef GET_OP_LIST
#undef GET_OP_LIST

::mlir::LLFORGE::LLForgeWriteOp
#endif  // GET_OP_LIST

#ifdef GET_OP_CLASSES
#undef GET_OP_CLASSES


//===----------------------------------------------------------------------===//
// Local Utility Method Definitions
//===----------------------------------------------------------------------===//

namespace mlir {
namespace LLFORGE {

static ::llvm::LogicalResult __mlir_ods_local_type_constraint_LLForgeOps1(
    ::mlir::Operation *op, ::mlir::Type type, ::llvm::StringRef valueKind,
    unsigned valueIndex) {
  if (!((type.isSignlessInteger(32)))) {
    return op->emitOpError(valueKind) << " #" << valueIndex
        << " must be 32-bit signless integer, but got " << type;
  }
  return ::mlir::success();
}
} // namespace LLFORGE
} // namespace mlir
namespace mlir {
namespace LLFORGE {

//===----------------------------------------------------------------------===//
// ::mlir::LLFORGE::LLForgeWriteOp definitions
//===----------------------------------------------------------------------===//

namespace detail {
} // namespace detail
LLForgeWriteOpAdaptor::LLForgeWriteOpAdaptor(LLForgeWriteOp op) : LLForgeWriteOpGenericAdaptor(op->getOperands(), op) {}

::llvm::LogicalResult LLForgeWriteOpAdaptor::verify(::mlir::Location loc) {
  return ::mlir::success();
}

void LLForgeWriteOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::Value val) {
  odsState.addOperands(val);
}

void LLForgeWriteOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::Value val) {
  odsState.addOperands(val);
  assert(resultTypes.size() == 0u && "mismatched number of results");
  odsState.addTypes(resultTypes);
}

void LLForgeWriteOp::build(::mlir::OpBuilder &, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  assert(operands.size() == 1u && "mismatched number of parameters");
  odsState.addOperands(operands);
  odsState.addAttributes(attributes);
  assert(resultTypes.size() == 0u && "mismatched number of return types");
  odsState.addTypes(resultTypes);
}

::llvm::LogicalResult LLForgeWriteOp::verifyInvariantsImpl() {
  {
    unsigned index = 0; (void)index;
    auto valueGroup0 = getODSOperands(0);

    for (auto v : valueGroup0) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_LLForgeOps1(*this, v.getType(), "operand", index++)))
        return ::mlir::failure();
    }
  }
  return ::mlir::success();
}

::llvm::LogicalResult LLForgeWriteOp::verifyInvariants() {
  return verifyInvariantsImpl();
}

::mlir::ParseResult LLForgeWriteOp::parse(::mlir::OpAsmParser &parser, ::mlir::OperationState &result) {
  ::mlir::OpAsmParser::UnresolvedOperand valRawOperand{};
  ::llvm::ArrayRef<::mlir::OpAsmParser::UnresolvedOperand> valOperands(&valRawOperand, 1);  ::llvm::SMLoc valOperandsLoc;
  (void)valOperandsLoc;
  ::mlir::Type valRawType{};
  ::llvm::ArrayRef<::mlir::Type> valTypes(&valRawType, 1);

  valOperandsLoc = parser.getCurrentLocation();
  if (parser.parseOperand(valRawOperand))
    return ::mlir::failure();
  {
    auto loc = parser.getCurrentLocation();(void)loc;
    if (parser.parseOptionalAttrDict(result.attributes))
      return ::mlir::failure();
  }
  if (parser.parseColon())
    return ::mlir::failure();

  {
    ::mlir::IntegerType type;
    if (parser.parseCustomTypeWithFallback(type))
      return ::mlir::failure();
    valRawType = type;
  }
  if (parser.resolveOperands(valOperands, valTypes, valOperandsLoc, result.operands))
    return ::mlir::failure();
  return ::mlir::success();
}

void LLForgeWriteOp::print(::mlir::OpAsmPrinter &_odsPrinter) {
  _odsPrinter << ' ';
  _odsPrinter << getVal();
  ::llvm::SmallVector<::llvm::StringRef, 2> elidedAttrs;
  _odsPrinter.printOptionalAttrDict((*this)->getAttrs(), elidedAttrs);
  _odsPrinter << ' ' << ":";
  _odsPrinter << ' ';
  {
    auto type = getVal().getType();
    if (auto validType = ::llvm::dyn_cast<::mlir::IntegerType>(type))
      _odsPrinter.printStrippedAttrOrType(validType);
   else
     _odsPrinter << type;
  }
}

} // namespace LLFORGE
} // namespace mlir
MLIR_DEFINE_EXPLICIT_TYPE_ID(::mlir::LLFORGE::LLForgeWriteOp)


#endif  // GET_OP_CLASSES

