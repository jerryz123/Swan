#ifndef BCD754D4_9604_4C5E_A1C0_5B4EE0416E80
#define BCD754D4_9604_4C5E_A1C0_5B4EE0416E80

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libwebp.hpp"

typedef struct dc_prediction_config_s : config_t {
    // A single 16x16 DCT block
    int num_rows;
    int num_cols;
    int block_size;
    int block_count;
    int *coordinations;
} dc_prediction_config_t;

typedef struct dc_prediction_input_s : input_t {
    uint8_t *src;
    uint8_t **block_src;
} dc_prediction_input_t;

typedef struct dc_prediction_output_s : output_t {
    uint8_t *dst;
    uint8_t **block_dst;
} dc_prediction_output_t;

#endif /* BCD754D4_9604_4C5E_A1C0_5B4EE0416E80 */
