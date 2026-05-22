#include "bms_ui.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*********************
 *      DEFINES
 *********************/
#define COLOR_BG          lv_color_make(0x00, 0x00, 0x00) // Deep Black
#define COLOR_GOLD        lv_color_make(0xFF, 0x9F, 0x00) // Chiral Gold (Accent/Highlight)
#define COLOR_CYAN        lv_color_make(0x00, 0xE5, 0xFF) // Chiral Cyan (Main primary color)
#define COLOR_RED         lv_color_make(0xFF, 0x17, 0x44) // Danger Red
#define COLOR_GRAY        lv_color_make(0x55, 0x66, 0x77) // Muted Gray
#define COLOR_DARK_GRAY   lv_color_make(0x11, 0x16, 0x1B) // Selected Background

/* Stepped Dark Blue tones for HUD gradient color scale bar */
#define COLOR_DARK_BLUE_1 lv_color_make(0x00, 0xAC, 0xC0) // 75% Chiral Cyan brightness
#define COLOR_DARK_BLUE_2 lv_color_make(0x00, 0x73, 0x80) // 50% Chiral Cyan brightness
#define COLOR_DARK_BLUE_3 lv_color_make(0x00, 0x39, 0x40) // 25% Chiral Cyan brightness

/**********************
 *  STATIC VARIABLES
 **********************/
static int current_page = 0;

/* Global state */
static uint8_t global_predicted_soc = 85;
static uint8_t host_online = 1;
static uint32_t rx_packet_count = 104;

/* Battery simulation physics state */
static float batt_soc = 85.0f;        // 0.0% to 100.0%
static float batt_ocv = 4.12f;        // Open Circuit Voltage
static float batt_u_real = 4.12f;     // Loaded terminal voltage
static float batt_i_real = 0.00f;     // Loaded current (A)
static float batt_t_real = 25.0f;     // Cell Temp (C)
static float batt_r_int = 0.05f;      // Internal resistance (Ohm)

/* Page 2 (CCCV) Settings */
static float target_u_set = 4.20f;    // V
static float target_i_set = 2.00f;    // A
static int charge_active = 0;

/* Page 3 (CC Discharge) Settings */
static float target_i_dis = 3.00f;    // A
static int discharge_active = 0;
static int low_volt_alert = 0;

/* Page 4 (System) Settings */
static int baud_rate_idx = 1;         // 0: 9600, 1: 115200
static int port_idx = 0;              // 0: UART0, 1: UART1

/* UI Elements */
static lv_obj_t * page_containers[4];

/* Global Header Components */
static lv_obj_t * lbl_header_mode;
static lv_obj_t * lbl_header_soc;
static lv_obj_t * bar_header_soc;
static lv_obj_t * lbl_header_link;

/* Global Footer Components */
static lv_obj_t * lbl_footer_indicator;

/* Page 1 (SoC Page) Components */
static lv_obj_t * lbl_p1_soc;
static lv_obj_t * lbl_p1_trend;
static lv_obj_t * lbl_p1_u;
static lv_obj_t * lbl_p1_i;
static lv_obj_t * lbl_p1_t;
static lv_obj_t * lbl_p1_r;

/* Page 2 (CCCV Page) Components */
static lv_obj_t * btn_p2_uset;
static lv_obj_t * btn_p2_iset;
static lv_obj_t * btn_p2_toggle;
static lv_obj_t * chart_p2;
static lv_chart_series_t * chart_p2_u_series;
static lv_chart_series_t * chart_p2_i_series;
static lv_obj_t * lbl_p2_readout;

/* Page 3 (CC Discharge Page) Components */
static lv_obj_t * btn_p3_idis;
static lv_obj_t * btn_p3_toggle;
static lv_obj_t * chart_p3;
static lv_chart_series_t * chart_p3_u_series;
static lv_chart_series_t * chart_p3_i_series;
static lv_obj_t * lbl_p3_readout;

/* Page 4 (System Page) Components */
static lv_obj_t * btn_p4_baud;
static lv_obj_t * btn_p4_port;
static lv_obj_t * lbl_p4_terminal;
static char terminal_buffer[256];

/* Styles */
static lv_style_t style_bg;
static lv_style_t style_text_gold;
static lv_style_t style_text_cyan;
static lv_style_t style_text_gray;
static lv_style_t style_btn_normal;
static lv_style_t style_btn_focused;
static lv_style_t style_btn_editing;

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void switch_page(int page_idx);
static void init_styles(void);
static void create_global_header_footer(void);
static void create_page_soc(void);
static void create_page_cccv(void);
static void create_page_discharge(void);
static void create_page_system(void);
static void bms_sim_tick(lv_timer_t * timer);
static void host_comm_tick(lv_timer_t * timer);
static void widget_click_handler(lv_event_t * e);
static void widget_key_handler(lv_event_t * e);
static void global_key_handler(lv_event_t * e);
static void button_focus_event_cb(lv_event_t * e);
static lv_obj_t * create_undersampled_bottom_bar(lv_obj_t * parent, int width);
static void child_created_event_cb(lv_event_t * e);
static lv_obj_t * get_hud_button_bottom_bar(lv_obj_t * btn);

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void bms_ui_init(void)
{
    /* Clear default group if active */
    lv_group_t * g = lv_group_get_default();
    if(g) {
        lv_group_remove_all_objs(g);
    } else {
        g = lv_group_create();
        lv_group_set_default(g);
    }

    /* Configure main screen background */
    lv_obj_t * scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, COLOR_BG, 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

    /* Setup base styles */
    init_styles();

    /* Create common Status Bar and Footer */
    create_global_header_footer();

    /* Create Page Containers */
    for(int i = 0; i < 4; i++) {
        page_containers[i] = lv_obj_create(scr);
        lv_obj_set_size(page_containers[i], 240, 102);
        lv_obj_set_pos(page_containers[i], 0, 18);
        lv_obj_add_style(page_containers[i], &style_bg, 0);
        lv_obj_add_flag(page_containers[i], LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(page_containers[i], LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_flag(page_containers[i], LV_OBJ_FLAG_CLICK_FOCUSABLE);
        
        /* Make sure it doesn't draw default borders/scrollbar or outline focused border */
        lv_obj_set_style_border_width(page_containers[i], 0, 0);
        lv_obj_set_style_pad_all(page_containers[i], 0, 0);
        lv_obj_set_scrollbar_mode(page_containers[i], LV_SCROLLBAR_MODE_OFF);
        lv_obj_set_style_outline_width(page_containers[i], 0, 0);
        lv_obj_set_style_outline_width(page_containers[i], 0, LV_STATE_FOCUSED);
    }

    /* Initialize each page view */
    create_page_soc();
    create_page_cccv();
    create_page_discharge();
    create_page_system();

    /* Set up terminal log message buffer */
    strcpy(terminal_buffer, "SYNC -> ACTIVE\nCHIRAL LINK ESTABLISHED\nSYS TELEMETRY STATUS: OK");
    lv_label_set_text(lbl_p4_terminal, terminal_buffer);

    /* Hook global key catcher on the screen to capture page switches & navigation scrolls */
    lv_obj_add_event_cb(scr, global_key_handler, LV_EVENT_KEY, NULL);

    /* Start page 0 (SoC Main page) */
    switch_page(0);

    /* Initialize Simulation Timers */
    lv_timer_create(bms_sim_tick, 200, NULL);
    lv_timer_create(host_comm_tick, 1000, NULL);
}

void bms_ui_update_soc(uint8_t soc_val)
{
    global_predicted_soc = soc_val;
    if(lbl_header_soc) {
        char buf[16];
        snprintf(buf, sizeof(buf), "%d%%", global_predicted_soc);
        lv_label_set_text(lbl_header_soc, buf);
        lv_obj_center(lbl_header_soc); // Keep centered inside the nested battery bar
    }
    if(bar_header_soc) {
        lv_bar_set_value(bar_header_soc, global_predicted_soc, LV_ANIM_OFF);
    }
    if(lbl_p1_soc && current_page == 0) {
        char buf[16];
        snprintf(buf, sizeof(buf), "%d%%", global_predicted_soc);
        lv_label_set_text(lbl_p1_soc, buf);
    }
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static lv_obj_t * get_hud_button_bottom_bar(lv_obj_t * btn)
{
    if(!btn) return NULL;
    uint32_t cnt = lv_obj_get_child_cnt(btn);
    for(uint32_t i = 0; i < cnt; i++) {
        lv_obj_t * child = lv_obj_get_child(btn, i);
        if(child && lv_obj_get_height(child) == 3) {
            return child;
        }
    }
    return NULL;
}

static void child_created_event_cb(lv_event_t * e)
{
    lv_obj_t * btn = lv_event_get_current_target(e);
    uint32_t cnt = lv_obj_get_child_cnt(btn);
    if(cnt < 2) return;
    
    lv_obj_t * bottom_bar = get_hud_button_bottom_bar(btn);
    if(bottom_bar) {
        lv_obj_move_to_index(bottom_bar, cnt - 1);
    }
}

static void button_focus_event_cb(lv_event_t * e)
{
    lv_obj_t * btn = lv_event_get_target(e);
    lv_event_code_t code = lv_event_get_code(e);
    
    lv_obj_t * bottom_bar = get_hud_button_bottom_bar(btn);
    if(!bottom_bar) return;
    
    if(code == LV_EVENT_FOCUSED) {
        if(!lv_obj_has_state(btn, LV_STATE_USER_1)) {
            lv_obj_remove_flag(bottom_bar, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(bottom_bar, LV_OBJ_FLAG_HIDDEN);
        }
    }
    else if(code == LV_EVENT_DEFOCUSED) {
        lv_obj_add_flag(bottom_bar, LV_OBJ_FLAG_HIDDEN);
    }
}

static lv_obj_t * create_undersampled_bottom_bar(lv_obj_t * parent, int width)
{
    /* Create a container for the bottom bar at the bottom of the button */
    /* Button height is 22. We place the bottom bar at y = 19, height = 3. */
    lv_obj_t * bar = lv_obj_create(parent);
    lv_obj_set_size(bar, width, 3);
    lv_obj_set_pos(bar, 0, 19);
    lv_obj_set_style_bg_opa(bar, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(bar, 0, 0);
    lv_obj_set_style_pad_all(bar, 0, 0);
    lv_obj_set_scrollbar_mode(bar, LV_SCROLLBAR_MODE_OFF);
    lv_obj_add_flag(bar, LV_OBJ_FLAG_HIDDEN); /* Hidden by default */
    
    /* Calculate segment widths dynamically to support varying widths if needed */
    int w_main = (width * 80) / 100;
    int w_dark1 = (width * 10) / 100;
    int w_dark2 = (width * 20) / 300;
    int w_dark3 = width - w_main - w_dark1 - w_dark2; // ensure exact total width
    
    /* For width = 85:
       w_main = 68 px
       w_dark1 = 9 px
       w_dark2 = 6 px
       w_dark3 = 2 px
       This yields exactly a 3:2:1 ratio for the 17px transition zone (9:6:2 px).
    */
    if (width == 85) {
        w_main = 68;
        w_dark1 = 9;
        w_dark2 = 6;
        w_dark3 = 2;
    }
    
    // Segment 1: Main Blue (Cyan)
    if(w_main > 0) {
        lv_obj_t * seg1 = lv_obj_create(bar);
        lv_obj_set_size(seg1, w_main, 3);
        lv_obj_set_pos(seg1, 0, 0);
        lv_obj_set_style_bg_color(seg1, COLOR_CYAN, 0);
        lv_obj_set_style_bg_opa(seg1, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(seg1, 0, 0);
        lv_obj_set_style_radius(seg1, 0, 0);
    }
    
    // Segment 2: Dark Blue 1
    if(w_dark1 > 0) {
        lv_obj_t * seg2 = lv_obj_create(bar);
        lv_obj_set_size(seg2, w_dark1, 3);
        lv_obj_set_pos(seg2, w_main, 0);
        lv_obj_set_style_bg_color(seg2, COLOR_DARK_BLUE_1, 0);
        lv_obj_set_style_bg_opa(seg2, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(seg2, 0, 0);
        lv_obj_set_style_radius(seg2, 0, 0);
    }
    
    // Segment 3: Dark Blue 2
    if(w_dark2 > 0) {
        lv_obj_t * seg3 = lv_obj_create(bar);
        lv_obj_set_size(seg3, w_dark2, 3);
        lv_obj_set_pos(seg3, w_main + w_dark1, 0);
        lv_obj_set_style_bg_color(seg3, COLOR_DARK_BLUE_2, 0);
        lv_obj_set_style_bg_opa(seg3, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(seg3, 0, 0);
        lv_obj_set_style_radius(seg3, 0, 0);
    }
    
    // Segment 4: Dark Blue 3
    if(w_dark3 > 0) {
        lv_obj_t * seg4 = lv_obj_create(bar);
        lv_obj_set_size(seg4, w_dark3, 3);
        lv_obj_set_pos(seg4, w_main + w_dark1 + w_dark2, 0);
        lv_obj_set_style_bg_color(seg4, COLOR_DARK_BLUE_3, 0);
        lv_obj_set_style_bg_opa(seg4, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(seg4, 0, 0);
        lv_obj_set_style_radius(seg4, 0, 0);
    }
    
    return bar;
}

static void init_styles(void)
{
    /* BG container style (Plain solid black, zero padding) */
    lv_style_init(&style_bg);
    lv_style_set_bg_color(&style_bg, COLOR_BG);
    lv_style_set_bg_opa(&style_bg, LV_OPA_COVER);
    lv_style_set_pad_all(&style_bg, 0);
    lv_style_set_border_width(&style_bg, 0);
    lv_style_set_radius(&style_bg, 0); // Explicitly square!

    /* Gold tech text (Accent color, restricted under 1/6 footprint) */
    lv_style_init(&style_text_gold);
    lv_style_set_text_color(&style_text_gold, COLOR_GOLD);
    lv_style_set_text_font(&style_text_gold, &lv_font_montserrat_12);

    /* Cyan tech text (Primary HUD color) */
    lv_style_init(&style_text_cyan);
    lv_style_set_text_color(&style_text_cyan, COLOR_CYAN);
    lv_style_set_text_font(&style_text_cyan, &lv_font_montserrat_12);

    /* Gray tech text */
    lv_style_init(&style_text_gray);
    lv_style_set_text_color(&style_text_gray, COLOR_GRAY);
    lv_style_set_text_font(&style_text_gray, &lv_font_montserrat_12);

    /* Buttons - normal flat look with gray borders (Death Stranding minimalist) */
    lv_style_init(&style_btn_normal);
    lv_style_set_bg_color(&style_btn_normal, COLOR_BG);
    lv_style_set_bg_opa(&style_btn_normal, LV_OPA_COVER);
    lv_style_set_border_width(&style_btn_normal, 1);
    lv_style_set_border_color(&style_btn_normal, COLOR_GRAY);
    lv_style_set_text_color(&style_btn_normal, COLOR_CYAN);
    lv_style_set_text_font(&style_btn_normal, &lv_font_montserrat_12);
    lv_style_set_pad_ver(&style_btn_normal, 3);
    lv_style_set_pad_hor(&style_btn_normal, 4);
    lv_style_set_radius(&style_btn_normal, 0); // Explicitly square!

    /* Buttons - focused (bright cyan border highlight) */
    lv_style_init(&style_btn_focused);
    lv_style_set_border_width(&style_btn_focused, 1); // Keep the clean normal border
    lv_style_set_border_color(&style_btn_focused, COLOR_GRAY); // Keep the normal gray color, no highlight outline
    lv_style_set_outline_width(&style_btn_focused, 0); // Completely disable the default LVGL focus outline frame!
    lv_style_set_bg_color(&style_btn_focused, COLOR_BG);
    lv_style_set_bg_opa(&style_btn_focused, LV_OPA_COVER);
    lv_style_set_radius(&style_btn_focused, 0); // Explicitly square!

    /* Buttons - editing (solid Gold background, Black text - extremely high contrast) */
    lv_style_init(&style_btn_editing);
    lv_style_set_bg_color(&style_btn_editing, COLOR_GOLD);
    lv_style_set_bg_opa(&style_btn_editing, LV_OPA_COVER);
    lv_style_set_border_width(&style_btn_editing, 1);
    lv_style_set_border_color(&style_btn_editing, COLOR_GOLD);
    lv_style_set_text_color(&style_btn_editing, COLOR_BG);
    lv_style_set_radius(&style_btn_editing, 0); // Explicitly square!
}

static void create_global_header_footer(void)
{
    lv_obj_t * scr = lv_screen_active();

    /* --- Status Bar Container --- */
    lv_obj_t * header = lv_obj_create(scr);
    lv_obj_set_size(header, 240, 18);
    lv_obj_set_pos(header, 0, 0);
    lv_obj_add_style(header, &style_bg, 0);
    lv_obj_set_style_border_width(header, 0, 0);
    lv_obj_set_style_pad_all(header, 0, 0);
    lv_obj_set_scrollbar_mode(header, LV_SCROLLBAR_MODE_OFF);

    /* Page Tag Label (Shifted left since 'BRIDGES' has been removed to reduce Gold/text density) */
    lbl_header_mode = lv_label_create(header);
    lv_obj_add_style(lbl_header_mode, &style_text_cyan, 0);
    lv_label_set_text(lbl_header_mode, "[ SoC LNK ]");
    lv_obj_set_pos(lbl_header_mode, 8, 3);

    /* 
     * Nested SoC & Battery Bar:
     * To maximize space and visual premium quality, we expand the battery bar size 
     * and place the dynamic SoC percentage number nested directly on top of it.
     */
    bar_header_soc = lv_bar_create(header);
    lv_obj_set_size(bar_header_soc, 60, 14);
    lv_obj_set_pos(bar_header_soc, 140, 2);
    lv_obj_set_style_bg_color(bar_header_soc, COLOR_BG, 0);
    lv_obj_set_style_bg_opa(bar_header_soc, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(bar_header_soc, 1, 0);
    lv_obj_set_style_border_color(bar_header_soc, COLOR_CYAN, 0); // Cyan border
    lv_obj_set_style_pad_all(bar_header_soc, 0, 0);
    lv_obj_set_style_bg_color(bar_header_soc, COLOR_CYAN, LV_PART_INDICATOR); // Cyan bar
    lv_obj_set_style_bg_opa(bar_header_soc, LV_OPA_50, LV_PART_INDICATOR);   // Tech opacity
    lv_obj_set_style_radius(bar_header_soc, 0, 0); // Square battery bar!
    lv_obj_set_style_radius(bar_header_soc, 0, LV_PART_INDICATOR); // Square battery bar indicator!
    lv_bar_set_value(bar_header_soc, global_predicted_soc, LV_ANIM_OFF);

    lbl_header_soc = lv_label_create(bar_header_soc);
    lv_obj_add_style(lbl_header_soc, &style_text_cyan, 0);
    lv_obj_set_style_text_color(lbl_header_soc, lv_color_white(), 0);
    char buf[16];
    snprintf(buf, sizeof(buf), "%d%%", global_predicted_soc);
    lv_label_set_text(lbl_header_soc, buf);
    lv_obj_center(lbl_header_soc);

    /* Online Link Indicator */
    lbl_header_link = lv_label_create(header);
    lv_obj_add_style(lbl_header_link, &style_text_cyan, 0);
    lv_label_set_text(lbl_header_link, "LNK");
    lv_obj_set_pos(lbl_header_link, 203, 3); // Repositioned to 203 so it's not too far right!

    /* Divider line between header and page */
    lv_obj_t * line = lv_line_create(scr);
    static lv_point_precise_t line_points[] = {{0, 18}, {240, 18}};
    lv_line_set_points(line, line_points, 2);
    lv_obj_set_style_line_width(line, 1, 0);
    lv_obj_set_style_line_color(line, COLOR_GRAY, 0);

    /* --- Footer Bar --- */
    lv_obj_t * footer = lv_obj_create(scr);
    lv_obj_set_size(footer, 240, 15);
    lv_obj_set_pos(footer, 0, 120);
    lv_obj_add_style(footer, &style_bg, 0);
    lv_obj_set_style_border_width(footer, 0, 0);
    lv_obj_set_style_pad_all(footer, 0, 0);
    lv_obj_set_scrollbar_mode(footer, LV_SCROLLBAR_MODE_OFF);

    /* Divider line above footer */
    lv_obj_t * line_f = lv_line_create(scr);
    static lv_point_precise_t line_f_points[] = {{0, 120}, {240, 120}};
    lv_line_set_points(line_f, line_f_points, 2);
    lv_obj_set_style_line_width(line_f, 1, 0);
    lv_obj_set_style_line_color(line_f, COLOR_GRAY, 0);

    /* ASCII standard page progress (Replaces the square box ● ○ ○ ○ characters) */
    lbl_footer_indicator = lv_label_create(footer);
    lv_obj_add_style(lbl_footer_indicator, &style_text_cyan, 0);
    lv_label_set_text(lbl_footer_indicator, "[ * - - - ]");
    lv_obj_set_pos(lbl_footer_indicator, 6, 1);

    /* Helper navigation tags (using small montserrat 12 font) */
    lv_obj_t * lbl_help = lv_label_create(footer);
    lv_obj_add_style(lbl_help, &style_text_gray, 0);
    lv_obj_set_style_text_font(lbl_help, &lv_font_montserrat_12, 0);
    lv_label_set_text(lbl_help, "[ROT]:NAV [ENT]:EDIT [SPC]:PAGE");
    lv_obj_set_pos(lbl_help, 92, 2);
}

static void create_page_soc(void)
{
    lv_obj_t * p = page_containers[0];
    lv_obj_add_event_cb(p, widget_key_handler, LV_EVENT_KEY, NULL);

    /* 
     * Left Area: High Contrast predicted SoC & Working Trend 
     * Shifted SoC color to Cyan (was Gold) to maintain Gold color footprint strictly under 1/6.
     */
    lbl_p1_soc = lv_label_create(p);
    lv_obj_set_style_text_font(lbl_p1_soc, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(lbl_p1_soc, COLOR_CYAN, 0);
    char buf[16];
    snprintf(buf, sizeof(buf), "%d%%", global_predicted_soc);
    lv_label_set_text(lbl_p1_soc, buf);
    lv_obj_set_pos(lbl_p1_soc, 10, 10);

    lbl_p1_trend = lv_label_create(p);
    lv_obj_add_style(lbl_p1_trend, &style_text_gray, 0);
    lv_label_set_text(lbl_p1_trend, "[STANDBY]");
    lv_obj_set_pos(lbl_p1_trend, 10, 44);

    lv_obj_t * lbl_desc = lv_label_create(p);
    lv_obj_add_style(lbl_desc, &style_text_gray, 0);
    lv_obj_set_style_text_font(lbl_desc, &lv_font_montserrat_12, 0);
    lv_label_set_text(lbl_desc, "CHIRAL EST. SOC");
    lv_obj_set_pos(lbl_desc, 10, 66);

    /* 
     * Right Area: 2x2 Telemetry parameter list with high spacing
     */
    lbl_p1_u = lv_label_create(p);
    lv_obj_add_style(lbl_p1_u, &style_text_cyan, 0);
    lv_label_set_text(lbl_p1_u, "U: 4.120 V");
    lv_obj_set_pos(lbl_p1_u, 125, 12);

    lbl_p1_i = lv_label_create(p);
    lv_obj_add_style(lbl_p1_i, &style_text_cyan, 0);
    lv_label_set_text(lbl_p1_i, "I:  0.00 A");
    lv_obj_set_pos(lbl_p1_i, 125, 32);

    lbl_p1_t = lv_label_create(p);
    lv_obj_add_style(lbl_p1_t, &style_text_cyan, 0);
    lv_label_set_text(lbl_p1_t, "T: 25.0 C");
    lv_obj_set_pos(lbl_p1_t, 125, 52);

    lbl_p1_r = lv_label_create(p);
    lv_obj_add_style(lbl_p1_r, &style_text_gray, 0);
    lv_label_set_text(lbl_p1_r, "R: 0.05 Ohm");
    lv_obj_set_pos(lbl_p1_r, 125, 72);
}

static void setup_hud_button(lv_obj_t * btn)
{
    lv_obj_set_size(btn, 85, 22);
    lv_obj_add_style(btn, &style_btn_normal, 0);
    lv_obj_add_style(btn, &style_btn_focused, LV_STATE_FOCUSED);
    lv_obj_add_style(btn, &style_btn_editing, LV_STATE_USER_1);

    /* Bulletproof focus outline and border local overrides to override default theme selection boxes */
    lv_obj_set_style_outline_width(btn, 0, LV_STATE_FOCUSED);
    lv_obj_set_style_outline_width(btn, 0, 0);
    lv_obj_set_style_border_width(btn, 1, LV_STATE_FOCUSED);
    lv_obj_set_style_border_color(btn, COLOR_GRAY, LV_STATE_FOCUSED);

    lv_obj_add_event_cb(btn, widget_click_handler, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(btn, widget_key_handler, LV_EVENT_KEY, NULL);
    lv_obj_add_event_cb(btn, child_created_event_cb, LV_EVENT_CHILD_CREATED, NULL);
    create_undersampled_bottom_bar(btn, 85);
    lv_obj_add_event_cb(btn, button_focus_event_cb, LV_EVENT_FOCUSED, NULL);
    lv_obj_add_event_cb(btn, button_focus_event_cb, LV_EVENT_DEFOCUSED, NULL);
}

static void create_page_cccv(void)
{
    lv_obj_t * p = page_containers[1];

    /* 
     * Left controls panel (x=4, width=85px) - extremely space-efficient 
     */
    btn_p2_uset = lv_button_create(p);
    lv_obj_set_pos(btn_p2_uset, 4, 8);
    setup_hud_button(btn_p2_uset);
    lv_obj_t * lbl_uset = lv_label_create(btn_p2_uset);
    lv_label_set_text(lbl_uset, "U: 4.20V");
    lv_obj_center(lbl_uset);

    btn_p2_iset = lv_button_create(p);
    lv_obj_set_pos(btn_p2_iset, 4, 36);
    setup_hud_button(btn_p2_iset);
    lv_obj_t * lbl_iset = lv_label_create(btn_p2_iset);
    lv_label_set_text(lbl_iset, "I: 2.00A");
    lv_obj_center(lbl_iset);

    btn_p2_toggle = lv_button_create(p);
    lv_obj_set_pos(btn_p2_toggle, 4, 64);
    setup_hud_button(btn_p2_toggle);
    lv_obj_set_style_text_color(btn_p2_toggle, COLOR_GOLD, 0);
    lv_obj_t * lbl_toggle = lv_label_create(btn_p2_toggle);
    lv_label_set_text(lbl_toggle, "CHG: OFF");
    lv_obj_center(lbl_toggle);

    /* 
     * Right Area (x=98, width=136px) - Expanded live chart for maximum visibility
     */
    chart_p2 = lv_chart_create(p);
    lv_obj_set_size(chart_p2, 136, 56);
    lv_obj_set_pos(chart_p2, 98, 8);
    lv_chart_set_type(chart_p2, LV_CHART_TYPE_LINE);
    lv_chart_set_point_count(chart_p2, 20); // 20 data points (low SRAM footprint!)
    lv_chart_set_range(chart_p2, LV_CHART_AXIS_PRIMARY_Y, 0, 5000); // 0V-5V / 0A-5A mapped to 0-5000
    lv_obj_set_style_bg_color(chart_p2, COLOR_BG, 0);
    lv_obj_set_style_border_color(chart_p2, COLOR_DARK_GRAY, 0);
    lv_obj_set_style_border_width(chart_p2, 1, 0);
    lv_obj_set_style_pad_all(chart_p2, 0, 0);
    lv_obj_set_style_line_color(chart_p2, COLOR_DARK_GRAY, LV_PART_ITEMS);
    
    /* Create series */
    chart_p2_u_series = lv_chart_add_series(chart_p2, COLOR_CYAN, LV_CHART_AXIS_PRIMARY_Y);
    chart_p2_i_series = lv_chart_add_series(chart_p2, COLOR_GOLD, LV_CHART_AXIS_PRIMARY_Y);

    /* Feed flat empty initial points */
    for(int idx = 0; idx < 20; idx++) {
        lv_chart_set_next_value(chart_p2, chart_p2_u_series, 0);
        lv_chart_set_next_value(chart_p2, chart_p2_i_series, 0);
    }

    lbl_p2_readout = lv_label_create(p);
    lv_obj_add_style(lbl_p2_readout, &style_text_gray, 0);
    lv_obj_set_style_text_font(lbl_p2_readout, &lv_font_montserrat_12, 0);
    lv_label_set_text(lbl_p2_readout, "U: 4.12V  I: 0.00A");
    lv_obj_set_pos(lbl_p2_readout, 98, 68);
}

static void create_page_discharge(void)
{
    lv_obj_t * p = page_containers[2];

    /* 
     * Left controls panel (x=4, width=85px)
     */
    btn_p3_idis = lv_button_create(p);
    lv_obj_set_pos(btn_p3_idis, 4, 20);
    setup_hud_button(btn_p3_idis);
    lv_obj_t * lbl_idis = lv_label_create(btn_p3_idis);
    lv_label_set_text(lbl_idis, "I: 3.00A");
    lv_obj_center(lbl_idis);

    btn_p3_toggle = lv_button_create(p);
    lv_obj_set_pos(btn_p3_toggle, 4, 52);
    setup_hud_button(btn_p3_toggle);
    lv_obj_set_style_text_color(btn_p3_toggle, COLOR_CYAN, 0);
    lv_obj_t * lbl_toggle = lv_label_create(btn_p3_toggle);
    lv_label_set_text(lbl_toggle, "DSC: OFF");
    lv_obj_center(lbl_toggle);

    /* 
     * Right Area (x=98, width=136px) - Expanded discharge chart 
     */
    chart_p3 = lv_chart_create(p);
    lv_obj_set_size(chart_p3, 136, 56);
    lv_obj_set_pos(chart_p3, 98, 8);
    lv_chart_set_type(chart_p3, LV_CHART_TYPE_LINE);
    lv_chart_set_point_count(chart_p3, 20); // 20 data points
    lv_chart_set_range(chart_p3, LV_CHART_AXIS_PRIMARY_Y, 0, 5000); // 0V-5V / 0A-10A mapped to 0-5000 (discharge scale)
    lv_obj_set_style_bg_color(chart_p3, COLOR_BG, 0);
    lv_obj_set_style_border_color(chart_p3, COLOR_DARK_GRAY, 0);
    lv_obj_set_style_border_width(chart_p3, 1, 0);
    lv_obj_set_style_pad_all(chart_p3, 0, 0);
    lv_obj_set_style_line_color(chart_p3, COLOR_DARK_GRAY, LV_PART_ITEMS);

    /* Create series */
    chart_p3_u_series = lv_chart_add_series(chart_p3, COLOR_CYAN, LV_CHART_AXIS_PRIMARY_Y);
    chart_p3_i_series = lv_chart_add_series(chart_p3, COLOR_GOLD, LV_CHART_AXIS_PRIMARY_Y);

    /* Feed flat empty initial points */
    for(int idx = 0; idx < 20; idx++) {
        lv_chart_set_next_value(chart_p3, chart_p3_u_series, 0);
        lv_chart_set_next_value(chart_p3, chart_p3_i_series, 0);
    }

    lbl_p3_readout = lv_label_create(p);
    lv_obj_add_style(lbl_p3_readout, &style_text_gray, 0);
    lv_obj_set_style_text_font(lbl_p3_readout, &lv_font_montserrat_12, 0);
    lv_label_set_text(lbl_p3_readout, "U: 4.12V  I: 0.00A");
    lv_obj_set_pos(lbl_p3_readout, 98, 68);
}

static void create_page_system(void)
{
    lv_obj_t * p = page_containers[3];

    /* 
     * Left Area parameters (x=4, width=85px)
     */
    btn_p4_baud = lv_button_create(p);
    lv_obj_set_pos(btn_p4_baud, 4, 20);
    setup_hud_button(btn_p4_baud);
    lv_obj_t * lbl_baud = lv_label_create(btn_p4_baud);
    lv_label_set_text(lbl_baud, "Baud:115K");
    lv_obj_center(lbl_baud);

    btn_p4_port = lv_button_create(p);
    lv_obj_set_pos(btn_p4_port, 4, 52);
    setup_hud_button(btn_p4_port);
    lv_obj_t * lbl_port = lv_label_create(btn_p4_port);
    lv_label_set_text(lbl_port, "Port:UART0");
    lv_obj_center(lbl_port);

    /* 
     * Right Area (x=98, width=136px) - Scrolled log terminal with monospace look using montserrat 12
     */
    lbl_p4_terminal = lv_label_create(p);
    lv_obj_add_style(lbl_p4_terminal, &style_text_gold, 0);
    lv_obj_set_style_text_font(lbl_p4_terminal, &lv_font_montserrat_12, 0);
    lv_obj_set_pos(lbl_p4_terminal, 98, 8);
    lv_obj_set_size(lbl_p4_terminal, 136, 80);
    lv_label_set_long_mode(lbl_p4_terminal, LV_LABEL_LONG_CLIP);
}

static void switch_page(int page_idx)
{
    if(page_idx < 0 || page_idx >= 4) return;

    /* Hide all views */
    for(int i = 0; i < 4; i++) {
        if(page_containers[i]) {
            lv_obj_add_flag(page_containers[i], LV_OBJ_FLAG_HIDDEN);
        }
    }

    /* Show target page container */
    if(page_containers[page_idx]) {
        lv_obj_remove_flag(page_containers[page_idx], LV_OBJ_FLAG_HIDDEN);
    }

    /* Update standard ASCII page indicators (removes box symbols) */
    if(lbl_footer_indicator) {
        switch(page_idx) {
            case 0: lv_label_set_text(lbl_footer_indicator, "[ * - - - ]"); break;
            case 1: lv_label_set_text(lbl_footer_indicator, "[ - * - - ]"); break;
            case 2: lv_label_set_text(lbl_footer_indicator, "[ - - * - ]"); break;
            case 3: lv_label_set_text(lbl_footer_indicator, "[ - - - * ]"); break;
        }
    }

    /* Update Page Name in Header */
    if(lbl_header_mode) {
        switch(page_idx) {
            case 0: lv_label_set_text(lbl_header_mode, "[ SoC LNK ]"); break;
            case 1: lv_label_set_text(lbl_header_mode, "[ CCCV CHG ]"); break;
            case 2: lv_label_set_text(lbl_header_mode, "[ CC DISCH ]"); break;
            case 3: lv_label_set_text(lbl_header_mode, "[ SYS REG ]"); break;
        }
    }

    /* Reset default group elements to match the visible page */
    lv_group_t * g = lv_group_get_default();
    if(g) {
        /* Release editing lock to prevent stuck inputs */
        lv_group_set_editing(g, false);
        
        /* Remove active styling from all potential inputs */
        if(btn_p2_uset) lv_obj_remove_state(btn_p2_uset, LV_STATE_USER_1);
        if(btn_p2_iset) lv_obj_remove_state(btn_p2_iset, LV_STATE_USER_1);
        if(btn_p3_idis) lv_obj_remove_state(btn_p3_idis, LV_STATE_USER_1);
        if(btn_p4_baud) lv_obj_remove_state(btn_p4_baud, LV_STATE_USER_1);
        if(btn_p4_port) lv_obj_remove_state(btn_p4_port, LV_STATE_USER_1);

        lv_group_remove_all_objs(g);

        switch(page_idx) {
            case 0:
                /* Page 0 now adds the container itself to always keep an focus element for event bubbling */
                lv_group_add_obj(g, page_containers[0]);
                lv_group_focus_obj(page_containers[0]);
                break;
            case 1:
                lv_group_add_obj(g, btn_p2_uset);
                lv_group_add_obj(g, btn_p2_iset);
                lv_group_add_obj(g, btn_p2_toggle);
                lv_group_focus_obj(btn_p2_uset);
                break;
            case 2:
                lv_group_add_obj(g, btn_p3_idis);
                lv_group_add_obj(g, btn_p3_toggle);
                lv_group_focus_obj(btn_p3_idis);
                break;
            case 3:
                lv_group_add_obj(g, btn_p4_baud);
                lv_group_add_obj(g, btn_p4_port);
                lv_group_focus_obj(btn_p4_baud);
                break;
        }
    }

    current_page = page_idx;
}

static void widget_click_handler(lv_event_t * e)
{
    lv_obj_t * obj = lv_event_get_target(e);
    lv_group_t * g = lv_group_get_default();
    if(!g) return;

    if(obj == btn_p2_uset || obj == btn_p2_iset || obj == btn_p3_idis || obj == btn_p4_baud || obj == btn_p4_port) {
        /* Toggle LVGL native group editing mode */
        if(lv_group_get_editing(g)) {
            lv_group_set_editing(g, false);
            lv_obj_remove_state(obj, LV_STATE_USER_1); // De-assert gold edit styling
            
            /* Show bottom bar when exiting edit mode */
            lv_obj_t * bottom_bar = get_hud_button_bottom_bar(obj);
            if(bottom_bar) lv_obj_remove_flag(bottom_bar, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_group_set_editing(g, true);
            lv_obj_add_state(obj, LV_STATE_USER_1);    // Assert gold edit styling
            
            /* Hide bottom bar when entering edit mode */
            lv_obj_t * bottom_bar = get_hud_button_bottom_bar(obj);
            if(bottom_bar) lv_obj_add_flag(bottom_bar, LV_OBJ_FLAG_HIDDEN);
        }
    }
    else if(obj == btn_p2_toggle) {
        /* Start/Stop Charging Simulation */
        charge_active = !charge_active;
        discharge_active = 0; /* Turn off discharge */
        low_volt_alert = 0;

        lv_obj_t * lbl = lv_obj_get_child(btn_p2_toggle, 0);
        if(charge_active) {
            lv_label_set_text(lbl, "CHG: ON");
            lv_obj_set_style_text_color(btn_p2_toggle, COLOR_GOLD, 0);
        } else {
            lv_label_set_text(lbl, "CHG: OFF");
            lv_obj_set_style_text_color(btn_p2_toggle, COLOR_GOLD, 0);
        }
        /* Reset discharge button label if visible */
        if(btn_p3_toggle) {
            lv_obj_t * p3_lbl = lv_obj_get_child(btn_p3_toggle, 0);
            lv_label_set_text(p3_lbl, "DSC: OFF");
            lv_obj_set_style_text_color(btn_p3_toggle, COLOR_CYAN, 0);
        }
    }
    else if(obj == btn_p3_toggle) {
        /* Start/Stop Discharging Simulation */
        discharge_active = !discharge_active;
        charge_active = 0; /* Turn off charge */
        low_volt_alert = 0;

        lv_obj_t * lbl = lv_obj_get_child(btn_p3_toggle, 0);
        if(discharge_active) {
            lv_label_set_text(lbl, "DSC: ON");
            lv_obj_set_style_text_color(btn_p3_toggle, COLOR_RED, 0);
        } else {
            lv_label_set_text(lbl, "DSC: OFF");
            lv_obj_set_style_text_color(btn_p3_toggle, COLOR_CYAN, 0);
        }
        /* Reset charge button label if visible */
        if(btn_p2_toggle) {
            lv_obj_t * p2_lbl = lv_obj_get_child(btn_p2_toggle, 0);
            lv_label_set_text(p2_lbl, "CHG: OFF");
            lv_obj_set_style_text_color(btn_p2_toggle, COLOR_GOLD, 0);
        }
    }
}

static void widget_key_handler(lv_event_t * e)
{
    lv_obj_t * obj = lv_event_get_target(e);
    lv_key_t key = lv_event_get_key(e);
    lv_group_t * g = lv_group_get_default();
    if(!g) return;

    if(lv_group_get_editing(g)) {
        /* Handle adjustment of value variables inside edit mode */
        if(key == LV_KEY_UP || key == LV_KEY_RIGHT) {
            if(obj == btn_p2_uset) {
                target_u_set += 0.01f;
                if(target_u_set > 5.00f) target_u_set = 5.00f;
                char buf[20];
                snprintf(buf, sizeof(buf), "U: %.2fV", target_u_set);
                lv_label_set_text(lv_obj_get_child(btn_p2_uset, 0), buf);
            }
            else if(obj == btn_p2_iset) {
                target_i_set += 0.05f;
                if(target_i_set > 5.00f) target_i_set = 5.00f;
                char buf[20];
                snprintf(buf, sizeof(buf), "I: %.2fA", target_i_set);
                lv_label_set_text(lv_obj_get_child(btn_p2_iset, 0), buf);
            }
            else if(obj == btn_p3_idis) {
                target_i_dis += 0.10f;
                if(target_i_dis > 10.00f) target_i_dis = 10.00f;
                char buf[20];
                snprintf(buf, sizeof(buf), "I: %.2fA", target_i_dis);
                lv_label_set_text(lv_obj_get_child(btn_p3_idis, 0), buf);
            }
            else if(obj == btn_p4_baud) {
                baud_rate_idx = (baud_rate_idx + 1) % 2;
                lv_label_set_text(lv_obj_get_child(btn_p4_baud, 0), baud_rate_idx ? "Baud:115K" : "Baud:9K6");
            }
            else if(obj == btn_p4_port) {
                port_idx = (port_idx + 1) % 2;
                lv_label_set_text(lv_obj_get_child(btn_p4_port, 0), port_idx ? "Port:UART1" : "Port:UART0");
            }
        }
        else if(key == LV_KEY_DOWN || key == LV_KEY_LEFT) {
            if(obj == btn_p2_uset) {
                target_u_set -= 0.01f;
                if(target_u_set < 0.00f) target_u_set = 0.00f;
                char buf[20];
                snprintf(buf, sizeof(buf), "U: %.2fV", target_u_set);
                lv_label_set_text(lv_obj_get_child(btn_p2_uset, 0), buf);
            }
            else if(obj == btn_p2_iset) {
                target_i_set -= 0.05f;
                if(target_i_set < 0.00f) target_i_set = 0.00f;
                char buf[20];
                snprintf(buf, sizeof(buf), "I: %.2fA", target_i_set);
                lv_label_set_text(lv_obj_get_child(btn_p2_iset, 0), buf);
            }
            else if(obj == btn_p3_idis) {
                target_i_dis -= 0.10f;
                if(target_i_dis < 0.00f) target_i_dis = 0.00f;
                char buf[20];
                snprintf(buf, sizeof(buf), "I: %.2fA", target_i_dis);
                lv_label_set_text(lv_obj_get_child(btn_p3_idis, 0), buf);
            }
            else if(obj == btn_p4_baud) {
                baud_rate_idx = (baud_rate_idx + 1) % 2;
                lv_label_set_text(lv_obj_get_child(btn_p4_baud, 0), baud_rate_idx ? "Baud:115K" : "Baud:9K6");
            }
            else if(obj == btn_p4_port) {
                port_idx = (port_idx + 1) % 2;
                lv_label_set_text(lv_obj_get_child(btn_p4_port, 0), port_idx ? "Port:UART1" : "Port:UART0");
            }
        }
        else if(key == LV_KEY_ESC || key == LV_KEY_ENTER) {
            /* Exit edit mode on ENTER/ESC */
            lv_group_set_editing(g, false);
            lv_obj_remove_state(obj, LV_STATE_USER_1);
            
            /* Show bottom bar when exiting edit mode */
            lv_obj_t * bottom_bar = get_hud_button_bottom_bar(obj);
            if(bottom_bar) lv_obj_remove_flag(bottom_bar, LV_OBJ_FLAG_HIDDEN);
        }
    } else {
        /* 
         * Navigation Mode Focus Switching:
         * Bidirectional continuous rotation across all pages and focused widgets.
         * If we rotate past the first/last widget of a page, we transition to the previous/next page naturally.
         */
        if(key == ' ' || key == LV_KEY_ESC) {
            int next_page = (current_page + 1) % 4;
            switch_page(next_page);
        }
        else if(current_page == 0) {
            if(key == LV_KEY_RIGHT || key == LV_KEY_DOWN) {
                switch_page(1);
            } else if(key == LV_KEY_LEFT || key == LV_KEY_UP) {
                switch_page(3);
                lv_group_focus_obj(btn_p4_port);
            }
        }
        else if(current_page == 1) {
            if(key == LV_KEY_RIGHT || key == LV_KEY_DOWN) {
                if(obj == btn_p2_toggle) {
                    switch_page(2);
                } else {
                    lv_group_focus_next(g);
                }
            } else if(key == LV_KEY_LEFT || key == LV_KEY_UP) {
                if(obj == btn_p2_uset) {
                    switch_page(0);
                } else {
                    lv_group_focus_prev(g);
                }
            }
        }
        else if(current_page == 2) {
            if(key == LV_KEY_RIGHT || key == LV_KEY_DOWN) {
                if(obj == btn_p3_toggle) {
                    switch_page(3);
                } else {
                    lv_group_focus_next(g);
                }
            } else if(key == LV_KEY_LEFT || key == LV_KEY_UP) {
                if(obj == btn_p3_idis) {
                    switch_page(1);
                    lv_group_focus_obj(btn_p2_toggle);
                } else {
                    lv_group_focus_prev(g);
                }
            }
        }
        else if(current_page == 3) {
            if(key == LV_KEY_RIGHT || key == LV_KEY_DOWN) {
                if(obj == btn_p4_port) {
                    switch_page(0);
                } else {
                    lv_group_focus_next(g);
                }
            } else if(key == LV_KEY_LEFT || key == LV_KEY_UP) {
                if(obj == btn_p4_baud) {
                    switch_page(2);
                    lv_group_focus_obj(btn_p3_toggle);
                } else {
                    lv_group_focus_prev(g);
                }
            }
        }
    }
}

static void global_key_handler(lv_event_t * e)
{
    lv_key_t key = lv_event_get_key(e);
    lv_group_t * g = lv_group_get_default();


    /* 
     * Switch page when space character ' ' is pressed or ESC key is pressed.
     * Ensure we are NOT in editing mode when switching pages, so rotation inputs work for page focusing.
     */
    if((key == ' ' || key == LV_KEY_ESC) && (!g || !lv_group_get_editing(g))) {
        int next_page = (current_page + 1) % 4;
        switch_page(next_page);
    }
    else if(current_page == 0 && (!g || !lv_group_get_editing(g))) {
        if(key == LV_KEY_RIGHT || key == LV_KEY_DOWN) {
            switch_page(1);
        } else if(key == LV_KEY_LEFT || key == LV_KEY_UP) {
            switch_page(3);
        }
    }
}

/**
 * Battery State & Chemistry simulator loop (runs every 200ms)
 */
static void bms_sim_tick(lv_timer_t * timer)
{
    (void)timer;

    /* Compute Open Circuit Voltage based on simulated cell SoC */
    float norm_soc = batt_soc / 100.0f;
    batt_ocv = 3.0f + 1.15f * norm_soc - 0.08f * (1.0f - norm_soc) * (1.0f - norm_soc) * (1.0f - norm_soc);

    if(charge_active) {
        /* CCCV Charging Simulation */
        if(batt_u_real < target_u_set) {
            /* Constant Current Phase */
            batt_i_real = target_i_set;
            batt_u_real = batt_ocv + batt_i_real * batt_r_int;
            
            /* If terminal voltage computed hits threshold, clamp and enter CV phase */
            if(batt_u_real >= target_u_set) {
                batt_u_real = target_u_set;
            }
        } else {
            /* Constant Voltage Phase - current decays exponentially */
            batt_u_real = target_u_set;
            batt_i_real = (target_u_set - batt_ocv) / batt_r_int;
            if(batt_i_real < 0.05f) {
                batt_i_real = 0.0f; /* Charge Complete */
            }
        }

        /* Update dynamic variables */
        batt_soc += (batt_i_real * 0.2f / 3600.0f) / 2.6f * 100.0f; /* Assumes 2.6Ah battery cell */
        if(batt_soc > 100.0f) batt_soc = 100.0f;

        /* Joule heating: Temp increases with I^2 * R, cools to ambient 25C */
        batt_t_real += (batt_i_real * batt_i_real * batt_r_int * 0.2f * 0.05f) - (batt_t_real - 25.0f) * 0.005f;
    }
    else if(discharge_active) {
        /* CC Discharging Simulation */
        batt_i_real = -target_i_dis; /* Negative current denotes discharge output */
        batt_u_real = batt_ocv + batt_i_real * batt_r_int;

        /* Update battery SoC */
        batt_soc += (batt_i_real * 0.2f / 3600.0f) / 2.6f * 100.0f;
        if(batt_soc < 0.0f) {
            batt_soc = 0.0f;
            discharge_active = 0;
            if(btn_p3_toggle) {
                lv_label_set_text(lv_obj_get_child(btn_p3_toggle, 0), "DSC: OFF");
                lv_obj_set_style_text_color(btn_p3_toggle, COLOR_CYAN, 0);
            }
        }

        /* Low Voltage Cutoff Check */
        if(batt_u_real < 2.80f) {
            discharge_active = 0;
            low_volt_alert = 1;
            batt_i_real = 0.00f;
            batt_u_real = batt_ocv;
            
            if(btn_p3_toggle) {
                lv_label_set_text(lv_obj_get_child(btn_p3_toggle, 0), "DSC: OFF");
                lv_obj_set_style_text_color(btn_p3_toggle, COLOR_CYAN, 0);
            }
        }

        batt_t_real += (batt_i_real * batt_i_real * batt_r_int * 0.2f * 0.05f) - (batt_t_real - 25.0f) * 0.005f;
    }
    else {
        /* Standby state (cooling and stabilization) */
        batt_i_real = 0.00f;
        batt_u_real = batt_ocv;
        batt_t_real -= (batt_t_real - 25.0f) * 0.005f;
    }

    /* Update Page 1 labels (ASCII codes prevent square box indicator issues) */
    if(current_page == 0) {
        char buf[20];
        snprintf(buf, sizeof(buf), "U: %.3f V", batt_u_real);
        lv_label_set_text(lbl_p1_u, buf);

        snprintf(buf, sizeof(buf), "I: %+.2f A", batt_i_real);
        lv_label_set_text(lbl_p1_i, buf);

        snprintf(buf, sizeof(buf), "T: %.1f C", batt_t_real);
        lv_label_set_text(lbl_p1_t, buf);

        if(charge_active) {
            lv_label_set_text(lbl_p1_trend, "[+ CHARGING]");
            lv_obj_set_style_text_color(lbl_p1_trend, COLOR_GOLD, 0);
        } else if(discharge_active) {
            lv_label_set_text(lbl_p1_trend, "[- DISCHG]");
            lv_obj_set_style_text_color(lbl_p1_trend, COLOR_RED, 0);
        } else {
            if(low_volt_alert) {
                lv_label_set_text(lbl_p1_trend, "[! L-VOLT]");
                lv_obj_set_style_text_color(lbl_p1_trend, COLOR_RED, 0);
            } else {
                lv_label_set_text(lbl_p1_trend, "[STANDBY]");
                lv_obj_set_style_text_color(lbl_p1_trend, COLOR_GRAY, 0); // Muted standby color
            }
        }
    }
    /* Update Page 2 charts & labels */
    else if(current_page == 1) {
        char buf[24];
        snprintf(buf, sizeof(buf), "U: %.2fV  I: %.2fA", batt_u_real, charge_active ? batt_i_real : 0.0f);
        lv_label_set_text(lbl_p2_readout, buf);

        /* Map real readings to chart range (0-5000) */
        int32_t c_u = (int32_t)(batt_u_real * 1000.0f);
        int32_t c_i = (int32_t)((charge_active ? batt_i_real : 0.0f) * 1000.0f);
        lv_chart_set_next_value(chart_p2, chart_p2_u_series, c_u);
        lv_chart_set_next_value(chart_p2, chart_p2_i_series, c_i);
    }
    /* Update Page 3 charts & labels */
    else if(current_page == 2) {
        char buf[24];
        if(low_volt_alert) {
            snprintf(buf, sizeof(buf), "CUTOFF! U < 2.80V");
            lv_label_set_text(lbl_p3_readout, buf);
            lv_obj_set_style_text_color(lbl_p3_readout, COLOR_RED, 0);
        } else {
            snprintf(buf, sizeof(buf), "U: %.2fV  I: %.2fA", batt_u_real, discharge_active ? -batt_i_real : 0.0f);
            lv_label_set_text(lbl_p3_readout, buf);
            lv_obj_set_style_text_color(lbl_p3_readout, COLOR_GRAY, 0);
        }

        /* Map discharge values to chart. Voltage 0-5V, current 0-10A mapped to 0-5000 */
        int32_t c_u = (int32_t)(batt_u_real * 1000.0f);
        int32_t c_i = (int32_t)((discharge_active ? -batt_i_real : 0.0f) * 500.0f); // 500 scaling yields 5000 at 10A
        lv_chart_set_next_value(chart_p3, chart_p3_u_series, c_u);
        lv_chart_set_next_value(chart_p3, chart_p3_i_series, c_i);
    }
}

/**
 * Host sync connection simulation (runs every 1000ms)
 */
static void host_comm_tick(lv_timer_t * timer)
{
    (void)timer;

    /* Simulate periodic packets from Host */
    rx_packet_count += (rand() % 2 + 1);
    
    /* Simulate a smart estimation model where host predicted SoC slowly trails real battery SoC */
    float difference = batt_soc - (float)global_predicted_soc;
    if(difference > 1.0f) {
        global_predicted_soc++;
    } else if(difference < -1.0f) {
        global_predicted_soc--;
    } else {
        /* Noise fluctuation */
        if(rand() % 10 == 0) {
            global_predicted_soc = (uint8_t)batt_soc;
        }
    }

    /* Keep values clamped */
    if(global_predicted_soc > 100) global_predicted_soc = 100;

    /* Push updates to the UI */
    bms_ui_update_soc(global_predicted_soc);

    /* Telemetry terminal string rolling simulation (Page 4) */
    static int log_cycle = 0;
    log_cycle++;
    
    char log_entry[64];
    switch(log_cycle % 4) {
        case 0:
            snprintf(log_entry, sizeof(log_entry), "RX -> SOC: %03d%%", global_predicted_soc);
            break;
        case 1:
            snprintf(log_entry, sizeof(log_entry), "RX -> PACKETS: %04d", rx_packet_count);
            break;
        case 2:
            snprintf(log_entry, sizeof(log_entry), "RX -> CELL_U: %.2fV", batt_u_real);
            break;
        case 3:
            snprintf(log_entry, sizeof(log_entry), "LINK -> SYN %s", baud_rate_idx ? "115K" : "9K6");
            break;
    }

    /* Shift lines of the buffer */
    char line1[64], line2[64], line3[64];
    line1[0] = '\0'; line2[0] = '\0'; line3[0] = '\0';

    /* Parse current lines from the label text */
    const char * current_txt = lv_label_get_text(lbl_p4_terminal);
    if(current_txt) {
        const char * p1 = strchr(current_txt, '\n');
        if(p1) {
            p1++; // skip newline
            const char * p2 = strchr(p1, '\n');
            if(p2) {
                int len1 = p2 - p1;
                if(len1 > 63) len1 = 63;
                strncpy(line1, p1, len1);
                line1[len1] = '\0';

                p2++; // skip newline
                const char * p3 = strchr(p2, '\n');
                if(p3) {
                    int len2 = p3 - p2;
                    if(len2 > 63) len2 = 63;
                    strncpy(line2, p2, len2);
                    line2[len2] = '\0';
                    
                    strncpy(line3, p3, 63);
                    line3[63] = '\0';
                } else {
                    strncpy(line2, p2, 63);
                    line2[63] = '\0';
                }
            }
        }
    }

    /* Format rolled buffer back into label */
    if(line1[0] != '\0') {
        snprintf(terminal_buffer, sizeof(terminal_buffer), "%s\n%s\n%s\n%s", line1, line2, line3, log_entry);
    } else {
        snprintf(terminal_buffer, sizeof(terminal_buffer), "%s", log_entry);
    }

    if(lbl_p4_terminal && current_page == 3) {
        lv_label_set_text(lbl_p4_terminal, terminal_buffer);
    }
}
