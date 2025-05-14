#include <stdint.h>
#include "driver/ledc.h"
#include "esp_err.h"

void Channels_init(uint8_t RLed, uint8_t GLed, uint8_t BLed);

void Duties_init(uint8_t RDuty, uint8_t GDuty, uint8_t BDuty);

