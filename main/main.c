#include <sys/param.h>
#include <math.h>

#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "protocol_examples_common.h"

#include "driver/pwm.h"

#include <esp_http_server.h>

// channel configuration
#define GPIO_OUTPUT_CHANNEL_RED 0
#define GPIO_OUTPUT_CHANNEL_GREEN 0
#define GPIO_OUTPUT_CHANNEL_BLUE 2

// pwm frequency
#define PWM_PERIOD 1000

// raises the brigtness value from 0-1 to this power
#define BRIGHTNESS_CURVE_CORRECTION 3

static const char *TAG = "rgbstrip";
int color[3];
static httpd_handle_t server = NULL;

const uint32_t pin_num[] = {GPIO_OUTPUT_CHANNEL_RED, GPIO_OUTPUT_CHANNEL_GREEN,
							GPIO_OUTPUT_CHANNEL_BLUE};

uint32_t duties[] = {0, 0, 0};
float phase[] = {0, 0, 0};

void update_strip() {
	duties[0] = PWM_PERIOD - (int)( PWM_PERIOD * pow( (float)color[0] / 0xff, BRIGHTNESS_CURVE_CORRECTION ) );
	duties[1] = PWM_PERIOD - (int)( PWM_PERIOD * pow( (float)color[1] / 0xff, BRIGHTNESS_CURVE_CORRECTION ) );
	duties[2] = PWM_PERIOD - (int)( PWM_PERIOD * pow( (float)color[2] / 0xff, BRIGHTNESS_CURVE_CORRECTION ) );
	pwm_set_duties(duties);
	pwm_start();
}

char *color_to_str() {
	char *value = (char *)malloc(sizeof(char) * (6 + 1)); // 6 char + 1 \0;
	sprintf(value, "%02x%02x%02x", color[0], color[1], color[2]);
	return value;
}

esp_err_t get_handler(httpd_req_t *req) {
	const char *resp_str = color_to_str();
	httpd_resp_send(req, resp_str, 6);

	return ESP_OK;
}

httpd_uri_t get = {.uri = "/", .method = HTTP_GET, .handler = get_handler};

esp_err_t post_handler(httpd_req_t *req) {
	char buf[6];
	int remaining = req->content_len;

	if (remaining != 6)
		return ESP_OK;

	httpd_req_recv(req, buf, 6);

	char r_hex[] = {buf[0], buf[1], '\0'};
	char g_hex[] = {buf[2], buf[3], '\0'};
	char b_hex[] = {buf[4], buf[5], '\0'};

	sscanf(r_hex, "%x", &color[0]);
	sscanf(g_hex, "%x", &color[1]);
	sscanf(b_hex, "%x", &color[2]);

	update_strip();

	httpd_resp_send_chunk(req, NULL, 0);
	return ESP_OK;
}

httpd_uri_t post = {.uri = "/", .method = HTTP_POST, .handler = post_handler};

httpd_handle_t start_webserver(void) {
	httpd_handle_t server = NULL;
	httpd_config_t config = HTTPD_DEFAULT_CONFIG();

	ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
	if (httpd_start(&server, &config) == ESP_OK) {
		ESP_LOGI(TAG, "Registering URI handlers");
		httpd_register_uri_handler(server, &get);
		httpd_register_uri_handler(server, &post);
		return server;
	}

	ESP_LOGI(TAG, "Error starting server!");
	return NULL;
}

void stop_webserver(httpd_handle_t server) { httpd_stop(server); }

static void disconnect_handler(void *arg, esp_event_base_t event_base, int32_t event_id,
							   void *event_data) {
	httpd_handle_t *server = (httpd_handle_t *)arg;
	if (*server) {
		ESP_LOGI(TAG, "Stopping webserver");
		stop_webserver(*server);
		*server = NULL;
	}
}

static void connect_handler(void *arg, esp_event_base_t event_base, int32_t event_id,
							void *event_data) {
	httpd_handle_t *server = (httpd_handle_t *)arg;
	if (*server == NULL) {
		ESP_LOGI(TAG, "Starting webserver");
		*server = start_webserver();
	}
}

void app_main() {
	ESP_ERROR_CHECK(nvs_flash_init());
	ESP_ERROR_CHECK(esp_netif_init());
	ESP_ERROR_CHECK(esp_event_loop_create_default());

	ESP_ERROR_CHECK(example_connect());

	ESP_ERROR_CHECK(
		esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &connect_handler, &server));
	ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED,
											   &disconnect_handler, &server));

	pwm_init(PWM_PERIOD, duties, 3, pin_num);
	pwm_set_phases(phase);
	pwm_start();

	server = start_webserver();
}
