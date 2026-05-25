# STM32F103C8T6 优化指南

## 1. 芯片资源

| 资源 | 容量 | 可用（扣除栈/向量表） |
|------|------|----------------------|
| Flash | 64 KB | ~58 KB |
| SRAM | 20 KB | ~16 KB |
| FPU | 无 | 软件浮点（慢 10-50x） |
| DMA2D | 无 | 纯软件渲染 |

**结论：当前配置完全不可用。** 仅 `LV_MEM_SIZE`（1MB）就是 SRAM 的 50 倍，字体数据（150-300KB）是 Flash 的 2-5 倍。必须全面优化。

---

## 2. Flash 预算

### 2.1 当前估算（未优化）

| 组件 | Flash 占用 |
|------|-----------|
| LVGL 核心（全部功能） | 150-200 KB |
| 字体（18 个 Montserrat 尺寸 + 其他） | 150-300 KB |
| 示例和演示 | 150-700 KB |
| Lottie/ThorVG/图像解码器 | 80-250 KB |
| 应用代码（bms_ui.c + 模拟） | 3-5 KB |
| **总计** | **530-1450 KB** |

### 2.2 优化后目标

| 组件 | Flash 占用 | 说明 |
|------|-----------|------|
| LVGL 核心（精简） | 30-40 KB | 仅启用使用的控件和功能 |
| Montserrat 12 | 5-8 KB | 主要 UI 字体 |
| Montserrat 28 | 13-22 KB | SoC 大字显示 |
| 应用代码 | 3-5 KB | bms_ui.c + 模拟逻辑 |
| HAL 驱动 | 2-4 KB | SPI 显示 + GPIO 输入 |
| **总计** | **53-79 KB** | 目标 < 58 KB |

**风险点：** Montserrat 28 占用 13-22KB，是最大的单项开销。如果 Flash 紧张，可以考虑：
- 用 Montserrat 24 替代（节省 2-4KB）
- 自定义一个仅包含数字 0-9 和 `%`、`.` 的精简字体（约 2-3KB）

---

## 3. SRAM 预算

### 3.1 显示缓冲区

| 方案 | 缓冲区大小 | 说明 |
|------|-----------|------|
| 全帧缓冲（不可行） | 64,800 B | 240×135×2 字节，超过 SRAM |
| 20 行部分缓冲 | 9,600 B | 240×20×2，刷新流畅 |
| 10 行部分缓冲 | 4,800 B | 240×10×2，推荐 |
| 单行缓冲 | 480 B | 最省内存，但刷新慢 |

**推荐：10 行部分缓冲（4,800 字节）。** 刷新 135 行需要 14 次 flush，每次传输 4,800 字节。SPI@18MHz 传输 4,800 字节约 2.1ms，总刷新时间约 30ms（33 FPS），可接受。

### 3.2 LVGL 内存池

```c
#define LV_MEM_SIZE (8 * 1024)  // 8 KB
```

88 个 LVGL 对象 × ~50 字节 ≈ 4.4 KB，加上内部开销约 6-8 KB。8 KB 足够。

### 3.3 总 SRAM 预算

| 组件 | SRAM 占用 |
|------|----------|
| 显示缓冲区（10 行） | 4,800 B |
| LVGL 内存池 | 8,192 B |
| 应用变量（模拟状态等） | 512 B |
| 栈 | 1,024 B |
| 向量表 + 系统 | 512 B |
| **总计** | **~15 KB** / 20 KB |

剩余约 5 KB 余量，可接受。

---

## 4. lv_conf.h 完整优化配置

### 4.1 色深（必须修改）

```c
// 行 29：32-bit → 16-bit
#define LV_COLOR_DEPTH  16
```

### 4.2 内存池（必须修改）

```c
// 行 71：1MB → 8KB
#define LV_MEM_SIZE  (8 * 1024)
```

### 4.3 字体（必须修改）

```c
// 行 669-698：仅保留实际使用的两个字体
#define LV_FONT_MONTSERRAT_8   0
#define LV_FONT_MONTSERRAT_10  0
#define LV_FONT_MONTSERRAT_12  1   // 主要 UI 字体
#define LV_FONT_MONTSERRAT_14  0   // 未使用（虽然是默认值，但可改为 12）
#define LV_FONT_MONTSERRAT_16  0
#define LV_FONT_MONTSERRAT_18  0
#define LV_FONT_MONTSERRAT_20  0
#define LV_FONT_MONTSERRAT_22  0
#define LV_FONT_MONTSERRAT_24  0
#define LV_FONT_MONTSERRAT_26  0
#define LV_FONT_MONTSERRAT_28  1   // SoC 大字显示
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

#define LV_FONT_MONTSERRAT_28_COMPRESSED  0
#define LV_FONT_DEJAVU_16_PERSIAN_HEBREW  0
#define LV_FONT_SOURCE_HAN_SANS_SC_16_CJK 0
#define LV_FONT_UNSCII_8  0

// 默认字体改为 12（节省 montserrat_14 的 Flash）
#define LV_FONT_DEFAULT  &lv_font_montserrat_12
```

### 4.4 SDL 驱动（必须禁用）

```c
// 行 1286
#define LV_USE_SDL  0
```

### 4.5 示例和演示（必须禁用）

```c
// 行 1468-1535
#define LV_BUILD_EXAMPLES  0
#define LV_BUILD_DEMOS     0
// 所有 LV_USE_DEMO_* 设为 0
```

### 4.6 大型库（必须禁用）

```c
#define LV_USE_LOTTIE           0   // 行 841：Lottie 动画（30-80 KB）
#define LV_USE_THORVG_INTERNAL  0   // 行 1073：矢量图形（50-150 KB）
#define LV_USE_VECTOR_GRAPHIC   0   // 行 1069
#define LV_USE_TINY_TTF         0   // 行 1051：TTF 解码器
#define LV_USE_LZ4_INTERNAL     0   // 行 1083：压缩库
#define LV_USE_LODEPNG          0   // 行 998：PNG 解码器
#define LV_USE_BMP              0   // 行 1004
#define LV_USE_TJPGD            0   // 行 1008：JPEG 解码器
#define LV_USE_QRCODE           0   // 行 1034
#define LV_USE_BARCODE          0   // 行 1037
#define LV_USE_RLE              0   // 行 1031
```

### 4.7 未使用的控件（必须禁用）

```c
#define LV_USE_ANIMIMG    0   // 行 787
#define LV_USE_ARCLABEL   0   // 行 791
#define LV_USE_CALENDAR   0   // 行 799
#define LV_USE_MENU       0   // 行 843
#define LV_USE_SPAN       0   // 行 853
#define LV_USE_IMGFONT    0   // 行 1217
```

### 4.8 主题（禁用未使用的）

```c
#define LV_USE_THEME_DEFAULT  0   // 行 886：UI 使用自定义样式
#define LV_USE_THEME_SIMPLE   0   // 行 899
#define LV_USE_THEME_MONO     0   // 行 902
```

### 4.9 渲染优化

```c
// 行 208：禁用圆角/阴影/倾斜渲染（UI 已经全部使用直角）
#define LV_DRAW_SW_COMPLEX  0

// 行 230：禁用复杂渐变
#define LV_USE_DRAW_SW_COMPLEX_GRADIENTS  0

// 行 178-188：仅保留 RGB565 格式支持
#define LV_DRAW_SW_SUPPORT_RGB565        1
#define LV_DRAW_SW_SUPPORT_RGB565_SWAPPED 0
#define LV_DRAW_SW_SUPPORT_RGB565A8       1   // 透明度需要
#define LV_DRAW_SW_SUPPORT_RGB888         0
#define LV_DRAW_SW_SUPPORT_XRGB8888       0
#define LV_DRAW_SW_SUPPORT_ARGB8888       0
#define LV_DRAW_SW_SUPPORT_ARGB8888_PREMULTIPLIED 0
#define LV_DRAW_SW_SUPPORT_L8             0
#define LV_DRAW_SW_SUPPORT_AL88           0
#define LV_DRAW_SW_SUPPORT_A8             0
#define LV_DRAW_SW_SUPPORT_I1             0
```

### 4.10 其他节省

```c
#define LV_USE_FLOAT     0   // 行 652：禁用浮点支持（Cortex-M3 无 FPU）
#define LV_USE_MATRIX    0   // 行 656
#define LV_USE_LOG       0   // 行 459
#define LV_USE_ASSERT_NULL            0   // 行 505
#define LV_USE_ASSERT_MALLOC          0   // 行 506
#define LV_USE_ASSERT_STYLE           0   // 行 507
#define LV_USE_ASSERT_MEM_INTEGRITY   0   // 行 508
#define LV_USE_ASSERT_OBJ             0   // 行 509
#define LV_USE_CHECK_ARG              0   // 行 523
#define LV_USE_SYSMON                 0   // 行 1115
#define LV_USE_OBSERVER               0   // 行 1220
#define LV_OBJ_STYLE_CACHE            0   // 行 579
#define LV_USE_OBJ_ID                 0   // 行 582
#define LV_USE_OBJ_NAME               0   // 行 585
#define LV_USE_FS_STDIO               0   // 行 928
#define LV_USE_FONT_PLACEHOLDER       0   // 行 724
#define LV_LABEL_TEXT_SELECTION        0   // 行 830
#define LV_LABEL_LONG_TXT_HINT        0   // 行 831
```

---

## 5. 浮点数转定点数

### 5.1 问题

Cortex-M3 无 FPU，软件浮点运算极慢。`bms_ui.c` 使用 12 个 float 变量进行电池模拟，每次 `bms_sim_tick`（200ms）执行多次浮点乘法和除法。

### 5.2 定点方案

使用 Q16.16 定点格式（16 位整数 + 16 位小数），精度约 0.0000153，足够 BMS 应用。

```c
// 定点数类型定义
typedef int32_t fixed_t;
#define FIXED_SHIFT  16
#define FIXED_ONE    (1 << FIXED_SHIFT)          // 65536 = 1.0
#define FLOAT_TO_FIXED(f)  ((fixed_t)((f) * FIXED_ONE))
#define FIXED_TO_FLOAT(f)  ((float)(f) / FIXED_ONE)
#define FIXED_MUL(a, b)    ((fixed_t)(((int64_t)(a) * (b)) >> FIXED_SHIFT))
#define FIXED_DIV(a, b)    ((fixed_t)(((int64_t)(a) << FIXED_SHIFT) / (b)))

// 示例：将电池模拟变量改为定点
static fixed_t batt_soc    = FLOAT_TO_FIXED(85.0f);
static fixed_t batt_ocv    = FLOAT_TO_FIXED(4.12f);
static fixed_t batt_u_real = FLOAT_TO_FIXED(4.12f);
static fixed_t batt_i_real = 0;
static fixed_t batt_t_real = FLOAT_TO_FIXED(25.0f);
static fixed_t batt_r_int  = FLOAT_TO_FIXED(0.05f);

// OCV 计算改为定点
// 原始: batt_ocv = 3.0f + 1.15f * norm_soc - 0.08f * (1.0f - norm_soc)^3
static fixed_t compute_ocv(fixed_t norm_soc) {
    fixed_t one = FIXED_ONE;
    fixed_t one_minus = one - norm_soc;
    fixed_t one_minus_2 = FIXED_MUL(one_minus, one_minus);
    fixed_t one_minus_3 = FIXED_MUL(one_minus_2, one_minus);
    return FLOAT_TO_FIXED(3.0f)
         + FIXED_MUL(FLOAT_TO_FIXED(1.15f), norm_soc)
         - FIXED_MUL(FLOAT_TO_FIXED(0.08f), one_minus_3);
}
```

### 5.3 显示格式化

```c
// 定点数转字符串（避免 sprintf + float）
static void fixed_to_str(char *buf, fixed_t val, int decimals) {
    int32_t int_part = val >> FIXED_SHIFT;
    int32_t frac_part = val & (FIXED_ONE - 1);
    // 将小数部分转为十进制
    int32_t frac_dec = 0;
    int32_t mul = 1;
    for(int i = 0; i < decimals; i++) {
        frac_dec = frac_dec * 10 + (frac_part * 10) / FIXED_ONE;
        frac_part = (frac_part * 10) % FIXED_ONE;
        mul *= 10;
    }
    snprintf(buf, 16, "%ld.%0*ld", int_part, decimals, frac_dec);
}
```

---

## 6. 控件简化

### 6.1 Chart → 自定义绘制

`lv_chart` 控件代码约 5-10KB Flash。可以用 `lv_canvas` 或直接在 `lv_obj` 上用 `lv_draw_line` 绘制折线图，节省约 5KB。

但如果 Flash 预算允许（53-79KB < 58KB 目标），保留 `lv_chart` 更简单。

### 6.2 底部渐变条 → 简化

当前 `create_undersampled_bottom_bar` 创建 7 个容器 × 4 个子段 = 35 个额外对象。可以改为：
- 单个 `lv_obj` + 自定义绘制回调
- 或直接用按钮的 border 颜色变化替代

### 6.3 日志终端

`terminal_buffer[256]` 占用 256 字节 SRAM。如果不需要完整日志，可以缩减为 128 字节或移除。

---

## 7. 构建系统

### 7.1 工具链

```cmake
# cmake/arm-none-eabi-gcc.cmake
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_C_FLAGS "-mcpu=cortex-m3 -mthumb -Os -ffunction-sections -fdata-sections")
set(CMAKE_EXE_LINKER_FLAGS "-Wl,--gc-sections -specs=nosys.specs -specs=nano.specs")
```

`-Os` 优化大小，`-ffunction-sections -fdata-sections` + `--gc-sections` 移除未使用的代码。

### 7.2 链接脚本

确保链接脚本中的 Flash/SRAM 大小正确：
```
MEMORY {
    FLASH (rx) : ORIGIN = 0x08000000, LENGTH = 64K
    RAM   (rwx): ORIGIN = 0x20000000, LENGTH = 20K
}
```

---

## 8. HAL 驱动配置

### 8.1 SPI 显示（ST7789/ILI9341）

```c
// SPI1 配置：18MHz 时钟（72MHz / 4）
hspi1.Instance = SPI1;
hspi1.Init.Mode = SPI_MODE_MASTER;
hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_4;
hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
hspi1.Init.CLKPol = SPI_POLARITY_LOW;
hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;

// DMA 传输（可选，减少 CPU 占用）
// SPI TX DMA → DMA1 Channel 3
```

### 8.2 GPIO 输入（3 个按键替代编码器）

```c
// PA0: LEFT  (上一页)
// PA1: RIGHT (下一页)
// PA2: ENTER (确认/进入编辑)
// PA3: ESC   (返回/退出编辑)

static void read_cb(lv_indev_t *indev, lv_indev_data_t *data)
{
    static uint32_t last_key = 0;
    uint32_t key = 0;

    if(HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_RESET) key = LV_KEY_LEFT;
    else if(HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_1) == GPIO_PIN_RESET) key = LV_KEY_RIGHT;
    else if(HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_2) == GPIO_PIN_RESET) key = LV_KEY_ENTER;
    else if(HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_3) == GPIO_PIN_RESET) key = LV_KEY_ESC;

    data->key = key;
    data->state = (key != 0) ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
}
```

---

## 9. 可行性结论

| 指标 | 目标 | 优化后估算 | 结论 |
|------|------|-----------|------|
| Flash | < 58 KB | 53-79 KB | **边缘可行**，Montserrat 28 是最大变量 |
| SRAM | < 16 KB | ~15 KB | **可行**，有 5KB 余量 |
| 刷新率 | > 20 FPS | ~33 FPS | **可行** |
| 按键响应 | < 100ms | ~5ms | **可行** |

**最终建议：**
- Flash 紧张时，用 Montserrat 24 替代 28（节省 2-4KB），或自定义仅含数字的精简字体（节省 10-20KB）
- 如果 Flash 仍超 64KB，考虑移除 `lv_chart` 控件，改用自定义绘制（节省 5-10KB）
- SRAM 充足，无需进一步优化
- 如果需要更高刷新率，可将 SPI 时钟提升到 36MHz（STM32F103 SPI1 最大 18MHz，APB2/2）

---

## 10. BMS-Core 项目集成方案

### 10.1 现有硬件资源

BMS-Core 项目已具备完整的硬件驱动，无需从头编写：

| 驱动 | 文件 | 接口 | 状态 | 用途 |
|------|------|------|------|------|
| INA226 | `BSP/Src/ina226.cpp` | I2C1 (PB8/PB9) | 已集成 | 电压/电流/功率测量 |
| DAC8562 | `BSP/Src/dac8562.cpp` | SPI1 (PA5/PA6/PA7), CS=PA4 | 已集成 | 模拟输出 |
| ST7789 | `BSP/Src/st7789.cpp` | SPI1 共享, DC/RES/CS/BLK 待分配 | 已实现未实例化 | 240×135 LCD |
| TIM2 | `Core/Src/tim.c` | PA0/PA1 | 已配置未使用 | 旋转编码器 |

### 10.2 外设引脚分配

```
已占用:
  I2C1:  PB8 (SCL), PB9 (SDA)     → INA226
  I2C2:  PB10 (SCL), PB11 (SDA)   → 空闲（可用于扩展）
  SPI1:  PA5 (SCK), PA6 (MISO), PA7 (MOSI) → DAC8562 + ST7789 共享
  DAC CS: PA4 (软件控制)
  TIM2:  PA0 (CH1), PA1 (CH2)     → 旋转编码器
  UART1: PA9 (TX), PA10 (RX)      → 调试串口
  USB:   PA11 (DM), PA12 (DP)     → 未使用
  LED:   PC13
  SWD:   PA13, PA14

待分配（ST7789 LCD 引脚）:
  DC:    建议 PB0
  RES:   建议 PB1
  CS:    建议 PB2
  BLK:   建议 PB3（PWM 背光调节）或直接 VCC
```

### 10.3 SPI1 共享互斥

DAC8562 和 ST7789 共享 SPI1，通过各自的 CS 引脚互斥访问：

```
DAC8562: PA4 = CS（低有效）
ST7789:  PB2 = CS（低有效，待分配）

规则：同一时刻只有一个设备的 CS 为低
  - DAC8562 操作前: HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET)
  - DAC8562 操作后: HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET)
  - ST7789 flush 前: HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, GPIO_PIN_RESET)
  - ST7789 flush 后: HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, GPIO_PIN_SET)
```

ST7789 驱动已内置 CS 控制（通过 `Pins` 结构体），无需额外处理。

### 10.4 LVGL 集成步骤

#### 第一步：添加 LVGL 库

```bash
cd BMS-Core/Middlewares
git submodule add https://github.com/lvgl/lvgl.git
git checkout v9.1  # 或最新稳定版
```

在 `CMakeLists.txt` 中添加：
```cmake
add_subdirectory(Middlewares/lvgl)
target_link_libraries(${PROJECT_NAME} lvgl)
```

#### 第二步：创建 lv_conf.h

从 `lv_conf_template.h` 复制，应用第 4 节的全部优化配置。

#### 第三步：ST7789 → LVGL 显示驱动桥接

```c
// App/Src/lv_port_disp.cpp
#include "lvgl.h"
#include "st7789.hpp"

static lv_display_t *disp;
static St7789::Pins lcd_pins = {
    .cs_port = GPIOB, .cs_pin = GPIO_PIN_2,
    .dc_port = GPIOB, .dc_pin = GPIO_PIN_0,
    .res_port = GPIOB, .res_pin = GPIO_PIN_1,
    .blk_port = GPIOB, .blk_pin = GPIO_PIN_3,
};
static St7789 lcd(&hspi1, lcd_pins);

// 10 行部分缓冲（4,800 字节）
#define BUF_LINES  10
static uint16_t buf1[240 * BUF_LINES];

static void flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    lcd.flush_cb(area->x1, area->y1, area->x2, area->y2, (uint16_t *)px_map);
    lv_display_flush_ready(disp);
}

void lv_port_disp_init(void)
{
    lcd.init(St7789::LANDSCAPE);
    disp = lv_display_create(240, 135);
    lv_display_set_flush_cb(disp, flush_cb);
    lv_display_set_buffers(disp, buf1, NULL, sizeof(buf1), LV_DISPLAY_RENDER_MODE_PARTIAL);
}
```

#### 第四步：TIM2 编码器 → LVGL 输入设备

```c
// App/Src/lv_port_indev.cpp
#include "lvgl.h"
#include "tim.h"

static lv_indev_t *indev_enc;

static void read_cb(lv_indev_t *indev, lv_indev_data_t *data)
{
    static int32_t last_count = 0;
    int32_t count = (int32_t)__HAL_TIM_GET_COUNTER(&htim2);
    int32_t diff = (count - last_count) / 4;  // 编码器 4 倍频
    last_count = count;

    data->enc_diff = diff;

    // 编码器按下 → ENTER，可额外接一个 GPIO 按键做 ESC
    // PA2 = ENTER（编码器按键），PA3 = ESC（独立按键）
    if(HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_2) == GPIO_PIN_RESET)
        data->key = LV_KEY_ENTER;
    else if(HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_3) == GPIO_PIN_RESET)
        data->key = LV_KEY_ESC;
    else
        data->key = 0;

    data->state = (data->key != 0) ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
}

void lv_port_indev_init(void)
{
    HAL_TIM_Encoder_Start(&htim2, TIM_CHANNEL_ALL);

    indev_enc = lv_indev_create();
    lv_indev_set_type(indev_enc, LV_INDEV_TYPE_ENCODER);
    lv_indev_set_read_cb(indev_enc, read_cb);
    lv_indev_set_group(indev_enc, lv_group_get_default());
}
```

#### 第五步：SysTick 添加 lv_tick_inc

```c
// Core/Src/stm32f1xx_it.c → SysTick_Handler
void SysTick_Handler(void)
{
    HAL_IncTick();
    lv_tick_inc(1);  // 添加此行
}
```

#### 第六步：主循环集成

```c
// App/Src/app.cpp
#include "lvgl.h"
#include "lv_port_disp.h"
#include "lv_port_indev.h"
#include "bms_ui.h"
#include "ina226.hpp"

extern I2C_HandleTypeDef hi2c1;
static Ina226 sensor(&hi2c1);

void app_init(void)
{
    lv_init();
    lv_port_disp_init();
    lv_port_indev_init();
    bms_ui_init();

    sensor.init(2000, 15000);  // 2mOhm, 15A max
}

void app_run(void)
{
    // 读取真实硬件数据
    int32_t voltage_mV = sensor.getBusVoltage_mV();
    int32_t current_mA = sensor.getCurrent_mA();

    // 替换模拟变量（需要在 bms_ui.c 中暴露 setter 或使用 Model 层）
    // bms_model_set_voltage(voltage_mV);
    // bms_model_set_current(current_mA);

    lv_timer_handler();
    HAL_Delay(5);  // 约 200Hz LVGL 刷新
}
```

### 10.5 模拟变量 → 真实硬件映射

| PC 模拟变量 | 来源 | 替换为 |
|-------------|------|--------|
| `batt_soc` (float) | 计算值 | INA226 电压查表或库仑计 |
| `batt_ocv` (float) | SOC 多项式 | INA226 开路电压测量 |
| `batt_u_real` (float) | 模拟值 | `sensor.getBusVoltage_mV()` |
| `batt_i_real` (float) | 模拟值 | `sensor.getCurrent_mA()` |
| `batt_t_real` (float) | 模拟值 | NTC 热敏电阻 ADC 读数（需添加 ADC） |
| `batt_r_int` (float) | 固定值 | ΔV/ΔI 实时计算 |
| `charge_active` (int) | 模拟 toggle | DAC8562 输出使能 |
| `discharge_active` (int) | 模拟 toggle | DAC8562 输出使能 |
| `target_u_set` (float) | 用户设置 | DAC8562 通道 A 输出电压 |
| `target_i_set` (float) | 用户设置 | DAC8562 通道 B 输出电流限制 |
| `baud_rate_idx` (int) | 模拟设置 | USART1 实际配置 |
| `port_idx` (int) | 模拟设置 | 实际端口选择 |

### 10.6 需要解决的问题

#### 10.6.1 main.c / main.cpp 冲突
`Core/Src/main.cpp`（旧版内联代码）和 `Core/Src/main.c`（新版 app 架构）都定义了 `main()`。CMakeLists.txt 当前编译 `main.cpp`。需要：
- 删除或重命名 `main.cpp`
- 确保 `main.c` 被编译
- 或将 `main.c` 的逻辑合并到 `main.cpp`

#### 10.6.2 ST7789 引脚配置
ST7789 的 DC/RES/CS/BLK 引脚未在 CubeMX 中配置为 GPIO 输出。需要：
- 在 `.ioc` 文件中添加 PB0-PB3 为 GPIO_Output
- 或在代码中手动初始化

#### 10.6.3 INA226 → bms_ui.c 数据桥接
当前 `bms_ui.c` 的模拟变量都是 `static`，外部无法访问。需要：
- 方案 A：在 `bms_ui.h` 中添加 setter 函数（最小改动）
- 方案 B：实施 Model-View 分离（见 `docs/decoupling_proposal.md`）

#### 10.6.4 C++ → C 混编
BMS-Core 使用 C++17（BSP 驱动），但 LVGL 和 `bms_ui.c` 是 C。需要确保：
- `bms_ui.h` 使用 `extern "C"` 包裹
- LVGL 的 `lv_conf.h` 不与 C++ 冲突
- CMakeLists.txt 正确处理 C/C++ 混合编译

### 10.7 Flash 预算（BMS-Core 实际）

| 组件 | Flash 占用 |
|------|-----------|
| STM32 HAL 库（已启用模块） | 8-12 KB |
| INA226 驱动 (C++) | 1-2 KB |
| DAC8562 驱动 (C++) | 1-2 KB |
| ST7789 驱动 (C++) | 1-2 KB |
| LVGL 核心（精简） | 30-40 KB |
| Montserrat 12 + 28 | 18-30 KB |
| bms_ui.c | 3-5 KB |
| LVGL 集成胶水代码 | 1-2 KB |
| **总计** | **63-95 KB** |

**64KB Flash 极度紧张。** 必须：
1. 使用 Montserrat 24 替代 28（节省 2-4KB）
2. 或自定义数字精简字体（节省 10-20KB）
3. 确保 `-Os` + `--gc-sections` 移除所有未引用代码
4. 考虑移除 USB PCD 驱动（节省 2-4KB）
5. 考虑移除 USART1（节省 1-2KB，用 SWD 调试）

### 10.8 SRAM 预算（BMS-Core 实际）

| 组件 | SRAM 占用 |
|------|----------|
| LVGL 显示缓冲区（10 行） | 4,800 B |
| LVGL 内存池 | 8,192 B |
| INA226 驱动实例 | 64 B |
| DAC8562 驱动实例 | 32 B |
| ST7789 驱动实例 | 32 B |
| BMS 应用变量 | 256 B |
| 栈 | 1,024 B |
| 向量表 + 系统 | 512 B |
| **总计** | **~15 KB** / 20 KB |

SRAM 余量约 5KB，足够。
