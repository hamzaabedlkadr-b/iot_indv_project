#ifndef COMM_MQTT_H
#define COMM_MQTT_H

#include "esp_err.h"
#include "project_types.h"

esp_err_t comm_mqtt_init(project_context_t *ctx);
void comm_mqtt_task(void *pvParameters);

#endif

