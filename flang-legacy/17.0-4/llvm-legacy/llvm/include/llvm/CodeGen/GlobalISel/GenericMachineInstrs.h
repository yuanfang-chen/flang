//===- llvm/CodeGen/GlobalISel/GenericMachineInstrs.h -----------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file
/// Declares convenience wrapper classes for interpreting MachineInstr instances
/// as specific generic operations.
///
//===----------------------------------------------------------------------===//

#ifndef LLVM_CODEGEN_GLOBALISEL_GENERICMACHINEINSTRS_H
#define LLVM_CODEGEN_GLOBALISEL_GENERICMACHINEINSTRS_H

#include "llvm/IR/Instructions.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineMemOperand.h"
#include "llvm/CodeGen/TargetOpcodes.h"
#include "llvm/Support/Casting.h"

namespace llvm {

/// A base class for all GenericMachineInstrs.
class GenericMachineInstr : public MachineInstr {
public:
  GenericMachineInstr() = delete;

  /// Access the Idx'th operand as a register and return it.
  /// This assumes that the Idx'th operand is a Register type.
  Register getReg(unsigned Idx) const { return getOperand(Idx).getReg(); }

  static bool classof(const MachineInstr *MI) {
    return isPreISelGenericOpcode(MI->getOpcode());
  }
};

/// Represents any type of generic load or store.
/// G_LOAD, G_STORE, G_ZEXTLOAD, G_SEXTLOAD.
class GLoadStore : public GenericMachineInstr {
public:
  /// Get the source register of the pointer value.
  Register getPointerReg() const { return getOperand(1).getReg(); }

  /// Get the MachineMemOperand on this instruction.
  MachineMemOperand &getMMO() const { return **memoperands_begin(); }

  /// Returns true if the attached MachineMemOperand  has the atomic flag set.
  bool isAtomic() const { return getMMO().isAtomic(); }
  /// Returns true if the attached MachineMemOpeand as the volatile flag set.
  bool isVolatile() const { return getMMO().isVolatile(); }
  /// Returns true if the memory operation is neither atomic or volatile.
  bool isSimple() const { return !isAtomic() && !isVolatile(); }
  /// Returns true if this memory operation doesn't have any ordering
  /// constraints other than normal aliasing. Volatile and (ordered) atomic
  /// memory operations can't be reordered.
  bool isUnordered() const { return getMMO().isUnordered(); }

  /// Returns the size in bytes of the memory access.
  uint64_t getMemSize() const { return getMMO().getSize();
  } /// Returns the size in bits of the memory access.
  uint64_t getMemSizeInBits() const { return getMMO().getSizeInBits(); }

  static bool classof(const MachineInstr *MI) {
    switch (MI->getOpcode()) {
    case TargetOpcode::G_LOAD:
    case TargetOpcode::G_STORE:
    case TargetOpcode::G_ZEXTLOAD:
    case TargetOpcode::G_SEXTLOAD:
      return true;
    default:
      return false;
    }
  }
};

/// Represents any generic load, including sign/zero extending variants.
class GAnyLoad : public GLoadStore {
public:
  /// Get the definition register of the loaded value.
  Register getDstReg() const { return getOperand(0).getReg(); }

  static bool classof(const MachineInstr *MI) {
    switch (MI->getOpcode()) {
    case TargetOpcode::G_LOAD:
    case TargetOpcode::G_ZEXTLOAD:
    case TargetOpcode::G_SEXTLOAD:
      return true;
    default:
      return false;
    }
  }
};

/// Represents a G_LOAD.
class GLoad : public GAnyLoad {
public:
  static bool classof(const MachineInstr *MI) {
    return MI->getOpcode() == TargetOpcode::G_LOAD;
  }
};

/// Represents either a G_SEXTLOAD or G_ZEXTLOAD.
class GExtLoad : public GAnyLoad {
public:
  static bool classof(const MachineInstr *MI) {
    return MI->getOpcode() == TargetOpcode::G_SEXTLOAD ||
           MI->getOpcode() == TargetOpcode::G_ZEXTLOAD;
  }
};

/// Represents a G_SEXTLOAD.
class GSExtLoad : public GExtLoad {
public:
  static bool classof(const MachineInstr *MI) {
    return MI->getOpcode() == TargetOpcode::G_SEXTLOAD;
  }
};

/// Represents a G_ZEXTLOAD.
class GZExtLoad : public GExtLoad {
public:
  static bool classof(const MachineInstr *MI) {
    return MI->getOpcode() == TargetOpcode::G_ZEXTLOAD;
  }
};

/// Represents a G_STORE.
class GStore : public GLoadStore {
public:
  /// Get the stored value register.
  Register getValueReg() const { return getOperand(0).getReg(); }

  static bool classof(const MachineInstr *MI) {
    return MI->getOpcode() == TargetOpcode::G_STORE;
  }
};

/// Represents a G_UNMERGE_VALUES.
class GUnmerge : public GenericMachineInstr {
public:
  /// Returns the number of def registers.
  unsigned getNumDefs() const { return getNumOperands() - 1; }
  /// Get the unmerge source register.
  Register getSourceReg() const { return getOperand(getNumDefs()).getReg(); }

  static bool classof(const MachineInstr *MI) {
    return MI->getOpcode() == TargetOpcode::G_UNMERGE_VALUES;
  }
};

/// Represents G_BUILD_VECTOR, G_CONCAT_VECTORS or G_MERGE_VALUES.
/// All these have the common property of generating a single value from
/// multiple sources.
class GMergeLikeInstr : public GenericMachineInstr {
public:
  /// Returns the number of source registers.
  unsigned getNumSources() const { return getNumOperands() - 1; }
  /// Returns the I'th source register.
  Register getSourceReg(unsigned I) const { return getReg(I + 1); }

  static bool classof(const MachineInstr *MI) {
    switch (MI->getOpcode()) {
    case TargetOpcode::G_MERGE_VALUES:
    case TargetOpcode::G_CONCAT_VECTORS:
    case TargetOpcode::G_BUILD_VECTOR:
      return true;
    default:
      return false;
    }
  }
};

/// Represents a G_MERGE_VALUES.
class GMerge : public GMergeLikeInstr {
public:
  static bool classof(const MachineInstr *MI) {
    return MI->getOpcode() == TargetOpcode::G_MERGE_VALUES;
  }
};

/// Represents a G_CONCAT_VECTORS.
class GConcatVectors : public GMergeLikeInstr {
public:
  static bool classof(const MachineInstr *MI) {
    return MI->getOpcode() == TargetOpcode::G_CONCAT_VECTORS;
  }
};

/// Represents a G_BUILD_VECTOR.
class GBuildVector : public GMergeLikeInstr {
public:
  static bool classof(const MachineInstr *MI) {
    return MI->getOpcode() == TargetOpcode::G_BUILD_VECTOR;
  }
};

/// Represents a G_PTR_ADD.
class GPtrAdd : public GenericMachineInstr {
public:
  Register getBaseReg() const { return getReg(1); }
  Register getOffsetReg() const { return getReg(2); }

  static bool classof(const MachineInstr *MI) {
    return MI->getOpcode() == TargetOpcode::G_PTR_ADD;
  }
};

/// Represents a G_IMPLICIT_DEF.
class GImplicitDef : public GenericMachineInstr {
public:
  static bool classof(const MachineInstr *MI) {
    return MI->getOpcode() == TargetOpcode::G_IMPLICIT_DEF;
  }
};

/// Represents a G_SELECT.
class GSelect : public GenericMachineInstr {
public:
  Register getCondReg() const { return getReg(1); }
  Register getTrueReg() const { return getReg(2); }
  Register getFalseReg() const { return getReg(3); }

  static bool classof(const MachineInstr *MI) {
    return MI->getOpcode() == TargetOpcode::G_SELECT;
  }
};

/// Represent a G_ICMP or G_FCMP.
class GAnyCmp : public GenericMachineInstr {
public:
  CmpInst::Predicate getCond() const {
    return static_cast<CmpInst::Predicate>(getOperand(1).getPredicate());
  }
  Register getLHSReg() const { return getReg(2); }
  Register getRHSReg() const { return getReg(3); }

  static bool classof(const MachineInstr *MI) {
    return MI->getOpcode() == TargetOpcode::G_ICMP ||
           MI->getOpcode() == TargetOpcode::G_FCMP;
  }
};

/// Represent a G_ICMP.
class GICmp : public GAnyCmp {
public:
  static bool classof(const MachineInstr *MI) {
    return MI->getOpcode() == TargetOpcode::G_ICMP;
  }
};

/// Represent a G_FCMP.
class GFCmp : public GAnyCmp {
public:
  static bool classof(const MachineInstr *MI) {
    return MI->getOpcode() == TargetOpcode::G_FCMP;
  }
};

/// Represents overflowing binary operations.
/// Only carry-out:
/// G_UADDO, G_SADDO, G_USUBO, G_SSUBO, G_UMULO, G_SMULO
/// Carry-in and carry-out:
/// G_UADDE, G_SADDE, G_USUBE, G_SSUBE
class GBinOpCarryOut : public GenericMachineInstr {
public:
  Register getDstReg() const { return getReg(0); }
  Register getCarryOutReg() const { return getReg(1); }
  MachineOperand &getLHS() { return getOperand(2); }
  MachineOperand &getRHS() { return getOperand(3); }

  static bool classof(const MachineInstr *MI) {
    switch (MI->getOpcode()) {
    case TargetOpcode::G_UADDO:
    case TargetOpcode::G_SADDO:
    case TargetOpcode::G_USUBO:
    case TargetOpcode::G_SSUBO:
    case TargetOpcode::G_UADDE:
    case TargetOpcode::G_SADDE:
    case TargetOpcode::G_USUBE:
    case TargetOpcode::G_SSUBE:
    case TargetOpcode::G_UMULO:
    case TargetOpcode::G_SMULO:
      return true;
    default:
      return false;
    }
  }
};

/// Represents overflowing add/sub operations.
/// Only carry-out:
/// G_UADDO, G_SADDO, G_USUBO, G_SSUBO
/// Carry-in and carry-out:
/// G_UADDE, G_SADDE, G_USUBE, G_SSUBE
class GAddSubCarryOut : public GBinOpCarryOut {
public:
  bool isAdd() const {
    switch (getOpcode()) {
    case TargetOpcode::G_UADDO:
    case TargetOpcode::G_SADDO:
    case TargetOpcode::G_UADDE:
    case TargetOpcode::G_SADDE:
      return true;
    default:
      return false;
    }
  }
  bool isSub() const { return !isAdd(); }

  bool isSigned() const {
    switch (getOpcode()) {
    case TargetOpcode::G_SADDO:
    case TargetOpcode::G_SSUBO:
    case TargetOpcode::G_SADDE:
    case TargetOpcode::G_SSUBE:
      return true;
    default:
      return false;
    }
  }
  bool isUnsigned() const { return !isSigned(); }

  static bool classof(const MachineInstr *MI) {
    switch (MI->getOpcode()) {
    case TargetOpcode::G_UADDO:
    case TargetOpcode::G_SADDO:
    case TargetOpcode::G_USUBO:
    case TargetOpcode::G_SSUBO:
    case TargetOpcode::G_UADDE:
    case TargetOpcode::G_SADDE:
    case TargetOpcode::G_USUBE:
    case TargetOpcode::G_SSUBE:
      return true;
    default:
      return false;
    }
  }
};

/// Represents overflowing add/sub operations that also consume a carry-in.
/// G_UADDE, G_SADDE, G_USUBE, G_SSUBE
class GAddSubCarryInOut : public GAddSubCarryOut {
public:
  Register getCarryInReg() const { return getReg(4); }

  static bool classof(const MachineInstr *MI) {
    switch (MI->getOpcode()) {
    case TargetOpcode::G_UADDE:
    case TargetOpcode::G_SADDE:
    case TargetOpcode::G_USUBE:
    case TargetOpcode::G_SSUBE:
      return true;
    default:
      return false;
    }
  }
};

/// Represents a call to an intrinsic.
class GIntrinsic final : public GenericMachineInstr {
public:
  Intrinsic::ID getIntrinsicID() const {
    return getOperand(getNumExplicitDefs()).getIntrinsicID();
  }

  bool is(Intrinsic::ID ID) const { return getIntrinsicID() == ID; }
  bool hasSideEffects() const {
    return getOpcode() == TargetOpcode::G_INTRINSIC_W_SIDE_EFFECTS;
  }

  static bool classof(const MachineInstr *MI) {
    switch (MI->getOpcode()) {
    case TargetOpcode::G_INTRINSIC:
    case TargetOpcode::G_INTRINSIC_W_SIDE_EFFECTS:
      return true;
    default:
      return false;
    }
  }
};

} // namespace llvm

#endif // LLVM_CODEGEN_GLOBALISEL_GENERICMACHINEINSTRS_H
