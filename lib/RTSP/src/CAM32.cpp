#include "CAM32.h"
#define TAG "CAM32"

camera_config_t esp32cam_aithinker_config{

    .pin_pwdn = 32,
    .pin_reset = -1,
    .pin_xclk = 0,
    .pin_sscb_sda = 26,
    .pin_sscb_scl = 27,
    .pin_d7 = 35,
    .pin_d6 = 34,
    .pin_d5 = 39,
    .pin_d4 = 36,
    .pin_d3 = 21,
    .pin_d2 = 19,
    .pin_d1 = 18,
    .pin_d0 = 5,
    .pin_vsync = 25,
    .pin_href = 23,
    .pin_pclk = 22,
    .xclk_freq_hz = 20000000,
    .ledc_timer = LEDC_TIMER_1,
    .ledc_channel = LEDC_CHANNEL_1,
    .pixel_format = PIXFORMAT_JPEG,
    // .frame_size = FRAMESIZE_UXGA, // needs 234K of framebuffer space
    // .frame_size = FRAMESIZE_SXGA, // needs 160K for framebuffer
    // .frame_size = FRAMESIZE_XGA, // needs 96K or even smaller FRAMESIZE_SVGA - can work if using only 1 fb
    .frame_size = FRAMESIZE_SVGA,
    .jpeg_quality = 12, //0-63 lower numbers are higher quality
    .fb_count = 2       // if more than one i2s runs in continous mode.  Use only with jpeg
};

void CAM32::done(void)
{
    if (fb) {
        //return the frame buffer back to the driver for reuse
        esp_camera_fb_return(fb);
        fb = NULL;
    }
}

void CAM32::run(void)
{
    if (fb)
        //return the frame buffer back to the driver for reuse
        esp_camera_fb_return(fb);

    fb = esp_camera_fb_get();
}

void CAM32::runIfNeeded(void)
{
    if (!fb)
        run();
}

int CAM32::getWidth(void)
{
    runIfNeeded();
    return fb->width;
}

int CAM32::getHeight(void)
{
    runIfNeeded();
    return fb->height;
}

size_t CAM32::getSize(void)
{
    runIfNeeded();
    if (!fb)
        return 0; // FIXME - this shouldn't be possible but apparently the new cam board returns null sometimes?
    return fb->len;
}

uint8_t *CAM32::getfb(void)
{
    runIfNeeded();
    if (!fb)
        return NULL; // FIXME - this shouldn't be possible but apparently the new cam board returns null sometimes?

    return fb->buf;
}

framesize_t CAM32::getFrameSize(void)
{
    return _cam_config.frame_size;
}

void CAM32::setFrameSize(framesize_t size)
{
    _cam_config.frame_size = size;
}

pixformat_t CAM32::getPixelFormat(void)
{
    return _cam_config.pixel_format;
}

void CAM32::setPixelFormat(pixformat_t format)
{
    switch (format)
    {
    case PIXFORMAT_RGB565:
    case PIXFORMAT_YUV422:
    case PIXFORMAT_GRAYSCALE:
    case PIXFORMAT_JPEG:
        _cam_config.pixel_format = format;
        break;
    default:
        _cam_config.pixel_format = PIXFORMAT_GRAYSCALE;
        break;
    }
}

esp_err_t CAM32::init(camera_config_t config)
{
    memset(&_cam_config, 0, sizeof(_cam_config));
    memcpy(&_cam_config, &config, sizeof(config));

    esp_err_t err = esp_camera_init(&_cam_config);
    if (err != ESP_OK)
    {
        printf("Camera probe failed with error 0x%x", err);
        return err;
    }
    // ESP_ERROR_CHECK(gpio_install_isr_service(0));

    return ESP_OK;
}
