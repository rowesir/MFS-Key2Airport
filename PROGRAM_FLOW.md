# Key2Airport 程序流程

> 版本：v3.29
> 最后更新：2026-09-23
> 说明：本文档按讨论逐步补充，后续控件逻辑逐节追加。代码实现以本文档为准。

---

## 1. 程序启动初始化

启动时做两件事：初始化主窗口 `Widget`，以及初始化作为其值成员的 `DialogEnum`（枚举/监听窗口）。此时模拟器可以还没启动，界面处于待机状态。

程序入口（`main.cpp`）另外做了两件事：把 `QT_QPA_PLATFORM` 设成 `windows:darkmode=0`（关掉 Windows 深色模式），以及把界面样式设成 `Fusion`。界面外观依赖这两项。

### 1.1 主窗口 Widget

| 控件 | 类型 | 初始状态 | 实现要点 |
| --- | --- | --- | --- |
| `cbConfig` | QComboBox | 空、失能 | 清空所有项；`setEnabled(false)` |
| `ckbAuto` | QCheckBox | 已勾选 | `setChecked(true)` |
| `lbInfo` | QLabel | `Standby...`，黑色 | `Waiting MFS...` 使用橘黄色；`Connected` / `Aircraft Loaded` 使用深绿色；错误不再写入此控件，见第 4、5 节 |
| `pbtnEnum` | QPushButton | 失能 | 仅在收到 `FLIGHT_START` 后使能；飞行结束、连接丢失或异常时失能。点击后打开 `DialogEnum` 并枚举全部输入事件填入 `TWEnumAll` |
| `pbtnTest` | QPushButton | 失能 | 与 `pbtnEnum` 使用相同的飞行状态启用规则；点击后打开 `DialogTest`，用于测试发送浮点型 InputEvent |
| `ckbRA` | QCheckBox | 已勾选 | `setChecked(true)`；飞机加载且勾选时在 SDK 工作线程周期读取标准 SimVar `RADIO HEIGHT` |
| `ckbLR` | QCheckBox | 已勾选 | `setChecked(true)`；飞机加载且勾选时在 SDK 工作线程读取接地状态和上次接地法向速度 |
| `lcdRA` | QLCDNumber | 显示 `----` | `display("----")` |
| `lcdLR` | QLCDNumber | 显示 `-----` | `display("-----")` |
| `lbPage` | QLabel | 空 | 清空文本 |
| `lbDetail` | QLabel | 空 | 清空文本；设置 `Qt::TextSelectableByMouse`，用户可选中文本复制 |

LCD 初始显示：用减号做占位，位数与各自的 `digitCount` 一致 —— `lcdRA` 是 4 位，显示 `----`；`lcdLR` 是 5 位，显示 `-----`。已核对 Qt 源码 `qlcdnumber.cpp`：`-`（minus）在段码表里是有效符号，且属于类文档列出的可显示字符，只有非法字符才会被替换成空格，所以这个方法可用。

配套的 `.ui` 调整：

- `cbConfig` 里现有的示例项 `Cessna 172 Cessna Cessna` 应删除，避免 Qt Designer 预览与运行时不一致。
- `lcdRA` / `lcdLR` 上的 `value` / `intValue` 属性（2500 / -1234）应删除，理由同上。

尚未定义初始状态的控件：`pbtnFolder`（待补充）。`pbtnConnect` 的状态机见第 4 节。

### 1.2 DialogEnum（枚举 / 监听窗口）

生命周期：`DialogEnum` 是 `Widget` 的**值成员**（不是指针），随 `Widget` 构造而构造、随其析构而析构。窗口可反复开关，因此界面初始化必须可以被重复调用。

控件名沿用现有 `.ui` 中的拼写（`TWListem`）。

初始化集中在一个函数里：

```cpp
class DialogEnum : public QDialog
{
    Q_OBJECT

public:
    explicit DialogEnum(QWidget *parent = nullptr);
    ~DialogEnum();

    void initUI();          // 建列 + 清表，可重复调用

private:
    Ui::DialogEnum *ui;
};
```

调用时机：

1. 构造函数末尾调用一次 —— 因此程序启动时两张表已经是空的，满足"程序开始时清空"的要求；
2. 每次打开窗口前再调用一次 —— 因此每次打开窗口都是干净的两张表。

当前打开方式：收到 `FLIGHT_START` 后使能 `pbtnEnum`；点击后先调用 `initUI()` 清空旧数据和枚举缓存，再以模态方式打开 `DialogEnum`，同时向 SDK 线程投递一次 `SimConnect_EnumerateInputEvents` 请求。枚举回包可能分成多个页面，每个页面中的 `SIMCONNECT_INPUT_EVENT_DESCRIPTOR` 都追加到枚举缓存，并按当前过滤条件显示到 `TWEnumAll`；每个 Hash 同时请求 `SimConnect_EnumerateInputEventParams` 获取参数签名，并调用 `SimConnect_SubscribeInputEvent` 开始监听。Dialog 关闭后取消本次打开期间的全部订阅；如果收到 `FLIGHT_END`、发生 SimConnect 异常、连接丢失或主动断开，主窗口会主动关闭 Dialog，随后同样取消监听。`TWListem` 不过滤：普通 Hash 每次通知都插入第 0 行；某个 Hash 在最近 1 秒内达到 8 条通知后进入高频折叠，只保留一行并更新 Value / Time；已折叠 Hash 在最近 1 秒内降到 4 条或更少时恢复普通插入，切换时不恢复旧行，当前通知作为新的第 0 行。参数回包到达后按 Hash 更新当前显示行的 Param / Size。

实现约定：`on_pbtnConnect_clicked` 和 `on_pbtnEnum_clicked` 必须声明在类的 `private slots:` 里。`setupUi` 的自动连接是 `QMetaObject::connectSlotsByName`，它只遍历元对象里注册过的方法，普通成员函数不会被连接。

`initUI()` 的职责：

1. 给 `TWEnumAll` 建立 3 列；
2. 给 `TWListem` 建立 6 列；
3. 两张表统一设置：不可编辑、不排序、禁止拖动列、列宽按窗口宽度平均分配（`QHeaderView::Stretch`）；
4. 清空两张表的所有行（`setRowCount(0)`）；
5. 清空 `TWEnumAll` 的枚举结果缓存，并将 `leFiltra` 清空；
6. 限制 `leFiltra` 只能输入 20 个以内的可打印非空白 ASCII 字符。

建列部分重复调用无害（会被覆盖），所以构造函数与打开前调用同一个函数即可。枚举结果缓存只服务于当前一次枚举，随 `initUI()` 清空。

参数签名缓存属于当前 Dialog 的本次枚举数据，随 `initUI()` 清空；每次打开窗口都会重新按当前枚举结果请求，避免沿用上一架飞机的数据。

#### 1.2.1 TWEnumAll 列定义

用途：显示枚举出的当前飞机全部控件，每条对应一个 `SIMCONNECT_INPUT_EVENT_DESCRIPTOR`。

| 列 | 表头 | 数据来源 | 显示形式 |
| --- | --- | --- | --- |
| 0 | Name | `SIMCONNECT_INPUT_EVENT_DESCRIPTOR::Name`（char[64]） | 原样字符串 |
| 1 | Hash | `::Hash`（UINT64） | 十进制字符串 |
| 2 | eType | `::eType` | `DOUBLE` / `STRING` |

3 列，与结构体三个字段一一对应。

`leFiltra` 用于实时过滤 `TWEnumAll`：输入为空时显示缓存中的全部行；输入非空时只显示 `Name` 包含完整过滤字符串的行，匹配忽略大小写。每次文本变化都会立即重建表格，不设置额外的过滤按钮。过滤框最多 20 个字符，空格及其他空白字符、非 ASCII 字符均不允许输入。

#### 1.2.2 TWListem 列定义

用途：显示监听期间收到的每一条 `SIMCONNECT_RECV_SUBSCRIBE_INPUT_EVENT`。

| 列 | 表头 | 数据来源 | 填充时机 |
| --- | --- | --- | --- |
| 0 | Hash | 结构体 `::Hash`（UINT64） | 收到通知时立即填充 |
| 1 | eType | 结构体 `::eType` | 收到通知时立即填充 |
| 2 | Value | 结构体 `::Value`（长度由 `eType` 决定） | 普通 Hash 每条通知填充新行；高频折叠 Hash 更新原行 |
| 3 | Param | `SimConnect_EnumerateInputEventParams` 返回的签名字符串，如 `;FLOAT64` | 回调返回后补填 |
| 4 | Size | 由 Param 换算出的字节数，即将来 `SetInputEvent` 的 `cbUnitSize`（如 `;FLOAT64` → 8） | 回调返回后补填 |
| 5 | Time | 程序收到该 Hash 最新监听通知的本地时间 | 每次通知到达时更新，格式 `HH:mm:ss` |

6 列。

Param / Size 是异步的：`EnumerateInputEventParams` 是"发请求 → 等回调"，因此一行的插入分两阶段完成 —— 先插入 Hash / eType / Value / Time，回调到了再补 Param / Size。参数按 Hash 缓存，同一个 Hash 只请求一次，后续通知直接取缓存填充；如果通知先到，则参数回包到达后更新该 Hash 的当前显示行。

已知边界情况：部分事件（例如部分除冰开关）的签名可能返回空串；如果该 Hash 的 `eType` 是 `DOUBLE`，则按 8 字节兜底，否则 Size 保持未知（-1），避免伪造不确定的字符串长度。

#### 1.2.3 两张表的通用行为

- **不可编辑**：`setEditTriggers(QAbstractItemView::NoEditTriggers)`。用户双击、按 F2 都无法修改单元格内容。
- **可复制**：Ctrl+C 复制光标所在的那一格。这是 Qt item view 的内置行为，不需要额外写代码。
- **不排序**：不启用 `setSortingEnabled`，点击表头不排序。
- **列宽自适应**：所有列用 `QHeaderView::Stretch`，按窗口宽度平均分配，并随窗口缩放。不用 `ResizeToContents` —— 表初始化时是空的，按内容算宽度会让每列只剩表头那么宽，全部挤在左边。
- **禁止拖动列**：表头不允许拖动换位。

### 1.3 DialogEnum 对外接口

外部（`Widget`）通过下面这些接口把数据送进两张表：

```cpp
// TWEnumAll：追加到末尾
void addEnumAll(const QString &name, quint64 hash, const QString &eType);

// TWListem：插入到第 0 行
void addListen(quint64 hash, const QString &eType, const QString &value,
               const QString &param, int size);

// 参数回包：缓存并回填 TWListem 中相同 Hash 的已有行
void setEnumParam(quint64 hash, const QString &param, int size);
```

`addEnumAll` / `addListen` 负责写入行，`setEnumParam` 负责异步回填参数列。

插入方向：

- `TWEnumAll` 往**尾部**添加（`insertRow(rowCount())`），枚举结果按到达顺序往下排。
- `TWListem` 根据 Hash 的通知频率自适应显示：普通 Hash 每次往**头部**添加（`insertRow(0)`）；最近 1 秒达到 8 条的 Hash 进入折叠，已有同 Hash 行合并为一行，后续只更新 Value、Time、eType、Param 和 Size；折叠 Hash 最近 1 秒降到 4 条或更少时恢复逐条插入。恢复时不恢复折叠前的旧行，只从触发恢复的最新通知开始。

Param / Size 的来源：`SIMCONNECT_RECV_SUBSCRIBE_INPUT_EVENT` 通知只提供 Hash、eType 和 Value；参数签名由 `SimConnect_EnumerateInputEventParams` 返回。Param 原样显示，Size 按签名逐项计算：`FLOAT64` 为 8 字节，`char[N]` 为 N 字节，多个参数取总和；无法识别的签名用 -1 表示未知。

参数类型：接口只收 `name / hash / eType / value / param / size` 这些普通字段，不收 SimConnect 结构体，这样 `DialogEnum` 不必依赖 SimConnect 头文件。

### 1.4 InputEvent 的 Inc / Dec 分支限制（旋钮绑定重点）

这是后续实现按键绑定旋钮时必须注意的边界条件。

#### 1.4.1 DevMode 中看到的层级不等于 SimConnect 枚举结果

实测 SF50 的开发者模式中，`SF50_AUTOPILOT_HEADING` 显示为一个父级 InputEvent，内部包含：

```text
SF50_AUTOPILOT_HEADING
├── SF50_AUTOPILOT_HEADING_Inc
├── SF50_AUTOPILOT_HEADING_Dec
├── SF50_AUTOPILOT_HEADING_Set
├── HEADING_BUG_INC
└── HEADING_BUG_DEC
```

其中 `HEADING_BUG_INC` 和 `HEADING_BUG_DEC` 位于父事件的 `[Inc]` / `[Dec]` 分支下，各自显示一个 `Number` 参数 `1.00`。这个 `1.00` 是执行该分支时传给行为逻辑的参数，不是 `SIMCONNECT_RECV_SUBSCRIBE_INPUT_EVENT::Value` 的左右方向值。

SimConnect 枚举和监听到的是父事件：

```text
Name:  SF50_AUTOPILOT_HEADING
Hash:  父事件 Hash
Value: 0
```

点击 DevMode 中的 `HEADING_BUG_INC` 或 `HEADING_BUG_DEC` 可以改变旋钮，但不会产生新的 Hash；监听回包中的 Value 仍可能是 0。这不是当前 `DOUBLE` 解析错误。官方 `SIMCONNECT_RECV_SUBSCRIBE_INPUT_EVENT` 的读取方式是把 `Value` 地址转换成 `double*` / 字符串指针；当前代码用 `memcpy` 从 `&inputEvent->Value` 读取，与官方方式等价。油门等事件能读出连续的 `0` 到 `100` 小数，也验证了这一点。

#### 1.4.2 当前 SimConnect API 不暴露 Inc / Dec 的独立 Hash

`SimConnect_EnumerateInputEvents` 只返回当前飞机对外暴露的父级 InputEvent 及其 Hash，不返回开发者模式中的 `_Inc`、`_Dec`、`_Toggle`、`_On`、`_Off` 等内部分支。`SimConnect_EnumerateInputEventParams` 和 `SimConnect_GetInputEvent` 都要求调用方已经拥有 Hash，不能按隐藏分支名称反查 Hash。即使自行对名称计算 CRC，也不能因此得到一个可供 `SetInputEvent` 使用的合法独立事件。

官方资料明确记录了这个限制：

- [Input Events API](https://docs.flightsimulator.com/msfs2024/html/6_Programming_APIs/SimConnect/API_Reference/InputEvents/Input_Events.htm)：`EnumerateInputEvents` 返回 InputEvent 及 Hash，`SetInputEvent` 设置指定父级 InputEvent 的值；
- [Asobo：EnumerateInputEvents 不返回 Inc / Dec](https://devsupport.flightsimulator.com/t/enumerateinputevents-doesnt-return-inc-dec-toggle-on-and-off-events/6598)：官方确认当前版本只暴露类似 `Set` 的事件，`Inc` / `Dec` 尚未通过该 API 暴露；
- [Asobo：Send B Events](https://devsupport.flightsimulator.com/t/send-b-events/8501)：官方确认当前 SimConnect 不直接暴露 `Inc`、`Dec` 或其他自定义 B Event 分支。

#### 1.4.3 对旋钮绑定的影响

当前 `Hash + Param + Value + Size` 方案可以直接支持按钮、开关、油门、推杆以及能够通过 Value 表达方向或位置的事件，但不能从 SF50 这种父级事件通知中推断旋钮左右方向。对同一个父 Hash，`Value=0` 不能区分 `Inc` 和 `Dec`。

后续发送层需要区分三种情况：

1. **数值型 Set 事件**：使用 `SimConnect_SetInputEvent(Hash, cbUnitSize, Value)`；
2. **传统 K / Sim Event**：如果 `HEADING_BUG_INC` / `HEADING_BUG_DEC` 实际是传统 Sim Event，可以按事件名称映射并使用 `SimConnect_TransmitClientEvent`，不需要 InputEvent Hash；
3. **B Event 的 Inc / Dec 分支**：需要通过 WASM / Gauge API 执行类似 `1 (>B:SF50_AUTOPILOT_HEADING_Inc)` 和 `1 (>B:SF50_AUTOPILOT_HEADING_Dec)` 的 RPN，当前纯 SimConnect 接口不能直接完成。

因此，旋钮绑定数据不能只保存父级 Hash，还需要保存 `Set / Inc / Dec` 操作类型以及对应的发送通道。当前 DialogEnum 的监听结果只能作为父级 InputEvent 的发现和参数分析工具，不能保证自动发现所有可执行的旋钮方向分支。

### 1.5 InputEvent 浮点发送测试

`pbtnTest` 只在收到 `FLIGHT_START` 后启用，点击后以模态方式打开 `DialogTest`。`DialogEnum` 顶部的 `btnTest` 也可以打开同一个 `DialogTest`，便于在枚举结果和监听数据旁直接测试 Hash；该按钮通过 `DialogEnum::testRequested` 请求 `Widget` 打开测试窗口，`DialogEnum` 不直接持有测试窗口对象。`DialogTest` 是 `Widget` 的值成员，但其 Qt 父窗口是 `DialogEnum`。通过 `DialogEnum` 打开时，`DialogTest` 使用非模态 `show()`，因此两个窗口可以同时操作；从主窗口 `pbtnTest` 打开时仍使用模态 `exec()`。关闭 `DialogEnum` 时会同时隐藏测试窗口；飞行结束、SimConnect 异常或连接丢失时两个窗口都会退出。

测试窗口控件：

- `lnHash`：只允许输入数字，最多 32 个字符。发送前检查不能为空，并转换为 `quint64`；空值或转换失败时弹出英文错误框。
- `sbValue`：待发送的浮点数。
- `btnGet`：根据 `lnHash` 异步读取 InputEvent 当前值；Hash 为空或转换失败时沿用英文错误提示。只有 `DOUBLE` 类型回包会更新 `sbValue`；如果返回类型是字符串，则弹出英文提示，不能获取字符串值。
- `btnSend`：发送当前 Hash 和浮点数。

发送请求通过 Qt 排队投递到 SDK 线程，在 `SimConnectClient` 中调用：

```cpp
SimConnect_SetInputEvent(handle, hash, sizeof(double), &value);
```

当前测试只支持单个浮点值，使用 `Hash + 8 字节 double`。独立测试调用不因 `GetInputEvent` / `SetInputEvent` 的直接返回错误弹出错误框或退出程序；调用失败只结束本次调用。配置规则执行时再由配置执行器根据该调用结果停止当前按键行为的后续规则。

读取请求通过 Qt 排队投递到 SDK 线程，在 `SimConnectClient` 中调用 `SimConnect_GetInputEvent`。该 API 是异步的，回包中的 `dwRequestID` 用于映射原始 Hash；字符串类型不会写入 `sbValue`。

### 1.6 无线电高度读取

`ckbRA` 勾选且收到确实的 `AircraftLoaded` 后，向 SDK 工作线程投递 `RADIO HEIGHT` 读取请求；如果 `AircraftLoaded` 先于 `FLIGHT_START` 到达，则先缓存该状态，待飞行状态成立后启动。相同的 `ckbRA` 状态还控制 RA 高度语音播报；`ckbLR` 使用独立的数据请求，不依赖无线电高度：

```cpp
SimConnect_AddToDataDefinition(handle, definitionId, "RADIO HEIGHT", "feet",
                               SIMCONNECT_DATATYPE_FLOAT64);
SimConnect_RequestDataOnSimObject(handle, requestId, definitionId,
                                  SIMCONNECT_OBJECT_ID_USER_AIRCRAFT,
                                  SIMCONNECT_PERIOD_SIM_FRAME, 0, 0, 5, 0);
```

数据回包解析后通过 Qt 信号更新主线程的 `lcdRA`。显示时不保留小数，以 5 英尺为显示分辨率并向上取整：例如 `2411` 显示为 `2415`；原始值不超过 `2500` 时正常显示，原始值超过 `2500` 时显示 `++++`。有效数值包括 `0`；只有没有有效回包、返回非有限值、请求失败或机型不支持该 SimVar 时显示 `----`。不会使用 `PLANE ALT ABOVE GROUND` 代替无线电高度。

取消勾选 `ckbRA`、飞行结束、飞机断线、SimConnect 异常、手动断开或程序正常关闭时，停止该请求并将 `lcdRA` 恢复为 `----`。如果机型没有可用的无线电高度实现，仅读取功能保持不可用，不影响其他 SimConnect 功能。

### 1.7 RA 高度语音播报

RA 语音播报与 `ckbRA` 共用启用条件：只有勾选 `ckbRA`、飞行有效且飞机已经加载时才工作。播报使用 SDK 返回的原始 `RADIO HEIGHT`，不使用 `lcdRA` 显示时按 5 英尺取整后的数值。

播报阈值为 `2500`、`1000`、`500`、`300`、`100`、`50`、`40`、`30`、`20`、`10` 英尺。只有 RA 从高到低穿过阈值时播报一次；起飞时 RA 从低到高经过阈值不会播报，保持在某个高度也不会循环播报。每个 WAV 使用独立的异步 `QSoundEffect`，因此下降过快时，所有后续穿过的高度都可以立即开始播放，任意多个高度之间都允许重叠，不会互相打断。

每个阈值使用 5 英尺滞回重新武装：播报后，RA 必须先上升到该阈值以上 5 英尺，下一次下降穿越时才允许再次播报。飞行开始、飞机重载、飞行结束、断线、异常、取消 `ckbRA` 或程序关闭时，停止正在播放的音频并清空所有阈值状态。

### 1.8 接地率读取

`ckbLR` 勾选且收到确实的 `AircraftLoaded` 后，向 SDK 工作线程投递独立的数据请求，读取几何离地高度 `PLANE ALT ABOVE GROUND`、标准 SimVar `PLANE TOUCHDOWN NORMAL VELOCITY`（单位 `feet per second`）和 `SIM ON GROUND`。接地率不通过普通垂直速度计算，而是使用模拟器记录的上一次接地法向速度，换算为 `feet per minute` 后更新 `lcdLR`。因此某些机型没有 `RADIO HEIGHT` 时，只要几何离地高度和接地相关 SimVar 可用，接地率仍可工作。

`100 ft` 只用于确认飞机已经起飞：几何离地高度首次达到该值时立即将 `lcdLR` 复位为 `-----`，不参与接地率数值计算。之后检测到 `SIM ON GROUND` 从空中状态变为地面状态时，直接读取模拟器提供的 `PLANE TOUCHDOWN NORMAL VELOCITY`，换算单位后以负号显示本次接地率；接地后的弹跳不会重复覆盖结果。下一次几何离地高度再次达到 `100 ft` 时，再次清空 `lcdLR` 并等待下一次接地。飞行开始、飞机重载、飞行结束、断线、异常、手动断开和程序关闭时，`lcdLR` 均恢复为 `-----`。

---

## 2. 输入监听

初始化完成后立即开始监听，无开关、无按钮。

### 2.1 总体要求

- 监听范围：本机**所有键盘**、**所有游戏控制器**，以及**鼠标的滚轮和侧键**（鼠标左右键、中键不监听）。
- 纯监听：不能影响原有输入行为，按键照常传给系统和游戏。
- 程序最小化、失去焦点、被其他窗口盖住时，监听必须照常工作。这是核心要求。

### 2.2 线程模型

- **输入线程**：键盘钩子、DirectInput 轮询、设备重枚举。只干监听，不放别的东西。
- **SDK 线程**：SimConnect 的连接、派发，以后的事件枚举与发送都放这里。
- **主线程**：只接收两个线程发来的信号、做界面显示。
- 为什么不把 SDK 和输入放一个线程：模拟器加载航班时 SimConnect 调用可能阻塞数秒，会把按键检测一起拖住（键盘钩子被拖住还可能被系统摘掉）。用定时器也解决不了，定时器只是换个时机阻塞，仍在同一个线程上。
- 输入线程将来要发送事件时，只把请求用信号投递给 SDK 线程，自己不等结果。
- **跨线程调用必须排队**：给另一个线程里的对象发请求，要用信号槽或 `QMetaObject::invokeMethod(..., Qt::QueuedConnection)`，不能直接调它的方法。直接调会在**调用者**线程里执行，函数里创建的子对象（`QTimer`、`QWinEventNotifier` 之类）会挂到错误的线程上——Qt 会报 `Cannot create children for a parent that is in a different thread`，而且那些对象之后会在错误的线程上工作（踩过一次：SDK 的重试和派发实际跑到了主线程）。
- **线程退出清理**：`InputListener`、`DirectInputListener` 和 `SimConnectClient` 都在所属工作线程中通过 `QThread::finished` 连接 `deleteLater()`，主线程不直接 `delete` 它们。关闭时先在 SDK 线程执行断开，再停止并等待线程，避免工作线程创建的 `QTimer` / `QWinEventNotifier` 被主线程析构。

### 2.3 键盘

- 方案：`SetWindowsHookEx(WH_KEYBOARD_LL, ...)` 低级键盘钩子。系统级生效，不依赖窗口焦点，后台可用。
- 被动：回调里无条件调用 `CallNextHookEx` 并把返回值原样返回，绝不返回非 0 去拦截。
- 回调里只做一件事：把按键交给界面层。绝不做耗时操作（Windows 对低级钩子有超时限制，超时会被系统静默摘掉钩子）。
- 程序退出时必须 `UnhookWindowsHookEx`。
- 按键标识用虚拟键码（VK）。
- 需要记录修饰键的按下/抬起状态，用于组合键显示。
- 键名来自固定的 VK → 名称表（常用键），未收录的用 `GetKeyNameText` 兜底。

### 2.4 lbDetail 显示规则

- 键盘显示按键本身的名字，不加前缀：按 B 显示 `B`，按 CTRL 显示 `CTRL`。
- 游戏控制器必须带设备名，否则不同外设的同号按键会显示成一个：例如 `T.16000M Button 3`、`TCA Q-Eng 1&2 Button 3`。lbDetail 上的字符串就是后续绑定的依据，不能有歧义。
- 支持组合键：按住修饰键再按其他键，显示成 `L CTRL+B` —— 修饰键在前、普通键在后，用 `+` 连接。
- 修饰键只包括键盘的 CTRL / SHIFT / ALT / WIN，**区分左右**：左 Ctrl 显示 `L CTRL`，右 Ctrl 显示 `R CTRL`，其余同理（`L SHIFT` / `R SHIFT` / `L ALT` / `R ALT` / `L WIN` / `R WIN`）。
- 游戏控制器按键是普通按键，不作为修饰键。
- 显示至少 10 秒：按下时启动 10 秒定时器，到点自动清空。留这么长是为了让用户有时间选中文本复制（复制出来的字符串就是后续配置表要用的绑定依据）。
- 10 秒内又有新按键按下：立即改显示新键，并重新计时。
- 长按产生的重复 keydown 只刷新计时，不改变显示内容。
- 不改变字体。

### 2.5 游戏控制器（DirectInput8）

- 只监听按键和 POV 帽，不监听轴。
- 显示必须带设备身份：`设备名 + 控件名`，例如 `T.16000M Button 3`、`T.16000M POV 0 Up`。因为不同设备的"Button 3"完全不同，而 lbDetail 上的字符串就是绑定依据，不区分就会出现"两个外设的按键触发同一个操作"。
- 设备名取 DirectInput 的 `DIDEVICEINSTANCE.tszInstanceName`（本机是 `T.16000M`、`TCA Q-Eng 1&2`）。如果同时接了两台同名设备，追加序号区分：`T.16000M #1 Button 3`、`T.16000M #2 Button 3`。
- 内部表示同样带设备身份 —— 不能用设备路径（换一个 USB 口路径就变），要用 VID/PID 加产品名。内部键按"来源 : 设备 : 控件"三段统一（**设计约定，目前还没落到代码里**，现在只有显示字符串）：

| 来源 | deviceKey | controlKey | 完整键示例 |
| --- | --- | --- | --- |
| 键盘 | `kbd` | VK 名 | `kbd:VK_B` |
| DirectInput | 产品标识（VID/PID） | 按钮序号 | `di:044F-B10A:BTN_3` |

通道已定：**DirectInput8**。因为实际接入的是图马斯特 T16000M 摇杆 + TCA 空客油门，这类设备 XInput 看不到，必须走 DirectInput。

已实测（本机接 T16000M + TCA Q-Eng 1&2）：

- 两台设备都能被 `EnumDevices(DI8DEVCLASS_GAMECTRL, ..., DIEDFL_ATTACHEDONLY)` 枚举到，`SetDataFormat(&c_dfDIJoystick2)` + `DISCL_BACKGROUND | DISCL_NONEXCLUSIVE` + `Acquire` + `GetDeviceState` 全部返回成功，也就是**后台非独占可读**。
- 设备名：`T.16000M`、`TCA Q-Eng 1&2`。
- **按钮名不用 DirectInput 返回的名字**：它是本地化的（本机返回 `按钮 0`、`按钮 1`……）。统一生成英文 `Button 0`、`Button 1`……，因为后续绑定以英文为准。
- **坑**：`EnumObjects` 返回的 `dwOfs` 不是 `DIJOYSTATE2` 里的下标（实测 T16000M 是 24..39、TCA 是 40..70），不能拿来索引。实测状态数据本身是标准 `DIJOYSTATE2` 布局（POV 在字节 32、静止值 `0xFFFFFFFF`，轴在 0..23），所以按钮要按**枚举顺序**索引 `rgbButtons[0..N-1]`。

通用性：按标准游戏控制器类别（`DI8DEVCLASS_GAMECTRL`）枚举，摇杆、手柄、油门、方向盘、脚舵都在内，没有任何针对特定型号的代码。上限是 `DIJOYSTATE2` 格式的 128 个按钮；不以游戏控制器身份出现的纯 HID 按钮盒不在这一类（若它模拟键盘输入，则由键盘钩子覆盖）。

热插拔：**不做定时重枚举**。`EnumDevices` 一次要 170 ms（实测），定时跑纯属浪费且会拖住同线程的键盘钩子。改成主窗口收到 `WM_DEVICECHANGE` 时通知工作线程重枚举，并且 500 ms 防抖（设备插拔会连着来好几个广播）。启动时枚举一次。

只监听不拦截：协作级别用 `DISCL_BACKGROUND | DISCL_NONEXCLUSIVE`，设备是共享的，本程序只调用 `GetDeviceState` 读取自己那一份数据，从不独占获取、不写设备属性、不设置力反馈，所以游戏那边的输入完全不受影响。

POV 帽：枚举 `DIDFT_POV` 对象，按序号读 `rgdwPOV[i]`，把 1/100 度的角度量化成 8 个方向（0=Up、4500=Up-Right、9000=Right、13500=Down-Right、18000=Down、22500=Down-Left、27000=Left、31500=Up-Left），居中值 `0xFFFFFFFF` 视为松开。方向发生变化时发一次事件，命名 `POV 0 Up`、`POV 0 Up-Right` 这样（带序号是因为可能有多个帽）。4 向帽不会报对角线值，同一套量化逻辑直接通用。实测 T16000M 报 1 个 POV（`帽状开关`，usage 0x39），TCA 油门没有 POV。

坑：`EnumObjects` 的过滤参数必须写成 `DIDFT_BUTTON | DIDFT_POV`。只传 `DIDFT_BUTTON` 时 POV 对象不会进回调，看上去就像"这个设备没有帽"。

### 2.6 鼠标滚轮与侧键

- 只监听四样：滚轮上、滚轮下、侧键 1（后退）、侧键 2（前进）。左右键和中键不监听。
- 显示名：`Mouse Wheel Up`、`Mouse Wheel Down`、`Mouse Side 1`、`Mouse Side 2`。
- 通道：DirectInput 鼠标（`GUID_SysMouse` + `c_dfDIMouse2` + `DISCL_BACKGROUND | DISCL_NONEXCLUSIVE`），复用现有的 10 ms 轮询，不需要枚举设备。
- 不用鼠标低级钩子（`WH_MOUSE_LL`）的原因：它会把全系统鼠标输入串到本程序的线程上，而工作线程在设备插拔时会卡 170 ms（实测），那样会造成全机鼠标卡顿。键盘能承受这个代价，鼠标不行。
- 不用 RawInput 的原因：RawInput 会把每一次鼠标移动都作为 `WM_INPUT` 送进来（高刷新率鼠标每秒上千次），白白增加主线程负载。DirectInput 轮询只在需要时读状态，没有这个问题。
- 滚轮读的是**相对量**：每次读取拿到的是"上次读取之后累计的滚动量"，正值向上。所以一次轮询最多发一次事件，连续滚动会合并成一次，不会刷出一串重复。
- 支持组合键：滚轮或侧键事件发生时，先查一遍修饰键的**物理状态**（`GetAsyncKeyState`），按住哪个就加哪个前缀，例如 `L CTRL+Mouse Wheel Up`、`R SHIFT+Mouse Side 1`。修饰键集合和顺序与 2.4 一致（CTRL / SHIFT / ALT / WIN，区分左右）。用 `GetAsyncKeyState` 而不是和键盘监听器共享状态，是为了让两个监听器互不依赖——鼠标在事件发生的那一刻直接问系统要真实状态，最省事也最稳。
- 已知风险：如果游戏独占鼠标，可能读不到，需要实机确认；真读不到的话退路是 RawInput（`RIDEV_INPUTSINK`）或鼠标低级钩子。

轮询频率：实测 Qt 定时器在 Windows 上无需调整系统计时精度即可跑准 —— 16 ms 实测 16.0 ms、4 ms 实测 4.0 ms、1 ms（配合 `timeBeginPeriod(1)`）实测 1.0 ms。另实测 `SetEventNotification` 在本机这些设备上返回 `0x800700AA`（ERROR_BUSY），事件驱动不可用，只能轮询。

当前取值 **10 ms（100 Hz）**。空闲 CPU 实测（单核占比，6 秒采样）：不轮询时约 1.6%、16 ms 约 2.6%、10 ms 约 2.3%、4 ms 约 3.4%。受采样精度限制，16 ms 与 10 ms 两个数字的差别在噪声范围内，4 ms 是唯一明显更高的一档。1 ms 需要 `timeBeginPeriod(1)`、会抬高全系统计时精度，不采用。

---

## 3. 控件互斥保护

规则：任何可点击控件的槽函数执行期间，主窗口里**所有**可点击控件都处于失能状态，包括触发它的那个；槽函数退出时按进入前的状态还原。

- 还原成"进入前的状态"，不是一律恢复可用。进入前就是失能的（例如启动时 `cbConfig`、`pbtnEnum` 本来就是失能），退出后仍然失能。
- 槽函数执行期间被改动过使能状态的控件不再还原，保留改动结果（例如连接成功后把 `pbtnEnum` 打开，退出后它就是可用的）。
- 范围：主窗口内的 `QAbstractButton` 和 `QComboBox`，即 `pbtnConnect`、`pbtnEnum`、`pbtnFolder`、`cbConfig`、`ckbAuto`、`ckbRA`、`ckbLR`。实现上遍历子控件自动收集，以后新增控件自动生效。`lbInfo`、`lbDetail`、`lbPage`、`lcdRA`、`lcdLR` 不算（不可点击）；`DialogEnum` 里的控件不算（独立窗口，打开时是模态，主窗口点不到）。
- 用法：每个槽函数第一行写 `ControlGuard guard(this);`，构造时快照并失能，析构时还原，所以中途 `return` 也会正确恢复。
- 槽函数之间不会互相调用，因此不需要嵌套计数。
- 已知边界：槽函数"主动保持失能"（控件本来可用、执行期间要求它保持失能）检测不到——Qt 的 `setEnabled` 在状态没变化时不产生任何事件。真需要这种语义得在槽里显式声明。

---

## 4. 连接按钮（pbtnConnect）

用成员变量 `connected` 判断状态，初始为 `false`（程序刚打开就是未连接）。

未连接状态下点击：

- `connected` 置为 `true`；
- `cbConfig`、`ckbAuto` 保持失能（不是只在槽函数执行期间失能，而是之后都不恢复）；
- `lbInfo` 显示 `Waiting MFS...`；
- 按钮文字变为 `Disconn`；
- 按钮图标变为 `:/Resoure/CoilRed.png`，图标尺寸不变（仍是 18x18）。

已连接状态下点击：

- `connected` 置回 `false`；
- `ckbAuto` 恢复可用（它是连接时被失能的，断开要还原；直接 `setEnabled(true)`，守卫会识别为"槽函数改过"从而保留）；
- `cbConfig` 保持失能（启动时它就是失能，连接时只是又被失能一次，断开后维持原状）；
- `lbInfo` 恢复 `Standby...`；
- 按钮文字恢复 `Connect`，图标恢复 `:/Resoure/CoilBalck.png`。

因此连一次再断一次，界面应回到与启动时完全一致的状态。

调用方式：连接与断开**不是直接调** `simClient` 的方法，而是通过 `QMetaObject::invokeMethod(..., Qt::QueuedConnection)` 投递到 SDK 线程执行（原因见 2.2）。SDK 层本身见第 5 节。

实现注意：`cbConfig`、`ckbAuto` 的失能是用 `ControlGuard::keepDisabled()` 声明的，不能只在槽里调 `setEnabled(false)`。原因是槽函数执行期间守卫已经把这两个控件置为失能了，槽里再调一次是空操作、检测不到，守卫退出时会按进入前的状态把它们恢复回来（`ckbAuto` 本来可用，就会被错误地恢复成可用）。`keepDisabled()` 就是明确告诉守卫"这个控件退出时保持失能"。

---

## 5. SimConnect 客户端

类 `SimConnectClient`，跑在**自己的 SDK 线程**里（与输入线程分开，见 2.2）。以后的枚举、飞机识别、事件发送都放这个线程。

飞行状态使用 MSFS 2024 的 Flow Event API，不使用 `Sim` / `SimStart` / `SimStop` 推断是否处于真正飞行。连接成功后调用 `SimConnect_SubscribeToFlowEvent`，在 `SIMCONNECT_RECV_ID_FLOW_EVENT` 中处理 `SIMCONNECT_FLOW_EVENT`。参考：[SimConnect_SubscribeToFlowEvent](https://docs.flightsimulator.com/msfs2024/html/6_Programming_APIs/SimConnect/API_Reference/Events_And_Data/SimConnect_SubscribeToFlowEvent.htm)、[SIMCONNECT_FLOW_EVENT](https://docs.flightsimulator.com/msfs2024/html/6_Programming_APIs/SimConnect/API_Reference/Structures_And_Enumerations/SIMCONNECT_FLOW_EVENT.htm)、[SIMCONNECT_RECV_FLOW_EVENT](https://docs.flightsimulator.com/msfs2024/html/6_Programming_APIs/SimConnect/API_Reference/Structures_And_Enumerations/SIMCONNECT_RECV_FLOW_EVENT.htm)。

连接：

- `SimConnect_Open(&h, "Key2Airport", nullptr, 0, hEvent, SIMCONNECT_OPEN_CONFIGINDEX_LOCAL)`
- 不传窗口句柄，改用**事件句柄**：连接成功后模拟器会在这个事件上发信号，SDK 线程用 `QWinEventNotifier` 接住，再调 `SimConnect_CallDispatch` 派发。这样整条链路都在 SDK 线程里，不依赖主线程的窗口消息。
- `ConfigIndex` 用 `SIMCONNECT_OPEN_CONFIGINDEX_LOCAL`，强制连本机，避免被遗留的 `SimConnect.cfg` 指到别的机器上去。
- 模拟器没启动时 `SimConnect_Open` 直接返回失败，用 1 秒的定时器重试直到成功。这段等待期界面就停在 `Waiting MFS...`。
- 模拟器退出时收到 `SIMCONNECT_RECV_ID_QUIT`：关闭当前句柄但**保持重试**，自动重连（详见 5.1）。
- 手动断开：`SimConnect_Close` + 关闭事件句柄，界面复位回 `Standby...`。

部署：

- exe 运行时必须能加载 `SimConnect.dll`，否则进程直接起不来。
- `.pro` 里加了 `QMAKE_POST_LINK`，链接完成后自动把 SDK 里的 `SimConnect.dll` 拷到产物目录（`$(DESTDIR)`）。

### 5.1 状态通知

连接和飞行状态的判断分为两层：Flow Event 判断是否真正进入/退出飞行，`AircraftLoaded` 只作为飞行中的飞机加载通知，不能单独驱动界面状态。

| 状态 | SimConnect 机制 | 本工程的信号 |
| --- | --- | --- |
| 已连接 | `SimConnect_Open` 成功 | `connected()` |
| 真正飞行开始 | `SimConnect_SubscribeToFlowEvent`，收到 `SIMCONNECT_FLOW_EVENT_FLIGHT_START` | `flightStarted()`，界面显示 `Aircraft Loaded` |
| 飞行中飞机加载 | `AircraftLoaded`，回包是 `SIMCONNECT_RECV_ID_EVENT_FILENAME`，带 `szFileName`；仅在已进入飞行后转发 | `aircraftLoaded(QString)` |
| 飞行结束 | Flow Event 的 `SIMCONNECT_FLOW_EVENT_FLIGHT_END` 或 `SIMCONNECT_FLOW_EVENT_BACK_TO_MAIN_MENU` | `flightEnded()`，界面回到 `Connected` |
| 异常 / 错误退出 | `SIMCONNECT_RECV_ID_EXCEPTION`（`dwException` 是错误码）；连接被结束是 `SIMCONNECT_RECV_ID_QUIT` | `simError(quint32)`，主线程弹出英文错误框；用户确认后自动断开 |

连接成功后会立刻订阅 `AircraftLoaded` 和 Flow Event。主菜单预览产生的 `AircraftLoaded` 会被忽略；收到 `FLIGHT_START` 后才把界面切到 `Aircraft Loaded`。如果 `AircraftLoaded` 与 `FLIGHT_START` 的先后顺序发生变化，飞行开始事件仍然作为界面状态的最终判据。

**连接时机边界（当前功能限制）**：`FLIGHT_START` / `FLIGHT_END` 是发生时主动推送的事件，不是可以随时查询的持久状态。SimConnect 不会因为客户端晚加入而补发已经发生的 Flow Event。因此，如果程序在 `FLIGHT_START` 发生后才点击连接，客户端无法收到本次进入飞行的通知，`flightActive` 会保持为 `false`，界面会显示 `Connected`，而不是 `Aircraft Loaded`。`SimConnect_RequestSystemState("Sim")`、`AircraftLoaded` 和 `FlightLoaded` 等主动查询结果无法稳定区分真正飞行与主菜单预览，所以当前不将它们作为权威判据。当前功能范围是：应在进入飞行前连接；连接期间可以准确接收后续的进入和退出飞行事件。中途连接场景暂不处理。

`FlightLoaded`、`FLT_LOAD`、`FLT_LOADED` 只表示 `.flt` 文件加载，不表示飞行状态；生涯模式的跳过、传送和剧情流程也不能当作退出飞行。"连不上"这个状态不需要单独通知，界面停在 `Waiting MFS...` 就是在重试。

**实测结论（2026-09-22，事件探针）**：主菜单自己也在跑一个模拟，并且会加载一次预览飞机。典型序列：

```
SimStop=0, Sim=0                     ← 模拟停止
FlightLoaded  ...\CustomFlight\CustomFlight.FLT
SimStart=0, Sim=1                    ← 菜单里的预览模拟启动了
AircraftLoaded ...\asobo_cj4\...\aircraft.cfg     ← 预览飞机加载
```

由此可知：

- **`Sim` 不能用来判断"在飞行中"**——退出飞行 0.8 秒后它会因为菜单预览又变回 1；
- **`AircraftLoaded` 在主菜单也会发**（刚进游戏、还没进正式飞行就会发一次）；
- 官方新增的 Flow Event 提供了专门的 `FLIGHT_START`、`FLIGHT_END` 和 `BACK_TO_MAIN_MENU` 边界事件。

当前实现采用 Flow Event 作为飞行状态边界：

- `FLIGHT_START` → 界面 `Aircraft Loaded`；
- `FLIGHT_END` / `BACK_TO_MAIN_MENU` → 界面 `Connected`；
- 飞行外收到的 `AircraftLoaded` 不再更新界面。

`SimStart` / `SimStop` 仍可能在加载、重置等过程中成对出现，因此不作为飞行状态判据。

重连策略：

| 情况 | 行为 |
| --- | --- |
| `SimConnect_Open` 返回 `E_FAIL`（模拟器没运行） | 正常，1 秒后继续重试，界面停在 `Waiting MFS...` |
| `SimConnect_Open` 返回其他错误（异常情况） | 停止重试，界面复位回 `Standby...` 并显示错误 |
| `SIMCONNECT_RECV_ID_QUIT`（模拟器退出、连接断开） | **自动重连**：关闭当前枚举窗口和句柄，但保持重试，界面停在 `Waiting MFS...`（按钮仍是 `Disconn`），模拟器重新起来后自动连上，并自动重新订阅飞机 / 飞行事件 |
| `SIMCONNECT_RECV_ID_EXCEPTION` | SDK 线程先把错误码报给界面，不立即断开；用户确认错误框后由主线程执行断开操作 |
| 用户手点 `Disconn` | 停止重试，界面复位回 `Standby...` |

只有两种情况会停止重试：用户手动断开，以及 `SimConnect_Open` 返回非 `E_FAIL` 的异常错误。

还没接、但以后可能用得上的系统事件：`SimStart` / `SimStop`（已在探针里观察过，成对触发）、`Pause` / `Pause_EX1`（暂停 / 恢复，`dwData` 含义未整理）。

错误处理：`onSimError` 弹出标题为 `SimConnect Error` 的英文错误框，显示错误描述以及十进制 / 十六进制错误码；用户点击确定后执行与再次点击 `pbtnConnect` 相同的断开操作。`onSimError` 不修改 `lbInfo`，错误状态不显示在主界面状态文本中。

`aircraftLoaded` 信号里带的是 `szFileName` 原始字符串（飞机配置文件的路径）。要显示成机型号而不是路径的话，得先看这个字符串实际长什么样，或者改成读 `TITLE` / `ATC MODEL` 这类 SimVar。

重试间隔现在固定 1 秒，也可以调。

---

## 6. 待补充

- `pbtnFolder` 的行为（初始状态、点击后做什么）
- 飞机识别：现在只有 `AircraftLoaded` 的文件路径，要显示机型 / 做按机型匹配，得读 `TITLE`、`ATC MODEL` 这类 SimVar
- 正式事件发送流程（按键 → 匹配 → `SetInputEvent`）；当前仅有第 1.5 节的浮点测试接口，旋钮的 `Inc / Dec` 分支受第 1.4 节限制，不能假定所有事件都能通过父 Hash 发送
- 配置文件的读写与按机型切换
- 界面文案定稿：`Connected` / `Aircraft Loaded` / `Sim Error` 目前都是占位

---

## 7. 已知问题

### 7.1 主菜单被当成"飞机已加载"

- **原因**：主菜单自身也在跑模拟并加载预览飞机，因此会发 `AircraftLoaded`。官方文档也说明 `AircraftLoaded` 只是飞机飞行动力学文件变化通知，不是“飞行已开始”通知。
- **处理方式**：改用 MSFS 2024 Flow Event：`FLIGHT_START` 进入飞行状态，`FLIGHT_END` 或 `BACK_TO_MAIN_MENU` 退出飞行状态；只有真正进入飞行后，`AircraftLoaded` 才会继续转发。
- **当前状态**：已在代码中处理。`Sim`、`SimStart`、`SimStop`、`FlightLoaded` 均不再作为飞行状态判据。

---

## 8. 变更记录

| 日期 | 版本 | 变更 |
| --- | --- | --- |
| 2026-09-21 | v0.1 | 初稿：程序启动初始化、两张表的列定义、TWListem 行插入规则 |
| 2026-09-21 | v0.2 | lcdRA / lcdLR 初始显示改为 `----`；明确复制为单元格级、无需额外代码；`label_3` 移出待补充 |
| 2026-09-21 | v0.3 | 新增 1.3 DialogEnum 对外接口：TWEnumAll 尾部追加、TWListem 头部插入、Param/Size 句柄补填 |
| 2026-09-21 | v0.4 | 接口简化为两个：`addListen` 一次写入 5 列，去掉句柄与补填接口 |
| 2026-09-21 | v0.5 | `lbDetail` 加入初始化清单；`lcdLR` 占位符改为 5 位；记录 `pbtnConnect` 的临时调试入口与 slot 声明约定 |
| 2026-09-21 | v0.6 | 列宽模式由 `ResizeToContents` 改为 `Stretch`，随窗口宽度自动分配 |
| 2026-09-21 | v0.7 | 新增第 2 节输入监听：总体要求、键盘钩子、lbDetail 显示规则、游戏控制器表示法建议 |
| 2026-09-21 | v0.8 | lbDetail 改为显示具体按键名并支持组合键；控制器只监听按键、不加显示前缀 |
| 2026-09-21 | v0.9 | 修饰键区分左右（`L CTRL`）；控制器通道定为 DirectInput8，记录实际设备 |
| 2026-09-21 | v1.0 | DirectInput 实测结论：设备枚举/后台读取成功、按钮名照用、dwOfs 不可作索引、轮询与重枚举策略 |
| 2026-09-21 | v1.1 | 按钮名改英文 `Button N`；补充通用性、只监听不拦截、轮询频率实测（4 ms） |
| 2026-09-21 | v1.2 | 轮询频率由 4 ms 调整为 10 ms（100 Hz），补充各档空闲 CPU 实测 |
| 2026-09-21 | v1.3 | 增加 POV 帽支持：8 方向量化、`POV N 方向` 命名；轴仍不监听 |
| 2026-09-21 | v1.4 | 修正 `EnumObjects` 过滤参数（`DIDFT_BUTTON \| DIDFT_POV`），否则 POV 对象不会被枚举 |
| 2026-09-21 | v1.5 | 监听整体移入工作线程（主线程只收信号显示）；取消定时重枚举，改为 `WM_DEVICECHANGE` 触发 + 500 ms 防抖 |
| 2026-09-21 | v1.6 | 新增第 3 节控件互斥保护：槽函数执行期间失能全部可点击控件，退出时按进入前状态还原 |
| 2026-09-21 | v1.7 | 新增第 4 节连接按钮状态；补充 `ControlGuard::keepDisabled()` 及其必要性 |
| 2026-09-21 | v1.8 | 控制器显示改为"设备名 + 控件名"，同名设备追加 `#1`/`#2`，避免两台外设的同号按键显示成一个 |
| 2026-09-21 | v1.9 | lbDetail 显示时长 1 秒改为 10 秒；lbDetail 设为可选中复制 |
| 2026-09-21 | v2.0 | 新增 2.6 鼠标滚轮与侧键（DirectInput 鼠标轮询）；2.1 监听范围同步更新 |
| 2026-09-21 | v2.1 | 鼠标滚轮/侧键支持修饰键组合（`L CTRL+Mouse Wheel Up` 这类） |
| 2026-09-21 | v2.2 | 补全 pbtnConnect 的断开分支：状态复位、`ckbAuto` 恢复可用、文案与图标还原 |
| 2026-09-21 | v2.3 | 新增第 5 节 SimConnect 客户端：连接/重试/退出检测/断开、事件句柄派发、DLL 部署 |
| 2026-09-21 | v2.5 | 第 5 节补充 5.1 状态通知：已连接 / 已加载飞机 / 异常三种状态的 SimConnect 机制与对应信号 |
| 2026-09-21 | v2.6 | 2.2 线程模型改为双工作线程；补充"跨线程调用必须排队"的规则 |
| 2026-09-21 | v2.7 | 5.1 补充重连策略：模拟器退出后自动重连，只有手动断开或异常错误才停止 |
| 2026-09-21 | v2.8 | 5.1 增加"退出飞行"状态：订阅系统事件 `Sim`，回 0 时界面从 `Aircraft Loaded` 回到 `Connected` |
| 2026-09-22 | v2.9 | 与代码逐条核对并修正：枚举窗口入口已失效、`lbInfo`/`pbtnEnum` 初始状态说明、SDK 独立线程、自动重连、`Sim` 已接入、待补充清单重写 |
| 2026-09-22 | v3.0 | 用事件探针实测更正"退出飞行"判定：菜单自身也会跑模拟并重发 `AircraftLoaded`，改用 `FlightLoaded`(CustomFlight) 识别菜单、`Sim`=0 清除菜单模式；信号 `simRunningChanged` 改为 `flightEnded` |
| 2026-09-22 | v3.1 | 撤回 v3.0 里基于 `FlightLoaded` 的菜单判据（依据未确认，猜反会破坏"进入飞行"）；新增第 7 节记录未解决的"主菜单被当成飞机已加载"问题与验证方案 |
| 2026-09-22 | v3.2 | 接入 MSFS 2024 Flow Event：使用 `FLIGHT_START`、`FLIGHT_END`、`BACK_TO_MAIN_MENU` 判断真正飞行边界；主菜单的 `AircraftLoaded` 不再触发 `Aircraft Loaded` |
| 2026-09-22 | v3.3 | 增加 `lbInfo` 状态颜色；`onSimError` 改为英文错误框并显示错误码，确认后自动断开，不再写入 `lbInfo` |
| 2026-09-22 | v3.4 | 明确连接时机边界：Flow Event 不会补发，飞行开始后才连接时无法恢复当前飞行状态；中途连接暂不处理 |
| 2026-09-22 | v3.5 | `FLIGHT_START` 后使能 `pbtnEnum`；点击后枚举全部输入事件并填入 `TWEnumAll`；飞行结束、断开或异常时失能 |
| 2026-09-22 | v3.6 | `DialogEnum` 缓存 `TWEnumAll` 的枚举结果；新增 `leFiltra` 实时按 Name 过滤（忽略大小写），并限制为最多 20 个非空白可打印 ASCII 字符 |
| 2026-09-22 | v3.7 | 打开 `DialogEnum` 时按枚举结果订阅全部输入事件；通知填入 `TWListem` 第 0 行，关闭窗口后取消订阅 |
| 2026-09-22 | v3.8 | 为每个 Hash 请求并解析输入事件参数签名；按 `FLOAT64` / `char[N]` 计算 Size，并异步回填 `TWListem` 的 Param / Size |
| 2026-09-22 | v3.9 | 修复工作线程对象由主线程析构导致的 Qt 定时器跨线程清理警告；改为所属线程 `deleteLater()` 并在退出前执行 SDK 断开 |
| 2026-09-22 | v3.10 | `TWListem` 按 Hash 去重：新 Hash 插入第 0 行，相同 Hash 后续只更新原行 |
| 2026-09-22 | v3.11 | 主窗口和 `DialogEnum` 默认置顶；`TWListem` 新增 `Time` 列，显示每个 Hash 最新通知时间（精确到秒） |
| 2026-09-22 | v3.12 | `TWListem` 按 Hash 通知频率自适应折叠：1 秒内达到 8 条时折叠，降到 4 条或更少时恢复逐条插入 |
| 2026-09-22 | v3.13 | `FLIGHT_END` 或 `onSimError` 发生时主动关闭 `DialogEnum`，并沿用模态返回后的流程取消输入事件监听 |
| 2026-09-22 | v3.14 | 记录 SF50 旋钮限制：父级 InputEvent 的 Hash / Value 不包含 `Inc` / `Dec` 方向，当前 SimConnect 无法枚举或反查这些内部分支 Hash |
| 2026-09-22 | v3.15 | 修复模拟器连接丢失时未关闭 `DialogEnum` 的问题；连接丢失会主动关闭枚举窗口，并沿用模态返回后的流程取消输入事件监听 |
| 2026-09-22 | v3.16 | 新增 `DialogTest` 浮点 InputEvent 测试：Hash 数字校验、SDK 线程发送，以及与 `DialogEnum` 一致的飞行状态和异常退出逻辑 |
| 2026-09-22 | v3.17 | `DialogEnum` 新增 `btnTest`，通过主窗口打开共享的 `DialogTest`；支持嵌套模态返回，并沿用飞行结束、异常和连接丢失时的双窗口退出逻辑 |
| 2026-09-22 | v3.18 | `DialogTest` 新增 `btnGet` 浮点 InputEvent 读取；字符串类型弹出英文提示；从 `DialogEnum` 打开测试窗口时改为非模态，支持两个窗口同时操作 |
| 2026-09-22 | v3.19 | 新增 `RADIO HEIGHT` 周期读取：在 SDK 工作线程读取并更新 `lcdRA`；无有效数据或机型不支持时显示 `----`，退出和状态切换时停止请求并复位 |
| 2026-09-22 | v3.20 | 修复 `FLIGHT_START` 与 `AircraftLoaded` 到达顺序不固定导致无线电高度请求未启动的问题；以 `FLIGHT_START` 作为初始启动点，并保留 `AircraftLoaded` 触发 |
| 2026-09-22 | v3.21 | 调整无线电高度显示：隐藏小数、按 5 英尺向上取整，超过 2500 英尺显示 `++++` |
| 2026-09-22 | v3.22 | 新增接地率读取：使用几何离地高度确认起飞，接地时读取 `PLANE TOUCHDOWN NORMAL VELOCITY`，再次起飞后清空旧结果并重新记录 |
| 2026-09-22 | v3.23 | 接地率与无线电高度均改为必须在对应 `ckb` 勾选且收到 `AircraftLoaded` 后才启动；接地率不依赖无线电高度 |
| 2026-09-22 | v3.24 | 接地率改用 `PLANE ALT ABOVE GROUND` 判断离地，不再依赖 `RADIO HEIGHT`；RA 与接地率改为独立数据请求 |
| 2026-09-22 | v3.25 | 明确 `100 ft` 仅用于确认起飞并复位 `lcdLR`；接地率直接读取 `PLANE TOUCHDOWN NORMAL VELOCITY`，不使用 100 ft 参与数值计算 |
| 2026-09-22 | v3.26 | 接地率显示统一增加负号；兼容 SimVar 返回正负号方向差异，避免显示双负号 |
| 2026-09-22 | v3.27 | 新增 RA 高度异步语音播报；使用原始 `RADIO HEIGHT` 下降穿越阈值触发，起飞不播报，各高度使用独立音效允许重叠播放 |
| 2026-09-22 | v3.28 | 明确所有 RA 播报阈值均使用独立音效，下降过快时任意多个高度播报都允许同时重叠 |
| 2026-09-23 | v3.29 | `onSimDisconnected` 主动关闭 `DialogEnum`，断开时与测试窗口一起退出 |
