# BMS UI 解耦方案

## 1. 现状分析

### 1.1 当前架构

`bms_ui.c`（1621 行）是一个单体文件，包含以下三类逻辑：

| 层次 | 职责 | 行数（约） | 依赖 |
|------|------|-----------|------|
| 测试/模拟逻辑 | 电池物理模型、CCCV 充电、CC 放电、主机通信模拟 | ~300 行 | LVGL 定时器、`lv_tick_get()` |
| 渲染/UI 逻辑 | 控件创建、样式定义、页面布局、图表更新 | ~800 行 | LVGL 控件 API |
| 程序逻辑 | 事件回调、按键处理、焦点管理、页面切换 | ~500 行 | LVGL 事件系统 |

### 1.2 耦合点分析

**模拟 → UI（直接写入控件）：**
- `bms_sim_tick` 直接调用 `lv_label_set_text()` 更新 Page 0 标签
- `bms_sim_tick` 直接调用 `lv_chart_set_next_value()` 更新 Page 1/2 图表
- `bms_sim_tick` 直接修改 `footer_circle` 颜色/透明度
- `bms_sim_tick` 在自动停止时直接修改 toggle 按钮的标签和样式
- `host_comm_tick` 直接写入 `lbl_p4_terminal` 日志文本

**UI → 模拟（事件回调直接修改模拟变量）：**
- `widget_click_handler` 直接修改 `charge_active`、`discharge_active`
- `widget_key_handler` 直接修改 `target_u_set`、`target_i_set`、`target_i_dis`

**模拟读取 UI 状态：**
- `bms_sim_tick` 检查 `current_page` 决定更新哪些控件
- `bms_sim_tick` 检查 `btn_p2_toggle` 指针来更新按钮标签
- 事件回调检查 `charge_active`/`discharge_active` 来锁定焦点

**核心问题：** 模拟逻辑需要知道 LVGL 控件指针，UI 逻辑需要知道模拟变量，两者双向直接引用，无法独立编译或测试。

---

## 2. 目标架构：Model-View-Controller

### 2.1 分层概览

```
┌─────────────────────────────────────────────────┐
│                   main.c                         │
│         lv_init() → bms_model_init()             │
│         bms_ui_init() → while(1) loop            │
└────────────┬────────────────┬────────────────────┘
             │                │
     ┌───────▼───────┐  ┌────▼────────────┐
     │  bms_model.c  │  │   bms_ui.c      │
     │  (Model)      │  │   (View)        │
     │               │  │                 │
     │  电池物理     │  │  LVGL 控件      │
     │  CCCV 充电    │  │  样式/布局      │
     │  CC 放电      │  │  图表/标签      │
     │  主机通信     │  │  页面切换       │
     │               │  │                 │
     │  纯 C 数据    │  │  只读 Model     │
     │  无 LVGL 依赖 │  │  通过 struct    │
     └───────▲───────┘  └────▲────────────┘
             │                │
     ┌───────┴───────────────┴────────────┐
     │           bms_input.c              │
     │           (Controller)             │
     │                                    │
     │  LVGL 事件回调                     │
     │  按键/点击 → 修改 Model            │
     │  焦点管理                           │
     │  编辑模式状态机                     │
     └────────────────────────────────────┘
```

### 2.2 各层职责

**Model（`bms_model.h/c`）— 纯数据 + 计算，零 LVGL 依赖：**
- 电池物理状态（SOC、OCV、电压、电流、温度、内阻）
- 充放电设置（目标电压、目标电流、使能标志）
- 系统设置（波特率、端口）
- 主机通信状态（在线状态、包计数、预测 SOC）
- 两个 tick 函数：`bms_model_tick_200ms()` 和 `bms_model_tick_1000ms()`
- 导出一个只读数据结构供 View 读取

**View（`bms_ui.c`）— LVGL 控件，只读访问 Model：**
- 控件创建（`create_page_*`、`create_global_header_footer`）
- 样式定义（`style_*`）
- 一个 UI 刷新函数 `bms_ui_refresh()`，从 Model 读取数据更新控件
- 不直接修改任何 Model 变量

**Controller（`bms_input.c`）— 事件处理，连接 Model 和 View：**
- LVGL 事件回调（`widget_click_handler`、`widget_key_handler` 等）
- 接收用户输入 → 调用 Model 的 setter 函数
- 管理页面切换、焦点导航、编辑模式状态机
- 不直接操作 LVGL 控件属性（除了焦点/编辑状态管理）

---

## 3. 接口定义

### 3.1 Model 接口

```c
// bms_model.h
#pragma once
#include <stdint.h>
#include <stdbool.h>

// ===== 只读数据结构（View 和 Controller 读取）=====
typedef struct {
    // 电池物理状态
    float soc;           // 0-100%
    float ocv;           // 开路电压 (V)
    float voltage;       // 端电压 (V)
    float current;       // 电流 (A)，放电为负
    float temperature;   // 温度 (°C)
    float r_internal;    // 内阻 (Ω)

    // 充电设置
    float charge_u_set;  // 目标电压 (V)
    float charge_i_set;  // 目标电流 (A)
    bool  charge_active; // 充电使能

    // 放电设置
    float discharge_i_set;  // 目标电流 (A)
    bool  discharge_active; // 放电使能
    bool  low_volt_alert;   // 低压告警

    // 系统设置
    int   baud_rate_idx;    // 0:9600, 1:115200
    int   port_idx;         // 0:UART0, 1:UART1

    // 主机通信
    uint8_t  predicted_soc;
    uint8_t  host_online;
    uint32_t rx_packet_count;

    // 状态标志（自动停止等）
    bool charge_auto_stopped;
    bool discharge_auto_stopped;
} bms_state_t;

// ===== 初始化 =====
void bms_model_init(void);

// ===== Tick 函数（由定时器或 main loop 调用）=====
void bms_model_tick_200ms(void);
void bms_model_tick_1000ms(void);

// ===== 只读访问 =====
const bms_state_t * bms_model_get_state(void);

// ===== Controller 调用的 setter =====
void bms_model_set_charge_u(float voltage);
void bms_model_set_charge_i(float current);
void bms_model_set_charge_enable(bool enable);
void bms_model_set_discharge_i(float current);
void bms_model_set_discharge_enable(bool enable);
void bms_model_set_baud_rate(int idx);
void bms_model_set_port(int idx);
```

### 3.2 View 接口

```c
// bms_ui.h
#pragma once
#include "bms_model.h"

// ===== 初始化 =====
void bms_ui_init(void);

// ===== 刷新（从 Model 读取数据，更新所有可见控件）=====
// 由 main loop 周期调用，或在 Model 状态变化后调用
void bms_ui_refresh(const bms_state_t *state);

// ===== Host 通信更新（保留现有接口）=====
void bms_ui_update_soc(uint8_t soc_val);
```

### 3.3 Controller 接口

```c
// bms_input.h
#pragma once

// ===== 注册所有 LVGL 事件回调 =====
// 在 bms_ui_init() 之后调用，将回调绑定到 UI 控件
void bms_input_bind(void);

// ===== 页面管理 =====
int  bms_input_get_current_page(void);
void bms_input_switch_page(int page_idx);
```

---

## 4. 数据流

### 4.1 Model → View（周期刷新）

```
main loop:
  bms_model_tick_200ms()       // 计算电池物理
  bms_model_tick_1000ms()      // 计算主机通信
  state = bms_model_get_state()
  bms_ui_refresh(state)        // 读取 state，更新 LVGL 控件
  lv_timer_handler()           // 处理 LVGL 事件和动画
```

`bms_ui_refresh()` 内部：
```c
void bms_ui_refresh(const bms_state_t *s)
{
    // Page 0: SoC 显示
    if(current_page == 0) {
        update_soc_labels(s->soc, s->voltage, s->current, s->temperature, s->r_internal);
    }
    // Page 1: CCCV 充电
    if(current_page == 1) {
        update_charge_chart(s->voltage, s->current);
        update_charge_readout(s->voltage, s->current);
        update_charge_button_state(s->charge_auto_stopped);
    }
    // ... Page 2, 3
    // Header/Footer
    update_header_soc(s->predicted_soc);
    update_footer_circle(s->charge_active, s->discharge_active);
}
```

### 4.2 Controller → Model（用户输入）

```
LVGL 事件 → bms_input 回调 → Model setter
```

```c
// bms_input.c 中的回调示例
static void on_charge_toggle_click(lv_event_t *e)
{
    const bms_state_t *s = bms_model_get_state();
    bms_model_set_charge_enable(!s->charge_active);
}
```

### 4.3 Controller → View（焦点/页面管理）

Controller 直接操作 LVGL 焦点和页面切换（这是 View 层不关心的导航逻辑）：
```c
// 页面切换
void bms_input_switch_page(int page_idx)
{
    // 隐藏所有页面容器，显示目标页面
    // 重建 LVGL focus group
    // 更新 header/footer 指示器
}
```

---

## 5. 迁移策略

### 5.1 分步迁移（推荐）

**第一步：提取 Model（最小侵入）**
- 将 `bms_ui.c` 中的模拟变量（`batt_soc`、`charge_active` 等）提取到 `bms_model.c`
- 将 `bms_sim_tick` 和 `host_comm_tick` 的计算逻辑移到 `bms_model_tick_*()`
- `bms_model.c` 导出 `bms_state_t` 只读结构
- `bms_ui.c` 中的 timer 回调改为读取 `bms_model_get_state()` 后更新控件
- **此时不改变事件回调**，它们仍然直接修改 Model 变量（通过 setter）

**第二步：提取 Controller**
- 将 `widget_click_handler`、`widget_key_handler`、`global_key_handler` 移到 `bms_input.c`
- 将 `footer_menu_focus_cb`、`button_focus_event_cb` 移到 `bms_input.c`
- Controller 通过 Model setter 修改状态，不再直接写模拟变量
- View 的 `bms_ui_refresh()` 从 Model 读取状态更新控件

**第三步：清理 View**
- `bms_ui.c` 仅保留控件创建、样式定义、`bms_ui_refresh()`
- 移除所有对模拟变量的直接引用
- 移除 timer 创建（移到 main loop 或 Controller）

### 5.2 迁移后的文件结构

```
src/
├── main.c              # 平台入口（PC/STM32）
├── hal/
│   └── hal.c           # 平台 HAL（SDL/SPI/LTDC）
├── model/
│   ├── bms_model.h     # Model 公共接口
│   └── bms_model.c     # 电池物理、充放电、主机通信
├── view/
│   ├── bms_ui.h        # View 公共接口
│   └── bms_ui.c        # LVGL 控件、样式、刷新
├── controller/
│   ├── bms_input.h     # Controller 公共接口
│   └── bms_input.c     # 事件回调、焦点管理、页面切换
```

---

## 6. 权衡与风险

### 6.1 优势

- **可测试性**：`bms_model.c` 可以独立编译和单元测试，不需要 LVGL
- **可移植性**：Model 层零 LVGL 依赖，可直接用于嵌入式固件
- **可维护性**：修改 UI 布局不影响物理模型，修改物理算法不影响 UI
- **复用性**：同一 Model 可驱动不同的 View（LCD、串口、上位机）

### 6.2 风险

- **实时性**：周期刷新模式可能引入 1-2 帧延迟（200ms tick + LVGL 渲染），对 BMS 应用可接受
- **代码量增加**：从 1 个文件变为 3 个文件 + 3 个头文件，结构更清晰但代码总量增加约 20%
- **迁移工作量**：约 2-3 天工作量，需要仔细处理状态同步
- **过度工程**：如果项目规模不再增长，当前单文件结构也可接受

### 6.3 建议

- 如果项目会接入真实 BMS 硬件 → **强烈建议解耦**，Model 层可直接移植到嵌入式固件
- 如果仅作为 PC 模拟器演示 → 当前结构足够，解耦收益有限
- 推荐先执行第一步（提取 Model），验证可行性后再决定是否继续
