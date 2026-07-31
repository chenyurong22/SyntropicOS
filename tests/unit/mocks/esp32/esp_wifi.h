#ifndef MOCK_ESP_WIFI_H
#define MOCK_ESP_WIFI_H

#include <stdint.h>

#define WIFI_INIT_CONFIG_DEFAULT() \
    {                              \
    }
typedef void *wifi_init_config_t;
typedef struct {
    struct {
        uint8_t ssid[32];
        uint8_t password[64];
    } sta;
} wifi_config_t;

typedef int esp_err_t;
typedef void esp_netif_t;

typedef struct {
    struct {
        uint32_t addr;
    } ip;
} esp_netif_ip_info_t;

typedef struct {
    esp_netif_ip_info_t ip_info;
} ip_event_got_ip_t;

#define WIFI_STORAGE_RAM 0
#define WIFI_MODE_STA 1
#define WIFI_IF_STA 0
#define WIFI_EVENT_STA_DISCONNECTED 1

#define IPSTR "%d.%d.%d.%d"
#define IP2STR(ip) 127, 0, 0, 1

#define ESP_OK 0
#define ESP_ERROR_CHECK(x) (void)(x)

static inline const char *esp_err_to_name(esp_err_t e)
{
    (void)e;
    return "OK";
}
static inline int esp_netif_init(void)
{
    return 0;
}
static inline int esp_event_loop_create_default(void)
{
    return 0;
}
static inline void *esp_netif_create_default_wifi_sta(void)
{
    return (void *)1;
}
static inline esp_netif_t *esp_netif_get_handle_from_ifkey(const char *k)
{
    (void)k;
    return (esp_netif_t *)1;
}
static inline int esp_netif_get_ip_info(esp_netif_t *n, esp_netif_ip_info_t *i)
{
    (void)n;
    (void)i;
    return 0;
}

static inline int esp_wifi_init(wifi_init_config_t *cfg)
{
    (void)cfg;
    return 0;
}
static inline int esp_wifi_set_storage(int s)
{
    (void)s;
    return 0;
}
static inline int esp_wifi_set_mode(int m)
{
    (void)m;
    return 0;
}
static inline int esp_wifi_set_config(int i, wifi_config_t *c)
{
    (void)i;
    (void)c;
    return 0;
}
static inline int esp_wifi_start(void)
{
    return 0;
}
static inline int esp_wifi_connect(void)
{
    return 0;
}

#endif /* MOCK_ESP_WIFI_H */
