#ifndef HX711_H
#define HX711_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    int32_t offset;      // 去皮偏移
    float scale;         // 标定系数：raw/count per gram
} HX711_Handle_t;

/**
 * @brief 初始化 HX711 驱动参数
 * @param hx711 驱动句柄
 */
void HX711_init(HX711_Handle_t *hx711);

/**
 * @brief 判断 HX711 是否准备好（DOUT=0 表示准备好）
 * @return true 已准备好，false 未准备好
 */
bool HX711_isReady(void);

/**
 * @brief 等待 HX711 准备好，带超时
 * @param timeoutLoops 超时循环次数
 * @return true 成功，false 超时
 */
bool HX711_waitReady(uint32_t timeoutLoops);

/**
 * @brief 读取 HX711 24bit 原始值（A通道，128增益）
 * @return 原始 ADC 值
 */
int32_t HX711_readRaw(void);

/**
 * @brief 多次平均读取原始值
 * @param times 读取次数
 * @return 平均后的原始值
 */
int32_t HX711_readAverage(uint8_t times);

/**
 * @brief 去皮
 * @param hx711 驱动句柄
 * @param times 平均次数
 */
void HX711_tare(HX711_Handle_t *hx711, uint8_t times);

/**
 * @brief 设置标定系数
 * @param hx711 驱动句柄
 * @param scale raw/count per gram
 */
void HX711_setScale(HX711_Handle_t *hx711, float scale);

/**
 * @brief 获取净重（单位：g）
 * @param hx711 驱动句柄
 * @param times 平均次数
 * @return 重量，单位 g
 */
float HX711_getWeight(HX711_Handle_t *hx711, uint8_t times);

/**
 * @brief 用已知砝码做标定
 * @param hx711 驱动句柄
 * @param knownWeight_g 已知重量（g）
 * @param times 平均次数
 * @return 标定后的 scale
 */
float HX711_calibrate(HX711_Handle_t *hx711, float knownWeight_g, uint8_t times);

#ifdef __cplusplus
}
#endif

#endif /* HX711_H */

