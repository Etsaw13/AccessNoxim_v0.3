- co-sim.h
    - 设置热仿真的一些参数

- flp.h/.c
    - 

- NoximBuffer
    - a FIFO

- NoximCmdLineParser
    - 命令行配置

- getNodeRoutingTable
    - 全局路由表
    -   NoximRoutingTableNoC (全网表): 选择 当前节点 ID
        ↓
        NoximRoutingTableNode (节点表): 选择 输入链路 (来源)
        ↓
        NoximRoutingTableLink (链路表): 选择 目的节点 ID
        ↓
        NoximAdmissibleOutputs (输出集): 获取 允许的输出链路列表

- NoximGlobalStats
    - 全局统计

- NoximGlobalTrafficTable
    - 全局流量表

- NoximLocalRoutingTable
    - 本地路由表

- NoximLog
    - 日志文件

- NoximMain
    - 主函数
    - NoximNoC
    - NoximLog
    - NoximGlobalStats

- NoximNoC
    - buildMesh 负责把网络搭好；entry 每周期收集功耗、算温度、做节流决策，再把结果写到日志和拓扑配置里
    - buildMesh()
        - NoximVLink
        - NoximTile
    - entry() [sc_method]
        - mod: 0 ------------------- P-clean ------------------- P-1
        - 0: 结束clean + 功耗/温度计算 + 紧急决策
        - 1..: 正常运行（可触发10k日志）         
        - P-clean: 进入clean（并记log） 
        - ...P-1: clean持续，等下个 mod=0 退出clean

- NoximPower
    - 事件驱动的能耗累加器 + 功率换算器

- 

- 

- NoximVLink
    - 模式1：mesh
    - 模式2：crossbar
    