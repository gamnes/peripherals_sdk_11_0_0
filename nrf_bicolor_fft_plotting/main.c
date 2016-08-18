/* Copyright (c) 2014 Nordic Semiconductor. All Rights Reserved.
 *
 * The information contained herein is property of Nordic Semiconductor ASA.
 * Terms and conditions of usage are described in detail in NORDIC
 * SEMICONDUCTOR STANDARD SOFTWARE LICENSE AGREEMENT.
 *
 * Licensees are granted free, non-transferable use of the information. NO
 * WARRANTY of ANY KIND is provided. This heading must NOT be removed from
 * the file.
 *
 */

/** @file
 * @defgroup nrf_adc_example main.c
 * @{
 * @ingroup nrf_adc_example
 * @brief ADC Example Application main file.
 *
 * This file contains the source code for a sample application using ADC.
 *
 * @image html example_board_setup_a.jpg "Use board setup A for this example."
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "nrf.h"
#include "nrf_drv_saadc.h"
#include "nrf_drv_ppi.h"
#include "nrf_drv_timer.h"
#include "boards.h"
#include "app_uart.h"
#include "app_error.h"
#include "nrf_delay.h"
#include "app_util_platform.h"
#include <string.h>

#include "arm_math.h"
#include "arm_const_structs.h"

#define UART_TX_BUF_SIZE 256 /**< UART TX buffer size. */
#define UART_RX_BUF_SIZE 1   /**< UART RX buffer size. */

#define SAMPLES_IN_BUFFER 128
#define FFT_SIZE SAMPLES_IN_BUFFER
volatile uint8_t state = 1;

static const nrf_drv_timer_t   m_timer = NRF_DRV_TIMER_INSTANCE(0);
static nrf_saadc_value_t       m_buffer_pool[2][SAMPLES_IN_BUFFER];
static nrf_ppi_channel_t       m_ppi_channel;
static uint32_t                m_adc_evt_counter;

static float testInput_f32_9khz[SAMPLES_IN_BUFFER * 2];
static float testOutput[SAMPLES_IN_BUFFER];

static float nrf_samples[FFT_SIZE * 2];
static float magnitudes[FFT_SIZE];

//extern float testInput_f32_10khz[SAMPLES_IN_BUFFER * 2];  // For testing


/**
 * @brief UART events handler.
 */
void uart_events_handler(app_uart_evt_t * p_event)
{
}

/**
 * @brief UART initialization.
 */
void uart_config(void)
{
    uint32_t                     err_code;
    const app_uart_comm_params_t comm_params =
    {
        RX_PIN_NUMBER,
        TX_PIN_NUMBER,
        RTS_PIN_NUMBER,
        CTS_PIN_NUMBER,
        APP_UART_FLOW_CONTROL_DISABLED,
        false,
        UART_BAUDRATE_BAUDRATE_Baud38400
    };

    APP_UART_FIFO_INIT(&comm_params,
                       UART_RX_BUF_SIZE,
                       UART_TX_BUF_SIZE,
                       uart_events_handler,
                       APP_IRQ_PRIORITY_LOW,
                       err_code);

    APP_ERROR_CHECK(err_code);
}

void timer_handler(nrf_timer_event_t event_type, void* p_context)
{

}

void saadc_sampling_event_init(void)
{
    ret_code_t err_code;
    err_code = nrf_drv_ppi_init();
    APP_ERROR_CHECK(err_code);

    err_code = nrf_drv_timer_init(&m_timer, NULL, timer_handler);
    APP_ERROR_CHECK(err_code);

    /* setup m_timer for compare event every 1ms */
    //uint32_t ticks = nrf_drv_timer_ms_to_ticks(&m_timer, 400);
    //uint32_t ticks = nrf_drv_timer_ms_to_ticks(&m_timer, 2);
    uint32_t ticks = nrf_drv_timer_us_to_ticks(&m_timer, 104);  // Every 104 uS should give roughly 9615 Hz
    nrf_drv_timer_extended_compare(&m_timer, NRF_TIMER_CC_CHANNEL0, ticks, NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK, false);
    nrf_drv_timer_enable(&m_timer);

    uint32_t timer_compare_event_addr = nrf_drv_timer_compare_event_address_get(&m_timer, NRF_TIMER_CC_CHANNEL0);
    uint32_t saadc_sample_event_addr = nrf_drv_saadc_sample_task_get();

    /* setup ppi channel so that timer compare event is triggering sample task in SAADC */
    err_code = nrf_drv_ppi_channel_alloc(&m_ppi_channel);
    APP_ERROR_CHECK(err_code);
    
    err_code = nrf_drv_ppi_channel_assign(m_ppi_channel, timer_compare_event_addr, saadc_sample_event_addr);
    APP_ERROR_CHECK(err_code);
}

void saadc_sampling_event_enable(void)
{
    ret_code_t err_code = nrf_drv_ppi_channel_enable(m_ppi_channel);
    APP_ERROR_CHECK(err_code);
}

uint8_t samples_ready = false;
void saadc_callback(nrf_drv_saadc_evt_t const * p_event)
{
    if (p_event->type == NRF_DRV_SAADC_EVT_DONE)
    {
        ret_code_t err_code;
     
        //err_code = nrf_drv_saadc_buffer_convert(p_event->data.done.p_buffer, SAMPLES_IN_BUFFER);
        //APP_ERROR_CHECK(err_code);

        //printf("ADC event number: %d\r\n",(int)m_adc_evt_counter);
        samples_ready = true;
        //for (int i = 30; i < SAMPLES_IN_BUFFER; i++)
        //{
        //    printf("%d:%d\r\n", i, p_event->data.done.p_buffer[i]);
        //}
        m_adc_evt_counter++;
    }
}

void saadc_init(void)
{
    ret_code_t err_code;
    nrf_saadc_channel_config_t channel_config =
            NRF_DRV_SAADC_DEFAULT_CHANNEL_CONFIG_SE(NRF_SAADC_INPUT_AIN0);
    err_code = nrf_drv_saadc_init(NULL, saadc_callback);
    APP_ERROR_CHECK(err_code);

    err_code = nrf_drv_saadc_channel_init(0, &channel_config);
    APP_ERROR_CHECK(err_code);

    err_code = nrf_drv_saadc_buffer_convert(m_buffer_pool[0],SAMPLES_IN_BUFFER);
    APP_ERROR_CHECK(err_code);
    
    //err_code = nrf_drv_saadc_buffer_convert(m_buffer_pool[1],SAMPLES_IN_BUFFER);
    //APP_ERROR_CHECK(err_code);
}

/**
 * @brief Function for main application entry.
 */
int main(void)
{
    uint32_t strIndex = 0;
    uint8_t str[64];
    float maxValue;
    uint32_t testIndex = 0;
    
    uart_config();

    //printf("\n\rSAADC HAL simple example.\r\n");
    saadc_sampling_event_init();
    saadc_init();
    saadc_sampling_event_enable();

    app_uart_flush();
    
    uint8_t c = ' ';
    while(1)
    {
        
        uint8_t ret = app_uart_get(&c);
        if (ret != NRF_ERROR_NOT_FOUND) {
            // Check for overflow
            if (strIndex >= sizeof(str) - 1) {
              strIndex = 0;  
            }
            //printf("%c", c);
            // Check for end of command
            if (c != ';') {
                str[strIndex] = c;
                strIndex++;
            }
            else {
                // Parse the command because end of command encountered
                if (strcmp((char*)str, "GET FFT_SIZE") == 0  ||  strcmp((char*)str, "F") == 0) {
                //if (strcmp((char*)str, "F") == 0) {
                    printf("%d\r\n", FFT_SIZE);
                }
                if (strcmp((char*)str, "GET SAMPLE_RATE_HZ") == 0) {
                //if (strcmp((char*)str, "H") == 0) {
                    printf("%d\r\n", 9600);
                }
                if (strcmp((char*)str, "GET MAGNITUDES") == 0  ||  strcmp((char*)str, "M") == 0) {
                //if (strcmp((char*)str, "M") == 0) {
                    for (int i = 0; i < FFT_SIZE; i++) {
                        char array[100];
                        int numWritten = 0;
                        numWritten = sprintf(array, "%f\r\n", magnitudes[i]);
                        for (uint32_t i = 0; i < numWritten; i++) {
                            while(app_uart_put(array[i]) != NRF_SUCCESS);
                        }
                        //printf("%.2f\r\n", magnitudes[i]);
                    }
                }
//                if (strcmp((char*)str, "GET SAMPLE_RATE_HZ") == 0) {
//                    printf("%d\n", 9600);
//                    start_sequence_done += 1;
//                }
                // Clear the command buffer
                memset(str, 0, sizeof(str));
                strIndex = 0;
            }
        }
        //__WFE();
        if (samples_ready) {
            //printf("\n\tSamples are ready to be used.\r\n");
            for (int i = 0; i < FFT_SIZE; i++) {
                nrf_samples[i*2] = (float)m_buffer_pool[0][i];
                //testInput_f32_9khz[i*2] = (i % 2) ? 800 : 400;
                nrf_samples[i*2 + 1] = 0;
            }
            
            arm_cfft_f32(&arm_cfft_sR_f32_len128, nrf_samples, /* ifftFlag */ 0, /* doBitReverse */ 1);
            arm_cmplx_mag_f32(nrf_samples, magnitudes, /* fftsize */ FFT_SIZE);
            //arm_max_f32(magnitudes, FFT_SIZE, &maxValue, &testIndex);
            //printf("\n\rMax Value: %f, at index: %d.\r\n", maxValue, testIndex);
            
            //for (int i = 0; i < SAMPLES_IN_BUFFER; i++) {
                //printf("\n\ri: %d, Value in testOutput: %f\r\n", i, magnitudes[i]);
                //nrf_delay_ms(1);
            //}
            
            samples_ready = false;
            
            ret_code_t err_code;
            err_code = nrf_drv_saadc_buffer_convert(m_buffer_pool[0],SAMPLES_IN_BUFFER);
            APP_ERROR_CHECK(err_code);
        }
    }
}


/** @} */
