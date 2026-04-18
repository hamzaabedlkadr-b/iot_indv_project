#ifndef SIGNAL_INPUT_H
#define SIGNAL_INPUT_H

#include "esp_err.h"
#include "project_types.h"

esp_err_t signal_input_init(project_context_t *ctx);
void signal_input_task(void *pvParameters);

#endif

