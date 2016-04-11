#include <pebble.h>

Window *my_window;
TextLayer *time_layer, *date_layer, *battery_percent, *bt_layer, *health1_layer, *health2_layer;
Layer *battery_layer;
int battery_level;
static GFont s_time_font, s_medium_font, s_small_font;

GColor colourBlue;
GColor colourRed;
GColor colourOrange;
GColor colourGreen;
GColor colourBrightG;

#define KEY_COLOR_RED_BG     0
#define KEY_COLOR_GREEN_BG   1
#define KEY_COLOR_BLUE_BG    2
#define KEY_HIGH_CONTRAST    3

static void update_time() {
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);
  
  static char s_buffer[8];
  static char date_buffer[16];
  strftime(s_buffer, sizeof(s_buffer), clock_is_24h_style() ?
      "%H:%M" : "%l:%M", tick_time);
  strftime(date_buffer, sizeof(date_buffer), "%a %d %b", tick_time);
  
  text_layer_set_text(time_layer, s_buffer);
  text_layer_set_text(date_layer, date_buffer);
}

static void draw_battery(Layer *layer, GContext *ctx) {
  graphics_context_set_stroke_color(ctx, GColorWhite);
  graphics_context_set_fill_color(ctx, GColorWhite);
  
  graphics_draw_rect(ctx, GRect(0, 0, 22, 13));
  graphics_draw_rect(ctx, GRect(22, 2, 2, 9));

  // Can't draw a 1 pixel rectangle.  Use 13 to ensure 10% => 2px height
  int perc = 16 * battery_level/100;
  if (battery_level < 20) {
    graphics_context_set_stroke_color(ctx, colourRed);
    graphics_context_set_fill_color(ctx, colourRed);
  } else if (battery_level < 31) {
    graphics_context_set_stroke_color(ctx, colourOrange);
    graphics_context_set_fill_color(ctx, colourOrange);
  } else {
    graphics_context_set_stroke_color(ctx, colourGreen);
    graphics_context_set_fill_color(ctx, colourGreen);
  }
  graphics_fill_rect(ctx, GRect(2, 2, 2+perc, 9), 0, GCornerNone);
  
}

static void battery_handler(BatteryChargeState state) {
  // Record the new battery level
  battery_level = state.charge_percent;
  layer_mark_dirty(battery_layer);
  static char buf[32];    /* <-- implicit NUL-terminator at the end here */

  snprintf(buf, sizeof(buf), "%d%%", battery_level);
  text_layer_set_text(battery_percent, buf);
  if (state.is_charging) {
    text_layer_set_text(battery_percent, "Charging");
  }
}

static void bluetooth_handler(bool connected) {
  if (connected == true) {
    text_layer_set_text_color(bt_layer, colourBlue);
    text_layer_set_text(bt_layer, "    Bluetooth");
  } else {
    text_layer_set_text_color(bt_layer, colourRed);
    text_layer_set_text(bt_layer, "No connection");
  }
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
}

#if defined(PBL_HEALTH)

static void health_update() {
  HealthMetric metricStep = HealthMetricStepCount;
  HealthMetric metricRestCal = HealthMetricRestingKCalories;
  HealthMetric metricActiveCal = HealthMetricActiveKCalories;
  time_t start = time_start_of_today();
  time_t end = time(NULL);

  // Check the metric has data available for today
  HealthServiceAccessibilityMask mask = health_service_metric_accessible(metricStep, 
    start, end);
  HealthServiceAccessibilityMask maskCal = health_service_metric_accessible(metricRestCal, 
    start, end);

  if(mask & HealthServiceAccessibilityMaskAvailable) {
    static char buf[] = "00000000000";   
    snprintf(buf, sizeof(buf), "%d Steps", (int)health_service_sum_today(metricStep));
    text_layer_set_text(health1_layer, buf);
  }
  if(maskCal & HealthServiceAccessibilityMaskAvailable) {
    static char buf[32];   
    snprintf(buf, sizeof(buf), "%d KCal", (int)health_service_sum_today(metricRestCal)+(int)health_service_sum_today(metricActiveCal));
    text_layer_set_text(health2_layer, buf);
  }
}

static void health_handler(HealthEventType event, void *context) {
  // Which type of event occured?
  switch(event) {
    case HealthEventSignificantUpdate:
      health_update();
      break;
    case HealthEventMovementUpdate:
      health_update();
      break;
    case HealthEventSleepUpdate:
      break;
  }
}


#endif

static void setBackground(int red, int green, int blue) {
	GColor bg_color = GColorFromRGB(red, green, blue);
	window_set_background_color(my_window, bg_color);
  text_layer_set_background_color(time_layer, bg_color);
  text_layer_set_background_color(date_layer, bg_color);
  text_layer_set_background_color(battery_percent, bg_color);
  text_layer_set_background_color(bt_layer, bg_color);
  text_layer_set_background_color(health1_layer, bg_color);
  text_layer_set_background_color(health2_layer, bg_color);
}

static void inbox_received_handler(DictionaryIterator *iter, void *context) {
  // High contrast selected?
  Tuple *high_contrast_t = dict_find(iter, KEY_HIGH_CONTRAST);
  if(high_contrast_t && high_contrast_t->value->int8 > 0) {  // Read boolean as an integer
    // Change color scheme
    window_set_background_color(my_window, GColorBlack);
    //text_layer_set_text_color(s_text_layer, GColorWhite);

    // Persist value
    persist_write_bool(KEY_HIGH_CONTRAST, true);
  } else {
    persist_write_bool(KEY_HIGH_CONTRAST, false);
  }

  // Color scheme?
  Tuple *color_red_t = dict_find(iter, KEY_COLOR_RED_BG);
  Tuple *color_green_t = dict_find(iter, KEY_COLOR_GREEN_BG);
  Tuple *color_blue_t = dict_find(iter, KEY_COLOR_BLUE_BG);
  if(color_red_t && color_green_t && color_blue_t) {
    // Apply the color if available
#if defined(PBL_BW)
    window_set_background_color(my_window, GColorWhite);
    //text_layer_set_text_color(s_text_layer, GColorBlack);
#elif defined(PBL_COLOR)
    int red = color_red_t->value->int32;
    int green = color_green_t->value->int32;
    int blue = color_blue_t->value->int32;

    // Persist values
    persist_write_int(KEY_COLOR_RED_BG, red);
    persist_write_int(KEY_COLOR_GREEN_BG, green);
    persist_write_int(KEY_COLOR_BLUE_BG, blue);

    setBackground(red, green, blue);
    //GColor bg_color = GColorFromRGB(red, green, blue);
    //window_set_background_color(my_window, bg_color);
    //text_layer_set_text_color(s_text_layer, gcolor_is_dark(bg_color) ? GColorWhite : GColorBlack);
#endif
  }
}
static void main_window_load(Window *window) {
  // Get the window info
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  window_set_background_color(window, GColorBlack);
  // Create the time layer
  time_layer = text_layer_create(
    GRect(0, 72, bounds.size.w, 56));

  // Improve the layout to be more like a watch
  text_layer_set_background_color(time_layer, GColorBlack);
  text_layer_set_text_color(time_layer, colourBrightG);
  text_layer_set_text(time_layer, "00:00");
  s_time_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_DS_DIGITAL_56));
  text_layer_set_font(time_layer, s_time_font);
  text_layer_set_text_alignment(time_layer, GTextAlignmentCenter);
  
  // Add it as a child layer to the windows root layer
  layer_add_child(window_layer, text_layer_get_layer(time_layer));
  
  // Date layer
  date_layer = text_layer_create(
    GRect(0, 140, bounds.size.w, 28));
  text_layer_set_background_color(date_layer, GColorBlack);
  text_layer_set_text_color(date_layer, GColorWhite);
  s_medium_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ROBOTO_18));
  text_layer_set_font(date_layer, s_medium_font);
  text_layer_set_text_alignment(date_layer, GTextAlignmentCenter);
    
  layer_add_child(window_layer, text_layer_get_layer(date_layer));
  
  // Battery layer (text)
  battery_percent = text_layer_create(
    GRect(28, 2, 47, 13));
  text_layer_set_background_color(battery_percent, GColorBlack);
  text_layer_set_text_color(battery_percent, GColorWhite);
  //text_layer_set_background_color(battery_percent, GColorClear);
  s_small_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ROBOTO_10));
  text_layer_set_font(battery_percent, s_small_font);
  text_layer_set_text_alignment(battery_percent, GTextAlignmentLeft);
  text_layer_set_text(battery_percent, "80%");
  layer_add_child(window_layer, text_layer_get_layer(battery_percent));
  
  // Battery layer (icon)
  battery_layer = layer_create(GRect(2,2,26,13));
  layer_add_child(window_layer, battery_layer);
  //battery_level = battery_state_service_peek().charge_percent;
  layer_set_update_proc(battery_layer, draw_battery);
  
  // update the battery percentage
  battery_handler(battery_state_service_peek());
  
  // bluetooth layer
  bt_layer = text_layer_create(
    GRect(75, 2, 69, 13));
  text_layer_set_background_color(bt_layer, GColorBlack);
  text_layer_set_text_color(bt_layer, colourBlue);
  //s_medium_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ROBOTO_18));
  text_layer_set_font(bt_layer, s_small_font);
  text_layer_set_text_alignment(bt_layer, GTextAlignmentCenter);
  text_layer_set_text(bt_layer, "    Bluetooth");
  layer_add_child(window_layer, text_layer_get_layer(bt_layer));
  
  // Subscribe to health events if we can
#if defined(PBL_HEALTH)
  health_service_events_subscribe(health_handler, NULL);
  
  health1_layer = text_layer_create(
    GRect(2, 25, 142, 28));
  text_layer_set_background_color(health1_layer, GColorBlack);
  text_layer_set_text_color(health1_layer, colourBrightG);
  text_layer_set_font(health1_layer, s_medium_font);
  text_layer_set_text_alignment(health1_layer, GTextAlignmentCenter);
  text_layer_set_text(health1_layer, "Waiting");
  layer_add_child(window_layer, text_layer_get_layer(health1_layer));
  
  health2_layer = text_layer_create(
    GRect(2, 53, 142, 28));
  text_layer_set_background_color(health2_layer, GColorBlack);
  text_layer_set_text_color(health2_layer, colourBrightG);
  text_layer_set_font(health2_layer, s_medium_font);
  text_layer_set_text_alignment(health2_layer, GTextAlignmentCenter);
  text_layer_set_text(health2_layer, "Waiting");
  layer_add_child(window_layer, text_layer_get_layer(health2_layer));
#endif
  }

static void main_window_unload(Window *window) {
  tick_timer_service_unsubscribe();
  battery_state_service_unsubscribe();
  bluetooth_connection_service_unsubscribe();
  text_layer_destroy(time_layer);
  text_layer_destroy(date_layer);
  text_layer_destroy(battery_percent);
  text_layer_destroy(health1_layer);
  text_layer_destroy(health2_layer);
  layer_destroy(battery_layer);
  text_layer_destroy(bt_layer);
  
}
void handle_init(void) {
  // Create the window element and assign to a pointer
  my_window = window_create();

  // Register with services
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  battery_state_service_subscribe(battery_handler);
  bluetooth_connection_service_subscribe(bluetooth_handler);
  
  colourBlue = COLOR_FALLBACK(GColorVeryLightBlue, GColorWhite);
  colourRed = COLOR_FALLBACK(GColorRed, GColorWhite);
  colourOrange = COLOR_FALLBACK(GColorOrange, GColorWhite);
  colourGreen = COLOR_FALLBACK(GColorGreen, GColorWhite);
  colourBrightG = COLOR_FALLBACK(GColorBrightGreen, GColorWhite);
  
  // Set handlers to manage the elements in the window
  window_set_window_handlers(my_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });
  
  app_message_register_inbox_received(inbox_received_handler);
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
  
  
  // Show the window on the watch with animated=true
  window_stack_push(my_window, true);
  
  update_time();
}

void handle_deinit(void) {
  window_destroy(my_window);
}

int main(void) {
  handle_init();
  app_event_loop();
  handle_deinit();
}
