#include "soc/rtc_cntl_reg.h"
#include "soc/rtc_io_reg.h"
#include "soc/soc_ulp.h"

.set dht_pin, 10

    .bss
    .global payload
payload:
    .long 0

    .text
    .global entry
entry:
    WRITE_RTC_REG(RTC_GPIO_ENABLE_W1TS_REG, RTC_GPIO_ENABLE_W1TS_S + dht_pin, 1, 1)

    WRITE_RTC_REG(RTC_GPIO_OUT_W1TC_REG, RTC_GPIO_OUT_DATA_W1TC_S + dht_pin, 1, 1)

    move r1, 36000
delay_18ms:
    sub r1, r1, 1
    jump delay_18ms, gt

    WRITE_RTC_REG(RTC_GPIO_ENABLE_W1TC_REG, RTC_GPIO_ENABLE_W1TC_S + dht_pin, 1, 1)

wait_for_handshake:
    READ_RTC_REG(RTC_GPIO_IN_REG, RTC_GPIO_IN_NEXT_S + dht_pin, 1)
    and r0, r0, 1
    jump wait_for_handshake, eq

    wake
    halt