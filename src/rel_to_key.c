#define DT_DRV_COMPAT zmk_input_processor_xy_mapper

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/dt-bindings/input/input-event-codes.h>
#include <stdlib.h>
#include <limits.h>

#include <zmk/keymap.h>
#include <zmk/event_manager.h>
#include <zmk/events/keycode_state_changed.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

/* Accumulators */
struct xy_mapper_data {
    int32_t x;
    int32_t y;
};

/* Config: threshold + key mapping */
struct xy_mapper_config {
    int32_t threshold;
    int invert_x;
    int invert_y;
    uint32_t key_pos_x; // Key for +X
    uint32_t key_neg_x; // Key for -X
    uint32_t key_pos_y; // Key for +Y
    uint32_t key_neg_y; // Key for -Y
};

/* Get threshold */
static int32_t xy_mapper_get_threshold(const struct device *dev,
                                       struct xy_mapper_data *data,
                                       const struct xy_mapper_config *config) {
    int32_t threshold = config->threshold;
    if (threshold <= 0) {
        LOG_WRN("%s: invalid threshold %d, clamping to 1", dev->name, threshold);
        threshold = 1;
    }
    return threshold;
}

/* Event handler */
static int xy_mapper_handle_event(
    const struct device *dev, struct input_event *event, uint32_t param1,
    uint32_t param2, struct zmk_input_processor_state *state) {

    struct xy_mapper_data *data = dev->data;
    const struct xy_mapper_config *config = dev->config;

    int32_t threshold = xy_mapper_get_threshold(dev, data, config);
    bool invert_x = config->invert_x != 0;
    bool invert_y = config->invert_y != 0;

    if (event->type != INPUT_EV_REL)
        return ZMK_INPUT_PROC_CONTINUE;

    // Accumulate movement
    if (event->code == INPUT_REL_X) {
        data->x += event->value;
    } else if (event->code == INPUT_REL_Y) {
        data->y += event->value;
    } else {
        return ZMK_INPUT_PROC_CONTINUE;
    }

    // Handle X axis
    while (abs(data->x) >= threshold) {
        uint32_t key = (data->x > 0) ? config->key_pos_x : config->key_neg_x;
        if (invert_x) key = (key == config->key_pos_x) ? config->key_neg_x : config->key_pos_x;
        data->x += (data->x > 0) ? -threshold : threshold;

        ZMK_EVENT_RAISE(zmk_keycode_state_changed_from_keycode(key, true));
        ZMK_EVENT_RAISE(zmk_keycode_state_changed_from_keycode(key, false));
    }

    // Handle Y axis
    while (abs(data->y) >= threshold) {
        uint32_t key = (data->y > 0) ? config->key_pos_y : config->key_neg_y;
        if (invert_y) key = (key == config->key_pos_y) ? config->key_neg_y : config->key_pos_y;
        data->y += (data->y > 0) ? -threshold : threshold;

        ZMK_EVENT_RAISE(zmk_keycode_state_changed_from_keycode(key, true));
        ZMK_EVENT_RAISE(zmk_keycode_state_changed_from_keycode(key, false));
    }

    // Stop original event from propagating
    event->value = 0;

    return ZMK_INPUT_PROC_CONTINUE;
}

/* Driver API */
static struct zmk_input_processor_driver_api xy_mapper_driver_api = {
    .handle_event = xy_mapper_handle_event,
};

/* Device instance macro */
#define XY_MAPPER_INST(n) \
  static struct xy_mapper_data xy_mapper_data_##n = {0}; \
  static const struct xy_mapper_config xy_mapper_config_##n = { \
      .threshold = DT_INST_PROP(n, threshold), \
      .invert_x = DT_INST_PROP(n, invert_x), \
      .invert_y = DT_INST_PROP(n, invert_y), \
      .key_pos_x = DT_INST_PROP(n, key_pos_x), \
      .key_neg_x = DT_INST_PROP(n, key_neg_x), \
      .key_pos_y = DT_INST_PROP(n, key_pos_y), \
      .key_neg_y = DT_INST_PROP(n, key_neg_y), \
  }; \
  DEVICE_DT_INST_DEFINE(n, NULL, NULL, \
                        &xy_mapper_data_##n, &xy_mapper_config_##n, \
                        POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, \
                        &xy_mapper_driver_api);

DT_INST_FOREACH_STATUS_OKAY(XY_MAPPER_INST)
