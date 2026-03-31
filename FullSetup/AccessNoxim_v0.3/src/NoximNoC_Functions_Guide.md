# NoximNoC.cpp 新手导读（结合 NoximNoC.h）

> 文件位置：`src/NoximNoC.cpp`，头文件：`src/NoximNoC.h`
>
> 这份文档面向第一次接触该文件的同学，目标是回答两个问题：
> 1) 每个函数是干什么的？
> 2) 函数之间是如何调用的？

---

## 1. 先看整体：NoximNoC 在系统里扮演什么角色

`NoximNoC` 是 NoC 仿真的**顶层 SystemC 模块**（见 `NoximNoC.h` 的 `SC_MODULE(NoximNoC)`）。

它主要负责：

- 构建 3D mesh（tile + vertical link）
- 连接所有方向信号（N/E/S/W/UP/DOWN）
- 在每个时钟周期执行顶层调度（`entry()`）
- 做功耗/温度联动
- 根据热状态触发节流与重构
- 输出 transient 日志与流量统计

在构造函数里（`NoximNoC.h`）：

- `buildMesh()`：建网与连线
- 创建 `Thermal_IF`
- 注册 `SC_METHOD(entry)`，并对 `reset` 和 `clock.pos()` 敏感

这意味着：**每个上升沿/复位事件都会进 `entry()`**。

---

## 2. NoximNoC.cpp 的函数清单与作用

下面按文件里的顺序介绍。

### 2.1 `buildMesh()`

**作用**：初始化整个 NoC 拓扑和边界状态。

主要做了：

1. 按参数加载全局路由表/流量表（若使用 table-based 模式）
2. 为每个 `(x,y,z)` 创建 `NoximTile`，为每个 `(x,y)` 创建 `NoximVLink`
3. 连接 router/PE 的所有方向信号（req/flit/ack、free_slots、NoP、RCA、on_off、TB、PDT、buf 等）
4. 初始化边界信号（不存在的邻居方向置 `NOT_VALID` 或 0）
5. 失效边界 router 的 reservation table 对应方向
6. 初始化 vertical free-slot 相关连接
7. 根据节流类型初始化 `throttling`（测试模式走 `_throt_case_setting`，否则 `_setNormal`）
8. 调 `findNonXLayer()` + `calROC()` 并下发 `RTM_set_var()`

一句话：**把“硬件网络+初始运行策略”一次性搭好**。

---

### 2.2 `entry()`

**作用**：顶层周期调度函数（每个时钟周期都可能进入）。

分两段：

- reset 期间：清功耗/温度/统计、恢复状态
- 正常运行：按 `TEMP_REPORT_PERIOD` 周期执行
  - clean stage 进入/退出
  - 稳态功耗累积
  - 瞬态功耗采样 + 温度计算 + 温度回写
  - 紧急节流决策 `EmergencyDecision()`
  - transient log
  - 每 10000/100000 周期导出流量分析文本（含 Matlab 绘图脚本）

一句话：**这是 NoC 顶层“控制循环”**。

---

### 2.3 `searchNode(const int id) const`

**作用**：根据节点 ID 找到 tile 指针。

内部调用 `id2Coord(id)`，然后返回 `t[x][y][z]`。

---

### 2.4 `transPwr2PtraceFile()`

**作用**：收集每个 tile 的**瞬态功耗**到 `instPowerTrace`，同时写 `results_log_pwr`。

每个 tile 记录三类分量（router/mem/fpmac），最后清该 tile 的 transient power 累计。

---

### 2.5 `steadyPwr2PtraceFile()`

**作用**：累积每个 tile 的**稳态功耗**到 `overallPowerTrace`。

同样按 router/mem/fpmac 三分量累加。

---

### 2.6 `setTemperature()`

**作用**：把热模型计算得到的温度写回各 tile，并更新热管理相关派生量。

主要步骤：

1. 更新 router 当前温度、上一温度
2. 计算 thermal budget（与阈值差）
3. 进行温度预测（`pre_temperature1` 等）
4. 更新 `consumption_rate`
5. 写 `thermal_factor`（位置相关系数）
6. 写 `penalty_factor`（温度越接近上限越惩罚）
7. 计算 `MTTT`（可理解为热预算可持续时间类指标）

一句话：**热状态从“温度值”扩展成“可用于决策的指标集合”**。

---

### 2.7 `EmergencyDecision()`

**作用**：根据 `NoximGlobalParams::throt_type` 选择节流策略。

可能调用：

- `GlobalThrottle`
- `DistributedThrottle`
- `TAVT`
- `TAVT_MAX`
- `Vertical`
- `Vertical_MAX`

若是 `THROT_NORMAL/THROT_TEST` 则直接返回，不改拓扑。

---

### 2.8 `GlobalThrottle(bool &isEmergency)`

**策略**：全局阈值触发。

- 任一 router 超温 -> 所有节点进 emergency
- 否则所有节点出 emergency

---

### 2.9 `DistributedThrottle(bool &isEmergency)`

**策略**：局部分布式触发。

- 对每个节点独立判断温度
- 超温节点进 emergency，未超温节点恢复正常

---

### 2.10 `TAVT(bool &isEmergency)`

**策略**：按垂直方向逐层处理（含 beltway 逻辑）。

- 若某层超温，会对该 `(x,y)` 柱上的低层执行节流（底层可保留）
- 更新 `beltway`
- 最后调用 `Reconfiguration()`

---

### 2.11 `TAVT_MAX(bool &isEmergency)`

**策略**：TAVT 的更激进版本。

- 超温后对更多层（到 z）进行节流
- 调 `Reconfiguration()`

---

### 2.12 `Vertical(bool &isEmergency)`

**策略**：垂直节流（底层通常不节流）+ beltway + 重构。

---

### 2.13 `Vertical_MAX(bool &isEmergency)`

**策略**：Vertical 的更激进版本，超温时可对整柱层节流。

---

### 2.14 `calROC(int &col_max, int &col_min, int &row_max, int &row_min, int non_beltway_layer)`

**作用**：根据 beltway 分布计算一个 ROI/ROC 矩形边界（行列范围）。

这个范围后续用于 `RTM_set_var()`，指导运行时路由/管理参数。

---

### 2.15 `setCleanStage()`

**作用**：进入 clean stage。

- PE 进入 clean stage
- 全部退出 emergency
- `throttling` 清空

---

### 2.16 `EndCleanStage()`

**作用**：退出 clean stage。

- 所有 PE 调 `OutOfCleanStage()`

---

### 2.17 `findNonXLayer(int &non_throt_layer, int &non_beltway_layer)`

**作用**：扫描 `throttling` / `beltway`，找出“最上层受影响层”的下一层编号。

结果用于后续重构参数。

---

### 2.18 `Reconfiguration()`

**作用**：热状态变化后的重构入口。

流程：

1. `findNonXLayer()`
2. `calROC()`
3. 对每个 PE 调 `RTM_set_var(...)`
4. 写 `transient_topology` 日志

一句话：**把“当前热状态”转成“全网运行参数”并下发**。

---

### 2.19 `_equal(...)`

**作用**：坐标比较工具函数。

用于 `_throt_case_setting()` 里描述固定测试图案。

---

### 2.20 `_CleanDone()`

**作用**：检查 clean stage 是否真正“清空”。

- 遍历所有 tile 的 PE 队列和 router buffer
- 只要有残留就返回 `false`
- 期间会打印大量调试信息

---

### 2.21 `TransientLog()`

**作用**：输出一个周期内的瞬态统计。

统计内容包括：

- 非空 buffer 数
- 节流节点数
- 最高温
- 多类 transmit 计数（adaptive/dor/dw/mid/beltway）
- 当前 throttling/beltway 拓扑图（字符画风格）

输出到：

- `transient_log_throughput`
- `transient_topology`

---

### 2.22 `_setThrot(int i,int j,int k)`

**作用**：把某节点设为节流状态（进入 emergency）。

---

### 2.23 `_setNormal(int i,int j,int k)`

**作用**：把某节点恢复正常（退出 emergency）。

---

### 2.24 `_throt_case_setting(int throt_case)`

**作用**：测试模式下按预设图案批量设置节流节点。

- `case 1..16` 定义不同热点布局
- 常用于实验复现实验场景

---

## 3. 调用关系（Call Graph）

下面是“谁调用谁”的简化视图。

```text
SC_CTOR(NoximNoC)
├─ buildMesh()
│  ├─ _throt_case_setting()  [THROT_TEST]
│  │  ├─ _setThrot()
│  │  └─ _setNormal()
│  ├─ _setNormal()           [非 THROT_TEST 的初始化]
│  ├─ findNonXLayer()
│  ├─ calROC()
│  └─ PE::RTM_set_var(...)
└─ SC_METHOD(entry)  [对 reset 和 clock.pos() 敏感]

entry()
├─ (reset 分支) power/temp/stat 初始化
└─ (正常分支)
   ├─ setCleanStage()        [周期尾部]
   ├─ EndCleanStage()        [周期边界]
   ├─ steadyPwr2PtraceFile() [过 warmup]
   ├─ transPwr2PtraceFile()  [cal_temp]
   ├─ Thermal_IF::Temperature_calc(...) [cal_temp]
   ├─ setTemperature()       [cal_temp]
   ├─ EmergencyDecision()
   │  ├─ GlobalThrottle() / DistributedThrottle()
   │  ├─ TAVT() / TAVT_MAX()
   │  │  └─ Reconfiguration()
   │  │     ├─ findNonXLayer()
   │  │     ├─ calROC()
   │  │     └─ PE::RTM_set_var(...)
   │  └─ Vertical() / Vertical_MAX()
   │     └─ Reconfiguration() -> 同上
   ├─ TransientLog()
   └─ Thermal_IF::steadyTmp(t) [仿真末尾且 cal_temp]
```

---

## 4. 新手建议：怎么读这份代码更轻松

推荐阅读顺序：

1. `SC_CTOR`（在 `.h`）先理解生命周期入口
2. `buildMesh()`（一次性初始化）
3. `entry()`（周期行为主线）
4. `setTemperature()`（热指标计算）
5. `EmergencyDecision()` + 各策略（行为分支）
6. `Reconfiguration()`（策略结果下发）
7. `TransientLog()`（观察系统输出）

你可以把它记成一句话：

**“buildMesh 负责把网络搭好；entry 每周期收集功耗、算温度、做节流决策，再把结果写到日志和拓扑配置里。”**

---

## 5. 关键成员（来自 NoximNoC.h）与你读代码时要关注的点

- `t[][][]`：所有 tile（核心对象）
- `v[][]`：垂直链接对象
- `grtable / gttable`：全局路由/流量表
- `HS_interface`：热模型接口
- `instPowerTrace / overallPowerTrace / TemperatureTrace`：功耗温度三大向量
- `_emergency / _clean`：全局状态开关

如果你只抓主线，优先盯这几个。

---

## 6. 一句话版总览

`NoximNoC.cpp` 是一个“**3D NoC 网络构建 + 周期热管理控制器**”：

- 初始化阶段建网与连线（`buildMesh`）
- 运行阶段按固定周期做功耗/温度计算（`entry`）
- 按策略执行节流与重构（`EmergencyDecision` + `Reconfiguration`）
- 持续输出可观测日志（`TransientLog` + traffic 分析）
