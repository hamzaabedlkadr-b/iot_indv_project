#ifndef SAMPLING_CONTROL_H
#define SAMPLING_CONTROL_H

#include "esp_err.h"
#include "project_types.h"

esp_err_t sampling_control_init(project_context_t *ctx);
void sampling_control_task(void *pvParameters);

#endif

