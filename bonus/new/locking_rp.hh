/**
 * @file
 * Declaration of a Locking replacement policy.
 * This policy "locks" a block upon a touch, protecting it from
 * immediate eviction. It prioritizes evicting unlocked blocks.
 */

#ifndef __MEM_CACHE_REPLACEMENT_POLICIES_LOCKING_RP_HH__
#define __MEM_CACHE_REPLACEMENT_POLICIES_LOCKING_RP_HH__

#include "mem/cache/replacement_policies/base.hh"
#include "params/LockingRP.hh"

class LockingRP : public BaseReplacementPolicy
{
  protected:
    /** LockingRP-specific implementation of replacement data. */
    struct LockingReplData : public ReplacementData
    {
        /** Tick on which the entry was last touched. */
        Tick lastTouchTick;

        /** Flag to indicate if the block is "locked" from eviction. */
        bool locked;

        /**
         * Default constructor.
         */
        LockingReplData() : lastTouchTick(0), locked(false) {}
    };

  public:
    /** Convenience typedef. */
    typedef LockingRPParams Params;

    /**
     * Construct and initialize this replacement policy.
     */
    LockingRP(const Params *p);

    /**
     * Destructor.
     */
    ~LockingRP() {}

    /**
     * Invalidate replacement data.
     */
    void invalidate(const std::shared_ptr<ReplacementData>& replacement_data)
                                                              const override;

    /**
     * Touch an entry to update its replacement data and lock it.
     */
    void touch(const std::shared_ptr<ReplacementData>& replacement_data) const
                                                                     override;

    /**
     * Reset replacement data for a new entry. It starts unlocked.
     */
    void reset(const std::shared_ptr<ReplacementData>& replacement_data) const
                                                                     override;

    /**
     * Find a replacement victim.
     */
    ReplaceableEntry* getVictim(const ReplacementCandidates& candidates) const
                                                                     override;

    /**
     * Instantiate a replacement data entry.
     */
    std::shared_ptr<ReplacementData> instantiateEntry() override;
};

#endif // __MEM_CACHE_REPLACEMENT_POLICIES_LOCKING_RP_HH__