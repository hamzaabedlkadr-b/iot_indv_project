#ifndef SIGNAL_PROCESSING_H
#define SIGNAL_PROCESSING_H

#include "esp_err.h"
#include "project_types.h"

esp_err_t signal_processing_init(project_context_t *ctx);
void signal_processing_task(void *pvParameters);

#endif

