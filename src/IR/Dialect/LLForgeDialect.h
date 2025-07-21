#ifndef LLFORGE_DIALECT_H
#define LLFORGE_DIALECT_H

#include "mlir/IR/Dialect.h"
#include "mlir/IR/IRMapping.h"
#include "mlir/IR/Operation.h"
#include "mlir/IR/OpDefinition.h"
#include "mlir/IR/OpImplementation.h"
#include "llvm/Support/SMLoc.h"
#include "mlir/Interfaces/CallInterfaces.h"
#include "mlir/Interfaces/CastInterfaces.h"
#include "mlir/Interfaces/FunctionInterfaces.h"
#include "mlir/Interfaces/SideEffectInterfaces.h"

#include "Gen_Files/LLForgeDialect.h.inc"

#define GET_OP_CLASSES
#include "Gen_Files/LLForgeOps.h.inc"


#endif //LLFORGE_DIALECT_H