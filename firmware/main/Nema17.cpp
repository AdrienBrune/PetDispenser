#include "Nema17.hpp"
#include "driver/gpio.h"
#include "driver/rmt_tx.h"
#include "debug.hpp"

#define STEP_PIN    (gpio_num_t)4
#define DIR_PIN     (gpio_num_t)5

static rmt_channel_handle_t ledc_chan = NULL;
static rmt_encoder_handle_t copy_encoder = NULL;

void Nema17::Init()
{
    gpio_set_direction(DIR_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(DIR_PIN, 0); // direction hardcoded

    rmt_tx_channel_config_t tx_chan_config = {
        .gpio_num = STEP_PIN,
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = 1000000, // 1 MHz -> 1 tick = 1 microseconde
        .mem_block_symbols = 64,
        .trans_queue_depth = 4,
    };
    rmt_copy_encoder_config_t copy_encoder_config = {};

    rmt_new_tx_channel(&tx_chan_config, &ledc_chan);
    rmt_new_copy_encoder(&copy_encoder_config, &copy_encoder);
    rmt_enable(ledc_chan);
}

void Nema17::Start(float turns)
{
#define CONTINOUS_MODE 0

    Stop();

    float numberOfRotations = (turns == CONTINOUS_MODE) ? 50 : turns;
    uint32_t stepDelay = _GetMotorStepDelayUs();
    uint32_t numberOfSteps = numberOfRotations * _GetMotorStepsPerTurn();

    DebugLogger::GetInstance().print(DEBUG_MOTOR, DEBUG_INFO, "Motor will start - %.1f rotations - %lu steps - %d RPM", numberOfRotations, numberOfSteps, _GetRpmSpeed());
    DebugLogger::GetInstance().print(DEBUG_MOTOR, DEBUG_INFO, "Estimated time: %d ms", (numberOfSteps * _GetMotorStepDelayUs()) / 1000);
    
    // Start RMT engine
    uint16_t high_time = 2; 
    uint16_t low_time = (stepDelay > high_time) ? (stepDelay - high_time) : 10;
    rmt_symbol_word_t step_symbol = {
        .duration0 = high_time,
        .level0 = 1,
        .duration1 = low_time,
        .level1 = 0,
    };
    rmt_transmit_config_t tx_config = {
        .loop_count = static_cast<int>(numberOfSteps) - 1, 
    };
    rmt_transmit(ledc_chan, copy_encoder, &step_symbol, sizeof(step_symbol), &tx_config);

    DebugLogger::GetInstance().print(DEBUG_MOTOR, DEBUG_INFO, "RMT engine started - low level %d us - high level %d us", low_time, high_time);
}

void Nema17::Stop()
{
    rmt_disable(ledc_chan);
    rmt_enable(ledc_chan);
}