NVmain + gem5 Simulation
===

**問題集**
---

* (Q1) GEM5 + NVMAIN BUILD UP (40%)
* (Q2) Enable L3 last level cache in GEM5 + NVMAIN (15%) ( 看到 log 裡面有 L3 cache 的資訊)
* (Q3) Config last level cache to 2 way and full way associative cache and test performance (15%)
必須跑 benchmark quicksort 在 2 way 跟 full way ( 直接在 L3 cache implement ，可以用 miss rate 判斷是否成功)
* (Q4) Modify last level cache policy based on frequency based replacement policy (15%)
* (Q5) Test the performance of write back and write through policy based on 4 way associative cache with isscc_pcm (15%)
必須跑 benchmark multiply 在 write through 跟 write back ( gem5 default 使用 write back ，可以用 write request
的數量判斷 write through 是否成功)
* Bonus (10%)
Design last level cache policy to reduce the energy consumption of pcm_based main memory
Baseline:LRU


**1. 環境建置**
---
```
sudo apt install build essential git m4 scons zlib1g zlib1g dev libprotobuf dev protobuf compiler libprotoc dev libgoogle perftools dev python3 dev python3 six python libboost all dev pkg
config

git clone https://github.com/SEAL UCSB/NVmain
```

[安裝GEM5](https://gem5.googlesource.com/public/gem5/+/525ce650e1a5bbe71c39d4b15598d6c003cc9f9e)


**2. Enable L3 last level cache**
---
* gem5/configs/common/Caches.py
    在 class L2Cache 後加入 L3Cache (複製L2Cache的內容)
```python=
class L3Cache(Cache):
    assoc = 64
    tag_latency = 32
    data_latency = 32
    response_latency = 20
    mshrs = 32
    tgts_per_mshr = 24
    write_buffers = 16
```

* gem5/configs/common/CacheConfig.py

    1. 修改config_cache() function，將if options.cpu_type=="O3_ARM_v7a_3"底下改成
    ```python=
    if options.cpu_type == "O3_ARM_v7a_3":
        try:
            from cores.arm.O3_ARM_v7a import *
        except:
            print("O3_ARM_v7a_3 is unavailable. Did you compile the O3 model?")
            sys.exit(1)

        dcache_class, icache_class, l2_cache_class, walk_cache_class, l3_cache_class = \
            O3_ARM_v7a_DCache, O3_ARM_v7a_ICache, O3_ARM_v7aL2, \
            O3_ARM_v7aWalkCache,O3_ARM_v7aL3
    else:
        dcache_class, icache_class, l2_cache_class, walk_cache_class, l3_cache_class= \
            L1_DCache, L1_ICache, L2Cache, None, L3Cache

        if buildEnv['TARGET_ISA'] == 'x86':
            walk_cache_class = PageTableWalkerCache
    ```
    2. 將if options.l2cache替換成以下
    ```python=
        if options.l2cache and options.l3cache:
        system.l2=l2_cache_class(
            clk_domain=system.cpu_clk_domain,
            size=options.l2_size,
            assoc=options.l2_assoc
        )
        system.l3=l3_cache_class(
            clk_domain=system.cpu_clk_domain,
            size=options.l3_size,
            assoc=options.l3_assoc
        )

        system.tol2bus=L2XBar(clk_domain=system.cpu_clk_domain)
        system.tol3bus=L3XBar(clk_domain=system.cpu_clk_domain)

        system.l2.cpu_side=system.tol2bus.master
        system.l2.mem_side=system.tol3bus.slave
        system.l3.cpu_side=system.tol3bus.master
        system.l3.mem_side=system.membus.slave
    
    elif operations.l2cache:
        # Provide a clock for the L2 and the L1-to-L2 bus here as they
        # are not connected using addTwoLevelCacheHierarchy. Use the
        # same clock as the CPUs.
        system.l2 = l2_cache_class(clk_domain=system.cpu_clk_domain,
                                   size=options.l2_size,
                                   assoc=options.l2_assoc)

        system.tol2bus = L2XBar(clk_domain = system.cpu_clk_domain)
        system.l2.cpu_side = system.tol2bus.master
        system.l2.mem_side = system.membus.slave
    ```

* gem5/src/mem/XBar.py
    新增class L3XBar
```python=
class L3XBar(CoherentXBar):
    width=32

    frontend_latency=1
    forward_latency=0
    response_latency=1
    snoop_response_latency=1
```
* gem5/src/cpu/BaseCPU.py
    從XBar.py導入L3XBar
    `from XBar import L3XBar`
    並在BaseCPU新增function
```python=
    def addThreeLevelCacheHierarchy(self, ic, dc, l3c, iwc=None, dwc=None):
        self.addPrivateSplitL1Caches(ic, dc, iwc, dwc)
        self.toL3bus=L3XBar()
        self.connectCachedPorts(self.toL3Bus)
        self.l3cache=l3c
        self.toL2Bus.master=self.l3cache.cpu_side
        self._cached_ports=['l3cache.mem_side']
```
* gem5/configs/common/Options.py
    在class addNoISAOptions底下新增一個參數，來啟用l3 cache
```python=
parser.add_option("--l3cache", action="store_true")
```

編譯
```
scons EXTRAS=../NVmain build/X86/gem5.opt
```
執行
```
./build/X86/gem5.opt configs/example/se.py -c tests/test-progs/hello/bin/x86/linux/hello --cpu-type=TimingSimpleCPU --caches --l2cache --l3cache --mem-type=NVMainMemory --nvmain-config=../NVmain/Config/PCM_ISSCC_2012_4GB.config
```

**3. Config last level cache to 2-way and full-way associative cache and test performance**
---

2-way set associative指令
```
./build/X86/gem5.opt configs/example/se.py -c ./quicksort --cpu-type=TimingSimpleCPU --caches --l2cache --l3cache --l3_assoc=2 --l1i_size=32kB --l1d_size=32kB --l2_size=128kB --l3_size=1MB --mem-type=NVMainMemory --nvmain-config=../NVmain/Config/PCM_ISSCC_2012_4GB.config   
```

full-way associative指令
```
./build/X86/gem5.opt configs/example/se.py -c ./quicksort --cpu-type=TimingSimpleCPU --caches --l2cache --l3cache --l3_assoc=1 --l1i_size=32kB --l1d_size=32kB --l2_size=128kB --l3_size=1MB --mem-type=NVMainMemory --nvmain-config=../NVmain/Config/PCM_ISSCC_2012_4GB.config
```

**4. Modify last level cache policy based on frequency based replacement policy**
---

將Caches.py當中 class L3Cache 加入replacement_policy
```python=
class L3Cache(Cache):
    assoc = 64
    tag_latency = 32
    data_latency = 32
    response_latency = 20
    mshrs = 32
    tgts_per_mshr = 24
    write_buffers = 16

    replacement_policy = Param.BaseReplacementPolicy(LFURP(),"Replacement policy")
```

指令
```
./build/X86/gem5.opt configs/example/se.py -c ./quicksort --cpu-type=TimingSimpleCPU --caches --l1i_size=32kB --l1d_size=32kB --l2cache --l2_size=128kB --l3cache --l3_size=1MB --mem-type=NVMainMemory --nvmain-config=../NVmain/Config/PCM_ISSCC_2012_4GB.config
```

**5. Test the performance of write back and write through policy based on 4-way associative cache with isscc_pcm**
---

src/mem/cache/base.cc 在1073行加入
```cpp=
        if (blk->isWritable()) {
            PacketPtr writeclean_pkt = writecleanBlk(blk, pkt->req->getDest(), pkt->id);
            writebacks.push_back(writeclean_pkt);
        }
```

執行
```
./build/X86/gem5.opt configs/example/se.py -c ./multiply --cpu-type=TimingSimpleCPU --caches --l2cache --l3cache --l3_assoc=4 --l1i_size=32kB --l1d_size=32kB --l2_size=128kB --l3_size=1MB --mem-type=NVMainMemory --nvmain-config=../NVmain/Config/PCM_ISSCC_2012_4GB.config
```

**bonus. Design last level cache policy to reduce the energy consumption of pcm_based main memory**
---
根據src/mem/cache/replacement_policy/lru_rp.cc以及lru_rp.hh修改
並修改src/mem/cache/replacement_policy/SConscript以及ReplacementPolicies.py
執行
```
./build/X86/gem5.opt configs/example/se.py -c ./quicksort_ --cpu-type=TimingSimpleCPU --caches --l1i_size=32kB --l1d_size=32kB --l2cache --l2_size=128kB --l3cache --l3_size=1MB --mem-type=NVMainMemory --nvmain-config=../NVmain/Config/PCM_ISSCC_2012_4GB.config
```