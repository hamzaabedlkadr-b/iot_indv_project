#ifndef COMM_LORAWAN_H
#define COMM_LORAWAN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_err.h"
#include "project_types.h"

esp_err_t comm_lorawan_init(project_context_t *ctx);
void comm_lorawan_task(void *pvParameters);

#ifdef __cplusplus
}
#endif

#endif
