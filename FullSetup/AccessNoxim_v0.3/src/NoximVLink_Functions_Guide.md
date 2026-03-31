# NoximVLink.cpp 新手导读（结合 NoximVLink.h）

> 文件位置：`src/NoximVLink.cpp`，头文件：`src/NoximVLink.h`
>
> 目标：帮助第一次学习该文件的同学理解：
> 1) 每个函数干什么
> 2) 函数之间怎么调用

---

## 1. NoximVLink 是什么？

`NoximVLink` 是 3D NoC 中每个 `(x,y)` 柱状位置上的**垂直链路模块**。

你可以把它理解为“楼层之间的电梯系统”：

- **向上方向**（DOWN 侧输入 -> UP 侧输出）
- **向下方向**（UP 侧输入 -> DOWN 侧输出）

并且支持两种工作模式：

1. `VERTICAL_MESH`：一一直通（同层号直连）
2. `VERTICAL_CROSSBAR`：可重映射（通过预留表做仲裁）

---

## 2. 头文件先看哪些最重要

在 `NoximVLink.h`：

- `SC_METHOD(entry); sensitive << clock.neg();`
  - 说明该模块在**时钟下降沿**执行 `entry()`
- 两张预留表：
  - `reservation_table_UP`
  - `reservation_table_DOWN`
- 核心私有函数：
  - `buildVLink()`
  - `entry()`
  - `Mesh()`
  - `CrossBar_UP()`
  - `CrossBar_DOWN()`
  - `routingFunction()` / `routingDownward()` / `routingTLAR()`

---

## 3. NoximVLink.cpp 所有函数作用

### 3.1 `buildVLink()`

**作用**：初始化轮询起点。

- `start_from_layer_U = 0`
- `start_from_layer_D = 0`

用于 CrossBar 模式中的轮询公平性（每轮从不同输入开始扫描）。

---

### 3.2 `entry()`

**作用**：模块主入口（每个负边沿触发）。

逻辑分三段：

1. **reset 分支**
   - 清空两张 reservation table
   - 把关键 ack 置 1（默认可接收）

2. **正常分支 + mesh 模式**
   - 调 `Mesh()`，做上下链路一一透传

3. **正常分支 + crossbar 模式**
   - 调 `CrossBar_UP()`
   - 调 `CrossBar_DOWN()`

若模式未知，`assert(false)`。

---

### 3.3 `CrossBar_UP()`

**方向**：`DOWN -> UP`。

这是一个“两阶段”流程：

#### 阶段1：Reserve（预留输出）
- 轮询所有 `req_rx_to_DOWN[i]`
- 若收到 flit：
  - 先 `ack_rx_to_DOWN[i] = 0`（占住输入）
  - 若是 `HEAD`：
    - 组装 `NoximRouteData`
    - 调 `routingFunction()` 算目标输出层 `tmp_dst_z`
    - 若输出可用：在 `reservation_table_UP` 里 `reserve(i, tmp_dst_z)`

#### 阶段2：Transmit（真正发送）
- 取该输入对应的预留输出 `o`
- 若 `o` 有效且 `ack_rx_to_UP[o] == 1`：
  - 把 flit/req 从 DOWN 输入转发到 UP 输出
  - 把 ack 回传给输入侧
  - 若是 `TAIL`：释放该输出口 reservation

并且每轮 `start_from_layer_U++`，实现轮询起点平移。

---

### 3.4 `CrossBar_DOWN()`

**方向**：`UP -> DOWN`。

结构和 `CrossBar_UP()` 类似，也是“Reserve + Transmit”两阶段。

关键差异：

- 输入来自 `req_tx_to_UP[i]`
- 输出写到 `*_to_DOWN[o-1]`
  - 这里用 `o-1` 是因为层索引映射规则（`o` 可能是物理层号，端口数组是链路段号）

同样：

- `HEAD` 时申请/预留输出
- `TAIL` 时释放预留
- `start_from_layer_D++` 轮询公平

---

### 3.5 `Mesh()`

**作用**：最简单的直连模式（不做跨层重映射）。

对每个垂直段 `i`：

- flit：`DOWN[i] -> UP[i]`，`UP[i] -> DOWN[i]`
- ack：对应回传
- req：对应回传

一句话：**同编号端口硬连通**。

---

### 3.6 `routingFunction(const NoximRouteData&)`

**作用**：根据全局路由算法，选择 VLink 的目标 z 层。

内部先把 `current/src/dst id` 转坐标，再根据 `NoximGlobalParams::routing_algorithm` 分派：

- `ROUTING_DOWNWARD` -> `routingDownward(...)`
- `ROUTING_DLADR / ROUTING_DLAR / ROUTING_DLDR` -> `routingTLAR(...)`
- 其他 -> 默认 `dst_coord.z`

---

### 3.7 `routingDownward(...)`

**作用**：Downward 策略计算目标层。

规则：

1. 先算 `layer = source.z + down_level`（超顶层则截断到顶层）
2. 若当前 `(x,y)` 已和目标 `(x,y)` 相同 -> 直接去 `destination.z`
3. 否则先去 `layer`

直观理解：

- 先把包“抬/放”到指定 downward 层做水平路由
- 到达目标列后再去最终目标层

---

### 3.8 `routingTLAR(...)`

**作用**：TLAR 类（DLAR/DLDR/DLADR）策略下决定目标层。

规则：

1. 若当前 `(x,y)` 已到目标列 -> 返回 `destination.z`
2. 否则看 `dw_layer_sel`：
   - `DW_BL`：强制最底层 `mesh_dim_z-1`
   - `DW_ODWL`：用 flit 携带的 `dw_layer`
   - 默认：`dw_layer`

---

## 4. 调用关系（Call Graph）

```text
SC_CTOR(NoximVLink)
├─ buildVLink()
└─ SC_METHOD(entry) [clock.neg()]

entry()
├─ if(reset)
│  ├─ reservation_table_UP.clear()
│  └─ reservation_table_DOWN.clear()
└─ else
   ├─ if(vertical_link == VERTICAL_MESH)
   │  └─ Mesh()
   └─ if(vertical_link == VERTICAL_CROSSBAR)
      ├─ CrossBar_UP()
      │  └─ routingFunction()
      │     ├─ routingDownward()
      │     └─ routingTLAR()
      └─ CrossBar_DOWN()
         └─ routingFunction()
            ├─ routingDownward()
            └─ routingTLAR()
```

---

## 5. 新手最该抓住的主线

请先记住这三句话：

1. `entry()` 是总入口：reset 或执行 mesh/crossbar。
2. crossbar 模式是“HEAD 预留、BODY/TAIL 按预留走、TAIL 释放”。
3. `routingFunction()` 只负责“这次垂直传输要去哪个 z 层”。

掌握这三点后，再看细节（ack/req 时序、`o-1` 映射）会轻松很多。

---

## 6. 和 NoximNoC 的关系（帮助建立全局视角）

在 `NoximNoC::buildMesh()` 中，每个 `(x,y)` 会创建一个 `NoximVLink`，并把各层 tile 的 `UP/DOWN` 端口接到 VLink 的对应数组端口上。

所以：

- `NoximNoC` 负责“搭线”
- `NoximVLink` 负责“在这根竖向线里怎么转发”

---

## 7. 一句话总结

`NoximVLink.cpp` 实现了 3D NoC 的垂直转发器：

- 简单模式下做逐层直通（`Mesh`）
- 高级模式下通过预留表 + 路由函数进行跨层仲裁转发（`CrossBar_UP/DOWN`）
