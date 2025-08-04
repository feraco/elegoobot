// Copyright 2015-2016 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#include "esp_http_server.h"
#include "esp_timer.h"
#include "esp_camera.h"
#include "img_converters.h"
#include "camera_index.h"
#include "Arduino.h"

#include "fb_gfx.h"
#include "fd_forward.h"
#include "fr_forward.h"

#define ENROLL_CONFIRM_TIMES 5
#define FACE_ID_SAVE_NUMBER 7

#define FACE_COLOR_WHITE 0x00FFFFFF
#define FACE_COLOR_BLACK 0x00000000
#define FACE_COLOR_RED 0x000000FF
#define FACE_COLOR_GREEN 0x0000FF00
#define FACE_COLOR_BLUE 0x00FF0000
#define FACE_COLOR_YELLOW (FACE_COLOR_RED | FACE_COLOR_GREEN)
#define FACE_COLOR_CYAN (FACE_COLOR_BLUE | FACE_COLOR_GREEN)
#define FACE_COLOR_PURPLE (FACE_COLOR_BLUE | FACE_COLOR_RED)

typedef struct
{
    size_t size;  //number of values used for filtering
    size_t index; //current value index
    size_t count; //value count
    int sum;
    int *values; //array to be filled with values
} ra_filter_t;

typedef struct
{
    httpd_req_t *req;
    size_t len;
} jpg_chunking_t;

#define PART_BOUNDARY "123456789000000000000987654321"
static const char *_STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;

static const char *_STREAM_BOUNDARY = "\r\n";
static const char *_STREAM_PART = "Len: %u\r\n";

static const char *_STREAM_BOUNDARY_test = "\r\n--" PART_BOUNDARY "\r\n";
static const char *_STREAM_PART_test = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

static ra_filter_t ra_filter;
httpd_handle_t stream_httpd = NULL;
httpd_handle_t camera_httpd = NULL;

static mtmn_config_t mtmn_config = {0};
static int8_t detection_enabled = 0;
static int8_t recognition_enabled = 0;
static int8_t is_enrolling = 0;
static face_id_list id_list = {0};

static ra_filter_t *ra_filter_init(ra_filter_t *filter, size_t sample_size)
{
    memset(filter, 0, sizeof(ra_filter_t));
    filter->values = (int *)malloc(sample_size * sizeof(int));
    if (!filter->values)
    {
        return NULL;
    }
    memset(filter->values, 0, sample_size * sizeof(int));

    filter->size = sample_size;
    return filter;
}

static int ra_filter_run(ra_filter_t *filter, int value)
{
    if (!filter->values)
    {
        return value;
    }
    filter->sum -= filter->values[filter->index];
    filter->values[filter->index] = value;
    filter->sum += filter->values[filter->index];
    filter->index++;
    filter->index = filter->index % filter->size;
    if (filter->count < filter->size)
    {
        filter->count++;
    }
    return filter->sum / filter->count;
}

static void rgb_print(dl_matrix3du_t *image_matrix, uint32_t color, const char *str)
{
    fb_data_t fb;
    fb.width = image_matrix->w;
    fb.height = image_matrix->h;
    fb.data = image_matrix->item;
    fb.bytes_per_pixel = 3;
    fb.format = FB_BGR888;
    fb_gfx_print(&fb, (fb.width - (strlen(str) * 14)) / 2, 10, color, str);
}

static int rgb_printf(dl_matrix3du_t *image_matrix, uint32_t color, const char *format, ...)
{
    char loc_buf[64];
    char *temp = loc_buf;
    int len;
    va_list arg;
    va_list copy;
    va_start(arg, format);
    va_copy(copy, arg);
    len = vsnprintf(loc_buf, sizeof(loc_buf), format, arg);
    va_end(copy);
    if (len >= sizeof(loc_buf))
    {
        temp = (char *)malloc(len + 1);
        if (temp == NULL)
        {
            return 0;
        }
    }
    vsnprintf(temp, len + 1, format, arg);
    va_end(arg);
    rgb_print(image_matrix, color, temp);
    if (len > 64)
    {
        free(temp);
    }
    return len;
}

static void draw_face_boxes(dl_matrix3du_t *image_matrix, box_array_t *boxes, int face_id)
{
    int x, y, w, h, i;
    uint32_t color = FACE_COLOR_YELLOW;
    if (face_id < 0)
    {
        color = FACE_COLOR_RED;
    }
    else if (face_id > 0)
    {
        color = FACE_COLOR_GREEN;
    }
    fb_data_t fb;
    fb.width = image_matrix->w;
    fb.height = image_matrix->h;
    fb.data = image_matrix->item;
    fb.bytes_per_pixel = 3;
    fb.format = FB_BGR888;
    for (i = 0; i < boxes->len; i++)
    {
        // rectangle box
        x = (int)boxes->box[i].box_p[0];
        y = (int)boxes->box[i].box_p[1];
        w = (int)boxes->box[i].box_p[2] - x + 1;
        h = (int)boxes->box[i].box_p[3] - y + 1;
        fb_gfx_drawFastHLine(&fb, x, y, w, color);
        fb_gfx_drawFastHLine(&fb, x, y + h - 1, w, color);
        fb_gfx_drawFastVLine(&fb, x, y, h, color);
        fb_gfx_drawFastVLine(&fb, x + w - 1, y, h, color);
#if 0
        // landmark
        int x0, y0, j;
        for (j = 0; j < 10; j+=2) {
            x0 = (int)boxes->landmark[i].landmark_p[j];
            y0 = (int)boxes->landmark[i].landmark_p[j+1];
            fb_gfx_fillRect(&fb, x0, y0, 3, 3, color);
        }
#endif
    }
}
//
static int run_face_recognition(dl_matrix3du_t *image_matrix, box_array_t *net_boxes)
{
    dl_matrix3du_t *aligned_face = NULL;
    int matched_id = 0;

    aligned_face = dl_matrix3du_alloc(1, FACE_WIDTH, FACE_HEIGHT, 3);
    if (!aligned_face)
    {
        Serial.println("Could not allocate face recognition buffer");
        return matched_id;
    }
    if (align_face(net_boxes, image_matrix, aligned_face) == ESP_OK)
    {
        if (is_enrolling == 1)
        {
            int8_t left_sample_face = enroll_face(&id_list, aligned_face);

            if (left_sample_face == (ENROLL_CONFIRM_TIMES - 1))
            {
                Serial.printf("Enrolling Face ID: %d\n", id_list.tail);
            }
            Serial.printf("Enrolling Face ID: %d sample %d\n", id_list.tail, ENROLL_CONFIRM_TIMES - left_sample_face);
            rgb_printf(image_matrix, FACE_COLOR_CYAN, "ID[%u] Sample[%u]", id_list.tail, ENROLL_CONFIRM_TIMES - left_sample_face);
            if (left_sample_face == 0)
            {
                is_enrolling = 0;
                Serial.printf("Enrolled Face ID: %d\n", id_list.tail);
            }
        }
        else
        {
            matched_id = recognize_face(&id_list, aligned_face);
            if (matched_id >= 0)
            {
                Serial.printf("Match Face ID: %u\n", matched_id);
                rgb_printf(image_matrix, FACE_COLOR_GREEN, "Hello Subject %u", matched_id);
            }
            else
            {
                Serial.println("No Match Found");
                rgb_print(image_matrix, FACE_COLOR_RED, "Intruder Alert!");
                matched_id = -1;
            }
        }
    }
    else
    {
        Serial.println("Face Not Aligned");
        //rgb_print(image_matrix, FACE_COLOR_YELLOW, "Human Detected");
    }

    dl_matrix3du_free(aligned_face);
    return matched_id;
}

static size_t jpg_encode_stream(void *arg, size_t index, const void *data, size_t len)
{
    jpg_chunking_t *j = (jpg_chunking_t *)arg;
    if (!index)
    {
        j->len = 0;
    }
    if (httpd_resp_send_chunk(j->req, (const char *)data, len) != ESP_OK)
    {
        return 0;
    }
    j->len += len;
    return len;
}
//图片帧捕获（图片）
static esp_err_t capture_handler(httpd_req_t *req)
{
    camera_fb_t *fb = NULL;
    esp_err_t res = ESP_OK;
    int64_t fr_start = esp_timer_get_time();

    fb = esp_camera_fb_get();
    if (!fb)
    {
        Serial.println("Camera capture failed");
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    httpd_resp_set_type(req, "image/jpeg");
    httpd_resp_set_hdr(req, "Content-Disposition", "inline; filename=capture.jpg");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");

    size_t out_len, out_width, out_height;
    uint8_t *out_buf;
    bool s;
    bool detected = false;
    int face_id = 0;
    if (!detection_enabled || fb->width > 400)
    {
        size_t fb_len = 0;
        if (fb->format == PIXFORMAT_JPEG)
        {
            fb_len = fb->len;
            res = httpd_resp_send(req, (const char *)fb->buf, fb->len);
        }
        else
        {
            jpg_chunking_t jchunk = {req, 0};
            res = frame2jpg_cb(fb, 80, jpg_encode_stream, &jchunk) ? ESP_OK : ESP_FAIL;
            httpd_resp_send_chunk(req, NULL, 0);
            fb_len = jchunk.len;
        }
        esp_camera_fb_return(fb);
        int64_t fr_end = esp_timer_get_time();
        Serial.printf("JPG: %uB %ums\n", (uint32_t)(fb_len), (uint32_t)((fr_end - fr_start) / 1000));
        return res;
    }

    dl_matrix3du_t *image_matrix = dl_matrix3du_alloc(1, fb->width, fb->height, 3);
    if (!image_matrix)
    {
        esp_camera_fb_return(fb);
        Serial.println("dl_matrix3du_alloc failed");
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    out_buf = image_matrix->item;
    out_len = fb->width * fb->height * 3;
    out_width = fb->width;
    out_height = fb->height;

    s = fmt2rgb888(fb->buf, fb->len, fb->format, out_buf);
    esp_camera_fb_return(fb);
    if (!s)
    {
        dl_matrix3du_free(image_matrix);
        Serial.println("to rgb888 failed");
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    box_array_t *net_boxes = face_detect(image_matrix, &mtmn_config);

    if (net_boxes)
    {
        detected = true;
        if (recognition_enabled)
        {
            face_id = run_face_recognition(image_matrix, net_boxes);
        }
        draw_face_boxes(image_matrix, net_boxes, face_id);
        free(net_boxes->score);
        free(net_boxes->box);
        free(net_boxes->landmark);
        free(net_boxes);
    }

    jpg_chunking_t jchunk = {req, 0};
    s = fmt2jpg_cb(out_buf, out_len, out_width, out_height, PIXFORMAT_RGB888, 90, jpg_encode_stream, &jchunk);
    dl_matrix3du_free(image_matrix);
    if (!s)
    {
        Serial.println("JPEG compression failed");
        return ESP_FAIL;
    }

    int64_t fr_end = esp_timer_get_time();
    Serial.printf("FACE: %uB %ums %s%d\n", (uint32_t)(jchunk.len), (uint32_t)((fr_end - fr_start) / 1000), detected ? "DETECTED " : "", face_id);
    return res;
}
//图片帧流（实时视频）AAP
static esp_err_t stream_handler(httpd_req_t *req)
{
    camera_fb_t *fb = NULL;
    esp_err_t res = ESP_OK;
    size_t _jpg_buf_len = 0;
    uint8_t *_jpg_buf = NULL;
    char *part_buf[64];
    dl_matrix3du_t *image_matrix = NULL;
    bool detected = false;
    int face_id = 0;
    int64_t fr_start = 0;
    int64_t fr_ready = 0;
    int64_t fr_face = 0;
    int64_t fr_recognize = 0;
    int64_t fr_encode = 0;

    detection_enabled = 1;
    static int64_t last_frame = 0;
    if (!last_frame)
    {
        last_frame = esp_timer_get_time();
    }

    res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
    if (res != ESP_OK)
    {
        return res;
    }

    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    while (true)
    {
        detected = false;
        face_id = 0;
        fb = esp_camera_fb_get(); //获取一帧图像
        if (!fb)
        {
            Serial.println("Camera capture failed");
            res = ESP_FAIL;
        }
        else
        {
            fr_start = esp_timer_get_time();
            fr_ready = fr_start;
            fr_face = fr_start;
            fr_encode = fr_start;
            fr_recognize = fr_start;
            if (!detection_enabled || fb->width > 400)
            {
                if (fb->format != PIXFORMAT_JPEG)
                {
                    bool jpeg_converted = frame2jpg(fb, 80, &_jpg_buf, &_jpg_buf_len);
                    esp_camera_fb_return(fb);
                    fb = NULL;
                    if (!jpeg_converted)
                    {
                        Serial.println("JPEG compression failed");
                        res = ESP_FAIL;
                    }
                }
                else
                {
                    _jpg_buf_len = fb->len;
                    _jpg_buf = fb->buf;
                }
            }
            else
            {
                image_matrix = dl_matrix3du_alloc(1, fb->width, fb->height, 3);
                if (!image_matrix)
                {
                    Serial.println("dl_matrix3du_alloc failed");
                    res = ESP_FAIL;
                }
                else
                {
                    if (!fmt2rgb888(fb->buf, fb->len, fb->format, image_matrix->item))
                    {
                        Serial.println("fmt2rgb888 failed");
                        res = ESP_FAIL;
                    }
                    else
                    {
                        fr_ready = esp_timer_get_time();
                        box_array_t *net_boxes = NULL;
                        if (detection_enabled)
                        {
                            net_boxes = face_detect(image_matrix, &mtmn_config);
                        }
                        fr_face = esp_timer_get_time();
                        fr_recognize = fr_face;
                        if (net_boxes || fb->format != PIXFORMAT_JPEG)
                        {
                            if (net_boxes)
                            {
                                detected = true;
                                if (recognition_enabled)
                                {
                                    face_id = run_face_recognition(image_matrix, net_boxes);
                                }
                                fr_recognize = esp_timer_get_time();
                                //rgb_printf(image_matrix, FACE_COLOR_GREEN, "Hello Subject %u", face_id);
                                draw_face_boxes(image_matrix, net_boxes, face_id);
                                free(net_boxes->score);
                                free(net_boxes->box);
                                free(net_boxes->landmark);
                                free(net_boxes);
                            }
                            if (!fmt2jpg(image_matrix->item, fb->width * fb->height * 3, fb->width, fb->height, PIXFORMAT_RGB888, 90, &_jpg_buf, &_jpg_buf_len))
                            {
                                Serial.println("fmt2jpg failed");
                                res = ESP_FAIL;
                            }
                            esp_camera_fb_return(fb);
                            fb = NULL;
                        }
                        else
                        {
                            _jpg_buf = fb->buf;
                            _jpg_buf_len = fb->len;
                        }
                        fr_encode = esp_timer_get_time();
                    }
                    dl_matrix3du_free(image_matrix);
                }
            }
        }
        if (res == ESP_OK)
        {
            size_t hlen = snprintf((char *)part_buf, 64, _STREAM_PART_test, _jpg_buf_len);
            res = httpd_resp_send_chunk(req, (const char *)part_buf, hlen);
        }
        if (res == ESP_OK)
        {
            res = httpd_resp_send_chunk(req, (const char *)_jpg_buf, _jpg_buf_len); //原始发送
        }
        if (res == ESP_OK)
        {
            res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY_test, strlen(_STREAM_BOUNDARY_test));
        }

        if (fb)
        {
            esp_camera_fb_return(fb);
            fb = NULL;
            _jpg_buf = NULL;
        }
        else if (_jpg_buf)
        {
            free(_jpg_buf);
            _jpg_buf = NULL;
        }
        if (res != ESP_OK)
        {
            break;
        }
    }
    last_frame = 0;
    return res;
}

static esp_err_t cmd_handler(httpd_req_t *req)
{
    char *buf;
    size_t buf_len;
    char variable[32] = {
        0,
    };
    char value[32] = {
        0,
    };

    buf_len = httpd_req_get_url_query_len(req) + 1;
    if (buf_len > 1)
    {
        buf = (char *)malloc(buf_len);
        if (!buf)
        {
            httpd_resp_send_500(req);
            return ESP_FAIL;
        }
        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK)
        {
            if (httpd_query_key_value(buf, "var", variable, sizeof(variable)) == ESP_OK &&
                httpd_query_key_value(buf, "val", value, sizeof(value)) == ESP_OK)
            {
            }
            else
            {
                free(buf);
                httpd_resp_send_404(req);
                return ESP_FAIL;
            }
        }
        else
        {
            free(buf);
            httpd_resp_send_404(req);
            return ESP_FAIL;
        }
        free(buf);
    }
    else
    {
        httpd_resp_send_404(req);
        return ESP_FAIL;
    }

    int val = atoi(value);
    Serial.println(val);
    Serial.println(variable);
    sensor_t *s = esp_camera_sensor_get();
    int res = 0;

    if (!strcmp(variable, "framesize"))
    {
        if (s->pixformat == PIXFORMAT_JPEG)
            res = s->set_framesize(s, (framesize_t)val);
    }
    else if (!strcmp(variable, "quality"))
        res = s->set_quality(s, val);
    else if (!strcmp(variable, "contrast"))
        res = s->set_contrast(s, val);
    else if (!strcmp(variable, "brightness"))
        res = s->set_brightness(s, val);
    else if (!strcmp(variable, "saturation"))
        res = s->set_saturation(s, val);
    else if (!strcmp(variable, "gainceiling"))
        res = s->set_gainceiling(s, (gainceiling_t)val);
    else if (!strcmp(variable, "colorbar"))
        res = s->set_colorbar(s, val);
    else if (!strcmp(variable, "awb"))
        res = s->set_whitebal(s, val);
    else if (!strcmp(variable, "agc"))
        res = s->set_gain_ctrl(s, val);
    else if (!strcmp(variable, "aec"))
        res = s->set_exposure_ctrl(s, val);
    else if (!strcmp(variable, "hmirror"))
        res = s->set_hmirror(s, val);
    else if (!strcmp(variable, "vflip"))
        res = s->set_vflip(s, val);
    else if (!strcmp(variable, "awb_gain"))
        res = s->set_awb_gain(s, val);
    else if (!strcmp(variable, "agc_gain"))
        res = s->set_agc_gain(s, val);
    else if (!strcmp(variable, "aec_value"))
        res = s->set_aec_value(s, val);
    else if (!strcmp(variable, "aec2"))
        res = s->set_aec2(s, val);
    else if (!strcmp(variable, "dcw"))
        res = s->set_dcw(s, val);
    else if (!strcmp(variable, "bpc"))
        res = s->set_bpc(s, val);
    else if (!strcmp(variable, "wpc"))
        res = s->set_wpc(s, val);
    else if (!strcmp(variable, "raw_gma"))
        res = s->set_raw_gma(s, val);
    else if (!strcmp(variable, "lenc"))
        res = s->set_lenc(s, val);
    else if (!strcmp(variable, "special_effect"))
        res = s->set_special_effect(s, val);
    else if (!strcmp(variable, "wb_mode"))
        res = s->set_wb_mode(s, val);
    else if (!strcmp(variable, "ae_level"))
        res = s->set_ae_level(s, val);
    else if (!strcmp(variable, "face_detect"))
    {
        detection_enabled = val;
        if (!detection_enabled)
        {
            recognition_enabled = 0;
        }
    }
    else if (!strcmp(variable, "face_enroll"))
        is_enrolling = val;
    else if (!strcmp(variable, "face_recognize"))
    {
        recognition_enabled = val;
        if (recognition_enabled)
        {
            detection_enabled = val;
        }
    }
    else
    {
        res = -1;
    }

    if (res)
    {
        return httpd_resp_send_500(req);
    }

    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    return httpd_resp_send(req, NULL, 0);
}

static esp_err_t status_handler(httpd_req_t *req)
{
    static char json_response[1024];

    sensor_t *s = esp_camera_sensor_get();
    char *p = json_response;
    *p++ = '{';

    p += sprintf(p, "\"framesize\":%u,", s->status.framesize);
    p += sprintf(p, "\"quality\":%u,", s->status.quality);
    p += sprintf(p, "\"brightness\":%d,", s->status.brightness);
    p += sprintf(p, "\"contrast\":%d,", s->status.contrast);
    p += sprintf(p, "\"saturation\":%d,", s->status.saturation);
    p += sprintf(p, "\"sharpness\":%d,", s->status.sharpness);
    p += sprintf(p, "\"special_effect\":%u,", s->status.special_effect);
    p += sprintf(p, "\"wb_mode\":%u,", s->status.wb_mode);
    p += sprintf(p, "\"awb\":%u,", s->status.awb);
    p += sprintf(p, "\"awb_gain\":%u,", s->status.awb_gain);
    p += sprintf(p, "\"aec\":%u,", s->status.aec);
    p += sprintf(p, "\"aec2\":%u,", s->status.aec2);
    p += sprintf(p, "\"ae_level\":%d,", s->status.ae_level);
    p += sprintf(p, "\"aec_value\":%u,", s->status.aec_value);
    p += sprintf(p, "\"agc\":%u,", s->status.agc);
    p += sprintf(p, "\"agc_gain\":%u,", s->status.agc_gain);
    p += sprintf(p, "\"gainceiling\":%u,", s->status.gainceiling);
    p += sprintf(p, "\"bpc\":%u,", s->status.bpc);
    p += sprintf(p, "\"wpc\":%u,", s->status.wpc);
    p += sprintf(p, "\"raw_gma\":%u,", s->status.raw_gma);
    p += sprintf(p, "\"lenc\":%u,", s->status.lenc);
    p += sprintf(p, "\"vflip\":%u,", s->status.vflip);
    p += sprintf(p, "\"hmirror\":%u,", s->status.hmirror);
    p += sprintf(p, "\"dcw\":%u,", s->status.dcw);
    p += sprintf(p, "\"colorbar\":%u,", s->status.colorbar);
    p += sprintf(p, "\"face_detect\":%u,", detection_enabled);
    p += sprintf(p, "\"face_enroll\":%u,", is_enrolling);
    p += sprintf(p, "\"face_recognize\":%u", recognition_enabled);
    *p++ = '}';
    *p++ = 0;
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    return httpd_resp_send(req, json_response, strlen(json_response));
}

static esp_err_t index_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html");
    
    // Serve the Blockly interface HTML
    const char* blockly_html = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Smart Robot Car Controller</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif; background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); min-height: 100vh; color: #333; }
        .header { background: rgba(255, 255, 255, 0.95); backdrop-filter: blur(10px); border-bottom: 1px solid rgba(255, 255, 255, 0.2); padding: 1rem 0; position: sticky; top: 0; z-index: 100; }
        .header-content { max-width: 1400px; margin: 0 auto; padding: 0 2rem; display: flex; justify-content: space-between; align-items: center; flex-wrap: wrap; gap: 1rem; }
        .title { font-size: 1.5rem; font-weight: 700; color: #2d3748; display: flex; align-items: center; gap: 0.5rem; }
        .robot-icon { font-size: 1.8rem; }
        .connection-panel { display: flex; align-items: center; gap: 1.5rem; flex-wrap: wrap; }
        .connection-status { display: flex; align-items: center; gap: 0.5rem; font-weight: 500; }
        .status-indicator { width: 12px; height: 12px; border-radius: 50%; background: #10b981; transition: background-color 0.3s ease; }
        .main-content { max-width: 1400px; margin: 0 auto; padding: 2rem; }
        .workspace-container { display: grid; grid-template-columns: 1fr 400px; gap: 2rem; min-height: calc(100vh - 200px); }
        .toolbox-panel { background: rgba(255, 255, 255, 0.95); backdrop-filter: blur(10px); border-radius: 16px; padding: 1.5rem; box-shadow: 0 8px 32px rgba(0, 0, 0, 0.1); }
        .toolbox-panel h3 { margin-bottom: 1rem; color: #2d3748; font-weight: 600; }
        .blockly-workspace { height: 600px; border-radius: 12px; overflow: hidden; border: 2px solid #e2e8f0; }
        .control-panel { display: flex; flex-direction: column; gap: 1.5rem; }
        .panel-section { background: rgba(255, 255, 255, 0.95); backdrop-filter: blur(10px); border-radius: 16px; padding: 1.5rem; box-shadow: 0 8px 32px rgba(0, 0, 0, 0.1); }
        .panel-section h3 { margin-bottom: 1rem; color: #2d3748; font-weight: 600; font-size: 1.1rem; }
        .camera-container { text-align: center; }
        .camera-feed { width: 100%; max-width: 320px; height: 240px; background: #f7fafc; border: 2px solid #e2e8f0; border-radius: 12px; object-fit: cover; margin-bottom: 1rem; }
        .camera-controls { display: flex; gap: 0.5rem; justify-content: center; }
        .camera-btn { padding: 0.5rem 1rem; border: 2px solid #667eea; background: transparent; color: #667eea; border-radius: 8px; font-weight: 500; cursor: pointer; transition: all 0.2s ease; }
        .camera-btn:hover { background: #667eea; color: white; transform: translateY(-1px); }
        .code-container { display: flex; flex-direction: column; gap: 1rem; }
        .code-output { background: #1a202c; color: #e2e8f0; padding: 1rem; border-radius: 8px; font-family: 'Monaco', 'Menlo', 'Ubuntu Mono', monospace; font-size: 0.875rem; line-height: 1.5; min-height: 120px; overflow-x: auto; border: 2px solid #2d3748; }
        .code-controls { display: flex; gap: 0.5rem; }
        .run-btn { padding: 0.5rem 1rem; background: #10b981; color: white; border: none; border-radius: 8px; font-weight: 500; cursor: pointer; transition: all 0.2s ease; flex: 1; }
        .run-btn:hover { background: #059669; transform: translateY(-1px); }
        .stop-btn { padding: 0.5rem 1rem; background: #ef4444; color: white; border: none; border-radius: 8px; font-weight: 500; cursor: pointer; transition: all 0.2s ease; flex: 1; }
        .stop-btn:hover { background: #dc2626; transform: translateY(-1px); }
        .sensor-grid { display: flex; flex-direction: column; gap: 1rem; }
        .sensor-item { display: flex; justify-content: space-between; align-items: center; padding: 0.75rem; background: #f7fafc; border-radius: 8px; border: 1px solid #e2e8f0; }
        .sensor-item label { font-weight: 500; color: #4a5568; }
        .sensor-item span { font-weight: 600; color: #2d3748; font-family: 'Monaco', 'Menlo', 'Ubuntu Mono', monospace; }
        .tracking-sensors { display: flex; gap: 0.5rem; }
        .tracking-sensors span { padding: 0.25rem 0.5rem; background: #e2e8f0; border-radius: 4px; font-size: 0.875rem; min-width: 40px; text-align: center; }
        @media (max-width: 1024px) { .workspace-container { grid-template-columns: 1fr; gap: 1rem; } .header-content { flex-direction: column; text-align: center; } .connection-panel { justify-content: center; } }
        @media (max-width: 640px) { .main-content { padding: 1rem; } }
        .blocklyToolboxDiv { background: #f8fafc !important; border-right: 2px solid #e2e8f0 !important; }
        .blocklyFlyout { background: #ffffff !important; }
        .blocklyMainBackground { stroke: none !important; fill: #ffffff !important; }
    </style>
</head>
<body>
    <div id="app">
        <header class="header">
            <div class="header-content">
                <h1 class="title">
                    <span class="robot-icon">🤖</span>
                    Smart Robot Car Controller
                </h1>
                <div class="connection-panel">
                    <div class="connection-status">
                        <span class="status-indicator"></span>
                        <span>Connected to Robot</span>
                    </div>
                </div>
            </div>
        </header>
        <main class="main-content">
            <div class="workspace-container">
                <div class="toolbox-panel">
                    <h3>Blocks</h3>
                    <div id="blocklyDiv" class="blockly-workspace"></div>
                </div>
                <div class="control-panel">
                    <div class="panel-section">
                        <h3>Camera Feed</h3>
                        <div class="camera-container">
                            <img id="cameraFeed" src="/stream" alt="Camera feed" class="camera-feed">
                            <div class="camera-controls">
                                <button id="startCamera" class="camera-btn">Start Camera</button>
                                <button id="stopCamera" class="camera-btn">Stop Camera</button>
                            </div>
                        </div>
                    </div>
                    <div class="panel-section">
                        <h3>Code Output</h3>
                        <div class="code-container">
                            <pre id="codeOutput" class="code-output"></pre>
                            <div class="code-controls">
                                <button id="runCode" class="run-btn">Run Code</button>
                                <button id="stopCode" class="stop-btn">Stop</button>
                            </div>
                        </div>
                    </div>
                    <div class="panel-section">
                        <h3>Sensor Data</h3>
                        <div class="sensor-grid">
                            <div class="sensor-item">
                                <label>Ultrasonic (cm)</label>
                                <span id="ultrasonicValue">--</span>
                            </div>
                            <div class="sensor-item">
                                <label>Battery (V)</label>
                                <span id="batteryValue">--</span>
                            </div>
                            <div class="sensor-item">
                                <label>Line Tracking</label>
                                <div class="tracking-sensors">
                                    <span id="trackingL">--</span>
                                    <span id="trackingM">--</span>
                                    <span id="trackingR">--</span>
                                </div>
                            </div>
                        </div>
                    </div>
                </div>
            </div>
        </main>
    </div>
    <script src="https://unpkg.com/blockly@10.4.3/blockly.min.js"></script>
    <script src="https://unpkg.com/blockly@10.4.3/javascript.min.js"></script>
    <script>
        // Embedded JavaScript for robot control
        class RobotController {
            constructor() {
                this.connected = true;
                this.sensorData = { ultrasonic: 0, battery: 0, lineTracking: { L: 0, M: 0, R: 0 } };
                this.isExecuting = false;
            }
            
            async sendCommand(command) {
                try {
                    const url = `/test1?var=${encodeURIComponent(JSON.stringify(command))}`;
                    const response = await fetch(url);
                    return response.ok;
                } catch (error) {
                    console.error('Command error:', error);
                    return false;
                }
            }
            
            async executeCommands(commands) {
                if (this.isExecuting) return;
                this.isExecuting = true;
                
                for (const command of commands) {
                    if (!this.isExecuting) break;
                    await this.executeCommand(command);
                    await new Promise(resolve => setTimeout(resolve, 50));
                }
                
                this.isExecuting = false;
            }
            
            async executeCommand(command) {
                if (!command || !command.method) return;
                
                switch (command.method) {
                    case 'moveForward':
                        await this.sendCommand({ N: 1, D1: command.params[0], T1: command.params[1] * 1000 });
                        await new Promise(resolve => setTimeout(resolve, command.params[1] * 1000));
                        break;
                    case 'moveBackward':
                        await this.sendCommand({ N: 2, D1: command.params[0], T1: command.params[1] * 1000 });
                        await new Promise(resolve => setTimeout(resolve, command.params[1] * 1000));
                        break;
                    case 'turnLeft':
                        await this.sendCommand({ N: 3, D1: 200, T1: command.params[0] * 1000 });
                        await new Promise(resolve => setTimeout(resolve, command.params[0] * 1000));
                        break;
                    case 'turnRight':
                        await this.sendCommand({ N: 4, D1: 200, T1: command.params[0] * 1000 });
                        await new Promise(resolve => setTimeout(resolve, command.params[0] * 1000));
                        break;
                    case 'stop':
                        await this.sendCommand({ N: 100 });
                        break;
                    case 'controlServo':
                        await this.sendCommand({ N: 4, D1: command.params[0], D2: command.params[1] });
                        break;
                    case 'setLEDColor':
                        await this.sendCommand({ N: 5, D1: 1, D2: command.params[0], D3: command.params[1], D4: command.params[2] });
                        break;
                    case 'wait':
                        await new Promise(resolve => setTimeout(resolve, command.params[0] * 1000));
                        break;
                }
            }
            
            stopAllCommands() {
                this.isExecuting = false;
                this.sendCommand({ N: 100 });
            }
        }
        
        // Initialize Blockly workspace
        function initBlockly() {
            // Define blocks
            Blockly.Blocks['robot_move_forward'] = {
                init: function() {
                    this.appendDummyInput()
                        .appendField("move forward speed")
                        .appendField(new Blockly.FieldNumber(200, 0, 255), "SPEED")
                        .appendField("for")
                        .appendField(new Blockly.FieldNumber(1, 0.1, 10, 0.1), "TIME")
                        .appendField("seconds");
                    this.setPreviousStatement(true, null);
                    this.setNextStatement(true, null);
                    this.setColour('#4CAF50');
                }
            };
            
            Blockly.Blocks['robot_move_backward'] = {
                init: function() {
                    this.appendDummyInput()
                        .appendField("move backward speed")
                        .appendField(new Blockly.FieldNumber(200, 0, 255), "SPEED")
                        .appendField("for")
                        .appendField(new Blockly.FieldNumber(1, 0.1, 10, 0.1), "TIME")
                        .appendField("seconds");
                    this.setPreviousStatement(true, null);
                    this.setNextStatement(true, null);
                    this.setColour('#4CAF50');
                }
            };
            
            Blockly.Blocks['robot_turn_left'] = {
                init: function() {
                    this.appendDummyInput()
                        .appendField("turn left for")
                        .appendField(new Blockly.FieldNumber(1, 0.1, 10, 0.1), "TIME")
                        .appendField("seconds");
                    this.setPreviousStatement(true, null);
                    this.setNextStatement(true, null);
                    this.setColour('#4CAF50');
                }
            };
            
            Blockly.Blocks['robot_turn_right'] = {
                init: function() {
                    this.appendDummyInput()
                        .appendField("turn right for")
                        .appendField(new Blockly.FieldNumber(1, 0.1, 10, 0.1), "TIME")
                        .appendField("seconds");
                    this.setPreviousStatement(true, null);
                    this.setNextStatement(true, null);
                    this.setColour('#4CAF50');
                }
            };
            
            Blockly.Blocks['robot_stop'] = {
                init: function() {
                    this.appendDummyInput()
                        .appendField("stop robot");
                    this.setPreviousStatement(true, null);
                    this.setNextStatement(true, null);
                    this.setColour('#4CAF50');
                }
            };
            
            Blockly.Blocks['robot_servo_control'] = {
                init: function() {
                    this.appendDummyInput()
                        .appendField("move servo")
                        .appendField(new Blockly.FieldDropdown([["horizontal", "1"], ["vertical", "2"]]), "SERVO")
                        .appendField("to angle")
                        .appendField(new Blockly.FieldNumber(90, 0, 180), "ANGLE");
                    this.setPreviousStatement(true, null);
                    this.setNextStatement(true, null);
                    this.setColour('#FF9800');
                }
            };
            
            Blockly.Blocks['robot_led_color'] = {
                init: function() {
                    this.appendDummyInput()
                        .appendField("set LED color to")
                        .appendField(new Blockly.FieldColour('#ff0000'), "COLOR");
                    this.setPreviousStatement(true, null);
                    this.setNextStatement(true, null);
                    this.setColour('#9C27B0');
                }
            };
            
            Blockly.Blocks['robot_wait'] = {
                init: function() {
                    this.appendDummyInput()
                        .appendField("wait")
                        .appendField(new Blockly.FieldNumber(1, 0.1, 10, 0.1), "TIME")
                        .appendField("seconds");
                    this.setPreviousStatement(true, null);
                    this.setNextStatement(true, null);
                    this.setColour('#607D8B');
                }
            };
            
            // Define generators
            Blockly.JavaScript['robot_move_forward'] = function(block) {
                const speed = block.getFieldValue('SPEED');
                const time = block.getFieldValue('TIME');
                return `robot.moveForward(${speed}, ${time});\n`;
            };
            
            Blockly.JavaScript['robot_move_backward'] = function(block) {
                const speed = block.getFieldValue('SPEED');
                const time = block.getFieldValue('TIME');
                return `robot.moveBackward(${speed}, ${time});\n`;
            };
            
            Blockly.JavaScript['robot_turn_left'] = function(block) {
                const time = block.getFieldValue('TIME');
                return `robot.turnLeft(${time});\n`;
            };
            
            Blockly.JavaScript['robot_turn_right'] = function(block) {
                const time = block.getFieldValue('TIME');
                return `robot.turnRight(${time});\n`;
            };
            
            Blockly.JavaScript['robot_stop'] = function(block) {
                return `robot.stop();\n`;
            };
            
            Blockly.JavaScript['robot_servo_control'] = function(block) {
                const servo = block.getFieldValue('SERVO');
                const angle = block.getFieldValue('ANGLE');
                return `robot.controlServo(${servo}, ${angle});\n`;
            };
            
            Blockly.JavaScript['robot_led_color'] = function(block) {
                const color = block.getFieldValue('COLOR');
                const r = parseInt(color.substr(1, 2), 16);
                const g = parseInt(color.substr(3, 2), 16);
                const b = parseInt(color.substr(5, 2), 16);
                return `robot.setLEDColor(${r}, ${g}, ${b});\n`;
            };
            
            Blockly.JavaScript['robot_wait'] = function(block) {
                const time = block.getFieldValue('TIME');
                return `robot.wait(${time});\n`;
            };
            
            const toolbox = {
                kind: 'categoryToolbox',
                contents: [
                    {
                        kind: 'category',
                        name: 'Movement',
                        colour: '#4CAF50',
                        contents: [
                            { kind: 'block', type: 'robot_move_forward' },
                            { kind: 'block', type: 'robot_move_backward' },
                            { kind: 'block', type: 'robot_turn_left' },
                            { kind: 'block', type: 'robot_turn_right' },
                            { kind: 'block', type: 'robot_stop' }
                        ]
                    },
                    {
                        kind: 'category',
                        name: 'Servo',
                        colour: '#FF9800',
                        contents: [
                            { kind: 'block', type: 'robot_servo_control' }
                        ]
                    },
                    {
                        kind: 'category',
                        name: 'Lighting',
                        colour: '#9C27B0',
                        contents: [
                            { kind: 'block', type: 'robot_led_color' }
                        ]
                    },
                    {
                        kind: 'category',
                        name: 'Control',
                        colour: '#607D8B',
                        contents: [
                            { kind: 'block', type: 'robot_wait' }
                        ]
                    }
                ]
            };
            
            const workspace = Blockly.inject('blocklyDiv', {
                toolbox: toolbox,
                grid: { spacing: 20, length: 3, colour: '#ccc', snap: true },
                zoom: { controls: true, wheel: true, startScale: 1.0, maxScale: 3, minScale: 0.3 },
                trashcan: true,
                sounds: false
            });
            
            return workspace;
        }
        
        // Initialize app
        document.addEventListener('DOMContentLoaded', function() {
            const robotController = new RobotController();
            const workspace = initBlockly();
            let isRunning = false;
            
            function updateCodeOutput() {
                const code = Blockly.JavaScript.workspaceToCode(workspace);
                document.getElementById('codeOutput').textContent = code;
            }
            
            function parseCodeToCommands(code) {
                const commands = [];
                const lines = code.split('\n').filter(line => line.trim());
                
                for (const line of lines) {
                    const trimmed = line.trim();
                    if (trimmed.startsWith('robot.')) {
                        const match = trimmed.match(/robot\.(\w+)\((.*)\)/);
                        if (match) {
                            const [, method, params] = match;
                            const parsedParams = params ? params.split(',').map(p => {
                                const trimmed = p.trim();
                                const num = parseFloat(trimmed);
                                return isNaN(num) ? trimmed : num;
                            }) : [];
                            commands.push({ method, params: parsedParams });
                        }
                    }
                }
                return commands;
            }
            
            workspace.addChangeListener(updateCodeOutput);
            
            document.getElementById('startCamera').addEventListener('click', () => {
                document.getElementById('cameraFeed').src = '/stream';
            });
            
            document.getElementById('stopCamera').addEventListener('click', () => {
                document.getElementById('cameraFeed').src = '';
            });
            
            document.getElementById('runCode').addEventListener('click', async () => {
                if (isRunning) return;
                
                isRunning = true;
                document.getElementById('runCode').disabled = true;
                document.getElementById('stopCode').disabled = false;
                
                try {
                    const code = Blockly.JavaScript.workspaceToCode(workspace);
                    const commands = parseCodeToCommands(code);
                    await robotController.executeCommands(commands);
                } catch (error) {
                    console.error('Error running code:', error);
                } finally {
                    isRunning = false;
                    document.getElementById('runCode').disabled = false;
                    document.getElementById('stopCode').disabled = true;
                }
            });
            
            document.getElementById('stopCode').addEventListener('click', () => {
                robotController.stopAllCommands();
                isRunning = false;
                document.getElementById('runCode').disabled = false;
                document.getElementById('stopCode').disabled = true;
            });
        });
    </script>
</body>
</html>
)rawliteral";
    
    return httpd_resp_send(req, blockly_html, strlen(blockly_html));
}
// //图片帧流（实时视频）Test
// static esp_err_t Test_handler(httpd_req_t *req)
// {
//     camera_fb_t *fb = NULL;
//     esp_err_t res = ESP_OK;
//     size_t _jpg_buf_len = 0;
//     uint8_t *_jpg_buf = NULL;
//     char *part_buf[64];
//     dl_matrix3du_t *image_matrix = NULL;
//     bool detected = false;
//     int face_id = 0;
//     int64_t fr_start = 0;
//     int64_t fr_ready = 0;
//     int64_t fr_face = 0;
//     int64_t fr_recognize = 0;
//     int64_t fr_encode = 0;

//     detection_enabled = 1;
//     static int64_t last_frame = 0;
//     if (!last_frame)
//     {
//         last_frame = esp_timer_get_time();
//     }

//     res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
//     if (res != ESP_OK)
//     {
//         return res;
//     }

//     httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
//     while (true)
//     {
//         detected = false;
//         face_id = 0;
//         fb = esp_camera_fb_get(); //获取一帧图像
//         if (!fb)
//         {
//             Serial.println("Camera capture failed");
//             res = ESP_FAIL;
//         }
//         else
//         {
//             fr_start = esp_timer_get_time();
//             fr_ready = fr_start;
//             fr_face = fr_start;
//             fr_encode = fr_start;
//             fr_recognize = fr_start;
//             if (!detection_enabled || fb->width > 400)
//             {
//                 if (fb->format != PIXFORMAT_JPEG)
//                 {
//                     bool jpeg_converted = frame2jpg(fb, 80, &_jpg_buf, &_jpg_buf_len);
//                     esp_camera_fb_return(fb);
//                     fb = NULL;
//                     if (!jpeg_converted)
//                     {
//                         Serial.println("JPEG compression failed");
//                         res = ESP_FAIL;
//                     }
//                 }
//                 else
//                 {
//                     _jpg_buf_len = fb->len;
//                     _jpg_buf = fb->buf;
//                 }
//             }
//             else
//             {
//                 image_matrix = dl_matrix3du_alloc(1, fb->width, fb->height, 3);
//                 if (!image_matrix)
//                 {
//                     Serial.println("dl_matrix3du_alloc failed");
//                     res = ESP_FAIL;
//                 }
//                 else
//                 {
//                     if (!fmt2rgb888(fb->buf, fb->len, fb->format, image_matrix->item))
//                     {
//                         Serial.println("fmt2rgb888 failed");
//                         res = ESP_FAIL;
//                     }
//                     else
//                     {
//                         fr_ready = esp_timer_get_time();
//                         box_array_t *net_boxes = NULL;
//                         if (detection_enabled)
//                         {
//                             net_boxes = face_detect(image_matrix, &mtmn_config);
//                         }
//                         fr_face = esp_timer_get_time();
//                         fr_recognize = fr_face;
//                         if (net_boxes || fb->format != PIXFORMAT_JPEG)
//                         {
//                             if (net_boxes)
//                             {
//                                 detected = true;
//                                 if (recognition_enabled)
//                                 {
//                                     face_id = run_face_recognition(image_matrix, net_boxes);
//                                 }
//                                 fr_recognize = esp_timer_get_time();
//                                 //rgb_printf(image_matrix, FACE_COLOR_GREEN, "Hello Subject %u", face_id);
//                                 draw_face_boxes(image_matrix, net_boxes, face_id);
//                                 free(net_boxes->score);
//                                 free(net_boxes->box);
//                                 free(net_boxes->landmark);
//                                 free(net_boxes);
//                             }
//                             if (!fmt2jpg(image_matrix->item, fb->width * fb->height * 3, fb->width, fb->height, PIXFORMAT_RGB888, 90, &_jpg_buf, &_jpg_buf_len))
//                             {
//                                 Serial.println("fmt2jpg failed");
//                                 res = ESP_FAIL;
//                             }
//                             esp_camera_fb_return(fb);
//                             fb = NULL;
//                         }
//                         else
//                         {
//                             _jpg_buf = fb->buf;
//                             _jpg_buf_len = fb->len;
//                         }
//                         fr_encode = esp_timer_get_time();
//                     }
//                     dl_matrix3du_free(image_matrix);
//                 }
//             }
//         }
//         if (res == ESP_OK)
//         {
//             size_t hlen = snprintf((char *)part_buf, 64, _STREAM_PART_test, _jpg_buf_len);
//             res = httpd_resp_send_chunk(req, (const char *)part_buf, hlen);
//         }
//         if (res == ESP_OK)
//         {
//             res = httpd_resp_send_chunk(req, (const char *)_jpg_buf, _jpg_buf_len); //原始发送
//         }
//         if (res == ESP_OK)
//         {
//             res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY_test, strlen(_STREAM_BOUNDARY_test));
//         }

//         if (fb)
//         {
//             esp_camera_fb_return(fb);
//             fb = NULL;
//             _jpg_buf = NULL;
//         }
//         else if (_jpg_buf)
//         {
//             free(_jpg_buf);
//             _jpg_buf = NULL;
//         }
//         if (res != ESP_OK)
//         {
//             break;
//         }
//     }
//     last_frame = 0;
//     return res;
// }

static esp_err_t Test1_handler(httpd_req_t *req)
{
    Serial.println("Test1_handler...");
    char *buf;
    size_t buf_len;
    char variable[32] = {
        0,
    };
    char value[32] = {
        0,
    };
    buf_len = httpd_req_get_url_query_len(req) + 1;
    if (buf_len > 1)
    {
        buf = (char *)malloc(buf_len);
        if (!buf)
        {
            httpd_resp_send_500(req);
            return ESP_FAIL;
        }
        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK)
        {
            if (httpd_query_key_value(buf, "var", variable, sizeof(variable)) == ESP_OK)
            {
               // Serial2.println(variable);
            }
            else
            {
                free(buf);
                httpd_resp_send_404(req);
                return ESP_FAIL;
            }
        }
        else
        {
            free(buf);
            httpd_resp_send_404(req);
            return ESP_FAIL;
        }
        free(buf);
    }
    else
    {
        httpd_resp_send_404(req);
        return ESP_FAIL;
    }

    // httpd_resp_send(req, (const char *)"index", 5);
    return ESP_OK;
}

static esp_err_t Test2_handler(httpd_req_t *req)
{
    Serial.println("Test2_handler...");

    //  httpd_resp_send(req, (const char *)"index", 5);
    httpd_resp_send(req, (const char *)"index", 5);
    return ESP_OK;
}
void startCameraServer()
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    httpd_uri_t index_uri = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = index_handler,
        .user_ctx = NULL};

    httpd_uri_t status_uri = {
        .uri = "/status",
        .method = HTTP_GET,
        .handler = status_handler,
        .user_ctx = NULL};

    httpd_uri_t cmd_uri = {
        .uri = "/control",
        .method = HTTP_GET,
        .handler = cmd_handler,
        .user_ctx = NULL};

    httpd_uri_t capture_uri = {
        .uri = "/capture",
        .method = HTTP_GET,
        .handler = capture_handler,
        .user_ctx = NULL};

    httpd_uri_t stream_uri = {
        .uri = "/stream",
        .method = HTTP_GET,
        .handler = stream_handler,
        .user_ctx = NULL};

    httpd_uri_t Test_uri = {
        .uri = "/Test",
        .method = HTTP_GET,
        //.handler = Test_handler,
        .handler = stream_handler,
        .user_ctx = NULL};

    httpd_uri_t Test1_uri = {
        .uri = "/test1",
        .method = HTTP_GET,
        .handler = Test1_handler,
        .user_ctx = NULL};

    httpd_uri_t Test2_uri = {
        .uri = "/test2",
        .method = HTTP_GET,
        .handler = Test2_handler,
        .user_ctx = NULL};

    ra_filter_init(&ra_filter, 20);

    mtmn_config.type = FAST;
    mtmn_config.min_face = 80;
    mtmn_config.pyramid = 0.707;
    mtmn_config.pyramid_times = 4;
    mtmn_config.p_threshold.score = 0.6;
    mtmn_config.p_threshold.nms = 0.7;
    mtmn_config.p_threshold.candidate_number = 20;
    mtmn_config.r_threshold.score = 0.7;
    mtmn_config.r_threshold.nms = 0.7;
    mtmn_config.r_threshold.candidate_number = 10;
    mtmn_config.o_threshold.score = 0.7;
    mtmn_config.o_threshold.nms = 0.7;
    mtmn_config.o_threshold.candidate_number = 1;

    face_id_init(&id_list, FACE_ID_SAVE_NUMBER, ENROLL_CONFIRM_TIMES);
    Serial.printf("Starting web server on port: '%d'\n", config.server_port);
    if (httpd_start(&camera_httpd, &config) == ESP_OK)
    {
        httpd_register_uri_handler(camera_httpd, &index_uri);
        httpd_register_uri_handler(camera_httpd, &cmd_uri);
        httpd_register_uri_handler(camera_httpd, &status_uri);
        httpd_register_uri_handler(camera_httpd, &capture_uri);
        httpd_register_uri_handler(camera_httpd, &Test_uri);
        httpd_register_uri_handler(camera_httpd, &Test1_uri);
        httpd_register_uri_handler(camera_httpd, &Test2_uri);
    }
    config.server_port += 1; //视频流端口
    config.ctrl_port += 1;
    Serial.printf("Starting stream server on port: '%d'\n", config.server_port);
    if (httpd_start(&stream_httpd, &config) == ESP_OK)
    {
        httpd_register_uri_handler(stream_httpd, &stream_uri);
    }
}