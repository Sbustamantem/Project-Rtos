#include <stdio.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "driver/uart.h"
#include "gpio.h"
#include "ch_duty.h"
#include "uart_lib.h"
#include "led_handler.h"
#include "adc.h"
#include "string.h"

//starting defines 

//for RGB led
#define Red_CH         LEDC_CHANNEL_0
#define Green_CH         LEDC_CHANNEL_1
#define Blue_CH         LEDC_CHANNEL_2
#define Red_GPIO              GPIO_NUM_27
#define Green_GPIO              GPIO_NUM_26
#define Blue_GPIO              GPIO_NUM_25
#define LED_FREQ     1000
#define initial_treshold_m_blue      0.0f
#define initial_treshold_m_green      20.0f
#define initial_treshold_m_red      40.0f
#define initial_treshold_M_blue      20.0f
#define initial_treshold_M_green      40.0f
#define initial_treshold_M_red      1000.0f

//for ADC
#define ADC_CH_NTC   ADC_CHANNEL_5 
#define ADC_CH_Pot   ADC_CHANNEL_3 
#define ADC_Unit      ADC_UNIT_1              
#define ADC_Attenuation     ADC_ATTEN_DB_12
#define NTC_DATA_TYPE true
#define POT_DATA_TYPE false

//for UART
#define UART_BUF_SIZE      1024 
#define Treshold_Led_start 0
#define Treshold_Led_end   2
#define Treshold_Set_start 4
#define Treshold_Set_end   6
#define Treshold_Set_Values_start 8
#define Treshold_Set_Values_end 10

//Global Varaibles 
static QueueHandle_t adc_queue = NULL;
static QueueHandle_t treshold_queue = NULL;

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

//structures for UART and Treshold settings
typedef struct {
    float min_th_red ;
    float max_th_red ;
    float min_th_green ;
    float max_th_green ;
    float min_th_blue ;
    float max_th_blue ;
} treshold_t;

treshold_t tresholds = {
    .min_th_red = initial_treshold_m_red,
    .max_th_red = initial_treshold_M_red,
    .min_th_green = initial_treshold_m_green,
    .max_th_green = initial_treshold_M_green,
    .min_th_blue = initial_treshold_m_blue,
    .max_th_blue = initial_treshold_M_blue  
};

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
SemaphoreHandle_t Mutex_2;

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
    float final_T = 0;
        //ADC data manage 
        while (1) {
            if(xSemaphoreTake(Mutex_1, portMAX_DELAY) == pdTRUE) {
           
            get_raw_data(ntc_adc_handle, &raw_val);
            raw_to_voltage(ntc_adc_handle, raw_val, &voltage_mv);
            
            Vo = voltage_mv / 1000.0;

            Rt = (R2*(Vi+Vo))/Vo;

            T = (beta*T0)/(log(Rt/R0)*T0 + beta);

            final_T = T - 273.15;
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

    adc_data_t pot_handler;
    pot_handler.type = POT_DATA_TYPE;
    pot_handler.value = 0;

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
            pot_handler.value = Pot_Vo;

            xQueueSend(adc_queue, &pot_handler, portMAX_DELAY);
            xSemaphoreGive(Mutex_1);

        }
        vTaskDelay(3000 / portTICK_PERIOD_MS);       
    }
}
//Task To Change The RGB LED Acorddingly 
void RGB_Change_task() {
    adc_data_t received_item;
    treshold_t current_tresholds = {0};
    float current_temperature = 0.0f;
    float current_voltage_pot = 0.0f;
    uint8_t bright_percent = 100; // 0-100% derived from pot, 0=dim, 100=bright
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
                bright_percent = (uint8_t)((current_voltage_pot / 4.8) * 100.0f);// Clamp brightness to 0-100%
                if (bright_percent > 100) bright_percent = 100;
            }
        if(xQueueReceive(treshold_queue, &current_tresholds, portMAX_DELAY) == pdTRUE) {
        }

            led_color_value_t new_led_state;
            if (current_tresholds.min_th_blue <= current_temperature && current_temperature < current_tresholds.max_th_blue) {
                new_led_state = LED_Color_Blue;
            } else if (current_tresholds.min_th_green <= current_temperature && current_temperature < current_tresholds.max_th_green) {
                new_led_state = LED_Color_Green;
            } else if (current_tresholds.min_th_red <= current_temperature && current_temperature < current_tresholds.max_th_red) {
                new_led_state = LED_Color_Red;
            } else {
                new_led_state = LED_Color_Base; // No color condition met, turn off
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
                        rgb_pwm_set_color(&led_rgb, &timer, 100, 100, bright_percent); // R & G off (100% duty), B adjusted
                        break;
                    case LED_Color_Green:
                        rgb_pwm_set_color(&led_rgb, &timer, 100, bright_percent, 100); // R & B off (100% duty), G adjusted
                        break;
                    case LED_Color_Red:
                        rgb_pwm_set_color(&led_rgb, &timer, bright_percent, 100, 100); // G & B off (100% duty), R adjusted
                        break;
                    case LED_Color_Base: 
                        break;
                    default:
                        rgb_pwm_set_color(&led_rgb, &timer, 100, 100, 100); // All off
                        break;
                }
            }

        }

    }
}

void uart_handle_task(){
    uart_init();
    while (1) {
        char buffer[UART_BUF_SIZE];
        size_t str_length = 0;
        str_length = uart_read_string(buffer, UART_BUF_SIZE, false); // Read string from UART
        if (str_length == 12) {
             int new_th_min = 0;
            int new_th_max = 0;
            uint8_t control_value = 0;
            uint8_t min_max_check = 0; // Variable to check if min or max is set
            char mini_buf[Treshold_Set_end-Treshold_Set_start + 1];
            char mini_buf2[Treshold_Set_end-Treshold_Set_start + 1]; // Buffer to hold the RGB settings
            buffer[str_length] = '\0'; // Null-terminate the string
            if (buffer[Treshold_Led_end-1] == 'D' && buffer[Treshold_Led_end-2] == 'L') {
                    switch (buffer[Treshold_Led_end]) {
                    {
                    case 'R':
                        control_value = LED_Color_Red;
                        break;
                    case 'G':
                        control_value = LED_Color_Green;
                        break;
                    case 'B':
                        control_value = LED_Color_Blue;
                        break;    
                    default:
                        return; // Invalid color, exit the loop
                        break;
                    }
                memcpy (mini_buf, &buffer[Treshold_Set_start], sizeof(mini_buf));
                memcpy (mini_buf2, &buffer[Treshold_Set_Values_start], sizeof(mini_buf2));
                mini_buf[sizeof(mini_buf)] = '\0'; // Null-terminate the string
                if (strcmp(mini_buf, "min") == 0) {
                    new_th_min = atoi(mini_buf2);
                    min_max_check |= 0x01; // Set min flag
                }
                else if (strcmp(mini_buf, "max") == 0) {
                    new_th_max = atoi(mini_buf2);
                    min_max_check |= 0x02; // Set max flag
                }
                
                if (xSemaphoreTake(Mutex_2, portMAX_DELAY) == pdTRUE) {
                if (min_max_check == 0x01) {
                    switch (control_value)
                    {
                    case LED_Color_Red:
                        tresholds.min_th_red = new_th_min;
                        break;
                    case LED_Color_Green:
                        tresholds.min_th_green = new_th_min;
                        break;
                    case LED_Color_Blue:
                        tresholds.min_th_blue = new_th_min;
                        break;
                    default:
                        break;
                    } 
                    }
                    // Parse the new thresholds from the buffer
                    else if (min_max_check == 0x02) {
                        switch (control_value)
                        {
                        case LED_Color_Red:
                            tresholds.max_th_red = new_th_max;
                            break;
                        case LED_Color_Green:
                        tresholds.max_th_green = new_th_max;
                        break;
                    case LED_Color_Blue:
                        tresholds.max_th_blue = new_th_max;
                        break;
                    default:
                        break;
                    }
                }

                    xQueueSend(treshold_queue, &tresholds, portMAX_DELAY);
                    xSemaphoreGive(Mutex_2);
                }
               
            }
        }
    }
    vTaskDelay(100 / portTICK_PERIOD_MS);
}
}


void app_main(void)
{
    Mutex_1 = xSemaphoreCreateMutex();
    Mutex_2 = xSemaphoreCreateMutex();
    pwm_timer_init(&timer);
    rgb_pwm_init(&led_rgb, &timer); //PWM setting

    if (Mutex_1 != NULL && Mutex_2 != NULL) {
        printf("Mutex created successfully\n");

        adc_queue = xQueueCreate(10, sizeof(adc_data_t));
        treshold_queue = xQueueCreate(10, sizeof(treshold_t));
        xQueueSend(treshold_queue, &tresholds, portMAX_DELAY);
       

        xTaskCreate(ntc_set, "ntc_task", 4098, NULL, 4, NULL);
        xTaskCreate(pot_set, "pot_task", 4098, NULL, 4, NULL);
        xTaskCreate(uart_handle_task, "uart_handle_task", 4098, NULL, 4, NULL);
        xTaskCreate(RGB_Change_task, "handle_duty_task", 4098, NULL, 3, NULL);

    } else {
        printf("Failed to create mutex\n");
        return;
    }    

    printf("-------------------Done-------------------");
}
