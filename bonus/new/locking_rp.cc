#include "mem/cache/replacement_policies/locking_rp.hh"

#include "mem/cache/base.hh" // 引入 CacheBlk 等基礎定義

#include <cassert>
#include <memory>

#include "params/LockingRP.hh"

LockingRP::LockingRP(const Params *p)
    : BaseReplacementPolicy(p)
{
}

void
LockingRP::invalidate(const std::shared_ptr<ReplacementData>& replacement_data)
const
{
    auto data = std::static_pointer_cast<LockingReplData>(replacement_data);
    data->lastTouchTick = Tick(0);
    data->locked = false;
}

void
LockingRP::touch(const std::shared_ptr<ReplacementData>& replacement_data) const
{
    auto data = std::static_pointer_cast<LockingReplData>(replacement_data);
    data->lastTouchTick = curTick();
    data->locked = true; // Lock the block on touch
}

void
LockingRP::reset(const std::shared_ptr<ReplacementData>& replacement_data) const
{
    auto data = std::static_pointer_cast<LockingReplData>(replacement_data);
    data->lastTouchTick = curTick();
    data->locked = false; // New entries start unlocked
}

ReplaceableEntry*
LockingRP::getVictim(const ReplacementCandidates& candidates) const
{
    assert(!candidates.empty());

    // --- Pass 1: Find the LRU victim among unlocked blocks ---
    ReplaceableEntry* unlockedVictim = nullptr;
    for (const auto& candidate : candidates) {
        auto data = std::static_pointer_cast<LockingReplData>(
            candidate->replacementData);

        if (!data->locked) {
            if (!unlockedVictim || data->lastTouchTick <
                std::static_pointer_cast<LockingReplData>(
                    unlockedVictim->replacementData)->lastTouchTick) {
                unlockedVictim = candidate;
            }
        }
    }

    if (unlockedVictim) {
        // Found a victim in unlocked blocks
        return unlockedVictim;
    }

    // --- Pass 2: All blocks are locked, find LRU and unlock all ---
    ReplaceableEntry* lruVictim = candidates[0];
    for (const auto& candidate : candidates) {
        auto data = std::static_pointer_cast<LockingReplData>(
            candidate->replacementData);

        // Find the absolute LRU victim
        if (data->lastTouchTick < std::static_pointer_cast<LockingReplData>(
                lruVictim->replacementData)->lastTouchTick) {
            lruVictim = candidate;
        }
        
        // Unlock all candidates for the next round
        data->locked = false;
    }

    return lruVictim;
}

std::shared_ptr<ReplacementData>
LockingRP::instantiateEntry()
{
    return std::shared_ptr<ReplacementData>(new LockingReplData());
}

LockingRP*
LockingRPParams::create()
{
    return new LockingRP(this);
}