#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zmk/input_processor.h>
#include <zmk/input_event.h>
#include <zmk/keycode.h>

struct rel_to_key_cfg {
    uint32_t x_positive_keycode;
    uint32_t x_negative_keycode;
    uint32_t y_positive_keycode;
    uint32_t y_negative_keycode;
    int16_t x_threshold;
    int16_t y_threshold;
};

struct rel_to_key_state {
    int16_t acc_x;
    int16_t acc_y;
};

static int rel_to_key_process(struct zmk_input_processor *processor,
                              struct zmk_input_event *event,
                              struct zmk_input_event *out_event) {
    struct rel_to_key_cfg *cfg = processor->config;
    struct rel_to_key_state *state = processor->state;

    if (event->type != INPUT_EV_REL) {
        return ZMK_INPUT_PROC_CONTINUE;
    }

    if (event->code == INPUT_REL_X) {
        state->acc_x += event->value;
        if (state->acc_x >= cfg->x_threshold) {
            zmk_keycode_state_changed(cfg->x_positive_keycode, true);
            zmk_keycode_state_changed(cfg->x_positive_keycode, false);
            state->acc_x = 0;
        } else if (state->acc_x <= -cfg->x_threshold) {
            zmk_keycode_state_changed(cfg->x_negative_keycode, true);
            zmk_keycode_state_changed(cfg->x_negative_keycode, false);
            state->acc_x = 0;
        }
    }

    if (event->code == INPUT_REL_Y) {
        state->acc_y += event->value;
        if (state->acc_y >= cfg->y_threshold) {
            zmk_keycode_state_changed(cfg->y_positive_keycode, true);
            zmk_keycode_state_changed(cfg->y_positive_keycode, false);
            state->acc_y = 0;
        } else if (state->acc_y <= -cfg->y_threshold) {
            zmk_keycode_state_changed(cfg->y_negative_keycode, true);
            zmk_keycode_state_changed(cfg->y_negative_keycode, false);
            state->acc_y = 0;
        }
    }

    return ZMK_INPUT_PROC_DROP;
}

ZMK_INPUT_PROCESSOR_DEFINE(rel_to_key, rel_to_key_process);