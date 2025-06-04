/* LEDC (LED Controller) basic example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "led_handler.h"
#include "driver/ledc.h"
#include "esp_err.h"

#define LEDC_TIMER              LEDC_TIMER_0
#define LEDC_MODE               LEDC_LOW_SPEED_MODE
#define LEDC_OUTPUT_IO          (2) // Define the output GPIO
#define LEDC_CHANNEL_Red        LEDC_CHANNEL_0
#define LEDC_CHANNEL_Green      LEDC_CHANNEL_1
#define LEDC_CHANNEL_Blue       LEDC_CHANNEL_2
#define LEDC_DUTY_RES           LEDC_TIMER_13_BIT // Set duty resolution to 13 bits
#define LEDC_FREQUENCY          (4000) // Frequency in Hertz. Set frequency at 4 kHz

/* Warning:
 * For ESP32, ESP32S2, ESP32S3, ESP32C3, ESP32C2, ESP32C6, ESP32H2, ESP32P4 targets,
 * when LEDC_DUTY_RES selects the maximum duty resolution (i.e. value equal to SOC_LEDC_TIMER_BIT_WIDTH),
 * 100% duty cycle is not reachable (duty cannot be set to (2 ** SOC_LEDC_TIMER_BIT_WIDTH)).
 */
void Channels_init(uint8_t RLed, uint8_t GLed, uint8_t BLed)
{
{    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_MODE,
        .duty_resolution  = LEDC_DUTY_RES,
        .timer_num        = LEDC_TIMER,
        .freq_hz          = LEDC_FREQUENCY,  // Set output frequency at 4 kHz
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));}

    ledc_channel_config_t ledc_channel_cfg = {
        .speed_mode     = LEDC_MODE,
        .timer_sel      = LEDC_TIMER,
        .intr_type      = LEDC_INTR_DISABLE,  
        .duty           = 0, // Set duty to 0%
        .hpoint         = 0
    };
    
    ledc_channel_cfg.channel = LEDC_CHANNEL_Red;
    ledc_channel_cfg.gpio_num = RLed;
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel_cfg));
    
    ledc_channel_cfg.channel = LEDC_CHANNEL_Green;
    ledc_channel_cfg.gpio_num = GLed;
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel_cfg));

    ledc_channel_cfg.channel = LEDC_CHANNEL_Blue;
    ledc_channel_cfg.gpio_num = BLed;
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel_cfg));   
    }
void Duties_init(uint8_t RDuty, uint8_t GDuty, uint8_t BDuty)
    {
     uint32_t max_duty = (1 << LEDC_DUTY_RES) - 1;

    // Rojo
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_Red, (RDuty   * max_duty) / 255));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_Red));

    // Verde
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_Green, (GDuty * max_duty) / 255));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_Green));

    // Azul
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_Blue, (BDuty  * max_duty) / 255));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_Blue));
    
    }
