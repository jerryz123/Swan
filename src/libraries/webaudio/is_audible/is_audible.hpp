#ifndef BCD754D4_9604_4C5E_A1C0_5B4EE0416E80
#define BCD754D4_9604_4C5E_A1C0_5B4EE0416E80

#include <stdint.h>
#include <stdlib.h>

#include "webaudio.hpp"

typedef struct is_audible_config_s : config_t {
    // 64K samples, 6 channels
    uint32_t data_size;
    uint32_t block_count;
    uint32_t number_of_channels;
} is_audible_config_t;

typedef struct is_audible_input_s : input_t {
    float ***data;
} is_audible_input_t;

typedef struct is_audible_output_s : output_t {
    int *return_value;
} is_audible_output_t;

#endif /* BCD754D4_9604_4C5E_A1C0_5B4EE0416E80 */
