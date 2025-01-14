#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

#define ADC_CHANNEL ADC_CHANNEL_3
#define TRIGGER_PIN GPIO_NUM_15
#define GLOBAL_ATTEN ADC_ATTEN_DB_12

static QueueHandle_t evt_queue = NULL;
static int adc_raw[1][10];
static adc_oneshot_unit_handle_t adc2_handle;
static adc_cali_handle_t cali_handle;

void adc_configure (void) {
    adc_oneshot_unit_init_cfg_t init_cfg = {
        .unit_id = ADC_UNIT_2,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_cfg, &adc2_handle));

    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_12,
        .atten = GLOBAL_ATTEN,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc2_handle, ADC_CHANNEL, &config));
}

void adc_calibration (void) {

    adc_cali_line_fitting_config_t cali_conf = {
        .unit_id = ADC_UNIT_2,
        .atten = GLOBAL_ATTEN,
        .bitwidth = ADC_BITWIDTH_12,
    };

    ESP_ERROR_CHECK(adc_cali_create_scheme_line_fitting(&cali_conf, &cali_handle));
}

// static void IRAM_ATTR trigger_pin_isr_handler(void *arg) {
//     int pin = (int)arg;
//     static uint32_t last_isr_time = 0;
//     uint32_t current_time = xTaskGetTickCountFromISR();
//
//     if (current_time - last_isr_time > 50 / portTICK_PERIOD_MS) {
//         last_isr_time = current_time;
//         xQueueSendFromISR(evt_queue, &pin, NULL);
//     }
// }

void potentiometer_task(void* arg) {
    int pin, voltage;

    while (1) {
            esp_err_t ret = adc_oneshot_get_calibrated_result(adc2_handle, cali_handle, ADC_CHANNEL, &voltage);

            if (ret == ESP_OK) {
                ESP_LOGI("adc", "ADC%d Channel[%d] Raw Data: %d", ADC_UNIT_2 + 1, ADC_CHANNEL, voltage - 128);
            } else {
                ESP_LOGE("adc", "Failed to read ADC data: %s", esp_err_to_name(ret));
            }

            vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void app_main(void) {
    adc_configure();
    adc_calibration();

    // gpio_reset_pin(TRIGGER_PIN);
    // gpio_set_direction(TRIGGER_PIN, GPIO_MODE_INPUT);
    // gpio_set_pull_mode(TRIGGER_PIN, GPIO_PULLUP_ONLY);
    // gpio_set_intr_type(TRIGGER_PIN, GPIO_INTR_NEGEDGE);

    evt_queue = xQueueCreate(10, sizeof(int));

    xTaskCreate(potentiometer_task, "potentiometer_task", 2048, NULL, 10, NULL);

    // gpio_install_isr_service(0);
    // gpio_isr_handler_add(TRIGGER_PIN, trigger_pin_isr_handler, (void*)TRIGGER_PIN);
}