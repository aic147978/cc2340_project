#include "hx711.h"

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <ti/drivers/GPIO.h>
#include "ti_drivers_config.h"

/*
 * HX711 时序说明：
 * 1. DOUT = 0 表示数据准备好
 * 2. 读取24位数据
 * 3. 再补1个脉冲，选择 A通道、增益128
 *
 * 注意：
 * - SCK 上电默认必须为低
 * - SCK 高电平保持过长可能导致 HX711 掉电
 */

static void HX711_delayShort(void)
{
    /* 这个延时不需要特别精确，只要保证时序别太快即可
     * 如果后面不稳定，可以适当调大循环次数
     */
    volatile uint32_t i;
    for (i = 0; i < 20; i++)
    {
        __asm(" nop");
    }
}

void HX711_init(HX711_Handle_t *hx711)
{
    if (hx711 == NULL)
    {
        return;
    }

    hx711->offset = 0;
    hx711->scale  = 1.0f;

    /* 确保时钟初始为低，避免进入 power down */
    GPIO_write(CONFIG_GPIO_HX711_SCK, 0);
}

bool HX711_isReady(void)
{
    return (GPIO_read(CONFIG_GPIO_HX711_DOUT) == 0);
}

bool HX711_waitReady(uint32_t timeoutLoops)
{
    while (timeoutLoops--)
    {
        if (HX711_isReady())
        {
            return true;
        }
    }
    return false;
}

int32_t HX711_readRaw(void)
{
    uint32_t data = 0;
    uint8_t i;

    /* 等待数据准备好 */
    while (!HX711_isReady())
    {
    }

    /* 读取24位 */
    for (i = 0; i < 24; i++)
    {
        GPIO_write(CONFIG_GPIO_HX711_SCK, 1);
        HX711_delayShort();

        data <<= 1;

        GPIO_write(CONFIG_GPIO_HX711_SCK, 0);
        HX711_delayShort();

        if (GPIO_read(CONFIG_GPIO_HX711_DOUT))
        {
            data++;
        }
    }

    /* 第25个脉冲：A通道，增益128 */
    GPIO_write(CONFIG_GPIO_HX711_SCK, 1);
    HX711_delayShort();
    GPIO_write(CONFIG_GPIO_HX711_SCK, 0);
    HX711_delayShort();

    /* 24位有符号数，做符号扩展 */
    if (data & 0x800000UL)
    {
        data |= 0xFF000000UL;
    }

    return (int32_t)data;
}

int32_t HX711_readAverage(uint8_t times)
{
    int64_t sum = 0;
    uint8_t i;

    if (times == 0)
    {
        times = 1;
    }

    for (i = 0; i < times; i++)
    {
        sum += HX711_readRaw();
    }

    return (int32_t)(sum / times);
}

void HX711_tare(HX711_Handle_t *hx711, uint8_t times)
{
    if (hx711 == NULL)
    {
        return;
    }

    hx711->offset = HX711_readAverage(times);
}

void HX711_setScale(HX711_Handle_t *hx711, float scale)
{
    if ((hx711 == NULL) || (scale == 0.0f))
    {
        return;
    }

    hx711->scale = scale;
}

float HX711_getWeight(HX711_Handle_t *hx711, uint8_t times)
{
    int32_t raw;
    int32_t net;

    if ((hx711 == NULL) || (hx711->scale == 0.0f))
    {
        return 0.0f;
    }

    raw = HX711_readAverage(times);
    net = raw - hx711->offset;

    return ((float)net / hx711->scale);
}

float HX711_calibrate(HX711_Handle_t *hx711, float knownWeight_g, uint8_t times)
{
    int32_t raw;
    int32_t net;

    if ((hx711 == NULL) || (knownWeight_g <= 0.0f))
    {
        return 0.0f;
    }

    raw = HX711_readAverage(times);
    net = raw - hx711->offset;

    hx711->scale = ((float)net / knownWeight_g);

    return hx711->scale;
}


