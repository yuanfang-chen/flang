//===-- R600ISelDAGToDAG.cpp - A dag to dag inst selector for R600 --------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//==-----------------------------------------------------------------------===//
//
/// \file
/// Defines an instruction selector for the R600 subtarget.
//
//===----------------------------------------------------------------------===//

#include "AMDGPU.h"
#include "AMDGPUISelDAGToDAG.h"
#include "MCTargetDesc/R600MCTargetDesc.h"
#include "R600.h"
#include "R600Subtarget.h"
#include "llvm/Analysis/ValueTracking.h"

namespace {
class R600DAGToDAGISel : public AMDGPUDAGToDAGISel {
  const R600Subtarget *Subtarget = nullptr;

  bool isConstantLoad(const MemSDNode *N, int cbID) const;
  bool SelectGlobalValueConstantOffset(SDValue Addr, SDValue &IntPtr);
  bool SelectGlobalValueVariableOffset(SDValue Addr, SDValue &BaseReg,
                                       SDValue &Offset);

public:
  R600DAGToDAGISel() = delete;

  explicit R600DAGToDAGISel(TargetMachine &TM, CodeGenOpt::Level OptLevel)
      : AMDGPUDAGToDAGISel(TM, OptLevel) {}

  void Select(SDNode *N) override;

  bool SelectADDRIndirect(SDValue Addr, SDValue &Base,
                          SDValue &Offset) override;
  bool SelectADDRVTX_READ(SDValue Addr, SDValue &Base,
                          SDValue &Offset) override;

  bool runOnMachineFunction(MachineFunction &MF) override;

  void PreprocessISelDAG() override {}

protected:
  // Include the pieces autogenerated from the target description.
#include "R600GenDAGISel.inc"
};
} // namespace

bool R600DAGToDAGISel::runOnMachineFunction(MachineFunction &MF) {
  Subtarget = &MF.getSubtarget<R600Subtarget>();
  return SelectionDAGISel::runOnMachineFunction(MF);
}

bool R600DAGToDAGISel::isConstantLoad(const MemSDNode *N, int CbId) const {
  if (!N->readMem())
    return false;
  if (CbId == -1)
    return N->getAddressSpace() == AMDGPUAS::CONSTANT_ADDRESS ||
           N->getAddressSpace() == AMDGPUAS::CONSTANT_ADDRESS_32BIT;

  return N->getAddressSpace() == AMDGPUAS::CONSTANT_BUFFER_0 + CbId;
}

bool R600DAGToDAGISel::SelectGlobalValueConstantOffset(SDValue Addr,
                                                       SDValue &IntPtr) {
  if (ConstantSDNode *Cst = dyn_cast<ConstantSDNode>(Addr)) {
    IntPtr =
        CurDAG->getIntPtrConstant(Cst->getZExtValue() / 4, SDLoc(Addr), true);
    return true;
  }
  return false;
}

bool R600DAGToDAGISel::SelectGlobalValueVariableOffset(SDValue Addr,
                                                       SDValue &BaseReg,
                                                       SDValue &Offset) {
  if (!isa<ConstantSDNode>(Addr)) {
    BaseReg = Addr;
    Offset = CurDAG->getIntPtrConstant(0, SDLoc(Addr), true);
    return true;
  }
  return false;
}

void R600DAGToDAGISel::Select(SDNode *N) {
  unsigned int Opc = N->getOpcode();
  if (N->isMachineOpcode()) {
    N->setNodeId(-1);
    return; // Already selected.
  }

  switch (Opc) {
  default:
    break;
  case AMDGPUISD::BUILD_VERTICAL_VECTOR:
  case ISD::SCALAR_TO_VECTOR:
  case ISD::BUILD_VECTOR: {
    EVT VT = N->getValueType(0);
    unsigned NumVectorElts = VT.getVectorNumElements();
    unsigned RegClassID;
    // BUILD_VECTOR was lowered into an IMPLICIT_DEF + 4 INSERT_SUBREG
    // that adds a 128 bits reg copy when going through TwoAddressInstructions
    // pass. We want to avoid 128 bits copies as much as possible because they
    // can't be bundled by our scheduler.
    switch (NumVectorElts) {
    case 2:
      RegClassID = R600::R600_Reg64RegClassID;
      break;
    case 4:
      if (Opc == AMDGPUISD::BUILD_VERTICAL_VECTOR)
        RegClassID = R600::R600_Reg128VerticalRegClassID;
      else
        RegClassID = R600::R600_Reg128RegClassID;
      break;
    default:
      llvm_unreachable("Do not know how to lower this BUILD_VECTOR");
    }
    SelectBuildVector(N, RegClassID);
    return;
  }
  }

  SelectCode(N);
}

bool R600DAGToDAGISel::SelectADDRIndirect(SDValue Addr, SDValue &Base,
                                          SDValue &Offset) {
  ConstantSDNode *C;
  SDLoc DL(Addr);

  if ((C = dyn_cast<ConstantSDNode>(Addr))) {
    Base = CurDAG->getRegister(R600::INDIRECT_BASE_ADDR, MVT::i32);
    Offset = CurDAG->getTargetConstant(C->getZExtValue(), DL, MVT::i32);
  } else if ((Addr.getOpcode() == AMDGPUISD::DWORDADDR) &&
             (C = dyn_cast<ConstantSDNode>(Addr.getOperand(0)))) {
    Base = CurDAG->getRegister(R600::INDIRECT_BASE_ADDR, MVT::i32);
    Offset = CurDAG->getTargetConstant(C->getZExtValue(), DL, MVT::i32);
  } else if ((Addr.getOpcode() == ISD::ADD || Addr.getOpcode() == ISD::OR) &&
             (C = dyn_cast<ConstantSDNode>(Addr.getOperand(1)))) {
    Base = Addr.getOperand(0);
    Offset = CurDAG->getTargetConstant(C->getZExtValue(), DL, MVT::i32);
  } else {
    Base = Addr;
    Offset = CurDAG->getTargetConstant(0, DL, MVT::i32);
  }

  return true;
}

bool R600DAGToDAGISel::SelectADDRVTX_READ(SDValue Addr, SDValue &Base,
                                          SDValue &Offset) {
  ConstantSDNode *IMMOffset;

  if (Addr.getOpcode() == ISD::ADD &&
      (IMMOffset = dyn_cast<ConstantSDNode>(Addr.getOperand(1))) &&
      isInt<16>(IMMOffset->getZExtValue())) {

    Base = Addr.getOperand(0);
    Offset = CurDAG->getTargetConstant(IMMOffset->getZExtValue(), SDLoc(Addr),
                                       MVT::i32);
    return true;
    // If the pointer address is constant, we can move it to the offset field.
  } else if ((IMMOffset = dyn_cast<ConstantSDNode>(Addr)) &&
             isInt<16>(IMMOffset->getZExtValue())) {
    Base = CurDAG->getCopyFromReg(CurDAG->getEntryNode(),
                                  SDLoc(CurDAG->getEntryNode()), R600::ZERO,
                                  MVT::i32);
    Offset = CurDAG->getTargetConstant(IMMOffset->getZExtValue(), SDLoc(Addr),
                                       MVT::i32);
    return true;
  }

  // Default case, no offset
  Base = Addr;
  Offset = CurDAG->getTargetConstant(0, SDLoc(Addr), MVT::i32);
  return true;
}

/// This pass converts a legalized DAG into a R600-specific
// DAG, ready for instruction scheduling.
FunctionPass *llvm::createR600ISelDag(TargetMachine &TM,
                                      CodeGenOpt::Level OptLevel) {
  return new R600DAGToDAGISel(TM, OptLevel);
}