#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>

static pthread_once_t g_init = PTHREAD_ONCE_INIT;
static pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t g_lcd_lock = PTHREAD_MUTEX_INITIALIZER;
static struct light_state_t g_notification;
static struct light_state_t g_battery;

struct led_data {
    const char *max_brightness;
    const char *brightness;
    const char *delay_on;
    const char *delay_off;
    int blink;
};

struct led_data redled = {
    .max_brightness = "/sys/class/leds/red/max_brightness",
    .brightness = "/sys/class/leds/red/brightness",
    .delay_on = "/sys/class/leds/red/delay_on",
    .delay_off = "/sys/class/leds/red/delay_off",
    .blink = 0
};

struct led_data greenled = {
    .max_brightness = "/sys/class/leds/green/max_brightness",
    .brightness = "/sys/class/leds/green/brightness",
    .delay_on = "/sys/class/leds/green/delay_on",
    .delay_off = "/sys/class/leds/green/delay_off",
    .blink = 0
};

struct led_data blueled = {
    .max_brightness = "/sys/class/leds/blue/max_brightness",
    .brightness = "/sys/class/leds/blue/brightness",
    .delay_on = "/sys/class/leds/blue/delay_on",
    .delay_off = "/sys/class/leds/blue/delay_off",
    .blink = 0
};

struct led_data lcd = {
    .max_brightness = "/sys/class/leds/lcd_backlight0/max_brightness",
    .brightness = "/sys/class/leds/lcd_backlight0/brightness",
    .delay_on = "/sys/class/leds/lcd_backlight0/delay_on",
    .delay_off = "/sys/class/leds/lcd_backlight0/delay_off",
    .blink = 0
};

void init_globals(void) {
    pthread_mutex_init(&g_lock, NULL);
    pthread_mutex_init(&g_lcd_lock, NULL);
}

static int write_int(const char *path, int val) {
    int fd = open(path, O_WRONLY);
    if (fd < 0) {
        fprintf(stderr, "Failed to open %s\n", path);
        return -1;
    }

    char buffer[20];
    int bytes = snprintf(buffer, sizeof(buffer), "%d\n", val);
    ssize_t amt = write(fd, buffer, (size_t)bytes);
    close(fd);
    return amt == -1 ? -1 : 0;
}

static int read_int(const char *path) {
    char buffer[12];
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "Failed to open %s\n", path);
        return -1;
    }

    int rc = read(fd, buffer, sizeof(buffer) - 1);
    close(fd);
    if (rc <= 0)
        return -1;

    buffer[rc] = 0;
    return strtol(buffer, NULL, 0);
}

static int is_lit(struct light_state_t const *state) {
    return state->color & 0x00ffffff;
}

static int rgb_to_brightness(struct light_state_t const *state) {
    int color = state->color & 0x00ffffff;
    return ((77 * ((color >> 16) & 0x00ff)) +
            (150 * ((color >> 8) & 0x00ff)) +
            (29 * (color & 0x00ff))) >> 8;
}

static int set_light_backlight(struct light_device_t *dev,
                               struct light_state_t const *state) {
    if (!dev)
        return -1;

    int brightness = rgb_to_brightness(state) * 39;
    
    int err = write_int(lcd.brightness, brightness);
    
    return err;
}

static int set_speaker_light_locked(struct light_device_t *dev,
                                    struct light_state_t const *state) {
    if (!dev)
        return -1;

    int red = (state->color >> 16) & 0xFF;
    int green = (state->color >> 8) & 0xFF;
    int blue = state->color & 0xFF;
    int delay_on = state->flashOnMS;
    int delay_off = state->flashOffMS;

    if (delay_on > 0 && delay_off > 0) {
        // Blinking logic
        if (red) {
            write_int(redled.delay_on, delay_on);
            write_int(redled.delay_off, delay_off);
        }
        if (green) {
            write_int(greenled.delay_on, delay_on);
            write_int(greenled.delay_off, delay_off);
        }
        if (blue) {
            write_int(blueled.delay_on, delay_on);
            write_int(blueled.delay_off, delay_off);
        }
    } else {
        // Static color
        write_int(redled.brightness, red);
        write_int(greenled.brightness, green);
        write_int(blueled.brightness, blue);
    }

    return 0;
}

static int close_lights(struct light_device_t *dev) {
    if (dev)
        free(dev);

    return 0;
}

static int open_lights(const struct hw_module_t *module, char const *name,
                       struct hw_device_t **device) {
    int (*set_light)(struct light_device_t *dev,
                     struct light_state_t const *state);

    if (0 == strcmp(LIGHT_ID_BACKLIGHT, name))
        set_light = set_light_backlight;
    else
        return -EINVAL;

    pthread_once(&g_init, init_globals);

    struct light_device_t *dev = malloc(sizeof(struct light_device_t));
    if (!dev)
        return -ENOMEM;

    memset(dev, 0, sizeof(*dev));
    dev->common.tag = HARDWARE_DEVICE_TAG;
    dev->common.version = 0;
    dev->common.module = (struct hw_module_t *)module;
    dev->common.close = (int (*)(struct hw_device_t *))close_lights;
    dev->set_light = set_light;

    *device = (struct hw_device_t *)dev;
    return 0;
}

static struct hw_module_methods_t lights_module_methods = {
    .open = open_lights,
};

struct hw_module_t HAL_MODULE_INFO_SYM = {
    .tag = HARDWARE_MODULE_TAG,
    .version_major = 1,
    .version_minor = 0,
    .id = LIGHTS_HARDWARE_MODULE_ID,
    .name = "Honor 8 LED driver",
    .author = "Honor 8 Dev Team.",
    .methods = &lights_module_methods,
};

