# one-euro-filter-c

[![C CI](https://github.com/smallfishworld/one-euro-filter-c/actions/workflows/ci.yml/badge.svg)](https://github.com/smallfishworld/one-euro-filter-c/actions/workflows/ci.yml)
[![MIT License](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)

[English](README.md) | **简体中文**

采用**纯 C99** 实现的轻量级通用 **One Euro Filter（一欧元滤波器）** 库，支持一维数值、二维坐标、三维位置。主要作用是在静止时抑制测量抖动，并在输入快速变化时自动减小跟踪延迟。

**v2.0.0 不再配置固定采样频率**。现在有两种调用方式：让库自动从单调时钟计算采样间隔，或者由调用方传入真实采样间隔 `dt`。

## 主要特性

- **自动采样间隔：** `oef_auto_update*_ex()` 自动读取单调时钟，以相邻两次**成功采样**的时间差计算 `dt`。
- **手动采样间隔：** `oef_update*_ex(..., dt_seconds, ...)` 用于外部采样时间戳或者离线数据回放。
- **跨平台时钟：** Linux/macOS 使用 `CLOCK_MONOTONIC`；Windows 使用 `QueryPerformanceCounter`；RTOS/裸机可注册时钟回调。
- **1D、2D、3D 通用接口：** 二维/三维采用向量速度计算共享截止频率，适合同单位坐标轴。
- **完整参数配置：** 最小截止频率、速度响应系数 `beta`、速度估计截止频率、可选最大截止频率。
- **高可靠性：** Doxygen 注释、错误码、失败回滚、无动态内存、无内部锁、MIT 许可及跨平台自动测试。

## 快速使用：二维坐标自动计时

```c
#include "one_euro_filter.h"
#include <stdio.h>

int main(void)
{
    oef_config_t config = oef_config_default();
    oef_auto_filter2d_t filter;
    oef_vec2_t input = {100.5f, 200.3f};
    oef_vec2_t output;

    /* 时钟参数传入 NULL：使用当前操作系统的单调时钟。 */
    oef_status_t status = oef_auto_init2d_ex(&filter, &config, NULL, NULL);
    if (status != OEF_OK) return 1;

    /* 每次收到新的二维坐标，直接调用，不需要配置频率或传入 dt。 */
    status = oef_auto_update2d_ex(&filter, input, &output);
    if (status != OEF_OK) {
        fprintf(stderr, "%s: %s\n", oef_status_name(status),
                oef_status_message(status));
        return 1;
    }
    printf("(%.3f, %.3f)\n", output.x, output.y);
    return 0;
}
```

示例循环连续运行得很快，部分平台的系统时钟可能对连续两次读取返回相同时间，因此三个示例使用**模拟单调时钟回调**，产生确定的非零时间间隔。生产代码传入 `NULL` 时则直接使用操作系统单调时钟。

提供三个独立、通用的可运行示例：[一维数值](examples/one_dimensional.c)、[二维坐标](examples/two_dimensional.c)、[三维坐标及自定义时钟](examples/three_dimensional.c)。

### 一维（1D）自动计时

```c
oef_config_t cfg = oef_config_default();
oef_auto_filter_t filter;
oef_auto_init_ex(&filter, &cfg, NULL, NULL);
float value;
oef_status_t ret = oef_auto_update_ex(&filter, 25.12f, &value);
```

### 三维（3D）自动计时

```c
oef_config_t cfg = oef_config_default();
oef_auto_filter3d_t filter;
oef_auto_init3d_ex(&filter, &cfg, NULL, NULL);
oef_vec3_t raw = {1.02f, 1.98f, 3.01f}, smooth;
oef_status_t ret = oef_auto_update3d_ex(&filter, raw, &smooth);
```

### 手动传入采样间隔（无需系统时钟）

如果数据包已经携带采样时间戳，建议按**原始采样时刻**计算时间差，而不是按处理线程执行时刻计算：

```c
oef_config_t cfg = oef_config_default();
oef_filter2d_t filter;
oef_init2d_ex(&filter, &cfg);
/* dt_seconds 由采样时间戳计算，单位为秒。 */
oef_vec2_t input = {100.5f, 200.3f}, smooth;
oef_status_t ret = oef_update2d_ex(&filter, input, 0.023f, &smooth);
```

**如果是异步队列、跨线程或者 DMA 数据管线，采集时间戳通常比函数被调用的时间更准确。** 这种情况下可保留手动 `dt` 接口。

## 一欧元滤波器算法原理

普通一阶低通滤波的滤波系数固定，导致“静止更稳”和“移动更快”难以兼顾：

\[
\hat{x}_t=\hat{x}_{t-1}+\alpha(x_t-\hat{x}_{t-1})
\]

**一欧元滤波器的核心是根据运动速度动态调整截止频率和滤波系数。** 每次采样经历五步：

**第一步：估计输入速度。**

\[
v_t=\frac{x_t-x_{t-1}}{\Delta t}
\]

这里 `dt` 即 \(\Delta t\)，是本次采样与上次采样的真实时间间隔，单位为**秒**。

**第二步：对速度进行低通滤波。**

\[
\alpha_d=\frac{2\pi f_d\Delta t}{1+2\pi f_d\Delta t}
\]

\[
\hat{v}_t=\hat{v}_{t-1}+\alpha_d(v_t-\hat{v}_{t-1})
\]

其中 \(f_d\) 为 `derivative_cutoff_hz`，用于防止输入噪声引起速度估计剧烈变化。

**第三步：根据速度调整截止频率。**

\[
f_{c,t}=f_{\min}+\beta|\hat{v}_t|
\]

其中 \(f_{\min}\) 为 `min_cutoff_hz`，\(\beta\) 为 `beta`。如设置 `max_cutoff_hz > 0`，还会将截止频率限制在指定上限以内。

**第四步：根据截止频率计算动态滤波系数。**

\[
\alpha_t=\frac{2\pi f_{c,t}\Delta t}{1+2\pi f_{c,t}\Delta t}
\]

**第五步：得到新的滤波输出。**

\[
\hat{x}_t=\hat{x}_{t-1}+\alpha_t(x_t-\hat{x}_{t-1})
\]

于是，**速度小时截止频率低、滤波更强；速度大时截止频率高、运动滞后更小**。一欧元滤波不会彻底锁死静止坐标，也不能自行修复上游数据跳变。

### 二维和三维如何计算

二维/三维先分别估计每个轴的平滑速度，再取向量速度模长：

\[
s_{2D}=\sqrt{\hat{v}_x^2+\hat{v}_y^2}
\]

\[
s_{3D}=\sqrt{\hat{v}_x^2+\hat{v}_y^2+\hat{v}_z^2}
\]

用 \(s\) 替代标量公式中的 \(|\hat{v}_t|\) 计算**同一个动态截止频率**，然后所有坐标轴共用相同的 \(\alpha_t\)。这种做法适合各坐标轴**物理单位一致**的情形；如果 X/Y/Z 单位不同或需要不同参数，请分别使用一维滤波器。

## 参数说明

`oef_config_default()` 默认值仅供开始调参：

| 配置字段 | 默认值 | 有效范围 | 作用 |
| --- | ---: | --- | --- |
| `min_cutoff_hz` | 0.8 | 有限数且 > 0 | 越小静止越稳，但滞后增加 |
| `beta` | 0.5 | 有限数且 ≥ 0 | 越大运动响应越快，也可能放大噪声 |
| `derivative_cutoff_hz` | 1.0 | 有限数且 > 0 | 对估计速度进行低通滤波 |
| `max_cutoff_hz` | 0 | 0 或有限数且 ≥ 最小截止频率 | 可选的截止频率上限，0 表示不限制 |

**特别注意：** 字段名中的 Hz 是**滤波器截止频率**，并不是输入采样帧率。v2.0 完全删除 `sample_frequency_hz`。`dt_seconds` 的单位为**秒**，速度的单位为**输入单位/秒**。不同单位的坐标（例如像素和米）需要重新调整 `beta`。

常见调参方法：先令 `beta=0`，调整 `min_cutoff_hz` 取得可接受的静止稳定性；然后逐步提高 `beta` 优化运动延迟；最后按实际数据调整速度截止频率及最大截止频率。

## 自动时钟和 RTOS 接入

库在每个自动滤波器实例中保存**上次成功更新的时间戳**。第一次有效样本直接输出原始值并保存时间。更新失败时，滤波器历史值、时间戳和输出全部保持不变；下次成功更新时的间隔将从**上次成功更新**算起。

- 时钟回退或两次采样时间相同 → `OEF_E_TIME`。
- 单调时钟读取失败、回调返回错误或时间戳非法 → `OEF_E_CLOCK`。
- `dt` 无法安全转换成正的有限 `float` → `OEF_E_RANGE`。
- 输入不合法、状态损坏 → 使用对应的通用错误码，失败不修改输出。

采集重新启动、时钟源切换、间隔非常长时，建议使用 `oef_auto_reset*_ex()` 同时清空滤波历史和时间基准。不要只对 `auto_filter.filter` 调用底层的 `oef_reset*`，否则时间历史会不一致。

裸机或 RTOS 请注册自己的**单调计时**回调，返回双精度秒数：

```c
/* tick_count() 是示意函数，请替换成你实际的 RTOS 高精度计数接口。 */
typedef struct { double ticks_per_second; } clock_config_t;

oef_status_t my_rtos_clock(void *ctx, double *seconds)
{
    clock_config_t *c = (clock_config_t *)ctx;
    if (c == NULL || seconds == NULL || c->ticks_per_second <= 0.0)
        return OEF_E_CLOCK;
    /* 实际代码必须正确处理计数器回绕。 */
    *seconds = (double)tick_count() / c->ticks_per_second;
    return OEF_OK;
}

clock_config_t timer = {1000.0};
oef_auto_filter2d_t filter;
oef_config_t cfg = oef_config_default();
oef_auto_init2d_ex(&filter, &cfg, my_rtos_clock, &timer);
```

示例中的 `tick_count()` 只是伪代码。你需要提供实际计数接口，并处理硬件计数器回绕。**不要使用 UTC、系统当前时间或可能被用户/NTP 调整的墙上时间**。不支持原生单调时钟的系统，如传入 `NULL` 回调，自动模式会返回 `OEF_E_CLOCK`，不会退化为墙上时间。

## API 与错误码

| 作用 | 1D | 2D | 3D |
| --- | --- | --- | --- |
| 手动传入 `dt` | `oef_update_ex` | `oef_update2d_ex` | `oef_update3d_ex` |
| 自动模式初始化 | `oef_auto_init_ex` | `oef_auto_init2d_ex` | `oef_auto_init3d_ex` |
| 自动模式更新 | `oef_auto_update_ex` | `oef_auto_update2d_ex` | `oef_auto_update3d_ex` |
| 自动模式复位 | `oef_auto_reset_ex` | `oef_auto_reset2d_ex` | `oef_auto_reset3d_ex` |
| 基础模式初始化 | `oef_init_ex` | `oef_init2d_ex` | `oef_init3d_ex` |
| 基础模式修改配置 | `oef_set_config_ex` | `oef_set_config2d_ex` | `oef_set_config3d_ex` |

完整错误码：`OEF_OK = 0`、`OEF_E_NULL = -1`、`OEF_E_CONFIG = -2`、`OEF_E_INPUT = -3`、`OEF_E_TIME = -4`、`OEF_E_STATE = -5`、`OEF_E_RANGE = -6`、`OEF_E_CLOCK = -7`。可用 `oef_status_name()` 与 `oef_status_message()` 获取文本。原有手动 `dt` 的 `bool` 接口仍可使用；配置固定频率的 `oef_step*()` 已删除。参见 [docs/API.md](docs/API.md)。

## 编译、测试与移植

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --config Debug --parallel
ctest --test-dir build -C Debug --output-on-failure
```

在 Linux/macOS 还可以运行 `make test`、`make example`。示例可执行文件分别是 `oef_example_1d`、`oef_example_2d`、`oef_example_3d`。嵌入式工程一般直接编译 `src/one_euro_filter.c`、`src/one_euro_auto.c` 和 `src/one_euro_clock.c`，并添加 `include/` 目录。`one_euro_clock.c` 在不支持原生时钟的平台会提供返回错误码的替代实现。如果只使用手动 `dt` API，也可只编译 `src/one_euro_filter.c`。

支持 `-std=c99 -Wall -Wextra -Wpedantic -Werror` 严格告警检查及可选 `OEF_ENABLE_SANITIZERS=ON`。详见 [贡献规范](CONTRIBUTING.md)、[版本记录](CHANGELOG.md)。

## 从 v1.1 迁移

**v2.0 是不兼容版本**：配置结构体删除了 `sample_frequency_hz`，固定频率 `oef_step*` 相关函数也被移除。

```c
/* v1.1:
 * cfg.sample_frequency_hz = 60.0f;
 * oef_step2d_ex(&filter, raw, &out);
 */

/* v2.0 自动计算时间差 */
oef_auto_filter2d_t auto_filter;
oef_auto_init2d_ex(&auto_filter, &cfg, NULL, NULL);
oef_auto_update2d_ex(&auto_filter, raw, &out);

/* 或者 v2.0 手动 dt，单位秒 */
oef_filter2d_t filter;
oef_init2d_ex(&filter, &cfg);
oef_update2d_ex(&filter, raw, 1.0f/60.0f, &out);
```

升级时必须使用 v2 的头文件和库重新编译使用方。原有手动 `oef_update*` 系列函数保留。

## 参考文献与许可证

原算法发表于 Casiez、Roussel、Vogel 在 CHI 2012 的论文 *1€ Filter: A Simple Speed-based Low-pass Filter for Noisy Input in Interactive Systems*。论文作者的 [1€ Filter 主页](https://cristal.univ-lille.fr/~casiez/1euro/)。

使用 [MIT License](LICENSE) 开源。
