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
#include "bb_optimizations.h"
#include "compiler_internals.h"
#include "dataflow_iterator.h"
#include "dataflow_iterator-inl.h"
#include "global_value_numbering.h"
#include "local_value_numbering.h"
#include "mir_graph_test.h"
#include "pass_driver.h"
#include "pass_me.h"
#include "reference_map_calculator.h"

namespace art {

class ReferenceMapCalculatorTest : public MirGraphTest {
 public:
  struct MIRDef {
    BasicBlockId bbid;
    NarrowDexOffset offset;
    int opcode;
    uint32_t vA;
    uint32_t vB;
    uint32_t vC;
  };

  struct RefInfo {
    static constexpr size_t kMaxRefs = 4;
    size_t num_refs;
    uint32_t refs[kMaxRefs];
  };

  void DoPrepareBasicBlocks(const BBDef* defs, size_t count) {
    cu_.mir_graph->block_id_map_.clear();
    cu_.mir_graph->block_list_.Reset();

    // There are at least 4 blocks: null, entry, exit and at least one bytecode block.
    ASSERT_LE(4u, count);
    ASSERT_EQ(kNullBlock, defs[0].type);
    ASSERT_EQ(kEntryBlock, defs[1].type);
    ASSERT_EQ(kExitBlock, defs[2].type);

    for (size_t i = 0u; i != count; ++i) {
      const BBDef* def = &defs[i];
      BasicBlock* bb = cu_.mir_graph->CreateNewBB(def->type);
      bb->fall_through = (def->num_successors >= 1) ? def->successors[0] : NullBasicBlockId;
      bb->taken = (def->num_successors >= 2) ? def->successors[1] : NullBasicBlockId;
      if (def->num_successors > 2) {
        bb->successor_block_list_type = kCatch;
        bb->successor_blocks->Resize(def->num_successors);
        for (size_t j = 2u; j != def->num_successors; ++j) {
          SuccessorBlockInfo* successor_block_info =
              static_cast<SuccessorBlockInfo*>(cu_.arena.Alloc(sizeof(SuccessorBlockInfo),
                                                               kArenaAllocSuccessor));
          successor_block_info->block = def->successors[j];
          successor_block_info->key = j;  // Not used by reference map calculator.
          bb->successor_blocks->Insert(successor_block_info);
        }
      }
      bb->predecessors = new (&cu_.arena) GrowableArray<BasicBlockId>(
          &cu_.arena, def->num_predecessors, kGrowableArrayPredecessors);
      for (size_t j = 0u; j != def->num_predecessors; ++j) {
        ASSERT_NE(0u, def->predecessors[j]);
        bb->predecessors->Insert(def->predecessors[j]);
      }
    }

    cu_.mir_graph->num_blocks_ = count;
    ASSERT_EQ(count, cu_.mir_graph->block_list_.Size());
    cu_.mir_graph->entry_block_ = cu_.mir_graph->block_list_.Get(1);
    ASSERT_EQ(kEntryBlock, cu_.mir_graph->entry_block_->block_type);
    cu_.mir_graph->exit_block_ = cu_.mir_graph->block_list_.Get(2);
    ASSERT_EQ(kExitBlock, cu_.mir_graph->exit_block_->block_type);
  }

  template <size_t count>
  void PrepareBasicBlocks(const BBDef (&defs)[count]) {
    DoPrepareBasicBlocks(defs, count);
  }

  void DoPrepareMIRs(const MIRDef* defs, size_t count) {
    mir_count_ = count;
    mirs_ = reinterpret_cast<MIR*>(cu_.arena.Alloc(sizeof(MIR) * count, kArenaAllocMIR));
    for (size_t i = 0u; i != count; ++i) {
      const MIRDef* def = &defs[i];
      MIR* mir = &mirs_[i];
      ASSERT_LT(def->bbid, cu_.mir_graph->block_list_.Size());
      BasicBlock* bb = cu_.mir_graph->block_list_.Get(def->bbid);
      bb->AppendMIR(mir);
      mir->dalvikInsn.opcode = static_cast<Instruction::Code>(def->opcode);
      mir->dalvikInsn.vA = def->vA;
      mir->dalvikInsn.vB = def->vB;
      mir->dalvikInsn.vC = def->vC;
      mir->offset = def->offset;
      if (def->opcode == kMirOpCheck) {
        DCHECK_NE(i + 1, count);
        mir->meta.throw_insn = &mirs_[i + 1];
      }
      if (mir->dalvikInsn.IsInvoke()) {
        mir->dalvikInsn.arg[0] = mir->dalvikInsn.vC;
      }
    }

    DexFile::CodeItem* code_item = static_cast<DexFile::CodeItem*>(
        cu_.arena.Alloc(sizeof(DexFile::CodeItem), kArenaAllocMisc));
    code_item->insns_size_in_code_units_ = 2u * count;
    code_item->registers_size_ = 7;
    cu_.mir_graph->current_code_item_ = code_item;
  }

  template <size_t count>
  void PrepareMIRs(const MIRDef (&defs)[count]) {
    DoPrepareMIRs(defs, count);
  }

  void DoPrepareShortys(const char** shortys, size_t count) {
    cu_.mir_graph->shorty_for_test_ = reinterpret_cast<const char**>(cu_.arena.Alloc(sizeof(char*) * count,
                                                                                     kArenaAllocMisc));
    for (size_t i = 0u; i != count; ++i) {
      cu_.mir_graph->shorty_for_test_[i] = shortys[i];
      if (i == 0u) {
        // The first shorty is the caller's shorty, so update the CompilationUnit
        // to also hold this information.
        cu_.shorty = shortys[0];
      }
    }
  }

  template <size_t count>
  void PrepareShortys(const char* (&shortys)[count]) {
    DoPrepareShortys(shortys, count);
  }

  void DoPrepareRefMaps(const RefInfo* ref_infos, size_t count) {
    ref_maps = reinterpret_cast<ArenaBitVector**>(cu_.arena.Alloc(sizeof(ArenaBitVector*) * count, kArenaAllocRefMaps));
    ASSERT_EQ(count, mir_count_);
    for (size_t i = 0u; i != count; ++i) {
      const RefInfo* ref_info = &ref_infos[i];
      if (ref_info->num_refs > 0) {
        ref_maps[i] = new (&(cu_.arena)) ArenaBitVector(&(cu_.arena), RefInfo::kMaxRefs, false);
        ref_maps[i]->ClearAllBits();
        for (size_t j = 0u; j != ref_info->num_refs; j++) {
          ref_maps[i]->SetBit(ref_info->refs[j]);
        }
      }
    }
  }

  template <size_t count>
  void PrepareRefMaps(const RefInfo (&ref_infos)[count]) {
    DoPrepareRefMaps(ref_infos, count);
  }

  template <typename Iterator>
  void WalkBasicBlocks(PassMEDataHolder* data, const Pass* pass) {
    DCHECK(data != nullptr);
    CompilationUnit* c_unit = data->c_unit;
    DCHECK(c_unit != nullptr);
    Iterator iterator(c_unit->mir_graph.get());
    bool change = false;
    for (BasicBlock* bb = iterator.Next(change); bb != nullptr; bb = iterator.Next(change)) {
      data->bb = bb;
      change = pass->Worker(data);
    }
  }

  void DoCalculateReferenceMaps() {
    // Disable NCE because all we need is type inference.
    cu_.disable_opt |= (1 << kNullCheckElimination);

    // Mark MIRGraph as needing GC map recalculation.
    cu_.mir_graph->ChangeGCMapRecalculationState(true);

    // Run the post-opts that initialize all of the data structures including ssa.
    cu_.mir_graph->CalculateBasicBlockInformation();

    // Prepare the data holder to be able to run the ME passes.
    PassMEDataHolder data_holder;
    data_holder.c_unit = &cu_;

    // First run the type inference because it is required for GC map calculation.
    const Pass* type_pass = GetPassInstance<TypeInference>();
    type_pass->Start(&data_holder);
    WalkBasicBlocks<RepeatingPreOrderDfsIterator>(&data_holder, type_pass);
    type_pass->End(&data_holder);

    // Now run the reference map calculator.
    const Pass* ref_map_pass = GetPassInstance<ReferenceMapCalculator>();
    ref_map_pass->Start(&data_holder);
    ref_map_pass->End(&data_holder);

    // Finally, compare the reference maps to those generated by the pass.
    for (size_t i = 0u; i < mir_count_; ++i) {
      const ArenaBitVector* test_map = ref_maps[i];
      const ArenaBitVector* compiler_map = cu_.mir_graph->GetGCMapAsBitVector(mirs_[i].offset);

      // Empty map could be either not allocated or have no bits set. Make sure both cases are accounted for.
      bool test_map_empty = test_map == nullptr ||
          (test_map != nullptr && test_map->GetHighestBitSet() < 0);
      bool compiler_map_empty = compiler_map == nullptr ||
          (compiler_map != nullptr && compiler_map->GetHighestBitSet() < 0);

      ASSERT_EQ(test_map_empty, compiler_map_empty);

      if (test_map != nullptr && compiler_map != nullptr) {
        ASSERT_TRUE(test_map->Equal(compiler_map));
      }
    }
  }

  ReferenceMapCalculatorTest() :
    MirGraphTest(), pool_(),
    cu_(&pool_),
    mir_count_(0u),
    mirs_(nullptr),
    ref_maps(nullptr) {
    cu_.mir_graph.reset(new MIRGraph(&cu_, &cu_.arena));
    cu_.access_flags = kAccPrivate;
  }

  ArenaPool pool_;
  CompilationUnit cu_;
  size_t mir_count_;
  MIR* mirs_;
  ArenaBitVector** ref_maps;

#define DEF_MIR(bb, offset, opcode, vA, vB, vC) \
    { bb, offset, opcode, vA, vB, vC }
#define DEF_REFS0() \
    { 0u, { } }
#define DEF_REFS1(v1) \
    { 1u, { v1 } }
#define DEF_REFS2(v1, v2) \
    { 2u, { v1, v2 } }
#define DEF_REFS3(v1, v2, v3) \
    { 3u, { v1, v2, v3 } }
#define DEF_REFS4(v1, v2, v3, v4) \
    { 4u, { v1, v2, v3, v4 } }
};

class ReferenceMapUnitTest004 : public ReferenceMapCalculatorTest {
 public:
  ReferenceMapUnitTest004();

 private:
  static const BBDef kRefTest004bbs[];
  static const MIRDef kRefTest004mirs[];
  static const char* kRefTest004shortys[];
  static const RefInfo kRefTest004refs[];
};

/**
 * @details This is the CFG for the following java code:
 *
 * private Object runTest() {
 *   Object x[] = new Object[2];
 *   Object y = null;
 *   try {
 *     y = new Object();
 *     x[2] = y;  // out-of-bound exception
 *   } catch(Exception ex) {
 *     if (y == null) {
 *       x[1] = new Object();
 *     }
 *   } finally {
 *     x[1] = y;
 *   };
 *   return y;
 * }
 */
const ReferenceMapUnitTest004::BBDef ReferenceMapUnitTest004::kRefTest004bbs[] = {
  /* BB00 */ DEF_BB(kNullBlock, DEF_SUCC0(), DEF_PRED0()),
  /* BB01 */ DEF_BB(kEntryBlock, DEF_SUCC1(3), DEF_PRED0()),
  /* BB02 */ DEF_BB(kExitBlock, DEF_SUCC0(), DEF_PRED1(16)),
  /* BB03 */ DEF_BB(kDalvikByteCode, DEF_SUCC4(8, 0, 4, 5), DEF_PRED1(1)),
  /* BB04 */ DEF_BB(kDalvikByteCode, DEF_SUCC1(19), DEF_PRED2(3, 8)),
  /* BB05 */ DEF_BB(kDalvikByteCode, DEF_SUCC1(18), DEF_PRED2(3, 8)),
  /* BB06 */ DEF_BB(kDalvikByteCode, DEF_SUCC2(0, 18), DEF_PRED4(9, 12, 13, 14)),
  /* BB07 */ DEF_BB(kDalvikByteCode, DEF_SUCC2(0, 19), DEF_PRED1(9)),
  /* BB08 */ DEF_BB(kDalvikByteCode, DEF_SUCC4(9, 0, 4, 5), DEF_PRED1(3)),
  /* BB09 */ DEF_BB(kDalvikByteCode, DEF_SUCC4(10, 0, 7, 6), DEF_PRED1(8)),
  /* BB10 */ DEF_BB(kDalvikByteCode, DEF_SUCC1(16), DEF_PRED1(9)),
  /* BB11 */ DEF_BB(kDalvikByteCode, DEF_SUCC2(0, 16), DEF_PRED2(15, 19)),
  /* BB12 */ DEF_BB(kDalvikByteCode, DEF_SUCC3(13, 0, 6), DEF_PRED1(19)),
  /* BB13 */ DEF_BB(kDalvikByteCode, DEF_SUCC3(14, 0, 6), DEF_PRED1(12)),
  /* BB14 */ DEF_BB(kDalvikByteCode, DEF_SUCC3(15, 0, 6), DEF_PRED1(13)),
  /* BB15 */ DEF_BB(kDalvikByteCode, DEF_SUCC1(11), DEF_PRED1(14)),
  /* BB16 */ DEF_BB(kDalvikByteCode, DEF_SUCC1(2), DEF_PRED2(10, 11)),
  /* BB17 */ DEF_BB(kExceptionHandling, DEF_SUCC0(), DEF_PRED1(18)),
  /* BB18 */ DEF_BB(kDalvikByteCode, DEF_SUCC1(17), DEF_PRED2(5, 6)),
  /* BB19 */ DEF_BB(kDalvikByteCode, DEF_SUCC2(12, 11), DEF_PRED2(4, 7)),
};

/**
 * @details
 *  0x0000: const/4 v0, #+2
 *  0x0001: const/4 v4, #+1
 *  0x0002: new-array v2, v0, java.lang.Object[] // type@9
 *  0x0004: const/4 v1, #+0
 *  0x0005: new-instance v0, java.lang.Object // type@4
 *  0x0007: invoke-direct {v0}, void java.lang.Object.<init>() // method@4
 *  0x000a: const/4 v1, #+2
 *  0x000b: aput-object v0, v2, v1
 *  0x000d: aput-object v0, v2, v4
 *  0x000f: return-object v0
 *  0x0010: move-exception v0
 *  0x0011: move-object v0, v1
 *  0x0012: if-nez v0, +10
 *  0x0014: const/4 v1, #+1
 *  0x0015: new-instance v3, java.lang.Object // type@4
 *  0x0017: invoke-direct {v3}, void java.lang.Object.<init>() // method@4
 *  0x001a: aput-object v3, v2, v1
 *  0x001c: aput-object v0, v2, v4
 *  0x001e: goto -15
 *  0x001f: move-exception v0
 *  0x0020: aput-object v1, v2, v4
 *  0x0022: throw v0
 *  0x0023: move-exception v1
 *  0x0024: move-object v5, v1
 *  0x0025: move-object v1, v0
 *  0x0026: move-object v0, v5
 *  0x0027: goto -7
 *  0x0028: move-exception v1
 *  0x0029: goto -23
 *
 * Note that for invokes we put type as 1 instead of 4 as in original java test case,
 * so we can create a shorty cache.
 */
const ReferenceMapUnitTest004::MIRDef ReferenceMapUnitTest004::kRefTest004mirs[] = {
  DEF_MIR(3, 0x0, Instruction::CONST_4, 0, 2, 0),
  DEF_MIR(3, 0x1, Instruction::CONST_4, 4, 1, 0),
  DEF_MIR(3, 0x2, Instruction::NEW_ARRAY, 2, 0, 9),
  DEF_MIR(3, 0x4, Instruction::CONST_4, 1, 0, 0),
  DEF_MIR(3, 0x5, kMirOpCheck, 0, 0, 0),
  DEF_MIR(8, 0x5, Instruction::NEW_INSTANCE, 0, 4, 0),
  DEF_MIR(8, 0x7, kMirOpCheck, 0, 0, 0),
  DEF_MIR(9, 0x7, Instruction::INVOKE_DIRECT, 1, 1, 0),
  DEF_MIR(9, 0xa, Instruction::CONST_4, 1, 2, 0),
  DEF_MIR(9, 0xb, kMirOpCheck, 0, 0, 0),
  DEF_MIR(10, 0xb, Instruction::APUT_OBJECT, 0, 2, 1),
  DEF_MIR(10, 0xd, Instruction::APUT_OBJECT, 0, 2, 4),
  DEF_MIR(16, 0xf, Instruction::RETURN_OBJECT, 0, 0, 0),
  DEF_MIR(4, 0x10, Instruction::MOVE_EXCEPTION, 0, 0, 0),
  DEF_MIR(4, 0x11, Instruction::MOVE_OBJECT, 0, 1, 0),
  DEF_MIR(19, 0x12, Instruction::IF_NEZ, 0, 10, 0),
  DEF_MIR(12, 0x14, Instruction::CONST_4, 1, 1, 0),
  DEF_MIR(12, 0x15, kMirOpCheck, 0, 0, 0),
  DEF_MIR(13, 0x15, Instruction::NEW_INSTANCE, 3, 4, 0),
  DEF_MIR(13, 0x17, kMirOpCheck, 0, 0, 0),
  DEF_MIR(14, 0x17, Instruction::INVOKE_DIRECT, 1, 1, 3),
  DEF_MIR(14, 0x1a, kMirOpCheck, 0, 0, 0),
  DEF_MIR(15, 0x1a, Instruction::APUT_OBJECT, 3, 2, 1),
  DEF_MIR(11, 0x1c, Instruction::APUT_OBJECT, 0, 2, 4),
  DEF_MIR(11, 0x1e, Instruction::GOTO, 0xfffffff1, 0, 0),
  DEF_MIR(5, 0x1f, Instruction::MOVE_EXCEPTION, 0, 0, 0),
  DEF_MIR(18, 0x20, Instruction::APUT_OBJECT, 1, 2, 4),
  DEF_MIR(18, 0x22, Instruction::THROW, 0, 0, 0),
  DEF_MIR(6, 0x23, Instruction::MOVE_EXCEPTION, 1, 0, 0),
  DEF_MIR(6, 0x24, Instruction::MOVE_OBJECT, 5, 1, 0),
  DEF_MIR(6, 0x25, Instruction::MOVE_OBJECT, 1, 0, 0),
  DEF_MIR(6, 0x26, Instruction::MOVE_OBJECT, 0, 5, 0),
  DEF_MIR(6, 0x27, Instruction::GOTO, 0xfffffff9, 0, 0),
  DEF_MIR(7, 0x28, Instruction::MOVE_EXCEPTION, 1, 0, 0),
  DEF_MIR(7, 0x29, Instruction::GOTO, 0xffffffe9, 0, 0),
};

/**
 * @details Entry "0" represents ourselves. Rest of entries represent
 * methods being called.
 */
const char* ReferenceMapUnitTest004::kRefTest004shortys[] = {
  "LL",  // ()Ljava/lang/Object; - "this" is first argument.
  "V",  // ()V
};

/**
 * @details
 * new-array v2_1,  v0_1, index #0x9@0x2:
 * Check1: new-instance v0_2, index #0x4@0x5: v1 v2
 * Check1: invoke-direct v0_2@0x7: v0 v1 v2
 * Check1: aput-object v0_2, v2_1, v1_2@0xb: v0 v2
 * aput-object v0_2, v2_1, v4_1@0xd: v0 v2
 * return-object v0_3@0xf: v0
 * move-exception v0_8@0x10: v1 v2
 * if-nez v0_4, 0x1c (+a)@0x12: v0 v2
 * Check1: new-instance v3_1, index #0x4@0x15: v0 v2
 * Check1: invoke-direct v3_1@0x17: v0 v2 v3
 * Check1: aput-object v3_1, v2_1, v1_4@0x1a: v0 v2 v3
 * aput-object v0_4, v2_1, v4_1@0x1c: v0 v2
 * goto, 0xf (-f)@0x1e: v0
 * move-exception v0_10@0x1f: v1 v2
 * aput-object v1_7, v2_1, v4_1@0x20: v0 v1 v2
 * throw v0_7@0x22: v0
 * move-exception v1_5@0x23: v0 v2
 * goto, 0x20 (-7)@0x27: v0 v1 v2
 * move-exception v1_3@0x28: v0 v2
 * goto, 0x12 (-17)@0x29: v0 v2
 */
const ReferenceMapUnitTest004::RefInfo ReferenceMapUnitTest004::kRefTest004refs[] = {
  DEF_REFS0(),
  DEF_REFS0(),
  DEF_REFS0(),
  DEF_REFS0(),
  DEF_REFS2(1, 2),
  DEF_REFS2(1, 2),     // Set same ref map as the "Check" since they share offset.
  DEF_REFS3(0, 1, 2),
  DEF_REFS3(0, 1, 2),  // Set same ref map as the "Check" since they share offset.
  DEF_REFS0(),
  DEF_REFS2(0, 2),
  DEF_REFS2(0, 2),     // Set same ref map as the "Check" since they share offset.
  DEF_REFS2(0, 2),
  DEF_REFS1(0),
  DEF_REFS2(1, 2),
  DEF_REFS0(),
  DEF_REFS2(0, 2),
  DEF_REFS0(),
  DEF_REFS2(0, 2),
  DEF_REFS2(0, 2),     // Set same ref map as the "Check" since they share offset.
  DEF_REFS3(0, 2, 3),
  DEF_REFS3(0, 2, 3),  // Set same ref map as the "Check" since they share offset.
  DEF_REFS3(0, 2, 3),
  DEF_REFS3(0, 2, 3),  // Set same ref map as the "Check" since they share offset.
  DEF_REFS2(0, 2),
  DEF_REFS1(0),
  DEF_REFS2(1, 2),
  DEF_REFS3(0, 1, 2),
  DEF_REFS1(0),
  DEF_REFS2(0, 2),
  DEF_REFS0(),
  DEF_REFS0(),
  DEF_REFS0(),
  DEF_REFS3(0, 1, 2),
  DEF_REFS2(0, 2),
  DEF_REFS2(0, 2),
};

ReferenceMapUnitTest004::ReferenceMapUnitTest004() : ReferenceMapCalculatorTest() {
  PrepareBasicBlocks(kRefTest004bbs);
  PrepareMIRs(kRefTest004mirs);
  PrepareShortys(kRefTest004shortys);
  PrepareRefMaps(kRefTest004refs);
  DoCalculateReferenceMaps();
}

TEST_F(ReferenceMapUnitTest004, RefMap004) {
}

}  // namespace art
