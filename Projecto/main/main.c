#include <stdio.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "driver/uart.h"
#include "gpio.h"
#include "ch_duty.h"
#include "adc.h"

//starting defines 

//for RGB led
#define Red_CH         LEDC_CHANNEL_0
#define Green_CH         LEDC_CHANNEL_1
#define Blue_CH         LEDC_CHANNEL_2
#define Red_GPIO              GPIO_NUM_27
#define Green_GPIO              GPIO_NUM_26
#define Blue_GPIO              GPIO_NUM_25
#define LED_FREQ     1000

//for ADC
#define ADC_CH_NTC   ADC_CHANNEL_5 
#define ADC_CH_Pot   ADC_CHANNEL_3 
#define ADC_Unit      ADC_UNIT_1              
#define ADC_Attenuation     ADC_ATTEN_DB_12
#define NTC_DATA_TYPE true
#define POT_DATA_TYPE false

//Global Varaibles 
static QueueHandle_t adc_queue = NULL;

//structures for ADC settings
adc_config_t ntc_adc_conf = {
    .unit_id = ADC_Unit,
    .channel = ADC_CH_NTC,
    .atten = ADC_Attenuation,
    .bitwidth = ADC_BITWIDTH_12,
};
adc_channel_handle_t ntc_adc_handle = NULL;

adc_config_t pot_adc_conf = {
    .unit_id = ADC_Unit,
    .channel = ADC_CH_Pot,
    .atten = ADC_Attenuation,
    .bitwidth = ADC_BITWIDTH_12,
};
adc_channel_handle_t pot_adc_handle = NULL;

typedef struct {
    float value;
    bool type;   //flase pot, true ntc
}adc_data_t;



//initial PWM configuration
pwm_timer_config_t timer = {.frequency_hz = 1000, .resolution_bit = LEDC_TIMER_10_BIT, .timer_num = LEDC_TIMER_0};
rgb_pwm_t led_rgb = {
    .red   = { .channel = Red_CH, .gpio_num = Red_GPIO, .duty_percent = 0 },
    .green = { .channel = Green_CH, .gpio_num = Green_GPIO, .duty_percent = 0 },
    .blue  = { .channel = Blue_CH, .gpio_num = Blue_GPIO, .duty_percent = 0 }
};

// LED Color Number
typedef enum {
    LED_Color_Blue,
    LED_Color_Green,
    LED_Color_Red,
    LED_Color_Base // For initial or off state, or if no conditions are met
} led_color_value_t;


//semaphoreHandler 
SemaphoreHandle_t Mutex_1;

//tasks

// ntc measure
void ntc_set(void *pvParameters) {
    (void)pvParameters; 

    adc_data_t ntc_item;
    ntc_item.type = NTC_DATA_TYPE;
    ntc_item.value = 0;

    const float beta = 3950.0;
    const float R0 = 10000.0;
    const float T0 =  298.15;
    const float R2 = 1000.0;

    const float Vi = 4.8;
    float Vo = 0; 
    float Rt = 0;
    float T = 0;

    // ADC for ntc
    set_adc(&ntc_adc_conf, &ntc_adc_handle);

    int raw_val = 0;
    int voltage_mv = 0;
        //ADC data manage 
        while (1) {
            if(xSemaphoreTake(Mutex_1, portMAX_DELAY) == pdTRUE) {
           
            get_raw_data(ntc_adc_handle, &raw_val);
            raw_to_voltage(ntc_adc_handle, raw_val, &voltage_mv);
            
            float Vo = voltage_mv / 1000.0;

            float Rt = (R2*(Vi+Vo))/Vo;

            float T = (beta*T0)/(log(Rt/R0)*T0 + beta);

            float final_T = T - 273.15;
            printf("T: %.2f °C\n------------------------\n", final_T);

            ntc_item.value = final_T;
            xQueueSend(adc_queue, &ntc_item, portMAX_DELAY);
            xSemaphoreGive(Mutex_1);
        }
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

//task for the potentiometer management

//declaring the type of Vout Of associated to the potetiontiometer
float Pot_Vo;
void pot_set(void *pvParameters) {
    (void)pvParameters; 

    adc_data_t pot_item;
    pot_item.type = POT_DATA_TYPE;
    pot_item.value = 0;

    set_adc(&pot_adc_conf, &pot_adc_handle);
    int raw_val = 0;
    int voltage_mv_pot = 0;    
    //ADC data manage
    while (1)
    {
        if(xSemaphoreTake(Mutex_1, portMAX_DELAY) == pdTRUE) {
            get_raw_data(pot_adc_handle, &raw_val);
            raw_to_voltage(pot_adc_handle, raw_val, &voltage_mv_pot);
            Pot_Vo = voltage_mv_pot / 1000.0;

            printf("Vout: %.2fV\n------------------------\n", Pot_Vo); 
            pot_item.value = Pot_Vo;

            xQueueSend(adc_queue, &Pot_Vo, portMAX_DELAY);
            xSemaphoreGive(Mutex_1);

        }
        vTaskDelay(30 / portTICK_PERIOD_MS);       
    }
}
//Task To Change The RGB LED Acorddingly 
void RGB_Change_task() {
    adc_data_t received_item;
    float current_temperature = 0.0f;
    float current_voltage_pot = 0.0f;
    uint8_t raw_brightness_percent = 0; // 0-100% derived from pot, 0=dim, 100=bright
    uint8_t common_anode_correction = 100; // 0-100%, 0=bright (anode), 100=dim (anode)
    led_color_value_t current_led_state = LED_Color_Base; // Initial state

    rgb_pwm_set_color(&led_rgb, &timer, 100, 100, 100); // Initial state: all off

    while (1)
    {
        if(xQueueReceive(adc_queue, &received_item, portMAX_DELAY) == pdTRUE)
        {
            if(received_item.type == NTC_DATA_TYPE) {
                current_temperature = received_item.value;
            
            } else if(received_item.type == POT_DATA_TYPE) {
                current_voltage_pot = received_item.value;
                raw_brightness_percent = (uint8_t)((current_voltage_pot / 4.8) * 100.0f);
                // Clamp brightness to 0-100%
                if (raw_brightness_percent > 100) raw_brightness_percent = 100;
                
                common_anode_correction = 100 - raw_brightness_percent;
            }

            led_color_value_t new_led_state;
            if (current_temperature < 20.0f) {
                new_led_state = LED_Color_Blue;
            } else if (current_temperature >= 20.0f && current_temperature < 40.0f) {
                new_led_state = LED_Color_Green;
            } else { // current_temperature >= 40.0f
                new_led_state = LED_Color_Red;
            }

            // Only update LEDs if state or brightness has changed significantly
            // This prevents continuous calls to set_color if values are stable.
            // A simple threshold could be added for brightness, but for now, any change triggers update.
            if (new_led_state != current_led_state || // Color state changed
                (received_item.type == POT_DATA_TYPE) // Or only potentiometer value changed, re-apply brightness
               )
            {
                current_led_state = new_led_state; // Update the current state

                switch (current_led_state) {
                    case LED_Color_Blue:
                        rgb_pwm_set_color(&led_rgb, &timer, 100, 100, common_anode_correction); // R & G off (100% duty), B adjusted
                        break;
                    case LED_Color_Green:
                        rgb_pwm_set_color(&led_rgb, &timer, 100, common_anode_correction, 100); // R & B off (100% duty), G adjusted
                        break;
                    case LED_Color_Red:
                        rgb_pwm_set_color(&led_rgb, &timer, common_anode_correction, 100, 100); // G & B off (100% duty), R adjusted
                        break;
                    case LED_Color_Base: 
                    default:
                        rgb_pwm_set_color(&led_rgb, &timer, 100, 100, 100); // All off
                        break;
                }
            }

        }

    }
}

void app_main(void)
{
    Mutex_1 = xSemaphoreCreateMutex();
    pwm_timer_init(&timer);
    rgb_pwm_init(&led_rgb, &timer); //PWM setting

    if (Mutex_1 != NULL) {
        printf("Mutex created successfully\n");

        adc_queue = xQueueCreate(10, sizeof(adc_data_t));
       

        xTaskCreate(ntc_set, "ntc_task", 4098, NULL, 4, NULL);
        xTaskCreate(pot_set, "pot_task", 4098, NULL, 4, NULL);
        xTaskCreate(RGB_Change_task, "handle_duty_task", 4098, NULL, 3, NULL);

    } else {
        printf("Failed to create mutex\n");
        return;
    }    

    printf("-------------------Done-------------------");
}
