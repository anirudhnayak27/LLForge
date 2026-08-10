#ifndef LL_OPS_H_
#define LL_OPS_H_

#include "dummy/IR/LLDialect.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/BuiltinAttributes.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/BuiltinTypes.h"
#include "mlir/Interfaces/SideEffectInterfaces.h"

#define GET_OP_CLASSES
#include "dummy/IR/LLOps.h.inc"

#endif  // LL_OPS_H_
