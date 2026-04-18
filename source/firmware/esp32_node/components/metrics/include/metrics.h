#ifndef METRICS_H
#define METRICS_H

#include <stdint.h>

#include "esp_err.h"
#include "project_types.h"

esp_err_t metrics_init(project_context_t *ctx);
uint64_t metrics_now_us(void);
void metrics_copy_snapshot(const project_context_t *ctx, timing_metrics_t *out_snapshot);
void metrics_task(void *pvParameters);

#endif

