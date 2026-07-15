#include <stdio.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"
#include "board.h"
#include "lcd_i2c.h"
#include "buttons.h"
#include "spiffs_log.h"
#include <stdint.h>

/* ---- States, straight from the Task 3A diagram ---- */
typedef enum {
    STATE_IDLE,
    STATE_CATEGORY_SELECTION,
    STATE_DETAIL_ENTRY,
    STATE_CONFIRMATION,
    STATE_SAVING,
    STATE_SYNC_ATTEMPT,
    STATE_SYNC_SUCCESS,
    STATE_SYNC_FAILED
} DeviceState;

/* ---- Events, one per button plus the automatic/simulated ones ---- */
typedef enum {
    EVENT_BTN_UP,
    EVENT_BTN_DOWN,
    EVENT_BTN_SELECT,
    EVENT_BTN_BACK,
    EVENT_SAVE_DONE,
    EVENT_SYNC_ACK,
    EVENT_SYNC_FAIL
} DeviceEvent;

/* ---- Report category, from the Task 3A diagram (5 categories) ---- */
typedef enum {
    CATEGORY_INFRASTRUCTURE,
    CATEGORY_MEDICAL,
    CATEGORY_FOOD,
    CATEGORY_SHELTER,
    CATEGORY_OTHER
} ReportCategory;

static const char *category_names[] = {
    "Infrastructure",
    "Medical",
    "Food",
    "Shelter",
    "Other"
};

/* ---- One field report. Field-for-field, this must match Intern 2's
   database schema — confirm with them before treating this as final. ---- */
typedef struct {
    uint32_t report_id;
    uint32_t timestamp;
    ReportCategory category;
    uint8_t severity;          // 1-5 scale
    uint16_t people_affected;
    char location[32];
    char device_id[16];
} FieldReport;

static FieldReport current_report;

/* Forward declarations — entry actions come in the next step */
void enter_state(DeviceState state);

DeviceState handle_event(DeviceState current, DeviceEvent event)
{
    switch (current) {

        case STATE_IDLE:
            if (event == EVENT_BTN_SELECT) return STATE_CATEGORY_SELECTION;
            break;

        case STATE_CATEGORY_SELECTION:
            if (event == EVENT_BTN_SELECT) return STATE_DETAIL_ENTRY;
            if (event == EVENT_BTN_BACK)   return STATE_IDLE;
            /* UP / DOWN stay in this state, handled elsewhere (moves highlight) */
            break;

        case STATE_DETAIL_ENTRY:
            if (event == EVENT_BTN_SELECT) return STATE_CONFIRMATION;
            if (event == EVENT_BTN_BACK)   return STATE_CATEGORY_SELECTION;
            /* UP / DOWN stay in this state (adjusts current field's value) */
            break;

        case STATE_CONFIRMATION:
            if (event == EVENT_BTN_SELECT) return STATE_SAVING;
            if (event == EVENT_BTN_BACK)   return STATE_DETAIL_ENTRY;
            break;

        case STATE_SAVING:
            if (event == EVENT_SAVE_DONE) return STATE_SYNC_ATTEMPT;
            break;

        case STATE_SYNC_ATTEMPT:
            if (event == EVENT_SYNC_ACK)  return STATE_SYNC_SUCCESS;
            if (event == EVENT_SYNC_FAIL) return STATE_SYNC_FAILED;
            break;

        case STATE_SYNC_SUCCESS:
        case STATE_SYNC_FAILED:
            /* both return to Idle automatically — handled in main loop, not here */
            break;
    }

    return current; // no matching transition, stay put
}
/* ---- Minimal navigation state (not part of DeviceState — this is just
   "which item am I highlighted on", separate from which screen we're on) ---- */
static int s_category_index = 0;
static int s_field_index = 0;

#define NUM_FIELDS 3 /* severity, people affected, location — matches the doc for now */

void enter_state(DeviceState state)
{
    switch (state) {

        case STATE_IDLE:
            printf("[STATE] Entering IDLE\n");
            s_category_index = 0;
            s_field_index = 0;
            lcd_cmd(0x01);
            vTaskDelay(pdMS_TO_TICKS(5));
            lcd_set_cursor(0, 0);
            lcd_print("Ready");
            lcd_set_cursor(1, 0);
            lcd_print("Press SELECT");
            break;

        case STATE_CATEGORY_SELECTION:
            printf("[STATE] Entering CATEGORY_SELECTION (index %d)\n", s_category_index);
            lcd_cmd(0x01);
            vTaskDelay(pdMS_TO_TICKS(5));
            lcd_set_cursor(0, 0);
            lcd_print("Category:");
            lcd_set_cursor(1, 0);
            char buf[32];
            snprintf(buf, sizeof(buf), "> %s", category_names[s_category_index]);
             lcd_print(buf);
            break;

        case STATE_DETAIL_ENTRY:
            printf("[STATE] Entering DETAIL_ENTRY (field %d)\n", s_field_index);
            lcd_cmd(0x01);
            vTaskDelay(pdMS_TO_TICKS(5));
            lcd_set_cursor(0, 0);
            lcd_print("Field:");
            lcd_set_cursor(1, 0);
            char fbuf[32];
            snprintf(fbuf, sizeof(fbuf), "#%d of %d", s_field_index + 1, NUM_FIELDS);
            lcd_print(fbuf);
            break;

        case STATE_CONFIRMATION:
            printf("[STATE] Entering CONFIRMATION — building summary\n");
            lcd_cmd(0x01);
            vTaskDelay(pdMS_TO_TICKS(5));
            lcd_set_cursor(0, 0);
            lcd_print("Confirm report?");
            lcd_set_cursor(1, 0);
            lcd_print("SELECT=yes");
            break;

        case STATE_SAVING:
            printf("[STATE] Entering SAVING — placeholder: report would be written here\n");
            printf("SAVING REPORT...\n");
            lcd_cmd(0x01);
            vTaskDelay(pdMS_TO_TICKS(5));
            lcd_set_cursor(0, 0);
            lcd_print("Saving report...");
            break;

        case STATE_SYNC_ATTEMPT:
            printf("[STATE] Entering SYNC_ATTEMPT — placeholder: would check WiFi/USB here\n");
            printf("SYNCING REPORTS...\n");
            lcd_cmd(0x01);
            vTaskDelay(pdMS_TO_TICKS(5));
            lcd_set_cursor(0, 0);
            lcd_print("Syncing...");
            break;

        case STATE_SYNC_SUCCESS:
            printf("[STATE] Entering SYNC_SUCCESS — placeholder: buffer would clear here\n");
            printf("SYNC SUCCESS\n");
            lcd_cmd(0x01);
            vTaskDelay(pdMS_TO_TICKS(5));
            lcd_set_cursor(0, 0);
            lcd_print("Sync Success");
            break;

        case STATE_SYNC_FAILED:
            printf("[STATE] Entering SYNC_FAILED — placeholder: report kept for retry\n");
            printf("SYNC FAILED\n");
            lcd_cmd(0x01);
            vTaskDelay(pdMS_TO_TICKS(5));
            lcd_set_cursor(0, 0);
            lcd_print("Sync Failed");
            break;
    }
}

/* ---- Helpers for in-state navigation that doesn't change DeviceState ---- */

static void category_selection_adjust(DeviceEvent event)
{
    #define NUM_CATEGORIES 5 /* Infrastructure, Medical, Food, Shelter, Other */
    if (event == EVENT_BTN_UP)   s_category_index = (s_category_index + NUM_CATEGORIES - 1) % NUM_CATEGORIES;
    if (event == EVENT_BTN_DOWN) s_category_index = (s_category_index + 1) % NUM_CATEGORIES;
}

static void detail_entry_adjust(DeviceEvent event)
{
    /* Placeholder: real per-field min/max ranges come once FieldReport exists (Step 5) */
    if (event == EVENT_BTN_UP)   printf("[DETAIL] field %d value up\n", s_field_index);
    if (event == EVENT_BTN_DOWN) printf("[DETAIL] field %d value down\n", s_field_index);
}

/* Returns true if DetailEntry is finished and should move to Confirmation */
static bool detail_entry_advance(void)
{
    s_field_index++;
    if (s_field_index >= NUM_FIELDS) {
        return true; /* all fields done */
    }
    return false; /* more fields remain */
}

void app_main(void)
{
    printf("SYSTEM START\n");

    spiffs_log_init();

    i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = LCD_SDA_PIN,
        .scl_io_num = LCD_SCL_PIN,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
    };
    i2c_master_bus_handle_t bus;
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &bus));
    lcd_i2c_init(bus);
    lcd_init();

    buttons_init();

    DeviceState state = STATE_IDLE;
    enter_state(state);

    bool sync_should_succeed = true; /* placeholder: alternates each attempt */

    while (1) {
        /* ---- Automatic states: no button needed, act after a short simulated delay ---- */
        if (state == STATE_SAVING) {
            vTaskDelay(pdMS_TO_TICKS(1000));
            state = handle_event(state, EVENT_SAVE_DONE);
            enter_state(state);
            continue;
        }
        if (state == STATE_SYNC_ATTEMPT) {
            vTaskDelay(pdMS_TO_TICKS(1000));
            DeviceEvent result = sync_should_succeed ? EVENT_SYNC_ACK : EVENT_SYNC_FAIL;
            sync_should_succeed = !sync_should_succeed; /* flip for next time */
            state = handle_event(state, result);
            enter_state(state);
            continue;
        }
        if (state == STATE_SYNC_SUCCESS || state == STATE_SYNC_FAILED) {
            vTaskDelay(pdMS_TO_TICKS(1500));
            state = STATE_IDLE;
            enter_state(state);
            continue;
        }

        /* ---- Button-driven states ---- */
        DeviceEvent event;
        bool have_event = true;

        if (button_pressed(BUTTON_SELECT_PIN)) event = EVENT_BTN_SELECT;
        else if (button_pressed(BUTTON_UP_PIN)) event = EVENT_BTN_UP;
        else if (button_pressed(BUTTON_DOWN_PIN)) event = EVENT_BTN_DOWN;
        else if (button_pressed(BUTTON_BACK_PIN)) event = EVENT_BTN_BACK;
        else have_event = false;

        if (have_event) {
            printf("[EVENT] button -> event %d in state %d\n", event, state);

            if (state == STATE_CATEGORY_SELECTION && (event == EVENT_BTN_UP || event == EVENT_BTN_DOWN)) {
                category_selection_adjust(event);
                enter_state(state); /* redraw with new highlight */
            }
            else if (state == STATE_DETAIL_ENTRY && (event == EVENT_BTN_UP || event == EVENT_BTN_DOWN)) {
                detail_entry_adjust(event);
            }
            else if (state == STATE_DETAIL_ENTRY && event == EVENT_BTN_SELECT) {
                if (detail_entry_advance()) {
                    state = STATE_CONFIRMATION;
                } /* else: stay in DetailEntry, just show the next field */
                enter_state(state);
            }
            else {
                DeviceState next = handle_event(state, event);
                if (next != state) {
                    if (state == STATE_CATEGORY_SELECTION && next == STATE_DETAIL_ENTRY) {
                        current_report.category = (ReportCategory)s_category_index;
                        printf("[REPORT] category set to %s\n", category_names[s_category_index]);
                    }
                    state = next;
                    enter_state(state);
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}