#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void app_main(void)
{
    int counter = 0;

    // Runs once at boot
    printf("ICHS Field Terminal — Boot OK\n");

    while (1)
    {
        printf("Heartbeat: %d\n", counter);
        counter++;

        vTaskDelay(pdMS_TO_TICKS(1000)); // 1 second delay
    }
}