#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_spiffs.h"

void init_spiffs() {
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = NULL,
        .max_files = 5,
        .format_if_mount_failed = true
    };

    esp_vfs_spiffs_register(&conf);
}

void app_main(void)
{
    printf("SPIFFS TEST START\n");

    init_spiffs();

    FILE *f = fopen("/spiffs/test.txt", "w");
    fprintf(f, "report_001: TEST DATA\n");
    fclose(f);

    printf("Written first line\n");

    char line[128];

    f = fopen("/spiffs/test.txt", "r");
    fgets(line, sizeof(line), f);
    fclose(f);

    printf("Read: %s", line);

    f = fopen("/spiffs/test.txt", "a");
    fprintf(f, "report_002: MORE DATA\n");
    fclose(f);

    printf("Appended second line\n");

    printf("\nFULL FILE:\n");

    f = fopen("/spiffs/test.txt", "r");

    while (fgets(line, sizeof(line), f)) {
        printf("%s", line);
    }

    fclose(f);

    printf("\nDONE\n");
}