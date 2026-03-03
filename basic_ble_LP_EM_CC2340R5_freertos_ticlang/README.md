# basic_ble_LP_EM_CC2340R5_freertos_ticlang

## nRF Connect 验证步骤

1. 打开 nRF Connect（Mobile），扫描设备。
   - 设备名以 SysConfig 中配置的 **Device Name** 为准。
2. 连接设备。
3. 展开自定义服务 `0xFFF0`，找到 Characteristic `0xFFF4`（Notify），点击启用 Notify（写入 CCCD `0x2902`）。
4. 观察 `0xFFF4` 上报的 4 字节数据。
   - 该 4 字节为 little-endian 的 `int32_t`。
   - 含义是 **去皮后重量**：`weight_g - tare_offset_g`。
5. 向 Characteristic `0xFFF3`（Write）发送命令并观察现象：
   - `0x01`：tare，记录当前 `weight_g` 为 `tare_offset_g`，后续 `0xFFF4` 上报变为去皮值（通常回到接近 0 后继续变化）。
   - `0x02`：zero，将 `weight_g` 与 `tare_offset_g` 同时清零，后续上报从 0 开始增长。
   - `0x10 xx`：设置上报周期，单位 10ms，实际周期=`xx*10ms`。
     - 示例：`0x10 0x14` => 200ms。
     - 允许范围：`0x05~0xC8`（50ms~2000ms），超出范围命令会被忽略。

