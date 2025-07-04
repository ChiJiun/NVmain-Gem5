/**
 * Copyright (c) 2018 Inria
 * All rights reserved.
 *
 * (Copyright notice retained from original)
 *
 * Authors: Daniel Carvalho (Original), Gemini (Refactoring)
 */

#include "mem/cache/replacement_policies/clean_first_rp.hh"
#include "mem/cache/base.hh"

#include <algorithm> // For std::min_element
#include <cassert>
#include <memory>

#include "params/CleanFirstRP.hh"
#include "sim/core.hh" // For curTick()

// The gem5 namespace is assumed to be open here in real usage
// but for clarity we can add it.
// namespace gem5 {

CleanFirstRP::CleanFirstRP(const Params *p)
    : BaseReplacementPolicy(p)
{
}

void
CleanFirstRP::invalidate(const std::shared_ptr<ReplacementData>& replacement_data) const
{
    // A block that is invalidated becomes the oldest.
    std::static_pointer_cast<CleanFirstReplData>(
        replacement_data)->lastTouchTick = Tick(0);
}

void
CleanFirstRP::touch(const std::shared_ptr<ReplacementData>& replacement_data) const
{
    // A touched block becomes the newest.
    std::static_pointer_cast<CleanFirstReplData>(
        replacement_data)->lastTouchTick = curTick();
}

void
CleanFirstRP::reset(const std::shared_ptr<ReplacementData>& replacement_data) const
{
    // A new block is the newest.
    std::static_pointer_cast<CleanFirstReplData>(
        replacement_data)->lastTouchTick = curTick();
}

ReplaceableEntry*
CleanFirstRP::getVictim(const ReplacementCandidates& candidates) const
{
    assert(!candidates.empty());

    // Find the best victim using std::min_element with a custom comparator.
    // The "minimum" element according to our rules is the best victim.
    auto victim_it = std::min_element(candidates.begin(), candidates.end(),
        [](const ReplaceableEntry* a, const ReplaceableEntry* b) {
            const auto* blk_a = static_cast<const CacheBlk*>(a);
            const auto* blk_b = static_cast<const CacheBlk*>(b);

            const auto data_a = std::static_pointer_cast<CleanFirstReplData>(a->replacementData);
            const auto data_b = std::static_pointer_cast<CleanFirstReplData>(b->replacementData);

            const bool is_dirty_a = blk_a->isDirty();
            const bool is_dirty_b = blk_b->isDirty();

            // Rule 1: Prioritize clean blocks. A clean block is "less than" a dirty one.
            if (is_dirty_a != is_dirty_b) {
                return !is_dirty_a; // If a is clean (false), it's preferred.
            }

            // Rule 2: If both are clean or both are dirty, use LRU.
            // The block with the older timestamp is "less than" (preferred victim).
            return data_a->lastTouchTick < data_b->lastTouchTick;
        }
    );

    // Dereference the iterator to get the pointer to the victim entry.
    return *victim_it;
}

std::shared_ptr<ReplacementData>
CleanFirstRP::instantiateEntry()
{
    return std::shared_ptr<ReplacementData>(new CleanFirstReplData());
}

// } // namespace gem5

// This would typically be in a separate file or handled by the build system.
// For example, in src/mem/cache/replacement_policies/SConscript
// And the definition in a file like src/python/gem5/configs/ruby/MESI_Two_Level.py
// For now, let's assume the params object is defined elsewhere.

// Dummy Param object for compilation context
struct CleanFirstRPParams : public ReplacementPolicyParams
{
    // gem5 will generate the real one from a SimObject definition
    // For example:
    // def create(self):
    //     return CleanFirstRP(self)
};

// This create function would be generated by the build system.
// CleanFirstRP*
// CleanFirstRPParams::create() const
// {
//     return new CleanFirstRP(this);
// }