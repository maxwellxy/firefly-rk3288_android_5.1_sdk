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

#include "base/bit_vector-inl.h"
#include "base/casts.h"
#include "dataflow_iterator.h"
#include "dataflow_iterator-inl.h"
#include "dex/verified_method.h"
#include "reference_map_calculator.h"
#include "verifier/dex_gc_map.h"
#include "verifier/method_verifier.h"
#include <algorithm>
#include <sstream>

namespace art {

bool ReferenceMapCalculator::Gate(const PassDataHolder* data) const {
  const PassMEDataHolder* me_data = down_cast<const PassMEDataHolder*>(data);
  MIRGraph* mir_graph = me_data->c_unit->mir_graph.get();

  return mir_graph->NeedGcMapRecalculation();
}

void ReferenceMapCalculator::LogReferenceMap(CompilationUnit* c_unit, ReferenceMapDataHolder* ref_data) const {
  if (c_unit->dex_file == nullptr) {
    LOG(INFO) << "ReferenceMapCalculator: Reference map is:";
  } else {
    LOG(INFO) << "ReferenceMapCalculator: Reference map for "
              << PrettyMethod(c_unit->method_idx, *c_unit->dex_file) << " is:";
  }

  for (auto it : ref_data->mir_to_ref_vrs) {
    std::stringstream ss;
    for (int32_t vr : it.second->Indexes()) {
      ss << " v" << vr;
    }
    LOG(INFO) << "\tReferenceMapCalculator: " << c_unit->mir_graph->GetDalvikDisassembly(it.first)
              << "@0x" << std::hex << it.first->offset << ":" << ss.str();
  }
}

void ReferenceMapCalculator::CheckAndFixOffsetProblem(MIRGraph* mir_graph, ReferenceMapDataHolder* ref_data) const {
  std::map<DexOffset, std::set<MIR*> > offset_to_mirs;

  for (auto it : ref_data->mir_to_ref_vrs) {
    offset_to_mirs[it.first->offset].insert(it.first);
  }

  for (auto it : offset_to_mirs) {
    if (it.second.size() > 1) {
      // Get the information from the first MIR. We will pick this one as baseline.
      std::set<MIR*>::iterator mir_it = it.second.begin();
      ArenaBitVector* first_map = ref_data->mir_to_ref_vrs.Get(*mir_it);

      // Walk through each instruction. If its map doesn't match the first one, give it a new offset.
      for (mir_it = std::next(mir_it); mir_it != it.second.end(); mir_it++) {
        MIR* mir = (*mir_it);

        ArenaBitVector* map = ref_data->mir_to_ref_vrs.Get(mir);
        if (first_map->Equal(map) == false) {
          // Since the maps don't match, assign new offset for instruction.
          DexOffset new_offset = mir_graph->GetNewOffset();

          // Since MIRs only hold narrow offsets, we need to check that we don't exceed.
          CHECK_LE(new_offset, std::numeric_limits<uint16_t>::max());
          mir->offset = new_offset;
        }
      }
    }
  }
}

void ReferenceMapCalculator::AddGCMapsToMirGraph(MIRGraph* mir_graph, ReferenceMapDataHolder* ref_data) const {
  for (auto it : ref_data->mir_to_ref_vrs) {
    mir_graph->RecordNewMirGCMap(it.first, it.second);
  }
}

void ReferenceMapCalculator::CrossCheckGCMap(MIRGraph* mir_graph, ReferenceMapDataHolder* ref_data) const {
  const std::vector<uint8_t>& gc_map_raw =
      mir_graph->GetCurrentDexCompilationUnit()->GetVerifiedMethod()->GetDexGcMap();
  verifier::DexPcToReferenceMap dex_gc_map(&(gc_map_raw)[0]);
  size_t verifier_map_entry_size = dex_gc_map.RegWidth();
  size_t compiler_calculated_entry_size = mir_graph->GetGCMapEntrySize();
  // The compiler should have made a map that is at least the size of the original.
  DCHECK_GE(compiler_calculated_entry_size, verifier_map_entry_size);

  for (auto it : ref_data->mir_to_ref_vrs) {
    MIR* mir = it.first;

    const uint8_t* verifier_raw_map = dex_gc_map.FindBitMap(mir->offset, false);
    const uint8_t* compiler_raw_map = mir_graph->GetGCMap(mir->offset, false);

    for (size_t entry = 0; entry < compiler_calculated_entry_size; entry++) {
      uint32_t compiler_map_entry = compiler_raw_map == nullptr ? 0 : compiler_raw_map[entry];
      uint32_t verifier_map_entry = entry >= verifier_map_entry_size ?
          0 : (verifier_raw_map == nullptr ? 0 : verifier_raw_map[entry]);

      if (compiler_map_entry != verifier_map_entry) {
        LOG(INFO) << "ReferenceMapCalculator: Found mismatched map entries @0x" << std::hex
                  << mir->offset << ". At index " << entry << " found compiler map: 0x"
                  << std::hex << compiler_map_entry << " and verifier map: 0x" << std::hex
                  << verifier_map_entry;
      }
    }
  }
}

bool ReferenceMapCalculator::HasSafepoint(MIRGraph* mir_graph, MIR* mir, ReferenceMapDataHolder* ref_data) const {
  // The point of this helper is to allow skipping MIRs that are broken into a check half.
  // This is because the real dataflow is expressed via the check half.
  auto it = ref_data->mirs_with_check.find(mir);
  if (it != ref_data->mirs_with_check.end()) {
    // This has a check half. The "safepoints" should be captured by the check so return false.
    return false;
  } else {
    return mir_graph->HasSafepoint(mir);
  }
}

void ReferenceMapCalculator::FindCheckInstructions(BasicBlock* bb, ReferenceMapDataHolder* ref_data) const {
  // Check instructions must be the last instructions of a block (since they are needed
  // to express the control flow to catch blocks.
  if (bb->last_mir_insn != nullptr) {
    MIR* last_insn = bb->last_mir_insn;
    if (static_cast<int>(last_insn->dalvikInsn.opcode) == kMirOpCheck) {
      // Store the mapping of the thrower to its matching check.
      MIR* throw_insn = last_insn->meta.throw_insn;
      ref_data->mirs_with_check.insert(throw_insn);
    }
  }
}

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
bool ReferenceMapCalculator::CalculateBBRefInfo(MIRGraph* mir_graph, BasicBlock* bb,
                                                ReferenceMapDataHolder* ref_data) const {
  if (bb->last_mir_insn == nullptr) {
    // No instructions and thus nothing to check here.
    return false;
  }

  // We want to go backwards through the block but the MIR does not provide a previous pointer.
  // So we walk forwards to insert into our vector (which we can then iterate through backwards).
  // At the same time, we check if any instruction has safepoint semantics so we don't waste time
  // computing the object registers live at end of block.
  bool has_safepoints = false;
  std::vector<MIR*> instructions;
  for (MIR* mir = bb->first_mir_insn; mir != nullptr; mir = mir->next) {
    instructions.push_back(mir);
    if (HasSafepoint(mir_graph, mir, ref_data)) {
      has_safepoints = true;
    }
  }

  if (has_safepoints == false) {
    // No instructions with safepoint and thus nothing to compute here.
    return false;
  }

  // Compute the set of live out ssa registers holding objects.
  // Do this by going through all successor live ins, determining which ssa was live out from current
  // block, and then tracking only if object.
  std::set<int32_t> ref_ssa_regs;
  ChildBlockIterator child_iter(bb, mir_graph);
  for (BasicBlock* child = child_iter.Next(); child != nullptr; child = child_iter.Next()) {
    CHECK(child->data_flow_info != nullptr);
    for (int32_t vr : child->data_flow_info->live_in_v->Indexes()) {
      int32_t live_out_ssa = bb->data_flow_info->vreg_to_ssa_map_exit[vr];

      // If it is a reference, add it to our set.
      if (mir_graph->IsObjectRegister(live_out_ssa) == true) {
        ref_ssa_regs.insert(live_out_ssa);
      }
    }
  }

  // Go through instruction backwards.
  for (auto it = instructions.rbegin(); it != instructions.rend(); it++) {
    MIR* mir = *it;

    // We must have ssa information for each mir.
    CHECK(mir->ssa_rep != nullptr);

    // The define is always last part of MIR. Thus we need to remove the
    // defines from set of what is live up to this point.
    for (int16_t def = 0; def < mir->ssa_rep->num_defs; def++) {
      int32_t def_ssa = mir->ssa_rep->defs[def];
      ref_ssa_regs.erase(def_ssa);
    }

    // Since the define may have clobbered an existing register, we ensure
    // to add all reference uses of this MIR as being live up to this point.
    for (int16_t use = 0; use < mir->ssa_rep->num_uses; use++) {
      int32_t use_ssa = mir->ssa_rep->uses[use];
      if (mir_graph->IsObjectRegister(use_ssa) == true) {
        ref_ssa_regs.insert(use_ssa);
      }
    }

    // Only record the GC maps for instructions that will have safepoint.
    if (HasSafepoint(mir_graph, mir, ref_data)) {
      // Create a new bitvector to hold the bits for references.
      ArenaBitVector* vrs = new (mir_graph->GetArena()) ArenaBitVector(mir_graph->GetArena(),
                                                                       mir_graph->GetNumOfCodeAndTempVRs(),
                                                                       false);
      vrs->ClearAllBits();

      for (int32_t live_ssa_reg : ref_ssa_regs) {
        int32_t vr = mir_graph->SRegToVReg(live_ssa_reg);
        vrs->SetBit(vr);
      }

      // Finish off by inserting it into the tracker.
      ref_data->mir_to_ref_vrs.Put(mir, vrs);
    }
  }

  // There are never any updates to the CFG itself.
  return false;
}

void ReferenceMapCalculator::Start(PassDataHolder* data) const {
  PassMEDataHolder* me_data = down_cast<PassMEDataHolder*>(data);
  CompilationUnit* c_unit = me_data->c_unit;
  MIRGraph* mir_graph = c_unit->mir_graph.get();
  ReferenceMapDataHolder ref_data;

  // Now iterate through all of the blocks to:
  // 1) Find all kMirOpCheck instructions. This is needed so that we can properly
  // capture where the "safepoint" is since codegen will end up moving the real
  // instruction in the place of the check.
  // 2) Calculate the reference information for each block.
  AllNodesIterator iter(mir_graph);
  for (BasicBlock* bb = iter.Next(); bb != nullptr; bb = iter.Next()) {
    FindCheckInstructions(bb, &ref_data);
  }
  iter.Reset();
  for (BasicBlock* bb = iter.Next(); bb != nullptr; bb = iter.Next()) {
    CalculateBBRefInfo(mir_graph, bb, &ref_data);
  }

  // Now fix any offset problems (overlapping offsets).
  CheckAndFixOffsetProblem(mir_graph, &ref_data);

  // Add the maps to the MIRGraph.
  AddGCMapsToMirGraph(mir_graph, &ref_data);

  // If verbosity is enabled, log the maps to logcat for manual verification.
  // Also print out differences by doing a cross check against the real GC map.
  if (c_unit->print_pass) {
    LogReferenceMap(c_unit, &ref_data);

    // Cross check GC map if requested.
    if (GetIntegerPassOption("CrossCheckMaps", c_unit) != 0) {
      CrossCheckGCMap(mir_graph, &ref_data);
    }
  }

  // Inform MIRGraph that it no longer needs to generate new maps (since they have
  // just been generated for current CFG).
  mir_graph->ChangeGCMapRecalculationState(false);
}

}  // namespace art
