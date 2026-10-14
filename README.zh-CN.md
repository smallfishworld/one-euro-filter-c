# one-euro-filter-c

[![C CI](https://github.com/smallfishworld/one-euro-filter-c/actions/workflows/ci.yml/badge.svg)](https://github.com/smallfishworld/one-euro-filter-c/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)

[English](README.md) | **简体中文**

一个采用**纯 C99** 实现、轻量且参数可配置的 **One Euro Filter（一欧元滤波器，1€ Filter）** 通用库。用于平滑传感器数值、二维坐标和三维位置等采样数据：输入近似静止时抑制抖动，移动时自动提高响应速度以减轻延迟。

- 支持 **一维（1D）、二维（2D）、三维（3D）**；每个滤波器实例有独立状态。
- **全部参数可配置**：最小截止频率、速度响应系数、速度估计截止频率、采样频率以及可选的最大截止频率。
- 支持**固定频率更新**（`oef_step*()`）与**实际帧间隔更新**（`oef_update*()`）。
- 可运行时修改参数、复位历史；遇到非法参数、非有限输入时保持状态和输出不变。
- **无需动态内存和操作系统接口**，仅依赖 C 标准数学函数，适用于 RTOS、裸机 MCU、Linux、Windows、macOS。
- 包含 CMake、Makefile、单元测试、三套可独立运行的示例、跨平台 CI。

## 一、算法原理

### 1.1 普通低通滤波有什么问题？

普通的一阶低通滤波（Low-Pass Filter，LPF）：

```math
y_t=(1-\alpha)y_{t-1}+\alpha x_t
```

其中 `x_t` 为当前输入，`y_t` 为本次输出，`y_{t-1}` 为上一帧输出；系数 `\alpha` 介于 0 和 1 之间：

- **`\alpha` 较小**：静止时更稳定，但移动时输出明显滞后。
- **`\alpha` 较大**：移动时跟随较快，但静止时噪声更容易穿透滤波器。

一欧元滤波器的关键思想是：**根据输入信号的变化速度，自动改变低通滤波器的截止频率，进而改变 `\alpha`。**

这样就不必始终在“静止不抖”和“运动不拖尾”之间选择一个固定的折中值。

### 1.2 完整计算流程（五步）

设当前原始采样值为 `x_t`，前一帧原始值为 `x_{t-1}`，实际采样间隔为 `\Delta t`（**秒**）。

**第一步：估计信号变化速度。**

```math
v_t=\frac{x_t-x_{t-1}}{\Delta t}
```

**第二步：平滑速度估计。**

原始速度也会因测量噪声而跳动，因此先进行一次低通滤波：

```math
\alpha_d=\frac{2\pi f_d\Delta t}{1+2\pi f_d\Delta t}
```

```math
\hat v_t=\hat v_{t-1}+\alpha_d(v_t-\hat v_{t-1})
```

`f_d` 即配置项 `derivative_cutoff_hz`（速度估计的低通截止频率），`\hat v_t` 是平滑后的速度。

**第三步：根据速度动态计算截止频率。**

```math
f_{c,t}=f_{\min}+\beta|\hat v_t|
```

- `f_{\min}`：`min_cutoff_hz`，几乎静止时的最小截止频率。
- `\beta`：`beta`，速度越大时，截止频率提升得有多快。
- 如果配置了 `max_cutoff_hz > 0`，还会限制 `f_{c,t}` 的上限。

**第四步：将截止频率换算为滤波系数。**

```math
\alpha_t=\frac{2\pi f_{c,t}\Delta t}{1+2\pi f_{c,t}\Delta t}
```

等价写法是：

```math
\alpha_t=\frac{\Delta t}{\Delta t+1/(2\pi f_{c,t})}
```

**第五步：计算最终输出。**

```math
\hat x_t=\hat x_{t-1}+\alpha_t(x_t-\hat x_{t-1})
```

综上：

- **变化速度较小** → 截止频率低 → `\alpha` 小 → 抖动抑制更强。
- **变化速度较大** → 截止频率高 → `\alpha` 大 → 位置追踪更及时。

**首次有效输入直接输出原始值**，并将历史速度初始化为零。

### 1.3 二维和三维如何扩展？

本库的 2D/3D 实现不是简单创建多个互不相关的 1D 滤波器，而是**根据向量运动速度计算一组公共的截止频率和 `\alpha`**，再将相同的 `\alpha` 用于所有坐标轴。

二维输入为 `(x,y)`，各轴分别估计和平滑速度后，计算：

```math
s_t=\sqrt{\hat v_x^2+\hat v_y^2}
```

三维输入为 `(x,y,z)`：

```math
s_t=\sqrt{\hat v_x^2+\hat v_y^2+\hat v_z^2}
```

再使用：

```math
f_{c,t}=f_{\min}+\beta s_t
```

这样，所有维度使用相同的平滑强度，方向响应更一致。**适用于各坐标轴单位相同的情形**。如果各轴是不同物理量或者需要分别调参，应使用多个独立的 1D 滤波器。

### 1.4 直观理解

以每秒采样 60 次（`\Delta t \approx 0.0167s`）为例：

| 当前截止频率 | 滤波系数 `\alpha`（约） | 效果 |
| --- | ---: | --- |
| 0.8 Hz | 0.077 | 静止更平滑，运动滞后较大 |
| 8 Hz | 0.456 | 平滑力度降低，跟随更快 |

这只是解释原理的例子。实际每帧的截止频率由配置和速度共同决定，不是固定在上表中。

## 二、快速上手

引入公共头文件：

```c
#include "one_euro_filter.h"
```

### 2.1 一维（1D）：传感器标量数据

```c
oef_config_t cfg = oef_config_default();
cfg.sample_frequency_hz = 50.0f; /* 50 Hz 固定采样率 */

oef_filter_t filter;
if (!oef_init(&filter, &cfg)) {
    /* 处理初始化错误 */
}

float filtered;
if (oef_step(&filter, 25.12f, &filtered)) {
    /* 使用 filtered */
}
```

可编译运行的完整示例：[examples/one_dimensional.c](examples/one_dimensional.c)。

### 2.2 二维（2D）：通用 (x,y) 坐标过滤

```c
oef_config_t cfg = oef_config_default();
cfg.sample_frequency_hz = 60.0f; /* 60 Hz */
cfg.min_cutoff_hz = 0.8f;
cfg.beta = 0.05f; /* 示例坐标单位为像素，参数与单位有关 */

oef_filter2d_t filter;
if (!oef_init2d(&filter, &cfg)) {
    /* 处理初始化错误 */
}

oef_vec2_t raw = {100.7f, 199.6f};
oef_vec2_t smooth;
if (oef_step2d(&filter, raw, &smooth)) {
    /* 使用 smooth.x / smooth.y */
}
```

完整示例：[examples/two_dimensional.c](examples/two_dimensional.c)。

### 2.3 三维（3D）：通用 (x,y,z) 空间坐标过滤

```c
oef_config_t cfg = oef_config_default();
cfg.max_cutoff_hz = 8.0f; /* 可选最大截止频率 */

oef_filter3d_t filter;
if (!oef_init3d(&filter, &cfg)) {
    /* 处理初始化错误 */
}

oef_vec3_t raw = {1.02f, 1.98f, 3.01f};
oef_vec3_t smooth;
float dt_seconds = 0.021f; /* 与前一采样的实际时间间隔（秒） */
if (oef_update3d(&filter, raw, dt_seconds, &smooth)) {
    /* 使用 smooth.x / smooth.y / smooth.z */
}
```

完整示例：[examples/three_dimensional.c](examples/three_dimensional.c)。

## 三、参数配置与调优

调用 `oef_config_default()` 得到**建议起始值**（不是适用于所有设备的最终值）：

| 配置项 | 默认值 | 有效范围 | 作用 |
| --- | ---: | --- | --- |
| `min_cutoff_hz` | `0.8` | 有限数且 > 0 | 越小静止时越稳，但运动跟随越慢 |
| `beta` | `0.5` | 有限数且 ≥ 0 | 越大移动时反应越快，但可能放大噪声 |
| `derivative_cutoff_hz` | `1.0` | 有限数且 > 0 | 控制速度估计的平滑程度 |
| `sample_frequency_hz` | `25.0` | 有限数且 > 0 | 固定频率更新接口 `oef_step*()` 的采样频率 |
| `max_cutoff_hz` | `0.0` | 0 或 ≥ 最小截止频率 | 可选上限，0 表示不限制 |

**注意量纲：** 截止频率以 **Hz** 为单位；`dt_seconds` 以**秒**为单位；速度是**输入坐标单位/秒**，因此 `beta` 的数值与输入单位有关。比如米和像素不能直接照搬同一组 `beta`。

推荐调参步骤：

1. 暂时设置 `beta = 0`，只调 `min_cutoff_hz`，让静止噪声达到满意的平滑程度。
2. 逐步增大 `beta`，直到运动时能及时响应，同时不会放大太多抖动。
3. 如果运动速度的估计反应太剧烈或太慢，再调 `derivative_cutoff_hz`。
4. 如果希望限制极端情况下的跟随速度，设置 `max_cutoff_hz`；最终应使用真实测量数据验证。

## 四、接口与行为说明

| 功能 | 1D | 2D | 3D |
| --- | --- | --- | --- |
| 初始化 | `oef_init` | `oef_init2d` | `oef_init3d` |
| 按实际间隔更新 | `oef_update` | `oef_update2d` | `oef_update3d` |
| 按固定频率更新 | `oef_step` | `oef_step2d` | `oef_step3d` |
| 运行时修改参数 | `oef_set_config` | `oef_set_config2d` | `oef_set_config3d` |
| 重置历史状态 | `oef_reset` | `oef_reset2d` | `oef_reset3d` |

- 初始化、参数修改和更新接口返回 `bool`；非法配置、NaN/Inf、无效 `dt` 或非有限中间结果会返回 `false`，不修改当前滤波器的历史状态和输出参数。
- `set_config` 只替换配置，保留历史；`reset` 仅清空历史，保留配置，下一次有效采样会直接透传。
- 采样间隔不稳定时应使用 `oef_update*()`，传入实际 `dt_seconds > 0`；此时 `sample_frequency_hz` 不参与计算。注意不要因为整数除法而算出零秒。
- **线程安全**：不同实例可以由不同线程独立使用；同一个实例被多个线程读写时，需要外部同步。
- 输入单位切换、追踪不相关的新对象、采集重新开始时，建议调用相应的 `reset`。
- 一欧元滤波擅长**时间方向上的噪声平滑**，不能自行消除异常离群点或上游定位算法的不连续，也无法保证静止输入的输出完全冻结。


### 4.1 完整错误码和健壮性接口（v1.1.0）

为了满足生产级 C 库的可诊断性要求，新增了返回 **`oef_status_t`** 的 `*_ex` API：成功时 `OEF_OK == 0`，错误时返回**库自己定义的负数错误码**，不依赖系统 `errno`。

| 状态码 | 数值 | 说明 |
| --- | ---: | --- |
| `OEF_OK` | 0 | 成功 |
| `OEF_E_NULL` | -1 | 必须提供的指针为 NULL |
| `OEF_E_CONFIG` | -2 | 新配置参数非法 |
| `OEF_E_INPUT` | -3 | 输入包含 NaN/无穷大 |
| `OEF_E_TIME` | -4 | 采样间隔非法（非正数、NaN/Inf） |
| `OEF_E_STATE` | -5 | 未正确初始化或已有状态/生效配置损坏 |
| `OEF_E_RANGE` | -6 | 计算结果超出可保存的 float 数值范围 |

```c
oef_config_t cfg = oef_config_default();
oef_filter2d_t filter;
oef_status_t st = oef_init2d_ex(&filter, &cfg);

oef_vec2_t position = {100.7f, 199.6f};
oef_vec2_t filtered;
if (st == OEF_OK) {
    st = oef_update2d_ex(&filter, position, 1.0f / 60.0f, &filtered);
}
if (st != OEF_OK) {
    /* 可记录 oef_status_name(st) 和 oef_status_message(st)。 */
    /* 两者返回静态字符串，无需释放，不依赖动态内存。 */
} else {
    /* 使用 filtered.x、filtered.y。 */
}
```

**错误处理约定：** 非法配置、空指针、NaN/Inf、无效时间间隔、历史状态异常、计算溢出均不会推进滤波器历史状态，也不会写坏输出。核心计算改用 `double` 中间变量，存回 `float` 前检查是否可表示。调用方必须**先初始化实例**，并确保多个线程不会无保护地同时修改同一个实例。

**向后兼容：** 原 v1.0 版本返回 `bool` 的初始化、参数修改、更新接口全部保留（成功仍返回 `true`）；原 `void` 复位接口也保留。需要精确获知原因时使用对应的 `*_ex` 版本。结构体公开字段布局和旧函数签名不变。

更多内容：[API 契约/错误码说明](docs/API.md)、[变更日志](CHANGELOG.md)、[贡献规范](CONTRIBUTING.md)。所有导出结构体、成员、宏和公共函数都补充了 Doxygen 注释。参考了 [Zephyr API/Doxygen 规范](https://docs.zephyrproject.org/latest/develop/api/design_guidelines.html) 与 [libuv 错误码设计](https://docs.libuv.org/en/stable/errors.html)，**没有添加对两者的依赖**。

### 4.2 单元测试与平台兼容

新增错误码全覆盖测试、异常参数/NULL/NaN/Inf 测试、数值极端情况、状态回滚验证、确定性 25,000 样本压力测试。保留旧 API 回归测试，并运行一维、二维、三维示例作为冒烟测试。CI 支持 Windows、macOS、Ubuntu，另外在 Linux 上进行 **ASan/UBSan** 检测。可通过 `OEF_ENABLE_SANITIZERS=ON` 配置开启检测（Clang/GCC），通过 `OEF_BUILD_DOCS=ON` 生成 Doxygen 文档（需安装 Doxygen）。

## 五、编译、测试和集成

### 使用 CMake（Linux / macOS / Windows）

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --config Debug --parallel
ctest --test-dir build -C Debug --output-on-failure
```

CTest 将执行两个单元测试套件，以及全部三个示例的基本运行测试。

示例可执行文件为：`oef_example_1d`、`oef_example_2d`、`oef_example_3d`。

Linux/macOS 示例运行命令：

```sh
./build/oef_example_1d
./build/oef_example_2d
./build/oef_example_3d
```

Windows 使用多配置生成器时，可执行文件一般位于 `build/Debug/` 下。

### 使用 Makefile（类 Unix 系统）

```sh
make
make test
make example    # 编译并运行 1D / 2D / 3D 三个示例
```

### 直接加入嵌入式工程

只需编译 `src/one_euro_filter.c`，并将 `include/` 加入编译器头文件搜索路径。部分平台需要链接 C 数学库（`-lm`）。

### 安装为 CMake 库

```sh
cmake --install build --prefix /your/install/prefix
```

```cmake
find_package(one_euro_filter 1.0 CONFIG REQUIRED)
target_link_libraries(your_target PRIVATE one_euro_filter::one_euro_filter)
```

## 六、参考文献与开源协议

原始算法由 Casiez、Roussel、Vogel 在 CHI 2012 论文 *1€ Filter: A Simple Speed-based Low-pass Filter for Noisy Input in Interactive Systems* 中提出。论文及原作者资料：[1€ Filter 主页](https://cristal.univ-lille.fr/~casiez/1euro/)。

本库的一维部分采用标准一欧元滤波思想；**二维/三维共享向量速度截止频率**以及**可选最大截止频率**属于本库的实现设计。

本项目采用 [MIT License](LICENSE) 开源。**v1.1.0** 保留 v1.0 所有原函数签名并增加带错误码接口。
