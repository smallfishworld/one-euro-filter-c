# one-euro-filter-c（中文说明）

[English](../README.md)

轻量级、纯 **C99** 的 One Euro Filter（一欧元滤波器）库，用于抑制坐标或传感器测量值的高频抖动，同时在信号快速移动时减少滞后。适用于声像仪 TOP 声源定位、触摸坐标、IMU/其他传感器等。

## 主要特性

- 支持 **1D 单值、2D 坐标、3D 坐标**，无需动态内存与操作系统 API。
- 2D/3D 使用**向量滤波速度的模长**计算共同截止频率，让各轴使用相同的平滑系数。
- 可配置最小截止频率、速度响应系数、速度低通截止频率、固定采样频率、最大截止频率。
- `oef_update*()` 支持变帧率（传入当前实际帧间隔）；`oef_step*()` 使用配置的固定频率。
- 支持运行时热修改配置、复位状态、非法输入拒绝（不更新内部状态和输出）。
- 可用于 Linux、Windows、macOS、RTOS 和裸机 MCU；只依赖 C 标准库数学函数（部分工具链链接时需要 `-lm`）。

## 声像仪 40×32 MAP 接入示例

```c
#include "one_euro_filter.h"

static oef_filter2d_t tracker;

void source_tracker_init(void)
{
    oef_config_t cfg = oef_config_default();
    cfg.min_cutoff_hz = 0.8f;
    cfg.beta = 0.5f;
    cfg.derivative_cutoff_hz = 1.0f;
    cfg.sample_frequency_hz = 25.0f;
    cfg.max_cutoff_hz = 0.0f; /* 0 = 不限制 */
    (void)oef_init2d(&tracker, &cfg);
}

void process_frame(float top_x, float top_y, float dt_seconds)
{
    oef_vec2_t raw = {top_x, top_y};
    oef_vec2_t stable;
    if (oef_update2d(&tracker, raw, dt_seconds, &stable)) {
        /* 将 stable.x / stable.y 用作云图中心与 TOP 标记中心 */
        /* screen_x = stable.x * 640.0f / 40.0f; */
        /* screen_y = stable.y * 480.0f / 32.0f; */
    }
}
```

建议处理顺序：**40×32 MAP → 亚网格 TOP 浮点定位 → 一欧元滤波 → 根据稳定坐标生成云图 → 缩放显示**。不要等放大到 1280×960 后才滤波。

## 参数说明

| 参数 | 可取值 | 影响 |
| --- | --- | --- |
| `min_cutoff_hz` | > 0 | 越小，静止越稳，但跟踪越慢 |
| `beta` | >= 0 | 越大，运动时响应越快，但可能放大噪声造成的小幅速度变化 |
| `derivative_cutoff_hz` | > 0 | 速度估计的一阶低通截止频率；一般从 1 Hz 开始 |
| `sample_frequency_hz` | > 0 | `oef_step*()` 使用的标称刷新率；`oef_update*()` 不使用 |
| `max_cutoff_hz` | 0 或 >= 最小值 | 限制最大响应速度，0 表示关闭上限 |

全部频率单位都是 **Hz**，帧间隔单位是 **秒**。`beta` 随输入坐标单位变化；网格坐标换成像素坐标时须重新调节 `beta`。默认初值为 `0.8 / 0.5 / 1.0 / 25 / 0`，不是经过实际声像仪校准的最终值。

## 接口

- 初始化：`oef_init` / `oef_init2d` / `oef_init3d`
- 变帧率更新：`oef_update` / `oef_update2d` / `oef_update3d`
- 固定帧率更新：`oef_step` / `oef_step2d` / `oef_step3d`
- 修改配置：`oef_set_config` / `oef_set_config2d` / `oef_set_config3d`
- 清空历史：`oef_reset` / `oef_reset2d` / `oef_reset3d`

一个滤波器实例只能由一个线程同时更新；不同实例可以独立并发。声源切换、采集重启、坐标单位切换时调用对应 `reset`。如果输入为 NaN/Inf 或帧间隔错误，更新接口返回 `false`，保留上一次的结果和状态。

## 编译测试

```sh
cmake -S . -B build
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

类 Unix 系统也可执行 `make test`。裸机项目可以直接把 `src/one_euro_filter.c` 和 `include/one_euro_filter.h` 放进工程中编译。

## 调参建议

声像仪云图静止仍抖动时，优先降低 `min_cutoff_hz`；移动滞后时，适当提高 `beta`。记录原始 TOP、滤波 TOP 和屏幕坐标进行对比。请注意：一欧元滤波只解决**时间维度上的坐标抖动**，不能修复跨两个整数网格时不连续的亚网格定位公式，也不能解决声场 MAP 自身的幅值/形状变化。想让显示位置完全不动，可以另加小幅显示死区，但硬死区可能造成慢速移动的阶梯感。

开源协议：MIT，见 [LICENSE](../LICENSE)。
