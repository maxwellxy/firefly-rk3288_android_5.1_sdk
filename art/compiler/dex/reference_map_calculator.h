/*
 * Copyright (C) 2014 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ART_COMPILER_DEX_REFERENCE_MAP_CALCULATOR_H_
#define ART_COMPILER_DEX_REFERENCE_MAP_CALCULATOR_H_

#include "compiler_internals.h"
#include "pass_me.h"

namespace art {

/**
 * @brief Pass is used to recalculate GC maps.
 * @details This is needed in presence of optimizations that remove writes to reference
 * registers and also in presence of optimizations using compiler temporaries.
 */
class ReferenceMapCalculator : public PassME {
 public:
  ReferenceMapCalculator() : PassME("ReferenceMapCalculator", kNoNodes) {
    default_options_.Put("CrossCheckMaps", OptionContent(0));
  }

  /**
   * @brief Used to control whether the pass needs to run.
   * @param data The pass data.
   * @return Returns true if pass should apply.
   */
  virtual bool Gate(const PassDataHolder* data) const;

  /**
   * @brief Used to prepare structures and then iterates through all blocks to calculate
   * reference information.
   * @param data The pass data.
   */
  virtual void Start(PassDataHolder* data) const;

 protected:
  /**
   * @brief Helper data structure for pass to hold information about MIRs and object references.
   */
  struct ReferenceMapDataHolder {
    SafeMap<MIR*, ArenaBitVector*> mir_to_ref_vrs;  /**!< Map of instructions to live reference bitmaps. */
    std::set<MIR*> mirs_with_check;              /**!< The instructions that are split into two halves. */
  };

  /**
   * @brief Used for verbosity to print the compiler generated reference map at each suspend point.
   * @param c_unit The compilation unit.
   * @param ref_data The pass data.
   */
  void LogReferenceMap(CompilationUnit* c_unit, ReferenceMapDataHolder* ref_data) const;

  /**
   * @brief This is used to fix case of overlapping offsets. This is done
   * because each suspend point may have a different map.
   * @details When overlapping offsets are found, a new is requested from
   * MIRGraph.
   * @param mir_graph The MIRGraph.
   * @param ref_data The pass data.
   */
  void CheckAndFixOffsetProblem(MIRGraph* mir_graph, ReferenceMapDataHolder* ref_data) const;

  /**
   * @brief Used to walk through all generated GC maps and add them to the MIRGraph.
   * @param mir_graph The MIRGraph.
   * @param ref_data The pass data.
   */
  void AddGCMapsToMirGraph(MIRGraph* mir_graph, ReferenceMapDataHolder* ref_data) const;

  /**
   * @brief Useful for debugging by checking the generated GC map against original.
   * @details There may be false positives because a reference dies as its use
   * while verifier may see it live for longer.
   * @param mir_graph The MIRGraph.
   * @param ref_data The pass data.
   */
  void CrossCheckGCMap(MIRGraph* mir_graph, ReferenceMapDataHolder* ref_data) const;

  /**
   * @brief Used to check whether an instruction will generate a safepoint.
   * @details The logic here takes into account whether instruction has been broken
   * up into a check half. This is because the safepoints are expressed at that point
   * in the control flow graph. So if an instruction has a check half, the check
   * will say it has safepoints while the instruction will say it does not.
   * @param mir_graph The MIRGraph.
   * @param mir The instruction to check if it has safepoints.
   * @param ref_data This holds a list of instructions that have a kMirOpCheck half.
   * @return Returns whether the instruction will generate a safepoint.
   */
  bool HasSafepoint(MIRGraph* mir_graph, MIR* mir, ReferenceMapDataHolder* ref_data) const;

  /**
   * @brief This is used to find the instructions with kMirOpCheck opcode.
   * @param bb The basic block to check.
   * @param ref_data This holds a list that is updated by this function to contain
   * the instruction for which the kMirOpCheck instruction corresponds to.
   */
  void FindCheckInstructions(BasicBlock* bb, ReferenceMapDataHolder* ref_data) const;

  /**
   * @brief Used to calculate the reference maps for instructions in basic block.
   * @details The algorithm that is used is as follows:
   * -#) Go through the successors and calculate union of all live-in vrs.
   * -#) Determine the current BB live out ssa registers by using the successor
   * live-in information.
   * -#) Record only the set of reference ssa registers. Keep this as working set.
   * -#) Walk through the instructions backwards.
   * -#) For each instruction, remove the define from working set. But add all
   * reference ssa registers to the working set. This covers all live ssa reference
   * registers up to this point.
   * @param data The pass data.
   * @return Always returns false because it does not change the control flow graph.
   */
  bool CalculateBBRefInfo(MIRGraph* mir_graph, BasicBlock* bb, ReferenceMapDataHolder* ref_data) const;
};

}  // namespace art

#endif  // ART_COMPILER_DEX_REFERENCE_MAP_CALCULATOR_H_
