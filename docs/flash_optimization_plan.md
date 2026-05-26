# BMS UI Flash 优化方案 — STM32F103C8T6 (64KB)

## 概述

当前 LVGL 全功能编译产物为 **4,775.1 KB**，目标芯片 STM32F103C8T6 仅有 **64 KB Flash**（可用 ~58 KB）。本文档是分阶段将 Flash 占用压缩至 58 KB 以内的可执行方案。

> **相关文档：**
> - SRAM 优化 → `stm32f103_optimization.md` Section 3
> - 移植流程 → `stm32_porting_guide.md`
> - MVC 架构 → `decoupling_proposal.md`

> **数据来源：** 所有 `.o` 文件大小来自 PC 模拟器 x86-64 构建的 `size` 工具实测。ARM Cortex-M3 使用 `-Os` + `--gc-sections` 后，实际 Flash 占用约为 `.o` text 段的 **40-60%**。

---

## 1. 现状分析与目标

### 1.1 芯片资源约束

| 资源 | 容量 | 可用（扣除栈/向量表） |
|------|------|----------------------|
| Flash | 64 KB | ~58 KB |
| SRAM | 20 KB | ~16 KB |
| FPU | 无 | 软件浮点（慢 10-50x） |
| DMA2D | 无 | 纯软件渲染 |

### 1.2 当前未优化 Flash 占用

| 组件 | 编译后大小 | 说明 |
|------|-----------|------|
| libs（lz4/lodepng/thorvg/tiny_ttf/qrcode 等） | **1,341.7 KB** | 全部不需要 |
| fonts（18 个 Montserrat + CJK + 其他） | **1,169.6 KB** | 仅需 2 个 |
| stdlib（标准库封装） | **1,056.6 KB** | 部分需要 |
| widgets（28 种控件） | **428.8 KB** | 仅需 6 种 |
| draw（渲染引擎 + 10 种混合格式） | **309.1 KB** | 仅需 2 种格式 |
| core（obj/event/style/group/timer） | **180.2 KB** | 全部需要 |
| misc（样式/动画/事件/文本/缓存） | **147.4 KB** | 部分需要 |
| themes（3 种主题） | **44.5 KB** | 不需要 |
| indev（输入设备） | **28.9 KB** | 需要 |
| layouts（flex + grid） | **24.2 KB** | 不需要 |
| display（显示驱动框架） | **22.3 KB** | 需要 |
| drivers（SDL 等） | **15.0 KB** | 不需要 |
| **总计** | **4,775.1 KB** | **目标 < 58 KB** |

### 1.3 优化目标

**总目标：Flash < 58 KB**

| 子目标 | 范围 |
|--------|------|
| LVGL 核心 + 渲染 | 30-45 KB |
| 字体 | 3.5-5 KB |
| 应用代码 | 3-5 KB |
| HAL 驱动 | 2-4 KB |
| 剩余空间 | > 5 KB |

### 1.4 关键发现

- **1,341.7 KB 库完全未使用** — LZ4、LodePNG、ThorVG、TinyTTF、QRCode、Barcode 等全部可禁用
- **1,103 KB 字体未使用** — 18 个 Montserrat 尺寸中仅用了 12 和 28，CJK/DejaVu/UNSCII 完全未用
- **LVGL 核心 + RGB565 渲染引擎是不可压缩瓶颈** — 即使最小配置，核心仍占 ~48-67 KB

---

## 2. Flash 预算跟踪表

> **使用方法：** 每完成一项优化，在"状态"列打勾，更新"实际值"列。

| 组件 | 未优化 (.o) | P1 配置 | P2 字体 | P3 源码 | -Os 后估算 | 状态 |
|------|------------|---------|---------|---------|-----------|------|
| libs（lz4/lodepng/thorvg 等） | 1,341.7 KB | **9.7 KB** ✅ | — | — | 4-6 KB | ✅ |
| fonts（18 Montserrat + CJK） | 1,169.6 KB | 43.4 KB | **6.5 KB** ✅ | — | 2.6-3.9 KB | ✅ |
| stdlib（标准库封装） | 1,056.6 KB | 22.1 KB ✅ | — | — | 9-13 KB | ✅ |
| widgets（28 种） | 428.8 KB | 62.0 KB ✅ | — | **30.5 KB** ✅ | 12-18 KB | ✅ |
| draw/blend（10 种格式） | 309.1 KB | 97.0 KB ✅ | — | **95.6 KB** ✅ | 38-57 KB | ✅ |
| core | 180.2 KB | 115.7 KB ✅ | — | — | 46-69 KB | ✅ |
| misc | 147.4 KB | 102.8 KB ✅ | — | — | 41-62 KB | ✅ |
| themes | 44.5 KB | 1.6 KB ✅ | — | — | 0.6-1 KB | ✅ |
| indev | 28.9 KB | 26.6 KB ✅ | — | — | 11-16 KB | ✅ |
| layouts | 24.2 KB | 1.1 KB ✅ | — | — | 0.4-0.7 KB | ✅ |
| display | 22.3 KB | 16.0 KB ✅ | — | — | 6-10 KB | ✅ |
| drivers | 15.0 KB | 12.1 KB ✅ | — | — | 5-7 KB | ✅ |
| 应用代码（MVC） | ~5 KB | 31.4 KB ✅ | — | — | 13-19 KB | ✅ |
| HAL 驱动（STM32） | N/A | — | — | — | ~15 KB | 🔲 |
| **总计** | **4,775.1 KB** | **513.5 KB** | **6.5 KB** | **480.6 KB** | **~192-288 KB** | |

> **实测数据（2026-05-26）：** Phase 1+2+3 后 LVGL .o 为 480.6 KB，应用 .o 为 37.7 KB（含 6.5 KB 子集字体），总计 518.3 KB。Phase 3 完成：lv_chart → lv_line（`74717c7`）、lv_bar → lv_obj（`24ce0d6`）、blend 非 NORMAL 移除（`224f280`）、blur dispatch 守卫（`746a639`）。`-Os` + `--gc-sections` 后估算 192-288 KB。**下一步：编译器优化（-Os --gc-sections nano.specs）+ STM32 实测。**

---

## 3. 第一阶段 — lv_conf.h 配置优化

**风险：低 | 影响：高 | 无代码变更**

### 3.1 色深

```c
// lv_conf.h 行 29
#define LV_COLOR_DEPTH  16   // 原值：32
```

**节省：** 所有像素缓冲区减半，混合引擎代码路径减少。

### 3.2 禁用不需要的库

```c
// lv_conf.h
#define LV_USE_LOTTIE           0   // 行 859：Lottie 动画
#define LV_USE_THORVG_INTERNAL  0   // 行 1098：矢量图形
#define LV_USE_VECTOR_GRAPHIC   0   // 行 1094
#define LV_USE_TINY_TTF         0   // 行 1076：TTF 解码器
#define LV_USE_LZ4_INTERNAL     0   // 行 1108：压缩库
#define LV_USE_LODEPNG          0   // 行 1016：PNG 解码器
#define LV_USE_BMP              0   // 行 1022
#define LV_USE_TJPGD            0   // 行 1026：JPEG 解码器
#define LV_USE_QRCODE           0   // 行 1052
#define LV_USE_BARCODE          0   // 行 1055
#define LV_USE_RLE              0   // 行 1049
#define LV_USE_IMGFONT          0   // 行 1242
```

**节省：~1,341.7 KB** (.o 文件)

### 3.3 禁用示例和演示

```c
#define LV_BUILD_EXAMPLES       0   // 行 1493
#define LV_BUILD_DEMOS          0   // 行 1496
// 所有 LV_USE_DEMO_* 设为 0（行 1504-1548）
```

### 3.4 禁用未使用的控件

**仅保留 6 种：** obj、label、button、bar、chart、line

```c
#define LV_USE_ANIMIMG      0   // 行 805
#define LV_USE_ARC          0   // 行 807
#define LV_USE_ARCLABEL     0   // 行 809
#define LV_USE_BUTTONMATRIX 0   // 行 815
#define LV_USE_CALENDAR     0   // 行 817
#define LV_USE_CANVAS       0   // 行 832
#define LV_USE_CHECKBOX     0   // 行 836
#define LV_USE_DROPDOWN     0   // 行 838
#define LV_USE_IMAGE        0   // 行 840
#define LV_USE_IMAGEBUTTON  0   // 行 842
#define LV_USE_KEYBOARD     0   // 行 844
#define LV_USE_LED          0   // 行 853
#define LV_USE_LIST         0   // 行 857
#define LV_USE_LOTTIE       0   // 行 859
#define LV_USE_MENU         0   // 行 861
#define LV_USE_MSGBOX       0   // 行 863
#define LV_USE_ROLLER       0   // 行 865
#define LV_USE_SCALE        0   // 行 867
#define LV_USE_SLIDER       0   // 行 869
#define LV_USE_SPAN         0   // 行 871
#define LV_USE_SPINBOX      0   // 行 877
#define LV_USE_SPINNER      0   // 行 879
#define LV_USE_SWITCH       0   // 行 881
#define LV_USE_TABLE        0   // 行 883
#define LV_USE_TABVIEW      0   // 行 885
#define LV_USE_TEXTAREA     0   // 行 887
#define LV_USE_TILEVIEW     0   // 行 892
#define LV_USE_WIN          0   // 行 894
#define LV_USE_3DTEXTURE    0   // 行 896
```

**节省：~345 KB** (.o 文件，428.8 KB 总计减去 ~83 KB 保留控件)

### 3.5 禁用主题

```c
#define LV_USE_THEME_DEFAULT    0   // 行 904
#define LV_USE_THEME_SIMPLE     0   // 行 917
#define LV_USE_THEME_MONO       0   // 行 920
```

**节省：44.5 KB** (.o 文件)

### 3.6 禁用布局

```c
#define LV_USE_FLEX             0   // 行 928
#define LV_USE_GRID             0   // 行 931
```

**节省：24.2 KB** (.o 文件)

### 3.7 禁用 SDL 和其他驱动

```c
#define LV_USE_SDL              0   // 行 1311
// 所有其他驱动设为 0（行 1323-1483）
```

**节省：15.0 KB** (.o 文件)

### 3.8 渲染引擎优化

```c
// 禁用复杂渲染（圆角/阴影/倾斜/弧形）
#define LV_DRAW_SW_COMPLEX              0   // 行 208
#define LV_USE_DRAW_SW_COMPLEX_GRADIENTS 0  // 行 230

// 仅保留 RGB565 + RGB565A8 格式
#define LV_DRAW_SW_SUPPORT_RGB565               1   // 行 178
#define LV_DRAW_SW_SUPPORT_RGB565_SWAPPED       0   // 行 179
#define LV_DRAW_SW_SUPPORT_RGB565A8             1   // 行 180：字体抗锯齿需要
#define LV_DRAW_SW_SUPPORT_RGB888               0   // 行 181
#define LV_DRAW_SW_SUPPORT_XRGB8888             0   // 行 182
#define LV_DRAW_SW_SUPPORT_ARGB8888             0   // 行 183
#define LV_DRAW_SW_SUPPORT_ARGB8888_PREMULTIPLIED 0  // 行 184
#define LV_DRAW_SW_SUPPORT_L8                   0   // 行 185
#define LV_DRAW_SW_SUPPORT_AL88                 0   // 行 186
#define LV_DRAW_SW_SUPPORT_A8                   1   // 行 187：字体抗锯齿需要（Bpp=4 字体依赖此格式）
#define LV_DRAW_SW_SUPPORT_I1                   0   // 行 188
```

**节省：~200 KB** (.o 文件，禁用 8 种混合格式)

### 3.9 禁用浮点和调试功能

```c
#define LV_USE_FLOAT                0   // 行 670：Cortex-M3 无 FPU
#define LV_USE_MATRIX               0   // 行 674
#define LV_USE_LOG                  0   // 行 459
#define LV_USE_ASSERT_NULL          0   // 行 505
#define LV_USE_ASSERT_MALLOC        0   // 行 506
#define LV_USE_ASSERT_STYLE         0   // 行 507
#define LV_USE_ASSERT_MEM_INTEGRITY 0   // 行 508
#define LV_USE_ASSERT_OBJ           0   // 行 509
#define LV_USE_CHECK_ARG            0   // 行 528
#define LV_USE_SYSMON               0   // 行 1140
#define LV_USE_OBSERVER             0   // 行 1245
#define LV_OBJ_STYLE_CACHE          0   // 行 597
#define LV_USE_OBJ_ID               0   // 行 600
#define LV_USE_OBJ_NAME             0   // 行 603
#define LV_USE_FS_STDIO             0   // 行 946
#define LV_USE_FONT_PLACEHOLDER     0   // 行 742
#define LV_LABEL_TEXT_SELECTION     0   // 行 848
#define LV_LABEL_LONG_TXT_HINT      0   // 行 849
```

### 3.10 内存池

```c
#define LV_MEM_SIZE     (8 * 1024)   // 行 71：1MB → 8KB
```

### 3.11 禁用内置字体

```c
#define LV_FONT_MONTSERRAT_8   0
#define LV_FONT_MONTSERRAT_10  0
#define LV_FONT_MONTSERRAT_12  0   // 使用精简子集替代
#define LV_FONT_MONTSERRAT_14  0
#define LV_FONT_MONTSERRAT_16  0
#define LV_FONT_MONTSERRAT_18  0
#define LV_FONT_MONTSERRAT_20  0
#define LV_FONT_MONTSERRAT_22  0
#define LV_FONT_MONTSERRAT_24  0
#define LV_FONT_MONTSERRAT_26  0
#define LV_FONT_MONTSERRAT_28  0   // 使用精简子集替代
#define LV_FONT_MONTSERRAT_30  0
#define LV_FONT_MONTSERRAT_32  0
#define LV_FONT_MONTSERRAT_34  0
#define LV_FONT_MONTSERRAT_36  0
#define LV_FONT_MONTSERRAT_38  0
#define LV_FONT_MONTSERRAT_40  0
#define LV_FONT_MONTSERRAT_42  0
#define LV_FONT_MONTSERRAT_44  0
#define LV_FONT_MONTSERRAT_46  0
#define LV_FONT_MONTSERRAT_48  0
#define LV_FONT_MONTSERRAT_28_COMPRESSED    0
#define LV_FONT_DEJAVU_16_PERSIAN_HEBREW    0
#define LV_FONT_SOURCE_HAN_SANS_SC_16_CJK   0
#define LV_FONT_UNSCII_8                    0
```

**节省：~1,100 KB** (.o 文件)

### 3.12 Phase 1 汇总

| 指标 | 预估值 | 实测值（2026-05-26） |
|------|--------|---------------------|
| Phase 1 后 LVGL .o 大小 | ~300 KB | **513.5 KB** |
| Phase 1 后应用 .o 大小 | ~5 KB | **37.9 KB** |
| Phase 1+2 后总 .o 大小 | ~300 KB | **550.6 KB** |
| `-Os` + `--gc-sections` 后估算 | 120-200 KB | **220-330 KB** |
| 距目标差距 | 62-142 KB | **162-272 KB** |
| **结论** | **Phase 1 不够，需 Phase 2 + 3** | **Phase 3 + 编译器优化必须** |

---

## 4. 第二阶段 — 字体精简

**风险：低 | 影响：中 | 使用外部工具**

### 4.1 字符集分析

**Montserrat 12** — 分析 MVC 模块所有字符串字面量，实际使用 **64 个字符**：

```
A B C D E F G H I J K L M N O P Q R S T U V W X Y Z
a b d e f g h i l m n o p r s t u y
0 1 2 3 4 5 6 7 8 9
! % * + - . : < > [ ] 空格
```

**Montserrat 28** — 仅用于 SoC 百分比显示（`%d%%`），仅需 **11 个字符**：

```
0 1 2 3 4 5 6 7 8 9 %
```

### 4.2 生成步骤

1. 访问 https://lvgl.io/tools/fontconverter
2. 上传 Montserrat 字体文件（TTF/OTF）
3. **Montserrat 12 子集：**
   - Size = 12, Bpp = 4
   - Range：`0x20-0x21,0x25,0x2A-0x2E,0x30-0x39,0x3A,0x3C-0x3E,0x41-0x5A,0x5B-0x5D,0x61-0x62,0x64-0x67,0x68-0x69,0x6D-0x6F,0x72-0x75`
   - 生成 → 保存为 `fonts/montserrat_12_subset.c`
4. **Montserrat 28 子集：**
   - Size = 28, Bpp = 4
   - Range：`0x25,0x30-0x39`
   - 生成 → 保存为 `fonts/montserrat_28_subset.c`

### 4.3 预期节省

| 字体 | 完整版 | 精简版 | 节省 |
|------|--------|--------|------|
| Montserrat 12（64 字符） | 5-8 KB | ~1.5-2 KB | 3-6 KB |
| Montserrat 28（11 字符） | 13-22 KB | ~2-3 KB | 10-19 KB |
| **总计** | **18-30 KB** | **3.5-5 KB** | **13-25 KB** |

> **实测：** 子集字体 .o 总计 **6.5 KB**（M12: 4.5 KB, M28: 2.0 KB），与预估基本一致。

> **注意：** 字体数据是 `const` 数组，存储在 Flash 的 `.rodata` 段，不受 `-Os` 压缩。

### 4.4 集成配置

```c
// lv_conf.h
LV_FONT_DECLARE(montserrat_12_subset);
LV_FONT_DECLARE(montserrat_28_subset);
#define LV_FONT_DEFAULT  &montserrat_12_subset
```

更新 `bms_ui_styles.c` 和 `bms_ui_pages.c` 中的字体引用：
- `lv_font_montserrat_12` → `montserrat_12_subset`
- `lv_font_montserrat_28` → `montserrat_28_subset`

### 4.5 关于 1bpp 字体（不推荐）

使用 Bpp=1 代替 Bpp=4 可额外节省 2-3 KB，但文字边缘锯齿明显，像素感强。对于 240x135 的小屏幕，文字可读性是核心体验，**不建议为了节省 2-3 KB 牺牲字体质量**。保持 Bpp=4 精简子集方案。

---

## 5. 第三阶段 — 渲染引擎与控件源码修改

**风险：高 | 影响：高 | 需修改 LVGL 源码**

> **状态：** ✅ Phase 1/2/3 已全部完成。Phase 3 于 `opt-f103` 分支实施，修改 LVGL 子模块 `dev-f103/v9.5` 分支。

### 5.1 blend_to_rgb565.c 精简 ✅ 已完成 (commit `224f280`)

**文件：** `lvgl/src/draw/sw/blend/lv_draw_sw_blend_to_rgb565.c`（69 KB 源码，23.4 KB .o）

该文件包含 5 种混合模式：NORMAL、ADDITIVE、SUBTRACTIVE、MULTIPLY、DIFFERENCE。BMS UI 仅使用 NORMAL。

**操作：** 将非 NORMAL 混合模式 else 分支替换为 `return` 早退。

**实测：** blend_to_rgb565.c.o 从 ~12 KB 降至 **2.9 KB**。

### 5.2 移除 lv_chart 控件 ✅ 已完成 (commit `74717c7`)

**文件：** `lvgl/src/widgets/chart/`（41.6 KB .o，第二大控件）

BMS UI 在 2 个页面使用 chart（CCCV 充电、CC 放电），各 2 个 series。

**替代方案：** 用 `lv_line` + `lv_line_set_points_mutable()` + `lv_line_set_y_invert(true)` 实现环形缓冲区折线图。

**实测：** lv_chart.c.o 从 41.6 KB 降至 **48 B**（空桩）。

### 5.3 移除 lv_bar 控件 ✅ 已完成 (commit `24ce0d6`)

**文件：** `lvgl/src/widgets/bar/`（13.6 KB .o）

BMS UI 仅在 header 使用 bar 显示 SoC 进度条。

**替代方案：** 用嵌套 `lv_obj`（外层=轨道，内层=指示器）+ `lv_obj_set_width()` 动态宽度。

**实测：** lv_bar.c.o 从 13.6 KB 降至 **48 B**（空桩）。

### 5.4 守卫 blur dispatch ✅ 部分完成 (commit `746a639`)

在 `lv_draw_sw.c` 的 render dispatch 中用 `#if LV_DRAW_SW_COMPLEX` 守卫 blur case，使 blur 模块在 `COMPLEX=0` 时可被链接器 gc 回收。

**实测：** blur.c.o = **7.0 KB**（dispatch 守卫后链接时可 gc）。

其余渲染子模块（box_shadow、grad、transform、img、vector）在当前配置下已通过 `LV_DRAW_SW_COMPLEX=0` 间接禁用。

### 5.5 禁用 A8 混合格式（不推荐）

**文件：** `lvgl/src/draw/sw/blend/lv_draw_sw_blend_to_a8.c`（22 KB 源码，~8 KB .o）

用于字体抗锯齿 alpha 混合。Bpp=4 精简子集字体依赖 A8 格式进行抗锯齿渲染。禁用后文字边缘锯齿严重，像素感强，在 240x135 小屏幕上可读性大幅下降。

**仅在 Flash 严重不足（差 <5 KB）且愿意牺牲文字质量时考虑。**

**节省：~3-5 KB `-Os` 后 | 代价：文字质量大幅下降**

### 5.6 Phase 3 汇总

| 方案 | .o 节省 | -Os 后节省 | 难度 | 风险 |
|------|---------|-----------|------|------|
| blend_to_rgb565 精简 | ~10 KB | 4-6 KB | 中 | 改 LVGL 源码 |
| 移除 chart | 41.6 KB | 17-25 KB | 中 | 需自定义绘制 |
| 移除 bar | 13.6 KB | 5-8 KB | 低 | 简单替代 |
| 移除渲染子模块 | ~30 KB | 10-20 KB | 中 | 需确认依赖 |
| ~~禁用 A8~~ | ~~8 KB~~ | ~~3-5 KB~~ | ~~低~~ | ~~不推荐：文字锯齿严重~~ |
| **全部应用** | **~95 KB** | **36-59 KB** | | |

---

## 6. 编译器与链接器优化

### 6.1 CMake 工具链文件

```cmake
# cmake/arm-none-eabi-gcc.cmake
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_C_FLAGS "-mcpu=cortex-m3 -mthumb -Os -ffunction-sections -fdata-sections -flto")
set(CMAKE_EXE_LINKER_FLAGS "-Wl,--gc-sections -specs=nosys.specs -specs=nano.specs -flto")
```

### 6.2 各标志说明

| 标志 | 作用 | 预期节省 |
|------|------|---------|
| `-Os` | 优化代码大小 | 基础 |
| `-ffunction-sections` | 每个函数独立 section | 配合 gc-sections |
| `-fdata-sections` | 每个数据项独立 section | 配合 gc-sections |
| `-Wl,--gc-sections` | 链接时移除未引用 section | 40-60% |
| `-specs=nano.specs` | 使用 newlib-nano（更小的 printf） | 5-10 KB |
| `-flto` | 链接时优化，跨模块内联 | 额外 5-10% |
| `-mthumb` | Thumb-2 指令集（16-bit，更小） | 基础 |

### 6.3 链接脚本

```
MEMORY {
    FLASH (rx) : ORIGIN = 0x08000000, LENGTH = 64K
    RAM   (rwx): ORIGIN = 0x20000000, LENGTH = 20K
}
```

### 6.4 预期效果

`-Os` + `--gc-sections` + `nano.specs` + `-flto` 组合可将 ARM 二进制大小降至 `.o` text 段的 **40-60%**。

> **注意：** 编译器标志本身不够。~300 KB .o → 120-180 KB，仍超 58 KB。必须配合 Phase 1-3 的配置和源码优化。

---

## 7. 最终 Flash 预算与可行性

### 7.1 逐组件最终估算

| 组件 | .o 大小 | -Os 后 | 说明 |
|------|---------|--------|------|
| LVGL core（obj/event/style/group/timer） | ~50 KB | ~25 KB | 不可避免 |
| 4 控件（label/button/obj + chart/bar 可选） | ~12-85 KB | ~5-35 KB | 取决于是否保留 chart/bar |
| blend_to_rgb565（仅 NORMAL） | ~13 KB | ~8 KB | |
| blend_to_a8（字体抗锯齿） | ~8 KB | ~4 KB | 可选禁用 |
| 核心绘制（rect/label/line/buf） | ~45 KB | ~22 KB | |
| 字体（2 个精简子集） | ~5 KB | ~5 KB | const 数据不压缩 |
| 应用代码（MVC） | ~5 KB | ~3 KB | |
| HAL 驱动 | ~20 KB | ~15 KB | INA226/DAC/ST7789 |
| STM32 系统 | ~8 KB | ~8 KB | 启动/向量表/HAL |

### 7.2 三种策略对比

| 策略 | 包含阶段 | 估算 Flash | 可行性 |
|------|---------|-----------|--------|
| **A — 保守** | P1 + P2 | 84-126 KB | 超 64 KB |
| **B — 推荐** | P1 + P2 + P3（移除 chart/bar/非 NORMAL blend） | 63-93 KB | 可能可行 |
| **C — 激进** | P1 + P2 + P3 + 移除更多渲染模块 | 54-82 KB | 高概率可行 |

### 7.3 风险与降级方案

如果 64 KB 仍不够：

| 方案 | 说明 | 工作量 |
|------|------|--------|
| 换 STM32F103CBT6 | 128 KB Flash，预算充裕 | 低（仅换芯片） |
| 自定义最小 UI 库 | 放弃 LVGL，预估 20-30 KB | 高（重写 UI） |
| 减少页面 | 从 4 页减为 2 页 | 中（简化功能） |

---

## 8. 实施检查清单

### Phase 1：lv_conf.h 配置（无代码变更）✅ 已完成

- [x] 3.1 色深 32→16
- [x] 3.2 禁用 11 个库（Lottie/ThorVG/TinyTTF/LZ4/lodepng/BMP/tjpgd/QRCode/Barcode/RLE/imgfont）
- [x] 3.3 禁用示例和演示
- [x] 3.4 禁用 22+ 未使用控件
- [x] 3.5 禁用 3 个主题
- [x] 3.6 禁用 flex/grid 布局
- [x] 3.7 禁用 SDL 和其他驱动
- [x] 3.8 渲染引擎优化（禁用复杂渲染 + 仅保留 RGB565/RGB565A8）
- [x] 3.9 禁用浮点/日志/断言/调试功能
- [x] 3.10 内存池 1MB→8KB
- [x] 3.11 禁用所有内置字体
- [x] 验证：PC 模拟器仍可正常构建运行
- [x] Git commit: `29cb6e3`

### Phase 2：字体精简 ✅ 已完成

- [x] 4.1 分析确认字符集（M12: 64 字符, M28: 11 字符）
- [x] 4.2 使用 LVGL 字体工具生成 M12 子集 → `fonts/montserrat_12_subset.c`
- [x] 4.3 使用 LVGL 字体工具生成 M28 子集 → `fonts/montserrat_28_subset.c`
- [x] 4.4 集成字体到项目（更新 lv_conf.h + 样式/页面文件）
- [x] 验证：PC 模拟器构建通过，UI 文字使用子集字体
- [x] Git commit: `e4a3bf1`

### Phase 3：源码修改 ✅ 已完成

- [x] 5.1 blend_to_rgb565.c 移除非 NORMAL 混合模式 → `224f280`
- [x] 5.2 移除 lv_chart，用 lv_line + 可变点数组替代 → `74717c7`
- [x] 5.3 移除 lv_bar，用嵌套 lv_obj + 动态宽度替代 → `24ce0d6`
- [x] 5.4 守卫 blur dispatch（`#if LV_DRAW_SW_COMPLEX`）→ `746a639`
- [x] 5.5 确认 A8 blend 已保留（`LV_DRAW_SW_SUPPORT_A8 = 1`）
- [x] 验证：PC 模拟器构建通过，UI 功能完整

### 编译器优化

- [ ] 6.1 创建 ARM 工具链文件
- [ ] 6.2 配置 `-Os -ffunction-sections -fdata-sections -flto`
- [ ] 6.3 配置 `--gc-sections -specs=nano.specs`
- [ ] 6.4 创建/更新链接脚本
- [ ] 验证：`size` 命令确认 Flash < 58 KB

---

## 附录 A：lv_conf.h 完整优化配置速查

```c
/*==================== COLOR ====================*/
#define LV_COLOR_DEPTH                      16

/*==================== MEMORY ====================*/
#define LV_MEM_SIZE                         (8 * 1024)

/*==================== HAL ====================*/
#define LV_DEF_REFR_PERIOD                  33
#define LV_DPI_DEF                          130

/*==================== OS ====================*/
#define LV_USE_OS                           LV_OS_NONE

/*==================== RENDERING ====================*/
#define LV_DRAW_SW_SUPPORT_RGB565           1
#define LV_DRAW_SW_SUPPORT_RGB565_SWAPPED   0
#define LV_DRAW_SW_SUPPORT_RGB565A8         1
#define LV_DRAW_SW_SUPPORT_RGB888           0
#define LV_DRAW_SW_SUPPORT_XRGB8888         0
#define LV_DRAW_SW_SUPPORT_ARGB8888         0
#define LV_DRAW_SW_SUPPORT_ARGB8888_PREMULTIPLIED 0
#define LV_DRAW_SW_SUPPORT_L8               0
#define LV_DRAW_SW_SUPPORT_AL88             0
#define LV_DRAW_SW_SUPPORT_A8               1   // 字体抗锯齿必需
#define LV_DRAW_SW_SUPPORT_I1               0
#define LV_DRAW_SW_COMPLEX                  0
#define LV_USE_DRAW_SW_COMPLEX_GRADIENTS    0

/*==================== FONTS ====================*/
#define LV_FONT_MONTSERRAT_8   0
#define LV_FONT_MONTSERRAT_10  0
#define LV_FONT_MONTSERRAT_12  0
#define LV_FONT_MONTSERRAT_14  0
#define LV_FONT_MONTSERRAT_16  0
#define LV_FONT_MONTSERRAT_18  0
#define LV_FONT_MONTSERRAT_20  0
#define LV_FONT_MONTSERRAT_22  0
#define LV_FONT_MONTSERRAT_24  0
#define LV_FONT_MONTSERRAT_26  0
#define LV_FONT_MONTSERRAT_28  0
#define LV_FONT_MONTSERRAT_30  0
#define LV_FONT_MONTSERRAT_32  0
#define LV_FONT_MONTSERRAT_34  0
#define LV_FONT_MONTSERRAT_36  0
#define LV_FONT_MONTSERRAT_38  0
#define LV_FONT_MONTSERRAT_40  0
#define LV_FONT_MONTSERRAT_42  0
#define LV_FONT_MONTSERRAT_44  0
#define LV_FONT_MONTSERRAT_46  0
#define LV_FONT_MONTSERRAT_48  0
#define LV_FONT_MONTSERRAT_28_COMPRESSED    0
#define LV_FONT_DEJAVU_16_PERSIAN_HEBREW    0
#define LV_FONT_SOURCE_HAN_SANS_SC_16_CJK   0
#define LV_FONT_UNSCII_8                    0
LV_FONT_DECLARE(montserrat_12_subset);
LV_FONT_DECLARE(montserrat_28_subset);
#define LV_FONT_DEFAULT                     &montserrat_12_subset
#define LV_FONT_FMT_TXT_LARGE              0
#define LV_USE_FONT_COMPRESSED              0
#define LV_USE_FONT_PLACEHOLDER             0

/*==================== WIDGETS ====================*/
#define LV_USE_ANIMIMG      0
#define LV_USE_ARC          0
#define LV_USE_ARCLABEL     0
#define LV_USE_BAR          0   // 或 1（如保留 bar）
#define LV_USE_BUTTON       1
#define LV_USE_BUTTONMATRIX 0
#define LV_USE_CALENDAR     0
#define LV_USE_CANVAS       0
#define LV_USE_CHART        0   // 或 1（如保留 chart）
#define LV_USE_CHECKBOX     0
#define LV_USE_DROPDOWN     0
#define LV_USE_IMAGE        0
#define LV_USE_IMAGEBUTTON  0
#define LV_USE_KEYBOARD     0
#define LV_USE_LABEL        1
#define LV_USE_LED          0
#define LV_USE_LINE         1
#define LV_USE_LIST         0
#define LV_USE_LOTTIE       0
#define LV_USE_MENU         0
#define LV_USE_MSGBOX       0
#define LV_USE_ROLLER       0
#define LV_USE_SCALE        0
#define LV_USE_SLIDER       0
#define LV_USE_SPAN         0
#define LV_USE_SPINBOX      0
#define LV_USE_SPINNER      0
#define LV_USE_SWITCH       0
#define LV_USE_TABLE        0
#define LV_USE_TABVIEW      0
#define LV_USE_TEXTAREA     0
#define LV_USE_TILEVIEW     0
#define LV_USE_WIN          0
#define LV_USE_3DTEXTURE    0

/*==================== THEMES ====================*/
#define LV_USE_THEME_DEFAULT    0
#define LV_USE_THEME_SIMPLE     0
#define LV_USE_THEME_MONO       0

/*==================== LAYOUTS ====================*/
#define LV_USE_FLEX             0
#define LV_USE_GRID             0

/*==================== LIBS ====================*/
#define LV_USE_LODEPNG          0
#define LV_USE_BMP              0
#define LV_USE_TJPGD            0
#define LV_USE_QRCODE           0
#define LV_USE_BARCODE          0
#define LV_USE_RLE              0
#define LV_USE_TINY_TTF         0
#define LV_USE_THORVG_INTERNAL  0
#define LV_USE_VECTOR_GRAPHIC   0
#define LV_USE_LZ4_INTERNAL     0
#define LV_USE_IMGFONT          0
#define LV_USE_OBSERVER         0
#define LV_USE_FS_STDIO         0

/*==================== DEBUG ====================*/
#define LV_USE_FLOAT                0
#define LV_USE_MATRIX               0
#define LV_USE_LOG                  0
#define LV_USE_ASSERT_NULL          0
#define LV_USE_ASSERT_MALLOC        0
#define LV_USE_ASSERT_STYLE         0
#define LV_USE_ASSERT_MEM_INTEGRITY 0
#define LV_USE_ASSERT_OBJ           0
#define LV_USE_CHECK_ARG            0
#define LV_USE_SYSMON               0
#define LV_OBJ_STYLE_CACHE          0
#define LV_USE_OBJ_ID               0
#define LV_USE_OBJ_NAME             0
#define LV_LABEL_TEXT_SELECTION     0
#define LV_LABEL_LONG_TXT_HINT      0

/*==================== DRIVERS ====================*/
#define LV_USE_SDL              0

/*==================== BUILD ====================*/
#define LV_BUILD_EXAMPLES       0
#define LV_BUILD_DEMOS          0
```

---

## 附录 B：字体字符集参考

### Montserrat 12（64 字符）

**用途：** 所有 UI 文字（标签、按钮、状态、读数）

**十六进制范围：** `0x20-0x21,0x25,0x2A-0x2E,0x30-0x39,0x3A,0x3C-0x3E,0x41-0x5A,0x5B-0x5D,0x61-0x62,0x64-0x67,0x68-0x69,0x6D-0x6F,0x72-0x75`

**可读字符：**
```
空格 ! % * + - . : 0 1 2 3 4 5 6 7 8 9 < = > [ \ ]
A B C D E F G H I J K L M N O P Q R S T U V W X Y Z
a b d e f g h i l m n o p r s t u y
```

### Montserrat 28（11 字符）

**用途：** SoC 百分比大字显示

**十六进制范围：** `0x25,0x30-0x39`

**可读字符：**
```
% 0 1 2 3 4 5 6 7 8 9
```

### LVGL 字体工具

https://lvgl.io/tools/fontconverter

---

## 附录 C：逐文件 .o 大小参考

### C.1 字体

| 字体文件 | 大小 | 需要？ |
|---------|------|--------|
| `lv_font_source_han_sans_sc_16_cjk.c` | 191.2 KB | 否 |
| `lv_font_montserrat_48.c` | 94.6 KB | 否 |
| `lv_font_montserrat_46.c` | 89.1 KB | 否 |
| `lv_font_montserrat_44.c` | 81.6 KB | 否 |
| `lv_font_montserrat_42.c` | 75.3 KB | 否 |
| `lv_font_montserrat_40.c` | 68.7 KB | 否 |
| `lv_font_montserrat_38.c` | 62.0 KB | 否 |
| `lv_font_montserrat_36.c` | 56.2 KB | 否 |
| `lv_font_montserrat_34.c` | 51.2 KB | 否 |
| `lv_font_montserrat_32.c` | 45.0 KB | 否 |
| `lv_font_montserrat_30.c` | 41.1 KB | 否 |
| `lv_font_dejavu_16_persian_hebrew.c` | 39.9 KB | 否 |
| `lv_font_montserrat_28.c` | 36.6 KB | **是** |
| `lv_font_montserrat_26.c` | 32.1 KB | 否 |
| `lv_font_montserrat_14_aligned.c` | 29.3 KB | 否 |
| `lv_font_montserrat_24.c` | 28.2 KB | 否 |
| `lv_font_montserrat_22.c` | 24.8 KB | 否 |
| `lv_font_montserrat_28_compressed.c` | 21.9 KB | 否 |
| `lv_font_montserrat_20.c` | 21.4 KB | 否 |
| `lv_font_montserrat_18.c` | 18.6 KB | 否 |
| `lv_font_montserrat_16.c` | 15.7 KB | 否 |
| `lv_font_montserrat_14.c` | 13.5 KB | **是** |
| `lv_font_montserrat_12.c` | 11.4 KB | **是** |
| `binfont_loader` | 7.8 KB | 否 |
| `lv_font_fmt_txt` | 5.1 KB | 是 |
| **字体总计** | **1,169.6 KB** | |

### C.2 控件

| 控件 | 大小 | 需要？ |
|------|------|--------|
| `lv_chart.c` | 41.6 KB | **是** |
| `lv_scale.c` | 32.2 KB | 否 |
| `lv_textarea.c` | 29.4 KB | 否 |
| `lv_span.c` | 23.6 KB | 否 |
| `lv_dropdown.c` | 23.3 KB | 否 |
| `lv_arc.c` | 23.0 KB | 否 |
| `lv_label.c` | 22.5 KB | **是** |
| `lv_image.c` | 22.2 KB | 否 |
| `lv_table.c` | 20.6 KB | 否 |
| `lv_buttonmatrix.c` | 18.3 KB | 否 |
| `lv_arclabel.c` | 17.1 KB | 否 |
| `lv_menu.c` | 16.5 KB | 否 |
| `lv_roller.c` | 14.2 KB | 否 |
| `lv_bar.c` | 13.6 KB | **是** |
| `lv_spinbox.c` | 13.2 KB | 否 |
| `lv_calendar.c` | 12.1 KB | 否 |
| `lv_slider.c` | 8.9 KB | 否 |
| `lv_canvas.c` | 8.5 KB | 否 |
| `lv_imagebutton.c` | 7.5 KB | 否 |
| `lv_keyboard.c` | 7.8 KB | 否 |
| `lv_tabview.c` | 7.1 KB | 否 |
| `lv_animimage.c` | 6.7 KB | 否 |
| `lv_line.c` | 5.6 KB | **是** |
| `lv_msgbox.c` | 5.7 KB | 否 |
| `lv_switch.c` | 4.7 KB | 否 |
| `lv_led.c` | 4.1 KB | 否 |
| `lv_checkbox.c` | 4.0 KB | 否 |
| `lv_lottie.c` | 3.5 KB | 否 |
| `lv_spinner.c` | 3.2 KB | 否 |
| `lv_button.c` | ~2 KB | **是** |
| `lv_obj.c` | ~0 KB | **是**（核心） |
| **控件总计** | **428.8 KB** | |

### C.3 渲染引擎

| 文件 | 大小 | 需要？ |
|------|------|--------|
| `blend_to_rgb565.c` | 23.4 KB | **是** |
| `blend_to_rgb565_swapped.c` | 24.2 KB | 否 |
| `blend_to_argb8888.c` | 16.0 KB | 否 |
| `blend_to_rgb888.c` | 13.4 KB | 否 |
| `blend_to_i1.c` | 14.2 KB | 否 |
| `blend_to_l8.c` | 10.9 KB | 否 |
| `blend_to_al88.c` | 10.6 KB | 否 |
| `blend_to_argb8888_premultiplied.c` | 9.7 KB | 否 |
| `blend_to_a8.c` | ~8 KB | **是** |
| `lv_draw_vector.c` | 16.6 KB | 否 |
| `lv_draw_sw_vector.c` | 7.3 KB | 否 |
| `lv_draw_sw_mask.c` | 14.4 KB | 是 |
| `lv_draw_sw_img.c` | 13.8 KB | 否 |
| `lv_draw_sw_transform.c` | 13.2 KB | 否 |
| `lv_draw_sw_box_shadow.c` | 9.8 KB | 否 |
| `lv_draw_sw_blur.c` | 6.9 KB | 否 |
| `lv_draw_sw_grad.c` | 6.5 KB | 否 |
| `lv_draw_label.c` | 8.7 KB | **是** |
| `lv_draw_rect.c` | 6.7 KB | **是** |
| `lv_draw_line.c` | ~5 KB | **是** |
| `lv_draw_buf.c` | 9.9 KB | 是 |
| `lv_draw.c` | 6.9 KB | 是 |
| **渲染总计** | **309.1 KB** | |

### C.4 库

| 库 | 大小 | 需要？ |
|----|------|--------|
| lz4（压缩） | 302.3 KB | 否 |
| lodepng（PNG 解码） | 116.1 KB | 否 |
| thorvg（矢量图形） | ~700 KB | 否 |
| tiny_ttf（TTF 解码） | 79.8 KB | 否 |
| qrcode（二维码） | 18.3 KB | 否 |
| bin_decoder（二进制图像） | 13.6 KB | 否 |
| barcode（条形码） | ~8 KB | 否 |
| rle（压缩） | ~3 KB | 否 |
| **库总计** | **1,341.7 KB** | |
