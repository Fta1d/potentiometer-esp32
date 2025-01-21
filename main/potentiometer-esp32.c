#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

#define ADC_CHANNEL ADC_CHANNEL_3
#define GLOBAL_ATTEN ADC_ATTEN_DB_12

#define NEG_TO_POS(x) if(x<0) x=-x

static QueueHandle_t evt_queue = NULL;
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

void adc_calibration_configure (void) {

    adc_cali_line_fitting_config_t cali_conf = {
        .unit_id = ADC_UNIT_2,
        .atten = GLOBAL_ATTEN,
        .bitwidth = ADC_BITWIDTH_12,
    };

    ESP_ERROR_CHECK(adc_cali_create_scheme_line_fitting(&cali_conf, &cali_handle));
}


void potentiometer_task(void* arg) {
    int voltage, current_voltage;
    int prev_voltage = 0, offset = 128, treshold = 30;

    while (1) {
        esp_err_t ret = adc_oneshot_get_calibrated_result(adc2_handle, cali_handle, ADC_CHANNEL, &voltage);
        current_voltage = voltage - offset;

        if (ret == ESP_OK) {
            int diff = current_voltage - prev_voltage;
            NEG_TO_POS(diff);

            if (diff > treshold)
                ESP_LOGI("adc", "ADC%d Channel[%d] Raw Data: %d", ADC_UNIT_2 + 1, ADC_CHANNEL, voltage - offset);
        } else {
            ESP_LOGE("adc", "Failed to read ADC data: %s", esp_err_to_name(ret));
        }

        prev_voltage = current_voltage;

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

void app_main(void) {
    adc_configure();
    adc_calibration_configure();

    evt_queue = xQueueCreate(10, sizeof(int));

    xTaskCreate(potentiometer_task, "potentiometer_task", 2048, NULL, 10, NULL);
}