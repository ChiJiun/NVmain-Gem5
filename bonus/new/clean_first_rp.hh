/**
 * Copyright (c) 2018 Inria
 * All rights reserved.
 *
 * (Copyright notice retained from original)
 *
 * This is a clean-first cache replacement policy. It prioritizes evicting
 * clean blocks to minimize energy consumption from writebacks. If no clean
 * blocks are available, it defaults to a standard LRU policy.
 *
 * Authors: Daniel Carvalho (Original), Gemini (Refactoring)
 */

#ifndef __MEM_CACHE_REPLACEMENT_POLICIES_CLEAN_FIRST_RP_HH__
#define __MEM_CACHE_REPLACEMENT_POLICIES_CLEAN_FIRST_RP_HH__

#include <memory>
#include "mem/cache/replacement_policies/base.hh"

// Forward declaration
struct CleanFirstRPParams;

/**
 * A Clean-First replacement policy.
 *
 * This policy aims to reduce energy consumption by preferentially selecting
 * a clean cache block as the victim. The selection among clean blocks is
 * based on the Least Recently Used (LRU) algorithm. If all candidate blocks
 * are dirty, it falls back to evicting the LRU block among them.
 */
class CleanFirstRP : public BaseReplacementPolicy
{
  protected:
    /**
     * CleanFirst-specific implementation of replacement data.
     * It only needs to store LRU information.
     */
    struct CleanFirstReplData : public ReplacementData
    {
        /** Tick on which the entry was last touched. */
        Tick lastTouchTick;

        /**
         * Default constructor. Invalidate data.
         */
        CleanFirstReplData() : lastTouchTick(0) {}
    };

  public:
    /** Convenience typedef. */
    typedef CleanFirstRPParams Params;

    /**
     * Construct and initialize this replacement policy.
     */
    CleanFirstRP(const Params *p);

    /**
     * Destructor.
     */
    ~CleanFirstRP() override = default;

    /**
     * Invalidate replacement data to make it a likely eviction candidate.
     */
    void invalidate(const std::shared_ptr<ReplacementData>& replacement_data)
        const override;

    /**
     * Update replacement data on a cache hit.
     */
    void touch(const std::shared_ptr<ReplacementData>& replacement_data) const
        override;

    /**
     * Set up replacement data for a new entry.
     */
    void reset(const std::shared_ptr<ReplacementData>& replacement_data) const
        override;

    /**
     * Find a victim block. This is the core logic of the policy.
     */
    ReplaceableEntry* getVictim(const ReplacementCandidates& candidates) const
        override;

    /**
     * Instantiate a replacement data entry.
     */
    std::shared_ptr<ReplacementData> instantiateEntry() override;
};

#endif // __MEM_CACHE_REPLACEMENT_POLICIES_CLEAN_FIRST_RP_HH__