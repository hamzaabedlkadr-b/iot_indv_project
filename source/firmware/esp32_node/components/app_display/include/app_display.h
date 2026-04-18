#ifndef APP_DISPLAY_H
#define APP_DISPLAY_H

#include "esp_err.h"
#include "project_types.h"

esp_err_t app_display_init(project_context_t *ctx);
void app_display_task(void *pvParameters);

#endif

