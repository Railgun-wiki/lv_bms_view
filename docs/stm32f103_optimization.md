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
