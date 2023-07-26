//===- AArch64MIPeepholeOpt.cpp - AArch64 MI peephole optimization pass ---===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This pass performs below peephole optimizations on MIR level.
//
// 1. MOVi32imm + ANDWrr ==> ANDWri + ANDWri
//    MOVi64imm + ANDXrr ==> ANDXri + ANDXri
//
// 2. MOVi32imm + ADDWrr ==> ADDWRi + ADDWRi
//    MOVi64imm + ADDXrr ==> ANDXri + ANDXri
//
// 3. MOVi32imm + SUBWrr ==> SUBWRi + SUBWRi
//    MOVi64imm + SUBXrr ==> SUBXri + SUBXri
//
//    The mov pseudo instruction could be expanded to multiple mov instructions
//    later. In this case, we could try to split the constant  operand of mov
//    instruction into two immediates which can be directly encoded into
//    *Wri/*Xri instructions. It makes two AND/ADD/SUB instructions instead of
//    multiple `mov` + `and/add/sub` instructions.
//
// 4. Remove redundant ORRWrs which is generated by zero-extend.
//
//    %3:gpr32 = ORRWrs $wzr, %2, 0
//    %4:gpr64 = SUBREG_TO_REG 0, %3, %subreg.sub_32
//
//    If AArch64's 32-bit form of instruction defines the source operand of
//    ORRWrs, we can remove the ORRWrs because the upper 32 bits of the source
//    operand are set to zero.
//
// 5. %reg = INSERT_SUBREG %reg(tied-def 0), %subreg, subidx
//     ==> %reg:subidx =  SUBREG_TO_REG 0, %subreg, subidx
//
// 6. %intermediate:gpr32 = COPY %src:fpr128
//    %dst:fpr128 = INSvi32gpr %dst_vec:fpr128, dst_index, %intermediate:gpr32
//     ==> %dst:fpr128 = INSvi32lane %dst_vec:fpr128, dst_index, %src:fpr128, 0
//
//    In cases where a source FPR is copied to a GPR in order to be copied
//    to a destination FPR, we can directly copy the values between the FPRs,
//    eliminating the use of the Integer unit. When we match a pattern of
//    INSvi[X]gpr that is preceded by a chain of COPY instructions from a FPR
//    source, we use the INSvi[X]lane to replace the COPY & INSvi[X]gpr
//    instructions.
//
// 7. If MI sets zero for high 64-bits implicitly, remove `mov 0` for high
//    64-bits. For example,
//
//   %1:fpr64 = nofpexcept FCVTNv4i16 %0:fpr128, implicit $fpcr
//   %2:fpr64 = MOVID 0
//   %4:fpr128 = IMPLICIT_DEF
//   %3:fpr128 = INSERT_SUBREG %4:fpr128(tied-def 0), killed %2:fpr64, %subreg.dsub
//   %6:fpr128 = IMPLICIT_DEF
//   %5:fpr128 = INSERT_SUBREG %6:fpr128(tied-def 0), killed %1:fpr64, %subreg.dsub
//   %7:fpr128 = INSvi64lane %5:fpr128(tied-def 0), 1, killed %3:fpr128, 0
//   ==>
//   %1:fpr64 = nofpexcept FCVTNv4i16 %0:fpr128, implicit $fpcr
//   %6:fpr128 = IMPLICIT_DEF
//   %7:fpr128 = INSERT_SUBREG %6:fpr128(tied-def 0), killed %1:fpr64, %subreg.dsub
//
//===----------------------------------------------------------------------===//

#include "AArch64ExpandImm.h"
#include "AArch64InstrInfo.h"
#include "MCTargetDesc/AArch64AddressingModes.h"
#include "llvm/CodeGen/MachineDominators.h"
#include "llvm/CodeGen/MachineLoopInfo.h"

using namespace llvm;

#define DEBUG_TYPE "aarch64-mi-peephole-opt"

namespace {

struct AArch64MIPeepholeOpt : public MachineFunctionPass {
  static char ID;

  AArch64MIPeepholeOpt() : MachineFunctionPass(ID) {
    initializeAArch64MIPeepholeOptPass(*PassRegistry::getPassRegistry());
  }

  const AArch64InstrInfo *TII;
  const AArch64RegisterInfo *TRI;
  MachineLoopInfo *MLI;
  MachineRegisterInfo *MRI;

  using OpcodePair = std::pair<unsigned, unsigned>;
  template <typename T>
  using SplitAndOpcFunc =
      std::function<std::optional<OpcodePair>(T, unsigned, T &, T &)>;
  using BuildMIFunc =
      std::function<void(MachineInstr &, OpcodePair, unsigned, unsigned,
                         Register, Register, Register)>;

  /// For instructions where an immediate operand could be split into two
  /// separate immediate instructions, use the splitTwoPartImm two handle the
  /// optimization.
  ///
  /// To implement, the following function types must be passed to
  /// splitTwoPartImm. A SplitAndOpcFunc must be implemented that determines if
  /// splitting the immediate is valid and returns the associated new opcode. A
  /// BuildMIFunc must be implemented to build the two immediate instructions.
  ///
  /// Example Pattern (where IMM would require 2+ MOV instructions):
  ///     %dst = <Instr>rr %src IMM [...]
  /// becomes:
  ///     %tmp = <Instr>ri %src (encode half IMM) [...]
  ///     %dst = <Instr>ri %tmp (encode half IMM) [...]
  template <typename T>
  bool splitTwoPartImm(MachineInstr &MI,
                       SplitAndOpcFunc<T> SplitAndOpc, BuildMIFunc BuildInstr);

  bool checkMovImmInstr(MachineInstr &MI, MachineInstr *&MovMI,
                        MachineInstr *&SubregToRegMI);

  template <typename T>
  bool visitADDSUB(unsigned PosOpc, unsigned NegOpc, MachineInstr &MI);
  template <typename T>
  bool visitADDSSUBS(OpcodePair PosOpcs, OpcodePair NegOpcs, MachineInstr &MI);

  template <typename T>
  bool visitAND(unsigned Opc, MachineInstr &MI);
  bool visitORR(MachineInstr &MI);
  bool visitINSERT(MachineInstr &MI);
  bool visitINSviGPR(MachineInstr &MI, unsigned Opc);
  bool visitINSvi64lane(MachineInstr &MI);
  bool runOnMachineFunction(MachineFunction &MF) override;

  StringRef getPassName() const override {
    return "AArch64 MI Peephole Optimization pass";
  }

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.setPreservesCFG();
    AU.addRequired<MachineLoopInfo>();
    MachineFunctionPass::getAnalysisUsage(AU);
  }
};

char AArch64MIPeepholeOpt::ID = 0;

} // end anonymous namespace

INITIALIZE_PASS(AArch64MIPeepholeOpt, "aarch64-mi-peephole-opt",
                "AArch64 MI Peephole Optimization", false, false)

template <typename T>
static bool splitBitmaskImm(T Imm, unsigned RegSize, T &Imm1Enc, T &Imm2Enc) {
  T UImm = static_cast<T>(Imm);
  if (AArch64_AM::isLogicalImmediate(UImm, RegSize))
    return false;

  // If this immediate can be handled by one instruction, do not split it.
  SmallVector<AArch64_IMM::ImmInsnModel, 4> Insn;
  AArch64_IMM::expandMOVImm(UImm, RegSize, Insn);
  if (Insn.size() == 1)
    return false;

  // The bitmask immediate consists of consecutive ones.  Let's say there is
  // constant 0b00000000001000000000010000000000 which does not consist of
  // consecutive ones. We can split it in to two bitmask immediate like
  // 0b00000000001111111111110000000000 and 0b11111111111000000000011111111111.
  // If we do AND with these two bitmask immediate, we can see original one.
  unsigned LowestBitSet = llvm::countr_zero(UImm);
  unsigned HighestBitSet = Log2_64(UImm);

  // Create a mask which is filled with one from the position of lowest bit set
  // to the position of highest bit set.
  T NewImm1 = (static_cast<T>(2) << HighestBitSet) -
              (static_cast<T>(1) << LowestBitSet);
  // Create a mask which is filled with one outside the position of lowest bit
  // set and the position of highest bit set.
  T NewImm2 = UImm | ~NewImm1;

  // If the split value is not valid bitmask immediate, do not split this
  // constant.
  if (!AArch64_AM::isLogicalImmediate(NewImm2, RegSize))
    return false;

  Imm1Enc = AArch64_AM::encodeLogicalImmediate(NewImm1, RegSize);
  Imm2Enc = AArch64_AM::encodeLogicalImmediate(NewImm2, RegSize);
  return true;
}

template <typename T>
bool AArch64MIPeepholeOpt::visitAND(
    unsigned Opc, MachineInstr &MI) {
  // Try below transformation.
  //
  // MOVi32imm + ANDWrr ==> ANDWri + ANDWri
  // MOVi64imm + ANDXrr ==> ANDXri + ANDXri
  //
  // The mov pseudo instruction could be expanded to multiple mov instructions
  // later. Let's try to split the constant operand of mov instruction into two
  // bitmask immediates. It makes only two AND instructions intead of multiple
  // mov + and instructions.

  return splitTwoPartImm<T>(
      MI,
      [Opc](T Imm, unsigned RegSize, T &Imm0,
            T &Imm1) -> std::optional<OpcodePair> {
        if (splitBitmaskImm(Imm, RegSize, Imm0, Imm1))
          return std::make_pair(Opc, Opc);
        return std::nullopt;
      },
      [&TII = TII](MachineInstr &MI, OpcodePair Opcode, unsigned Imm0,
                   unsigned Imm1, Register SrcReg, Register NewTmpReg,
                   Register NewDstReg) {
        DebugLoc DL = MI.getDebugLoc();
        MachineBasicBlock *MBB = MI.getParent();
        BuildMI(*MBB, MI, DL, TII->get(Opcode.first), NewTmpReg)
            .addReg(SrcReg)
            .addImm(Imm0);
        BuildMI(*MBB, MI, DL, TII->get(Opcode.second), NewDstReg)
            .addReg(NewTmpReg)
            .addImm(Imm1);
      });
}

bool AArch64MIPeepholeOpt::visitORR(MachineInstr &MI) {
  // Check this ORR comes from below zero-extend pattern.
  //
  // def : Pat<(i64 (zext GPR32:$src)),
  //           (SUBREG_TO_REG (i32 0), (ORRWrs WZR, GPR32:$src, 0), sub_32)>;
  if (MI.getOperand(3).getImm() != 0)
    return false;

  if (MI.getOperand(1).getReg() != AArch64::WZR)
    return false;

  MachineInstr *SrcMI = MRI->getUniqueVRegDef(MI.getOperand(2).getReg());
  if (!SrcMI)
    return false;

  // From https://developer.arm.com/documentation/dui0801/b/BABBGCAC
  //
  // When you use the 32-bit form of an instruction, the upper 32 bits of the
  // source registers are ignored and the upper 32 bits of the destination
  // register are set to zero.
  //
  // If AArch64's 32-bit form of instruction defines the source operand of
  // zero-extend, we do not need the zero-extend. Let's check the MI's opcode is
  // real AArch64 instruction and if it is not, do not process the opcode
  // conservatively.
  if (SrcMI->getOpcode() == TargetOpcode::COPY &&
      SrcMI->getOperand(1).getReg().isVirtual()) {
    const TargetRegisterClass *RC =
        MRI->getRegClass(SrcMI->getOperand(1).getReg());

    // A COPY from an FPR will become a FMOVSWr, so do so now so that we know
    // that the upper bits are zero.
    if (RC != &AArch64::FPR32RegClass &&
        ((RC != &AArch64::FPR64RegClass && RC != &AArch64::FPR128RegClass) ||
         SrcMI->getOperand(1).getSubReg() != AArch64::ssub))
      return false;
    Register CpySrc = SrcMI->getOperand(1).getReg();
    if (SrcMI->getOperand(1).getSubReg() == AArch64::ssub) {
      CpySrc = MRI->createVirtualRegister(&AArch64::FPR32RegClass);
      BuildMI(*SrcMI->getParent(), SrcMI, SrcMI->getDebugLoc(),
              TII->get(TargetOpcode::COPY), CpySrc)
          .add(SrcMI->getOperand(1));
    }
    BuildMI(*SrcMI->getParent(), SrcMI, SrcMI->getDebugLoc(),
            TII->get(AArch64::FMOVSWr), SrcMI->getOperand(0).getReg())
        .addReg(CpySrc);
    SrcMI->eraseFromParent();
  }
  else if (SrcMI->getOpcode() <= TargetOpcode::GENERIC_OP_END)
    return false;

  Register DefReg = MI.getOperand(0).getReg();
  Register SrcReg = MI.getOperand(2).getReg();
  MRI->replaceRegWith(DefReg, SrcReg);
  MRI->clearKillFlags(SrcReg);
  LLVM_DEBUG(dbgs() << "Removed: " << MI << "\n");
  MI.eraseFromParent();

  return true;
}

bool AArch64MIPeepholeOpt::visitINSERT(MachineInstr &MI) {
  // Check this INSERT_SUBREG comes from below zero-extend pattern.
  //
  // From %reg = INSERT_SUBREG %reg(tied-def 0), %subreg, subidx
  // To   %reg:subidx =  SUBREG_TO_REG 0, %subreg, subidx
  //
  // We're assuming the first operand to INSERT_SUBREG is irrelevant because a
  // COPY would destroy the upper part of the register anyway
  if (!MI.isRegTiedToDefOperand(1))
    return false;

  Register DstReg = MI.getOperand(0).getReg();
  const TargetRegisterClass *RC = MRI->getRegClass(DstReg);
  MachineInstr *SrcMI = MRI->getUniqueVRegDef(MI.getOperand(2).getReg());
  if (!SrcMI)
    return false;

  // From https://developer.arm.com/documentation/dui0801/b/BABBGCAC
  //
  // When you use the 32-bit form of an instruction, the upper 32 bits of the
  // source registers are ignored and the upper 32 bits of the destination
  // register are set to zero.
  //
  // If AArch64's 32-bit form of instruction defines the source operand of
  // zero-extend, we do not need the zero-extend. Let's check the MI's opcode is
  // real AArch64 instruction and if it is not, do not process the opcode
  // conservatively.
  if ((SrcMI->getOpcode() <= TargetOpcode::GENERIC_OP_END) ||
      !AArch64::GPR64allRegClass.hasSubClassEq(RC))
    return false;

  // Build a SUBREG_TO_REG instruction
  MachineInstr *SubregMI =
      BuildMI(*MI.getParent(), MI, MI.getDebugLoc(),
              TII->get(TargetOpcode::SUBREG_TO_REG), DstReg)
          .addImm(0)
          .add(MI.getOperand(2))
          .add(MI.getOperand(3));
  LLVM_DEBUG(dbgs() << MI << "  replace by:\n: " << *SubregMI << "\n");
  (void)SubregMI;
  MI.eraseFromParent();

  return true;
}

template <typename T>
static bool splitAddSubImm(T Imm, unsigned RegSize, T &Imm0, T &Imm1) {
  // The immediate must be in the form of ((imm0 << 12) + imm1), in which both
  // imm0 and imm1 are non-zero 12-bit unsigned int.
  if ((Imm & 0xfff000) == 0 || (Imm & 0xfff) == 0 ||
      (Imm & ~static_cast<T>(0xffffff)) != 0)
    return false;

  // The immediate can not be composed via a single instruction.
  SmallVector<AArch64_IMM::ImmInsnModel, 4> Insn;
  AArch64_IMM::expandMOVImm(Imm, RegSize, Insn);
  if (Insn.size() == 1)
    return false;

  // Split Imm into (Imm0 << 12) + Imm1;
  Imm0 = (Imm >> 12) & 0xfff;
  Imm1 = Imm & 0xfff;
  return true;
}

template <typename T>
bool AArch64MIPeepholeOpt::visitADDSUB(
    unsigned PosOpc, unsigned NegOpc, MachineInstr &MI) {
  // Try below transformation.
  //
  // ADDWrr X, MOVi32imm ==> ADDWri + ADDWri
  // ADDXrr X, MOVi64imm ==> ADDXri + ADDXri
  //
  // SUBWrr X, MOVi32imm ==> SUBWri + SUBWri
  // SUBXrr X, MOVi64imm ==> SUBXri + SUBXri
  //
  // The mov pseudo instruction could be expanded to multiple mov instructions
  // later. Let's try to split the constant operand of mov instruction into two
  // legal add/sub immediates. It makes only two ADD/SUB instructions intead of
  // multiple `mov` + `and/sub` instructions.

  // We can sometimes have ADDWrr WZR, MULi32imm that have not been constant
  // folded. Make sure that we don't generate invalid instructions that use XZR
  // in those cases.
  if (MI.getOperand(1).getReg() == AArch64::XZR ||
      MI.getOperand(1).getReg() == AArch64::WZR)
    return false;

  return splitTwoPartImm<T>(
      MI,
      [PosOpc, NegOpc](T Imm, unsigned RegSize, T &Imm0,
                       T &Imm1) -> std::optional<OpcodePair> {
        if (splitAddSubImm(Imm, RegSize, Imm0, Imm1))
          return std::make_pair(PosOpc, PosOpc);
        if (splitAddSubImm(-Imm, RegSize, Imm0, Imm1))
          return std::make_pair(NegOpc, NegOpc);
        return std::nullopt;
      },
      [&TII = TII](MachineInstr &MI, OpcodePair Opcode, unsigned Imm0,
                   unsigned Imm1, Register SrcReg, Register NewTmpReg,
                   Register NewDstReg) {
        DebugLoc DL = MI.getDebugLoc();
        MachineBasicBlock *MBB = MI.getParent();
        BuildMI(*MBB, MI, DL, TII->get(Opcode.first), NewTmpReg)
            .addReg(SrcReg)
            .addImm(Imm0)
            .addImm(12);
        BuildMI(*MBB, MI, DL, TII->get(Opcode.second), NewDstReg)
            .addReg(NewTmpReg)
            .addImm(Imm1)
            .addImm(0);
      });
}

template <typename T>
bool AArch64MIPeepholeOpt::visitADDSSUBS(
    OpcodePair PosOpcs, OpcodePair NegOpcs, MachineInstr &MI) {
  // Try the same transformation as ADDSUB but with additional requirement
  // that the condition code usages are only for Equal and Not Equal

  if (MI.getOperand(1).getReg() == AArch64::XZR ||
      MI.getOperand(1).getReg() == AArch64::WZR)
    return false;

  return splitTwoPartImm<T>(
      MI,
      [PosOpcs, NegOpcs, &MI, &TRI = TRI,
       &MRI = MRI](T Imm, unsigned RegSize, T &Imm0,
                   T &Imm1) -> std::optional<OpcodePair> {
        OpcodePair OP;
        if (splitAddSubImm(Imm, RegSize, Imm0, Imm1))
          OP = PosOpcs;
        else if (splitAddSubImm(-Imm, RegSize, Imm0, Imm1))
          OP = NegOpcs;
        else
          return std::nullopt;
        // Check conditional uses last since it is expensive for scanning
        // proceeding instructions
        MachineInstr &SrcMI = *MRI->getUniqueVRegDef(MI.getOperand(1).getReg());
        std::optional<UsedNZCV> NZCVUsed = examineCFlagsUse(SrcMI, MI, *TRI);
        if (!NZCVUsed || NZCVUsed->C || NZCVUsed->V)
          return std::nullopt;
        return OP;
      },
      [&TII = TII](MachineInstr &MI, OpcodePair Opcode, unsigned Imm0,
                   unsigned Imm1, Register SrcReg, Register NewTmpReg,
                   Register NewDstReg) {
        DebugLoc DL = MI.getDebugLoc();
        MachineBasicBlock *MBB = MI.getParent();
        BuildMI(*MBB, MI, DL, TII->get(Opcode.first), NewTmpReg)
            .addReg(SrcReg)
            .addImm(Imm0)
            .addImm(12);
        BuildMI(*MBB, MI, DL, TII->get(Opcode.second), NewDstReg)
            .addReg(NewTmpReg)
            .addImm(Imm1)
            .addImm(0);
      });
}

// Checks if the corresponding MOV immediate instruction is applicable for
// this peephole optimization.
bool AArch64MIPeepholeOpt::checkMovImmInstr(MachineInstr &MI,
                                            MachineInstr *&MovMI,
                                            MachineInstr *&SubregToRegMI) {
  // Check whether current MBB is in loop and the AND is loop invariant.
  MachineBasicBlock *MBB = MI.getParent();
  MachineLoop *L = MLI->getLoopFor(MBB);
  if (L && !L->isLoopInvariant(MI))
    return false;

  // Check whether current MI's operand is MOV with immediate.
  MovMI = MRI->getUniqueVRegDef(MI.getOperand(2).getReg());
  if (!MovMI)
    return false;

  // If it is SUBREG_TO_REG, check its operand.
  SubregToRegMI = nullptr;
  if (MovMI->getOpcode() == TargetOpcode::SUBREG_TO_REG) {
    SubregToRegMI = MovMI;
    MovMI = MRI->getUniqueVRegDef(MovMI->getOperand(2).getReg());
    if (!MovMI)
      return false;
  }

  if (MovMI->getOpcode() != AArch64::MOVi32imm &&
      MovMI->getOpcode() != AArch64::MOVi64imm)
    return false;

  // If the MOV has multiple uses, do not split the immediate because it causes
  // more instructions.
  if (!MRI->hasOneUse(MovMI->getOperand(0).getReg()))
    return false;
  if (SubregToRegMI && !MRI->hasOneUse(SubregToRegMI->getOperand(0).getReg()))
    return false;

  // It is OK to perform this peephole optimization.
  return true;
}

template <typename T>
bool AArch64MIPeepholeOpt::splitTwoPartImm(
    MachineInstr &MI,
    SplitAndOpcFunc<T> SplitAndOpc, BuildMIFunc BuildInstr) {
  unsigned RegSize = sizeof(T) * 8;
  assert((RegSize == 32 || RegSize == 64) &&
         "Invalid RegSize for legal immediate peephole optimization");

  // Perform several essential checks against current MI.
  MachineInstr *MovMI, *SubregToRegMI;
  if (!checkMovImmInstr(MI, MovMI, SubregToRegMI))
    return false;

  // Split the immediate to Imm0 and Imm1, and calculate the Opcode.
  T Imm = static_cast<T>(MovMI->getOperand(1).getImm()), Imm0, Imm1;
  // For the 32 bit form of instruction, the upper 32 bits of the destination
  // register are set to zero. If there is SUBREG_TO_REG, set the upper 32 bits
  // of Imm to zero. This is essential if the Immediate value was a negative
  // number since it was sign extended when we assign to the 64-bit Imm.
  if (SubregToRegMI)
    Imm &= 0xFFFFFFFF;
  OpcodePair Opcode;
  if (auto R = SplitAndOpc(Imm, RegSize, Imm0, Imm1))
    Opcode = *R;
  else
    return false;

  // Create new MIs using the first and second opcodes. Opcodes might differ for
  // flag setting operations that should only set flags on second instruction.
  // NewTmpReg = Opcode.first SrcReg Imm0
  // NewDstReg = Opcode.second NewTmpReg Imm1

  // Determine register classes for destinations and register operands
  MachineFunction *MF = MI.getMF();
  const TargetRegisterClass *FirstInstrDstRC =
      TII->getRegClass(TII->get(Opcode.first), 0, TRI, *MF);
  const TargetRegisterClass *FirstInstrOperandRC =
      TII->getRegClass(TII->get(Opcode.first), 1, TRI, *MF);
  const TargetRegisterClass *SecondInstrDstRC =
      (Opcode.first == Opcode.second)
          ? FirstInstrDstRC
          : TII->getRegClass(TII->get(Opcode.second), 0, TRI, *MF);
  const TargetRegisterClass *SecondInstrOperandRC =
      (Opcode.first == Opcode.second)
          ? FirstInstrOperandRC
          : TII->getRegClass(TII->get(Opcode.second), 1, TRI, *MF);

  // Get old registers destinations and new register destinations
  Register DstReg = MI.getOperand(0).getReg();
  Register SrcReg = MI.getOperand(1).getReg();
  Register NewTmpReg = MRI->createVirtualRegister(FirstInstrDstRC);
  // In the situation that DstReg is not Virtual (likely WZR or XZR), we want to
  // reuse that same destination register.
  Register NewDstReg = DstReg.isVirtual()
                           ? MRI->createVirtualRegister(SecondInstrDstRC)
                           : DstReg;

  // Constrain registers based on their new uses
  MRI->constrainRegClass(SrcReg, FirstInstrOperandRC);
  MRI->constrainRegClass(NewTmpReg, SecondInstrOperandRC);
  if (DstReg != NewDstReg)
    MRI->constrainRegClass(NewDstReg, MRI->getRegClass(DstReg));

  // Call the delegating operation to build the instruction
  BuildInstr(MI, Opcode, Imm0, Imm1, SrcReg, NewTmpReg, NewDstReg);

  // replaceRegWith changes MI's definition register. Keep it for SSA form until
  // deleting MI. Only if we made a new destination register.
  if (DstReg != NewDstReg) {
    MRI->replaceRegWith(DstReg, NewDstReg);
    MI.getOperand(0).setReg(DstReg);
  }

  // Record the MIs need to be removed.
  MI.eraseFromParent();
  if (SubregToRegMI)
    SubregToRegMI->eraseFromParent();
  MovMI->eraseFromParent();

  return true;
}

bool AArch64MIPeepholeOpt::visitINSviGPR(MachineInstr &MI, unsigned Opc) {
  // Check if this INSvi[X]gpr comes from COPY of a source FPR128
  //
  // From
  //  %intermediate1:gpr64 = COPY %src:fpr128
  //  %intermediate2:gpr32 = COPY %intermediate1:gpr64
  //  %dst:fpr128 = INSvi[X]gpr %dst_vec:fpr128, dst_index, %intermediate2:gpr32
  // To
  //  %dst:fpr128 = INSvi[X]lane %dst_vec:fpr128, dst_index, %src:fpr128,
  //  src_index
  // where src_index = 0, X = [8|16|32|64]

  MachineInstr *SrcMI = MRI->getUniqueVRegDef(MI.getOperand(3).getReg());

  // For a chain of COPY instructions, find the initial source register
  // and check if it's an FPR128
  while (true) {
    if (!SrcMI || SrcMI->getOpcode() != TargetOpcode::COPY)
      return false;

    if (!SrcMI->getOperand(1).getReg().isVirtual())
      return false;

    if (MRI->getRegClass(SrcMI->getOperand(1).getReg()) ==
        &AArch64::FPR128RegClass) {
      break;
    }
    SrcMI = MRI->getUniqueVRegDef(SrcMI->getOperand(1).getReg());
  }

  Register DstReg = MI.getOperand(0).getReg();
  Register SrcReg = SrcMI->getOperand(1).getReg();
  MachineInstr *INSvilaneMI =
      BuildMI(*MI.getParent(), MI, MI.getDebugLoc(), TII->get(Opc), DstReg)
          .add(MI.getOperand(1))
          .add(MI.getOperand(2))
          .addUse(SrcReg, getRegState(SrcMI->getOperand(1)))
          .addImm(0);

  LLVM_DEBUG(dbgs() << MI << "  replace by:\n: " << *INSvilaneMI << "\n");
  (void)INSvilaneMI;
  MI.eraseFromParent();
  return true;
}

// All instructions that set a FPR64 will implicitly zero the top bits of the
// register.
static bool is64bitDefwithZeroHigh64bit(MachineInstr *MI,
                                        MachineRegisterInfo *MRI) {
  if (!MI->getOperand(0).isReg() || !MI->getOperand(0).isDef())
    return false;
  const TargetRegisterClass *RC = MRI->getRegClass(MI->getOperand(0).getReg());
  if (RC != &AArch64::FPR64RegClass)
    return false;
  return MI->getOpcode() > TargetOpcode::GENERIC_OP_END;
}

bool AArch64MIPeepholeOpt::visitINSvi64lane(MachineInstr &MI) {
  // Check the MI for low 64-bits sets zero for high 64-bits implicitly.
  // We are expecting below case.
  //
  //  %1:fpr64 = nofpexcept FCVTNv4i16 %0:fpr128, implicit $fpcr
  //  %6:fpr128 = IMPLICIT_DEF
  //  %5:fpr128 = INSERT_SUBREG %6:fpr128(tied-def 0), killed %1:fpr64, %subreg.dsub
  //  %7:fpr128 = INSvi64lane %5:fpr128(tied-def 0), 1, killed %3:fpr128, 0
  MachineInstr *Low64MI = MRI->getUniqueVRegDef(MI.getOperand(1).getReg());
  if (Low64MI->getOpcode() != AArch64::INSERT_SUBREG)
    return false;
  Low64MI = MRI->getUniqueVRegDef(Low64MI->getOperand(2).getReg());
  if (!Low64MI || !is64bitDefwithZeroHigh64bit(Low64MI, MRI))
    return false;

  // Check there is `mov 0` MI for high 64-bits.
  // We are expecting below cases.
  //
  //  %2:fpr64 = MOVID 0
  //  %4:fpr128 = IMPLICIT_DEF
  //  %3:fpr128 = INSERT_SUBREG %4:fpr128(tied-def 0), killed %2:fpr64, %subreg.dsub
  //  %7:fpr128 = INSvi64lane %5:fpr128(tied-def 0), 1, killed %3:fpr128, 0
  // or
  //  %5:fpr128 = MOVIv2d_ns 0
  //  %6:fpr64 = COPY %5.dsub:fpr128
  //  %8:fpr128 = IMPLICIT_DEF
  //  %7:fpr128 = INSERT_SUBREG %8:fpr128(tied-def 0), killed %6:fpr64, %subreg.dsub
  //  %11:fpr128 = INSvi64lane %9:fpr128(tied-def 0), 1, killed %7:fpr128, 0
  MachineInstr *High64MI = MRI->getUniqueVRegDef(MI.getOperand(3).getReg());
  if (!High64MI || High64MI->getOpcode() != AArch64::INSERT_SUBREG)
    return false;
  High64MI = MRI->getUniqueVRegDef(High64MI->getOperand(2).getReg());
  if (High64MI && High64MI->getOpcode() == TargetOpcode::COPY)
    High64MI = MRI->getUniqueVRegDef(High64MI->getOperand(1).getReg());
  if (!High64MI || (High64MI->getOpcode() != AArch64::MOVID &&
                    High64MI->getOpcode() != AArch64::MOVIv2d_ns))
    return false;
  if (High64MI->getOperand(1).getImm() != 0)
    return false;

  // Let's remove MIs for high 64-bits.
  Register OldDef = MI.getOperand(0).getReg();
  Register NewDef = MI.getOperand(1).getReg();
  MRI->constrainRegClass(NewDef, MRI->getRegClass(OldDef));
  MRI->replaceRegWith(OldDef, NewDef);
  MI.eraseFromParent();

  return true;
}

bool AArch64MIPeepholeOpt::runOnMachineFunction(MachineFunction &MF) {
  if (skipFunction(MF.getFunction()))
    return false;

  TII = static_cast<const AArch64InstrInfo *>(MF.getSubtarget().getInstrInfo());
  TRI = static_cast<const AArch64RegisterInfo *>(
      MF.getSubtarget().getRegisterInfo());
  MLI = &getAnalysis<MachineLoopInfo>();
  MRI = &MF.getRegInfo();

  assert(MRI->isSSA() && "Expected to be run on SSA form!");

  bool Changed = false;

  for (MachineBasicBlock &MBB : MF) {
    for (MachineInstr &MI : make_early_inc_range(MBB)) {
      switch (MI.getOpcode()) {
      default:
        break;
      case AArch64::INSERT_SUBREG:
        Changed |= visitINSERT(MI);
        break;
      case AArch64::ANDWrr:
        Changed |= visitAND<uint32_t>(AArch64::ANDWri, MI);
        break;
      case AArch64::ANDXrr:
        Changed |= visitAND<uint64_t>(AArch64::ANDXri, MI);
        break;
      case AArch64::ORRWrs:
        Changed |= visitORR(MI);
        break;
      case AArch64::ADDWrr:
        Changed |= visitADDSUB<uint32_t>(AArch64::ADDWri, AArch64::SUBWri, MI);
        break;
      case AArch64::SUBWrr:
        Changed |= visitADDSUB<uint32_t>(AArch64::SUBWri, AArch64::ADDWri, MI);
        break;
      case AArch64::ADDXrr:
        Changed |= visitADDSUB<uint64_t>(AArch64::ADDXri, AArch64::SUBXri, MI);
        break;
      case AArch64::SUBXrr:
        Changed |= visitADDSUB<uint64_t>(AArch64::SUBXri, AArch64::ADDXri, MI);
        break;
      case AArch64::ADDSWrr:
        Changed |=
            visitADDSSUBS<uint32_t>({AArch64::ADDWri, AArch64::ADDSWri},
                                    {AArch64::SUBWri, AArch64::SUBSWri}, MI);
        break;
      case AArch64::SUBSWrr:
        Changed |=
            visitADDSSUBS<uint32_t>({AArch64::SUBWri, AArch64::SUBSWri},
                                    {AArch64::ADDWri, AArch64::ADDSWri}, MI);
        break;
      case AArch64::ADDSXrr:
        Changed |=
            visitADDSSUBS<uint64_t>({AArch64::ADDXri, AArch64::ADDSXri},
                                    {AArch64::SUBXri, AArch64::SUBSXri}, MI);
        break;
      case AArch64::SUBSXrr:
        Changed |=
            visitADDSSUBS<uint64_t>({AArch64::SUBXri, AArch64::SUBSXri},
                                    {AArch64::ADDXri, AArch64::ADDSXri}, MI);
        break;
      case AArch64::INSvi64gpr:
        Changed |= visitINSviGPR(MI, AArch64::INSvi64lane);
        break;
      case AArch64::INSvi32gpr:
        Changed |= visitINSviGPR(MI, AArch64::INSvi32lane);
        break;
      case AArch64::INSvi16gpr:
        Changed |= visitINSviGPR(MI, AArch64::INSvi16lane);
        break;
      case AArch64::INSvi8gpr:
        Changed |= visitINSviGPR(MI, AArch64::INSvi8lane);
        break;
      case AArch64::INSvi64lane:
        Changed |= visitINSvi64lane(MI);
        break;
      }
    }
  }

  return Changed;
}

FunctionPass *llvm::createAArch64MIPeepholeOptPass() {
  return new AArch64MIPeepholeOpt();
}
