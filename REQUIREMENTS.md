# Key2Airport 需求与设计文档

> 版本：v0.19（初稿，随讨论持续更新）
> 最后更新：2026-09-21
> 说明：本文档是需求与实现的基准。后续讨论产生的结论都要回写到本文档，代码实现以本文档为准。

---

## 1. 项目背景

微软模拟飞行 2024（MSFS 2024）自带的"设置 → 控制"（Controls）里，按键绑定项并不完整，具体表现为：

1. 所有飞机都缺少 VNAV 按钮的绑定项。自 2020 版上线以来该问题就存在，2024 版仍未解决。
2. 西锐（Cirrus）机型的部分旋钮、按钮没有对应绑定项。
3. 实际可用的控制项散落在各机型自己的仪表逻辑里（SimVar / LVar / B 事件 / H 事件），官方 UI 只暴露了其中一部分。

社区的通用解法是使用第三方软件把物理按键"翻译"成模拟器内部事件，常见工具：SPAD.neXt、AxisAndOhs（AAO）、MobiFlight、FSUIPC7。它们能力很强，但收费、复杂、偏重型。

本项目的目标：做一个轻量、免费、自己可控的小工具，用 SimConnect SDK 实现自定义按键/手柄绑定，补上官方缺失的绑定项。

---

## 2. 目标与非目标

### 2.1 目标

- 全局监听键盘按键与游戏控制器（摇杆/手柄/HID 面板）输入，即使 MSFS 处于前台焦点也能收到。
- 把捕获到的物理输入映射成"发送给模拟器的一个动作"。
- 动作的目标来源：在模拟器开发者工具里查到的控件 ID（InputEvent 名/hash、事件名、LVar 等）。
- 支持按机型保存/切换配置（VNAV 之类的绑定天然是"每架飞机一套"）。
- 可配置、可测试、可日志排查。

### 2.2 非目标（至少第一阶段）

- 不做机模/场景开发，不修改模拟器文件，不注入进程。
- 不做完整的 SimVar 监视器/开发工具替代品（后续可作为副产品）。
- 不替代官方控制设置：不改动、不删除用户已有的绑定。

---

## 3. 技术调研结论（关键，决定实现路线）

### 3.1 给模拟器"发控制"的三条通道

SimConnect 里能把控制送进座舱的通道有三类，能力与代价差别很大：

| 通道 | 典型形态 | SimConnect 调用 | 本项目可用性 |
| --- | --- | --- | --- |
| K 事件（Key Event） | `AP_MASTER`、`HEADING_BUG_SET`、`GPS_VNAV_BUTTON` | `MapClientEventToSimEvent` + `TransmitClientEvent(_EX1)` | 可用且最省事；但官方事件表里没有 VNAV 类事件 |
| InputEvent（B: 事件） | `AS1000_PFD_1_NAV_Mode_*`、`ELECTRICAL_Switch_Bus_1` | `EnumerateInputEvents` → `SetInputEvent(hash, ...)` | 本项目主通道，MSFS 2024 新 API |
| LVar / H 事件 | `XMLVAR_VNAVButtonValue`、`AS1000_PFD_COM_Small_INC` | 无直接 API，需 WASM 里执行 RPN | 第一阶段不做，见 §3.4 与 §8 |

### 3.2 InputEvent（B 事件）API 细节

来自官方文档（链接见 §10）：

- `SimConnect_EnumerateInputEvents(reqID)`：返回当前飞机所有可用 InputEvent 的名字 + CRC hash（分页，可能触发多次 `SIMCONNECT_RECV_ENUMERATE_INPUT_EVENTS`）。
- `SimConnect_EnumerateInputEventParams(hash)`：返回该事件的参数类型串，例如 `";FLOAT64"`、`";FLOAT64;char[256]"`。
- `SimConnect_GetInputEvent(reqID, hash)`：读取当前值。
- `SimConnect_SubscribeInputEvent(hash)`：订阅值变化；hash 传 0 表示订阅全部。
- `SimConnect_SetInputEvent(hash, cbUnitSize, pValue)`：按参数类型打包成一个连续缓冲区写入，不产生响应事件。
- `SimConnect_EnumerateControllers()`：列出当前接入的设备（`SIMCONNECT_CONTROLLER_ITEM`）。
- `SimConnect_MapInputEventToClientEvent_EX1()`：可以让 SimConnect 自己把 `"keyboard:a+B"`、`"joystick:0:button:0"` 这类输入映射到 client event，但它只能映射到 K 事件，不能触发 InputEvent hash。

SDK 头文件里的关键结构（已核对本地 `SimConnect.h`）：

```cpp
SIMCONNECT_INPUT_EVENT_DESCRIPTOR { char Name[64]; UINT64 Hash; SIMCONNECT_INPUT_EVENT_TYPE eType; };
SIMCONNECT_RECV_ENUMERATE_INPUT_EVENTS : dwArraySize + rgData[];        // 枚举结果
SIMCONNECT_RECV_GET_INPUT_EVENT        : dwRequestID + eType + Value;   // 读取结果
SIMCONNECT_RECV_SUBSCRIBE_INPUT_EVENT  : Hash + eType + Value;          // 订阅回调
SIMCONNECT_RECV_ENUMERATE_INPUT_EVENT_PARAMS : Hash + Value[MAX_PATH];  // 参数串
```

接收 ID：`SIMCONNECT_RECV_ID_ENUMERATE_INPUT_EVENTS` / `_GET_INPUT_EVENT` / `_SUBSCRIBE_INPUT_EVENT` / `_ENUMERATE_INPUT_EVENT_PARAMS` / `_CONTROLLERS_LIST`。

数据结构细节（已核对本地头文件）：

- 枚举结果是一个**结构体数组**（二进制块），不是字符串列表。每条 `SIMCONNECT_INPUT_EVENT_DESCRIPTOR` = `Name`（`char[64]`，事件名，最长 63 字符）+ `Hash`（`UINT64`）+ `eType`（`SIMCONNECT_INPUT_EVENT_TYPE`：`DOUBLE`=0 / `STRING`=1，表示"值"的类型）。
- 整段接收结构由 `#pragma pack(push,1)` … `pack(pop)` 包裹（头文件 467–1004 行），因此布局紧凑、无对齐填充：单条 64 + 8 + 4 = 76 字节。
- 外层 `SIMCONNECT_RECV_ENUMERATE_INPUT_EVENTS` 继承 `SIMCONNECT_RECV_LIST_TEMPLATE`，带 `dwRequestID / dwArraySize / dwEntryNumber / dwOutOf`，后两个字段就是分页信息。
- 调用 SDK 时用的是 **hash（数字）**；名字只用于显示、查表与写入配置，两者都建议保存。

四个结构体容易混，对照如下：

| 用途 | 结构体 | 字段 | 特点 |
| --- | --- | --- | --- |
| 枚举 | `SIMCONNECT_INPUT_EVENT_DESCRIPTOR` | `Name[64]` + `Hash` + `eType` | 唯一带**名字**的结构 |
| 座舱里操作触发的通知 | `SIMCONNECT_RECV_SUBSCRIBE_INPUT_EVENT` | `Hash` + `eType` + `Value` | **没有名字**；无数组/分页字段，一次报一个 |
| 主动读值 | `SIMCONNECT_RECV_GET_INPUT_EVENT` | `dwRequestID` + `eType` + `Value` | 连 hash 都没有，只能靠 requestID 记账 |
| 参数签名 | `SIMCONNECT_RECV_ENUMERATE_INPUT_EVENT_PARAMS` | `Hash` + `Value[MAX_PATH]` | `Value` 是分号串，如 `";FLOAT64"` |

由此：监听座舱操作拿到的**不是**枚举那个结构体 —— 字段大体对应，但**少一个名字、多一个值**，且顺序是 Hash 在前。要显示名字必须回查枚举表。

`Value` 在头文件里只是**占位**（`SIMCONNECT_DATAV(Value, eType,)` 展开成 `DWORD Value;`），实际长度由 `eType` 决定（`DOUBLE` = 8 字节；`STRING` = 定长字符数组）。解析时不要用 `sizeof(struct)`，应结合 dispatch 回调给出的 `cbData` 与字段偏移计算，并先判 `dwID`。另外，所有 RECV 结构都带 `SIMCONNECT_RECV` 头部（`dwSize` / `dwVersion` / `dwID`），"hash + eType + value"是头部之后的载荷。

读值必须按**指针**取：`double v = *(double*)&evt->Value;`。Asobo 开发者在开发支持论坛确认，直接强转 `(double)evt->Value` 会**永远得到 0.0**；字符串同理按指针取。

多参数事件的待验证点：绝大多数 InputEvent 只有 0 或 1 个参数（官方原话），但确实存在多参数事件（如 `";FLOAT64;char[256]"`、论坛提到过两个 double 的换频按钮）。`SetInputEvent` 是把参数按签名顺序打包成一整块缓冲区发送的；**订阅通知里是否也把全部参数打包过来、还是只给第一个，文档没有说明**，需要实测。判定办法很直接：用 `dwSize - offsetof(..., Value)` 算载荷长度——等于 8 就是单个 `DOUBLE`，大于 8 说明打包了多个参数，再按 `EnumerateInputEventParams` 的顺序拆。

另外不要把 `eType` 当作 `Set` 的打包依据：`eType` 只说明该事件对外表现的**值**是数值还是字符串；`SetInputEvent` 要发什么由 `EnumerateInputEventParams` 的参数列表决定（0 个、1 个或多个，类型有 `FLOAT64`、`char[n]` 等，论坛里出现过要两个 double 的事件）。两者不保证一致，以参数签名为准。

**枚举出来的到底是什么（重要）**

- 枚举的是"开发者显式声明成 InputEvent 的座舱交互项"，不是"座舱里有多少控件"的清单。
- 值的形态只有两种：`SIMCONNECT_INPUT_EVENT_TYPE` = `DOUBLE` 或 `STRING`；具体参数个数与类型由 `EnumerateInputEventParams` 决定（0 个、1 个或多个）。
- 所以按钮、开关、旋钮、多档开关、连续量、字符输入（如 G1000 控制面板的 `AS1000_CONTROL_PAD_*`）都可能出现在列表里，取决于机模作者怎么写。
- 但"增量型"控件（`_INC` / `_DEC` / `_TOGGLE` / `_ON` / `_OFF`）不在其中，只暴露 SET 语义。
- 不属于这条通道、因此枚举不到的：LVar、SimVar（A:）、K 事件、H 事件，以及用纯 MouseRect 实现的交互（如 PMDG 737，枚举还会抛异常）。
- 判断某架飞机有没有 InputEvent：Dev Mode 的 Behaviors Debug 窗口里选中该 SimObject，有 InputEvent 页签才会有枚举结果。
- 规模与噪声：官方没有公布数量，实际条数由机模作者决定。2024 比 2020 明显更多，且**同一个 name+hash 可能出现两三次重复**，另有 `UNKNOWN_*` 这类无意义条目（devsupport 12613）。因此"枚举总数"和"真正可用的事件数"是两个数，必须按 name+hash 去重并过滤 `UNKNOWN_` 前缀。
- 作为量级参考：Working Title 单独公布的 G1000 NXi 控制面板（GCU）按键与旋钮就有四五十项；一台航电就这个量级，整机自然到数百量级。

### 3.3 官方与社区现状（证据）

- 官方论坛确认"没有 VNAV 绑定项"，且这不是配置问题而是功能缺失（主题 704323、708215、506256）。社区回答统一是"用 SPAD.neXt / AAO / MobiFlight 之类的第三方软件"。
- VKB FSM-GA 参考帖（主题 506271）给出了 VNAV 的实际解法：用 MobiFlight 发送 RPN `(L:XMLVAR_VNAVButtonValue) ! (>L:XMLVAR_VNAVButtonValue)`，并明确写着 "VNAV is INOP/unmapped - there is no equivalent keybind mapping"。
- 开发支持论坛（主题 17947）确认：西锐 Vision Jet（Working Title/Asobo，2024 原生）的 VNAV 是 LVar `XMLVAR_VNAVButtonValue`，不存在对应的标准 SimVar 或键位。
- 官方 Key Events 事件表里有 `GPS_VNAV_BUTTON`，但那是给传统 GPS 用的，没有通用 `AP_VNAV` 类事件。

结论：VNAV 这类控件在不同飞机上的实现方式不同（有的可能是 B 事件，有的只有 LVar），所以工具必须按飞机解析目标，并且要能容纳 LVar/H 事件的后续扩展。

### 3.4 已确认的坑（实现时必须处理）

1. 只有 SET，没有 INC/DEC/TOGGLE：`EnumerateInputEvents` 只返回 SET 型事件，`_INC/_DEC/_TOGGLE/_ON/_OFF` 不返回（官方 by design，已进 backlog）。旋钮/开关的空翻需要"读当前值 → 取反/加一 → 写回"（devsupport 6598）。
2. B 事件不能当 K 事件发：`TransmitClientEvent_EX1` 发不了 B 事件，必须 `EnumerateInputEvents` → `SetInputEvent`（devsupport 8501）。
3. 参数要按签名打包：2024 是把所有参数按 `EnumerateInputEventParams` 的顺序打包成一整块缓冲区（2020 的行为不同，两者不兼容）；SDK 在 SU3 修复过参数处理 bug，版本太老会出错（devsupport 13472）。
4. 2024 写入后不回推：`SetInputEvent` 之后，订阅者收不到变化回调（2020 会）。状态显示要自己维护或轮询（devsupport 6598）。
5. 有些机模根本没有 InputEvent：PMDG 737 直接让 `EnumerateInputEvents` 抛异常（devsupport 18279）。必须容错，不能假设调用一定成功。
6. 移植老机模可能失效：2024 里部分从 2020 移植的飞机，原生 InputEvent 触发无效，只能退回 BVar/RPN 路径（devsupport 13472）。
7. LVar/H 事件无 SimConnect 直通：需要自己在 WASM 里执行 RPN，属于另一套工程（需要 MSFS WASM 工具链，编译 .wasm 模块并放到 Community 目录）。
8. 枚举时机有坑：飞机刚加载完立刻调用，可能返回一个泛型 `ERROR`；要等飞机完全加载（对应 Behaviors 窗口里 InputEvent 列表出现的那一刻）再枚举，社区实测需要数秒延迟（devsupport 6572）。
9. 历史上"只能在开发者模式下枚举"：SU13 期间正常模式下枚举返回空列表（已修，devsupport 6539）。属于需要版本探测的历史坑。
10. 订阅全部（`SubscribeInputEvent(0)`）不是所有机模都生效：官方机型可以在座舱里按按钮触发通知，但个别第三方机模（如 FSS E-Jets）不会推送（devsupport 6539）。
11. 学习模式在官方机型上可用：用户在座舱里操作控件时，默认机型确实会把对应 InputEvent 的变化推送给订阅者（devsupport 6539 的实测描述）。

### 3.5 现有同类工具（定位参考）

| 工具 | 做法 | 参考价值 |
| --- | --- | --- |
| MobiFlight | WASM + RPN + HubHop 预设库 | 覆盖面最广（能解决 LVar/H），可参考其"学习模式"与预设库 |
| AxisAndOhs (AAO) | 独立 SimConnect 客户端 + RPN | 与本项目路线最接近，作者即 devsupport 里反馈 InputEvent 问题的 LorbySI |
| SPAD.neXt | 独立客户端 + 大量设备驱动 | 商业产品，功能天花板 |
| FSUIPC7 | 独立客户端 | 老牌，偏通用 |

结论：本项目走"独立 SimConnect 客户端 + InputEvent 通道"这条路线是成熟的，AAO 已经证明可行；差异是本项目要更轻、更聚焦。

### 3.6 "自动识别我在按哪个控件"的可行性与限制

模拟器侧（座舱控件）：只能通过 `SubscribeInputEvent(hash)`（hash 传 0 = 订阅全部）拿到"某个 InputEvent 的值变了"，即 `SIMCONNECT_RECV_SUBSCRIBE_INPUT_EVENT{Hash, Value}`。据此反查名字，就能实现"我点一下座舱按钮，软件自动填好事件"。**这就是学习模式要用的 API。**

实测证据（devsupport 6539）：官方/默认机型下，用户在座舱里操作控件时，订阅者确实会收到对应 InputEvent 的通知；但个别第三方机模（如 Aerosoft/FSS E-Jets）即使订阅全部也收不到任何东西。

限制：

- 只覆盖被声明成 InputEvent 的控件；LVar / H 事件 / 纯 MouseRect 的控件读不到（VNAV、GCU 旋钮、PMDG 都属于这一类风险）。
- 只覆盖 SET 型；旋钮的 INC/DEC 不在枚举内，"我转旋钮"大概率读不到对应事件。
- 只知道"值变了"，不知道你在座舱里点的是哪个 3D 控件，名字仍需人工核对。
- 已知崩溃风险：SU14 之前"订阅全部 / 切换飞机"会导致模拟器 CTD（已修）；订阅鼠标事件在未接鼠标时仍会 CTD（2026-08 报告）。学习模式必须按需订阅 + `SimStop`/暂停时退订 + 版本检查。
- 记账差异：`GetInputEvent` 回包只有 `dwRequestID`（不含 hash），`SubscribeInputEvent` 回包只有 `Hash`（不含 requestID）。
- 回包不含名字：必须先完成枚举并建立 Hash → 名字映射，否则收到通知也不知道是哪个控件；通知里带的当前值正好可以用来判断"按下时该发什么值"。
- 旋钮类不确定：如果机模的旋钮只走 H 事件 / JS，底层 B 值不变，就不会有通知，需要逐机型实测。

硬件侧（你手上的物理按钮/旋钮）：确定可读——键盘 `vkCode`、RawInput 的"设备 + HID usage"、XInput 的按钮位。难点在于把 usage 变成可读名字，以及识别"旋钮 = 两个按钮(CW/CCW) 或一个相对轴"。

要覆盖 LVar/H 型控件的自动学习，必须走 WASM 侧监听（WASM 内可以读/订阅 LVar 与 B/H 变量）。

结论：把"学习模式"定位为辅助手段，人工核对（Dev Mode → Behaviors Debug）仍是兜底路径。

### 3.7 落地数据（接地率）能不能读 —— 可以

已核对官方 SimVar 表（`SimVars/Aircraft_SimVars/Aircraft_Misc_Variables`）：

| SimVar | 类型 / 单位 | 说明 |
| --- | --- | --- |
| `PLANE TOUCHDOWN NORMAL VELOCITY` | float，英尺/秒 | 上一次接地的法向速度，就是通常说的接地率（×60 = 英尺/分） |
| `PLANE_TOUCHDOWN_PITCH_DEGREES` | float，度 | 接地时的俯仰角 |
| `PLANE_TOUCHDOWN_BANK_DEGREES` | float，度 | 接地时的坡度 |
| `PLANE_TOUCHDOWN_HEADING_DEGREES_TRUE` / `_MAGNETIC` | float，度 | 接地航向 |
| `PLANE_TOUCHDOWN_LATITUDE` / `PLANE_TOUCHDOWN_LONGITUDE` | float，弧度 | 接地点坐标 |
| `SIM ON GROUND` | Bool | 是否在地面，用于判定"刚刚接地" |
| `VERTICAL SPEED` | float，英尺/分 | 瞬时垂速，可作辅助或兜底 |

要点：

- 这些值都是"上一次接地"的留存值，落地之后仍然读得到，不需要抢那一帧。
- **没有"接地"事件**：官方系统事件表里没有 Touchdown，Key Events 表里也没有对应事件，所以判定方式是轮询 —— 建立数据定义（`SIM ON GROUND` + `PLANE TOUCHDOWN NORMAL VELOCITY` 等）并用 `SIMCONNECT_PERIOD_SIM_FRAME`（或 `VISUAL_FRAME`）请求，捕捉 `SIM ON GROUND` 由 0 → 1 的时刻。
- 换算：fpm = ft/s × 60。
- 文档命名不太一致：接地率那个名字在官方表里是带空格的 `PLANE TOUCHDOWN NORMAL VELOCITY`，同组的其他变量却是下划线形式（`PLANE_TOUCHDOWN_*`），实测时两种写法都值得试。
- 风险：个别第三方机模可能不更新这些值，需要实测确认。

### 3.8 无线电高度（RA）能不能读 —— 可以，但要分清两个变量

| SimVar | 官方描述 | 单位 | 说明 |
| --- | --- | --- | --- |
| `RADIO HEIGHT` | Radar altitude | 英尺 | 无线电高度表（RA）语义 |
| `PLANE ALT ABOVE GROUND` | Altitude above the surface of the world, including obstacles like buildings | 英尺 | 模拟器自己算的离地高度，把建筑等地物也算进去 |
| `PLANE ALT ABOVE GROUND MINUS CG` | 同上，减去重心偏移 | 英尺 | 以机体参考点为准的离地高度 |
| `PLANE ALTITUDE` | 飞机高度 | 英尺 | 气压高度（MSL），不是离地高度 |

要点：

- `RADIO HEIGHT` 是 RA 语义的那个变量。它是模拟器侧的核心数据，不依赖机模有没有建模无线电高度表；但个别第三方机模是否会改写/清零它，需要实测确认。
- `PLANE ALT ABOVE GROUND` 覆盖面最广，任何飞机都读得到，但它不等于雷达高度（它把建筑物等地物也计算在内，是几何意义上的离地高度）。
- 真实 RA 的有效量程约 2500 ft；模拟器里超出量程时的返回值（上限值、0、还是继续）建议实测。
- 采样方式与 §3.7 相同：建数据定义，用 `SIMCONNECT_PERIOD_SIM_FRAME`（或 6Hz）周期请求。

用途：给没装 RA 的飞机做虚拟无线电高度显示、做低高度呼叫（callout）、做"最低高度"类提示。

---

## 4. 需求

### 4.1 功能需求（FR）

| 编号 | 需求 | 优先级 |
| --- | --- | --- |
| FR-1 | 连接模拟器：自动检测/重连，显示连接状态，模拟器退出时自动断开 | P0 |
| FR-2 | 识别当前飞机：机型切换时自动切换配置集 | P0 |
| FR-3 | 枚举当前飞机的 InputEvent（名称 + hash + 参数签名），支持搜索/过滤 | P0 |
| FR-4 | 手动触发：在列表里选中某个事件，输入值并发送（用于验证是否有效） | P0 |
| FR-5 | 键盘输入捕获：全局监听（含 MSFS 前台时），支持组合键与修饰键 | P0 |
| FR-6 | 游戏控制器捕获：按钮、POV/帽键、（可选）轴 | P0 |
| FR-7 | 映射管理：一条映射 = 物理输入 → 目标动作；支持新增/编辑/删除/复制/排序/启停 | P0 |
| FR-8 | 配置持久化：JSON 存盘，按飞机分 profile，可导入/导出 | P0 |
| FR-9 | 触发语义：按下触发、抬起触发、长按/自动重复（旋钮连续 INC/DEC 需要） | P1 |
| FR-10 | 状态处理：TOGGLE 型目标用"读值 → 取反 → 写回"实现；可显示当前值 | P1 |
| FR-11 | 可选屏蔽原按键：避免物理按键同时触发游戏内已有绑定（造成双动作） | P1 |
| FR-12 | 学习/探测模式：订阅当前飞机所有 InputEvent，把发生变化的项高亮，辅助找 ID | P1 |
| FR-13 | 设备热插拔：重新插拔控制器后自动恢复 | P1 |
| FR-14 | 日志：连接、调用、异常（含 SimConnect 异常码）可视化 | P1 |
| FR-15 | K 事件通道：支持按事件名发送（`AP_MASTER` 等），作为 InputEvent 的补充 | P2 |
| FR-16 | LVar/H 事件通道：通过 WASM 执行 RPN | P2（待定，见 §8） |

### 4.2 非功能需求（NFR）

- 延迟：从按键按下到模拟器响应，目标 < 50 ms，常规情况应在 20 ms 内。
- 稳定性：模拟器未运行时正常提示，不崩溃；异常调用不影响后续功能。
- 无侵入：不注入模拟器进程，不修改游戏文件；以普通用户权限运行也能工作。
- 资源占用：空闲时 CPU 接近 0，内存可控（常驻后台工具）。
- 可维护性：SimConnect 相关代码集中封装，业务逻辑不直接调 SDK。
- 国际化：界面中文为主，文案可扩展（暂不做多语言框架）。

---

## 5. 技术方案

### 5.1 技术栈与环境（现状）

- Qt 6.7.1 / MinGW 64-bit（Desktop Qt 6.7.1 MinGW 64-bit），C++17，Qt Widgets。
- SimConnect SDK（MSFS 2024）：`SimConnect SDK/include/SimConnect.h`，导入库 `SimConnect SDK/lib/SimConnect.lib`（运行时需要 `SimConnect.dll` 在 exe 旁）。
- `.pro` 已配置好 include/lib 路径（含带空格路径的 `$$quote()` 处理），链接已实测可用。
- 目标平台：Windows x64（SimConnect 仅 Windows）。

### 5.2 模块划分

```
UI 层（Qt Widgets）
  主窗口 / 映射编辑 / InputEvent 浏览器 / 日志面板
      |
      +--> MappingEngine（规则匹配/触发语义） <---> ProfileStore（JSON 配置/按机型）
      |            |
      |            +--> TargetDispatcher（InputEvent 通道 / K 事件通道 / 预留 RPN 通道）
      |                        |
      |                        +--> SimConnectClient（连接/分发/枚举/收发）
      |
      +--> InputCapture（键盘钩子 + RawInput/XInput）
```

### 5.3 与 SimConnect 的集成方式

- 连接：`SimConnect_Open(&h, "Key2Airport", hwnd, WM_USER+1, nullptr, 0)`，用 Qt 的 `QAbstractNativeEventFilter` 截获 `WM_USER+1` 后调用 `SimConnect_CallDispatch`；或用事件句柄 + `QWinEventNotifier` + `SimConnect_GetNextDispatch` 的轮询方式（二选一，优先前者）。
- 重连：模拟器未启动时 `SimConnect_Open` 返回 `E_FAIL`，用 `QTimer` 定时重试；收到 `SIMCONNECT_RECV_ID_QUIT` 视为断开。
- 飞机识别：订阅系统事件 `AircraftLoaded`（回调里带 `szFileName`/`szAircraft`），配合 `TITLE` / `ATC MODEL` 等 SimVar 做 profile 匹配。
- 线程模型：SimConnect 的 dispatch 由调用线程执行；MVP 阶段全部放主线程（GUI + 输入 + 分发），延迟敏感或长任务再拆工作线程。

### 5.4 输入捕获方案（Windows）

| 输入 | 方案 | 说明 |
| --- | --- | --- |
| 键盘 | `SetWindowsHookEx(WH_KEYBOARD_LL)` | 全局捕获（MSFS 前台也能收到）；返回值可吞掉按键，用于 FR-11 屏蔽 |
| 手柄/HID 设备 | RawInput（`RIDEV_INPUTSINK`）+ `WM_INPUT` | 可后台接收；能识别具体设备，适合摇杆/面板 |
| Xbox 手柄 | XInput（可叠加） | 振动、扳机轴更方便；设备识别能力弱于 RawInput |
| 老式 DirectInput 设备 | 预留 | 有需要再补 |

统一抽象为 `InputEvent{deviceId, controlId, phase(Down/Up/Repeat), value}`，上层不关心来源。Qt 6 已移除 QGamepad，手柄必须走上面这些 Win32 路径。

### 5.5 一次按键的数据流（示例：键盘 V → 某机型 VNAV）

1. 键盘钩子捕获 V 按下，生成 `InputEvent{keyboard, VK_V, Down}`。
2. `MappingEngine` 按当前飞机 profile 匹配规则，产出 `ActionCommand{target=InputEvent, name=..., hash=..., value=...}`。
3. `TargetDispatcher` 查 hash 缓存与参数签名，按签名打包缓冲区。
4. `SimConnect_SetInputEvent(h, hash, size, buf)` 发送。
5. 若目标是 TOGGLE 语义：先读当前值（`GetInputEvent` 或订阅缓存），取反后写回。

---

### 5.6 生命周期与 API 调用序列

**阶段 A：进程启动（模拟器可以还没开）**

1. `SimConnect_Open(&hSimConnect, "Key2Airport", hwnd, WM_USER+1, nullptr, 0)`。模拟器没运行时返回 `E_FAIL`，用 `QTimer` 定时重试。
2. 订阅系统事件：`SimConnect_SubscribeToSystemEvent(h, EVENT_AC_LOADED, "AircraftLoaded")`（另外常用 `"SimStart"` / `"SimStop"` / `"1sec"`）。
3. 安装输入捕获：键盘 `SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, nullptr, 0)`；手柄/HID `RegisterRawInputDevices(..., RIDEV_INPUTSINK)`。
4. 消息分发：在 Qt 的 native event filter 里捕获 `WM_USER+1`，调用 `SimConnect_CallDispatch(h, DispatchProc, ctx)`。

**阶段 B：飞机加载完成**

1. `SimConnect_EnumerateInputEvents(h, REQ_ENUM_IE)` → 分页收到 `SIMCONNECT_RECV_ENUMERATE_INPUT_EVENTS`，得到 `{Name, Hash, eType}` 列表。
2. 对需要用到的事件：`SimConnect_EnumerateInputEventParams(h, hash)` 取参数签名；`SimConnect_SubscribeInputEvent(h, hash)` 订阅变化（hash=0 订阅全部，用于学习模式）。
3. 用映射配置里的**事件名**在当前列表里查 hash，生成本次飞行的可执行映射表；查不到的标记为"该机型不支持"。

**阶段 C：按下按键（每次触发）**

1. 键盘钩子回调收到 V（`VK_V`）→ 匹配映射 → 打包参数。
2. `SimConnect_SetInputEvent(h, hash, cbUnitSize, buffer)` 发送。
3. 如果是状态型/空翻型目标：先用 `SimConnect_GetInputEvent(h, reqID, hash)`（或订阅缓存的值）算新值，再发送。

**阶段 D：退出 / 断开**

`SimConnect_Close(h)`，`UnhookWindowsHookEx`，释放 RawInput 注册。

本工程会用到的主要 API（签名以本地 `SimConnect.h` 为准）：

```cpp
HRESULT SimConnect_Open(HANDLE* phSimConnect, LPCSTR szName, HWND hWnd,
                        DWORD UserEventWin32, HANDLE hEventHandle, DWORD ConfigIndex);
HRESULT SimConnect_EnumerateInputEvents(HANDLE h, SIMCONNECT_DATA_REQUEST_ID requestID);
HRESULT SimConnect_EnumerateInputEventParams(HANDLE h, UINT64 hash);
HRESULT SimConnect_GetInputEvent(HANDLE h, SIMCONNECT_DATA_REQUEST_ID requestID, UINT64 hash);
HRESULT SimConnect_SubscribeInputEvent(HANDLE h, UINT64 hash);
HRESULT SimConnect_SetInputEvent(HANDLE h, UINT64 hash, DWORD cbUnitSize, void* value);
HRESULT SimConnect_CallDispatch(HANDLE h, DispatchProc fn, void* context);
```

注意：官方文档页里 `SetInputEvent` 的 Hash 写成了 `DWORD`，本地头文件是 `UINT64`，以头文件为准。

### 5.7 目标事件的参数与"触发"语义

- **参数签名**来自 `EnumerateInputEventParams` 返回的分号串，去掉开头的空段就是类型列表：
  - `";FLOAT64"` → 一个 `double`（8 字节）
  - `";FLOAT64;char[256]"` → `double` + `char[256]`
- **打包规则**：按类型列表顺序，1 字节对齐（`#pragma pack(1)`）拼成一块连续缓冲，`cbUnitSize` 就是这块缓冲的字节数。
- **触发语义**：InputEvent 是"设值"，不是"发一个空事件"。按下类控件要发什么值，取决于该事件是状态型还是脉冲型，需要实测确认（先 `GetInputEvent` 看当前值、再在座舱里手动操作并观察值变化）。
- **hash 的生命周期**：hash 与当前飞机/上下文相关，因此配置里保存的是**事件名**，运行期在枚举表里按名字查 hash，不要把 hash 写死。
- **调用形态**：`SimConnect_SetInputEvent(h, hash, cbUnitSize, pValue)` 只需要三样东西 —— hash、缓冲区字节数、缓冲区指针。单参数事件就是 `double v = 1.0; SetInputEvent(h, hash, sizeof(v), &v);`；多参数事件按签名用 `#pragma pack(1)` 的结构体打包后一次传入。
- **失败表现**：hash 无效或已过期会抛 `SIMCONNECT_EXCEPTION_SET_INPUT_EVENT_FAILED`；参数长度/顺序与签名不符时行为异常（SDK 在 SU3 修复过参数处理 bug）。发送前用当前飞机的枚举结果校验 hash，并对异常做容错。
- **不能把"监听得到的值"无脑照抄**：通知给出的是**状态变化后的值**，不是"要发送的动作"。要复现的是"状态"还是"动作"决定了发送方式——状态型控件需要读当前值后发目标值（toggle 就取反），脉冲型控件要看通知里的变化序列判断该发几次、发什么。因此学习模式应当记录"值的变化序列"，据此推荐 `valueMode`（`set` / `toggle` / `pulse`）。
- **`cbUnitSize` 以参数签名为准**：通知里的载荷长度只能当参考（多参数时是否给全尚未验证），发送前用 `EnumerateInputEventParams` 的结果决定打包与长度。
- **字符串型（`eType == STRING`）要按定长发送**：`cbUnitSize` 是该字段的**固定长度**（签名里的 `char[N]`，例如 256），不是 `strlen() + 1`；发送时把缓冲区补零到全长。长度可用通知载荷长度交叉验证。
- **签名 token 与字节数对照**（按 `;` 切分、跳过空段）：`FLOAT64` = 8 字节；`char[N]` = N 字节；空字符串 = 该事件没有声明参数。已确认出现的 token 只有 `FLOAT64` 与 `char[N]` 两类，遇到其他 token 按未知处理并记日志。
- **空签名不等于"不用发值"**：实测（Bonanza G36 的 `DEICE_Propeller_1`，`EnumerateInputEventParams` 返回空串）仍需发送 0 或 1 才能触发，且要求发的是与当前值相反的值。所以值仍要照常传，只是打包信息缺失。
- **官方文档不可信**：`SimConnect_EnumerateInputEventParams` 的 Syntax/Parameters 段在官方文档里被错写成了 `EnumerateInputEvents` 的内容（官方已确认的文档 bug，devsupport 18380），一律以实际返回的字符串为准。
- **多参数事件的实例**：官方文档的 `SetInputEvent` 页面自己就给了两参数例子（`";FLOAT64;char[256]"`）；开发支持论坛里 Asobo 明确说过 `AS1000_PFD_1_COM_SWAP` 需要两个 `double`（devsupport 13472）。所以这不是理论情况。
- **实现策略**：先支持 0 / 1 参数（覆盖绝大多数事件），但数据结构（配置里的 `value`）要预留数组形式的表达能力；遇到多于 1 个参数时，明确提示"该事件暂不支持"并记录日志，绝不能不打包就发送。

### 5.8 生命周期状态机（连接 / 飞机 / 退出）

状态划分：

- `Disconnected`：`SimConnect_Open` 失败，或收到 `SIMCONNECT_RECV_ID_QUIT`。
- `Connected`：Open 成功，但可能还在主菜单 —— 连接成功与"是否在飞行中"是两件独立的事。
- `InFlight`：收到 `AircraftLoaded`，或订阅 `Sim` 时返回 1。

关键系统事件（`SimConnect_SubscribeToSystemEvent`）：

| 事件名 | 触发时机 | 回包 | 用途 |
| --- | --- | --- | --- |
| `AircraftLoaded` | 飞机（飞行动力学文件）加载/更换 | `SIMCONNECT_RECV_EVENT_FILENAME.szFileName` | 重新枚举 InputEvent、切换 profile |
| `FlightLoaded` | 航班加载；结束飞行时会加载默认航班，所以开始与结束都会触发 | 同上（文件名） | 辅助判断 |
| `Sim` | 订阅时立即返回当前状态，之后运行/停止时通知 | `dwData`：1=运行，0=未运行 | 连接后立刻判断是否已在飞行中 |
| `SimStart` / `SimStop` | 进入/退出飞行；官方说明存在成对出现的情况（重置、开场画面、SHOW_OPENING_SCREEN=0） | `dwData` | 进入/退出飞行判定，**必须去抖** |
| `Pause` / `Pause_EX1` | ESC 暂停/恢复 | `dwData` | 暂停时降载或暂停发送 |
| `1sec` | 每秒 | — | 兜底轮询（例如周期读 `TITLE`） |

飞机标识字段（已核对官方 SimVar 表）：

| 字段 | 来源 | 粒度 | 说明 |
| --- | --- | --- | --- |
| `TITLE` | aircraft.cfg 的 `title` | 涂装级 | 界面上显示的飞机名；同一型号的不同涂装各不相同 |
| `LIVERY NAME` | livery.cfg 的名字 | 涂装级 | 涂装名 |
| `LIVERY FOLDER` | livery.cfg 所在目录 | 涂装级 | 可作涂装的稳定标识 |
| `ATC MODEL` | ATC 机型串 | 机型级 | 同型号不同涂装相同 |
| `ATC TYPE` | ATC 厂商/类别串 | 机型级 | 同型号不同涂装相同 |
| `ATC ID` | 尾号 | 每次飞行可变 | 不能当机型标识 |
| `AircraftLoaded.szFileName` | 加载事件回包 | — | 飞机文件路径，取目录名可作稳定键（实际内容待实测） |

profile 匹配分两层：**机型级**（`ATC MODEL` + `ATC TYPE`）作为通用配置，一份配置覆盖该机型所有涂装；**涂装级**（`TITLE` / `LIVERY FOLDER`）只在确实需要时为个别涂装做覆盖。

设计要点：

- `SimConnect_Open` 在主菜单就能成功，所以"连上"不等于"有飞机"。
- 连接成功后立即订阅 `Sim` 拿到当前状态；若已在飞行中，主动读一次 `TITLE`（`AircraftLoaded` 不会补发给我们）。
- 收到 `SimStop` / `Sim`=0 后，清空 InputEvent 表与 hash 缓存，等下一次 `AircraftLoaded` 重建。
- 收到 `SIMCONNECT_RECV_ID_QUIT` 视为模拟器退出，回到 `Disconnected`，并卸载输入捕获的相关状态。
- `SimStart`/`SimStop` 可能成对出现，退出判定要去抖（与 `Sim` 当前状态、`TITLE` 复核后再决定）。

---

## 6. 配置数据模型（草案）

```jsonc
{
  "version": 1,
  "profiles": [
    {
      "name": "Cirrus Vision Jet G2",
      "match": {
        "atcModel": ["SR22", "SR22T"],
        "atcType": ["CIRRUS"],
        "titles": []
      },
      "mappings": [
        {
          "id": "1",
          "enabled": true,
          "input": { "device": "keyboard", "code": "VK_V", "modifiers": [], "phase": "down" },
          "target": {
            "kind": "inputEvent",
            "name": "AS1000_PFD_1_VNAV_Mode",
            "hash": "11675888408130357189",
            "valueMode": "toggle",
            "value": 1
          },
          "suppressOriginal": true,
          "repeat": { "enabled": false, "intervalMs": 100 }
        }
      ]
    }
  ]
}
```

`match.atcModel` / `match.atcType` 是机型级匹配（覆盖该机型所有涂装）；`match.titles` 是涂装级匹配，非空且命中时覆盖机型级配置。`target.kind` 取 `inputEvent | keyEvent | rpn`（rpn 预留）；`valueMode` 取 `set | toggle | inc | dec | pressRelease`。

---

## 7. 里程碑

| 阶段 | 内容 | 验收标准 |
| --- | --- | --- |
| M0 骨架 | SimConnect 连接/重连、状态显示、枚举当前飞机 InputEvent 并列表 | 模拟器运行时能看到事件列表；关闭模拟器不崩 |
| M1 通道验证 | 手动选中并发送 InputEvent（含参数打包） | 至少一个真实控件（如 VNAV）被手动触发成功 |
| M2 输入映射 | 键盘捕获 + 映射引擎 + 配置存盘 | 按一个键能稳定触发目标动作 |
| M3 体验完善 | 手柄捕获、TOGGLE/长按/连发、按飞机 profile 自动切换、日志、热插拔 | 日常可替代官方绑定使用 |
| M4 扩展 | K 事件通道、（可选）WASM RPN 通道、HubHop 预设导入、学习模式增强 | 覆盖 LVar/H 型控件 |

---

## 8. 风险与对策

| 风险 | 影响 | 对策 |
| --- | --- | --- |
| 目标控件只有 LVar/H，没有 InputEvent | 纯 SimConnect 方案无法触发（如 Vision Jet 的 VNAV 在旧版本中就是 LVar） | 先做目标可用性验证（M1）；必要时上 WASM RPN 模块（M4） |
| 只暴露 SET，无 INC/DEC/TOGGLE | 旋钮/开关行为不自然或状态不同步 | 读值 + 取反/步进写回；旋钮用连发 + 步进值 |
| 参数签名/打包差异（2020 vs 2024、SU3 前后） | 调用失败或行为错误 | 运行时用 `EnumerateInputEventParams` 动态打包，不写死 |
| 第三方机模无 InputEvent（如 PMDG） | 调用抛异常 | 全面容错，并明确提示"该机型不支持" |
| 物理按键与游戏内绑定重复触发 | 双动作 | 提供屏蔽选项；推荐避开默认绑定的按键 |
| 模拟器更新导致 API 行为变化 | 功能回归 | 版本探测 + 日志；关键路径加集成测试 |
| 低级键盘钩子被安全软件关注 | 用户顾虑 | 开源、无注入、无网络、纯本地发送；不使用全局注入 DLL |

---

## 9. 待确认问题（需要拍板）

1. "西锐 G2"具体是哪一架？MSFS 2024 里有两架西锐：Cirrus Vision Jet G2（G3000 + 触摸控制器，VNAV 走 LVar）和 Cirrus SR22T（G1000 NXi + GCU 控制面板，旋钮走 `AS1000_CONTROL_PAD_*` / H 事件）。两者实现路径不同。
2. 需要绑定的完整控件清单：除 VNAV 外，旋钮具体是哪些（FMS 内外旋钮、COM/NAV/CRS/XPDR 等）。
3. 开发者工具里看到的 ID 长什么样：是 `B:xxx`（InputEvent）、`H:xxx`，还是 `L:xxx`？这直接决定是否需要 WASM 通道。
4. 是否接受后续引入 WASM 模块（若目标只有 LVar/H，这是唯一可行路径）。
5. 是否需要"屏蔽原按键"（吞掉按键，避免与游戏内绑定冲突）。
6. 只用键盘，还是必须支持游戏手柄/摇杆面板？（影响输入捕获层的优先级与工作量）
7. 项目名 Key2Airport 是否意味着后续还有"机场相关"的功能规划？如果有，需要提前设计模块边界。
8. 是否需要兼容 MSFS 2020？
9. 是否要把"落地记录 / 接地率"这类飞行数据功能纳入范围（见 §3.7）？

---

## 10. 参考资料

官方文档（MSFS 2024 SDK）：

- SimConnect SDK 总览：https://docs.flightsimulator.com/msfs2024/html/6_Programming_APIs/SimConnect/SimConnect_SDK.htm
- API 参考：https://docs.flightsimulator.com/msfs2024/html/6_Programming_APIs/SimConnect/SimConnect_API_Reference.htm
- Input Events 一览：https://docs.flightsimulator.com/msfs2024/html/6_Programming_APIs/SimConnect/API_Reference/InputEvents/Input_Events.htm
- 同目录下的单函数页：`SimConnect_EnumerateInputEvents.htm`、`SimConnect_EnumerateInputEventParams.htm`、`SimConnect_GetInputEvent.htm`、`SimConnect_SetInputEvent.htm`、`SimConnect_SubscribeInputEvent.htm`、`SimConnect_MapInputEventToClientEvent_EX1.htm`
- Key Events 事件总表：https://docs.flightsimulator.com/msfs2024/html/6_Programming_APIs/Key_Events/Key_Events.htm
- Simulation Variables 总表：https://docs.flightsimulator.com/msfs2024/html/6_Programming_APIs/SimVars/Simulation_Variables.htm
- SimConnect Inspector（开发模式调试工具）：https://docs.flightsimulator.com/msfs2024/html/2_DevMode/Menus/Tools/SimConnect_Inspector.htm

官方开发支持论坛（devsupport.flightsimulator.com）重点主题：

- 6598 EnumerateInputEvents 不返回 `_INC/_DEC/_TOGGLE/_ON/_OFF`（by design）
- 8501 B 事件不能当 K 事件发送，必须走 SetInputEvent
- 13472 2024 中 InputEvent 参数打包差异与 SU3 修复
- 18279 PMDG 737 调用 EnumerateInputEvents 抛异常
- 11745 模块化飞机的 B 事件问题（SU1 修复）
- 17947 Vision Jet 的 VNAV 是 LVar `XMLVAR_VNAVButtonValue`

用户论坛（forums.flightsimulator.com）：

- 704323 / 708215 / 506256：官方无 VNAV 绑定项，社区用第三方软件解决
- 506271：VKB FSM-GA 绑定参考，含 VNAV 的 MobiFlight RPN 方案

MobiFlight HubHop 预设库（LVar/事件对照数据源）：https://hubhop.mobiflight.com/

---

## 11. 变更记录

| 日期 | 版本 | 变更 |
| --- | --- | --- |
| 2026-09-21 | v0.1 | 初稿：背景、调研结论、需求、方案、风险、待确认问题 |
| 2026-09-21 | v0.2 | 补充 §5.6 生命周期与 API 调用序列、§5.7 参数与触发语义 |
| 2026-09-21 | v0.3 | §3.2 补充"枚举对象到底是什么"的说明 |
| 2026-09-21 | v0.4 | 新增 §3.6"自动识别控件"的可行性与限制 |
| 2026-09-21 | v0.5 | 新增 §5.8 生命周期状态机（连接/飞机/退出） |
| 2026-09-21 | v0.6 | 新增 §3.7 落地数据可读性；待确认问题补充第 9 条 |
| 2026-09-21 | v0.7 | §5.8 补充飞机标识字段表与两层匹配策略；§6 同步 match 结构 |
| 2026-09-21 | v0.8 | 新增 §3.8 无线电高度/离地高度可读性 |
| 2026-09-21 | v0.9 | §3.2/§3.4 补充枚举规模、去重、枚举时机、订阅兼容性等实测反馈 |
| 2026-09-21 | v0.10 | §3.6 明确学习模式所用 API、实测证据与回包限制 |
| 2026-09-21 | v0.11 | §3.2 补充枚举结果的数据结构细节（字段类型、紧凑布局、分页字段） |
| 2026-09-21 | v0.12 | §3.2 补充四个接收结构体对照表、`eType` 与参数签名的区别 |
| 2026-09-21 | v0.13 | §3.2 补充 `Value` 占位与长度计算注意事项 |
| 2026-09-21 | v0.14 | §3.2 补充多参数事件在订阅通知中的待验证点与判定办法 |
| 2026-09-21 | v0.15 | §5.7 补充 SetInputEvent 的调用形态与失败表现 |
| 2026-09-21 | v0.16 | §5.7 补充"监听值不能照抄"的语义说明与 cbUnitSize 取值依据 |
| 2026-09-21 | v0.17 | §5.7 补充字符串型按定长发送的注意事项 |
| 2026-09-21 | v0.18 | §3.2 补充"按指针取值"；§5.7 补充签名 token 对照、空签名与文档错误 |
| 2026-09-21 | v0.19 | §5.7 补充多参数事件实例与实现策略 |
