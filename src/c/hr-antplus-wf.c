#include <pebble.h>

static Window *s_window;
static TextLayer *s_time_layer;
static TextLayer *s_heart_rate_layer;
static TextLayer *s_battery_level;
// static Layer *s_line_layer;
static Layer *s_bottom_section_layer;

static void bottom_section_layer_update(Layer *layer, GContext *ctx){
  // graphics_draw_rect(ctx,GRect(0, 120, bounds.size.w/2, 25));

  graphics_context_set_fill_color(ctx,GColorDarkGreen);

  graphics_draw_circle(ctx,GPoint(50,80),5);

}

static void update_time() {
  // Get a tm structure
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);

  // Write the current hours and minutes into a buffer
  static char s_buffer[8];
  strftime(s_buffer, sizeof(s_buffer), clock_is_24h_style() ?
                                          "%H:%M" : "%I:%M", tick_time);

  // Display this time on the TextLayer
  text_layer_set_text(s_time_layer, s_buffer);
}

// ----- Callbacks

// Time
static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
}

static void battery_callback(BatteryChargeState state) {
  // Record the new battery level
  static char batt_buffer[4];
  snprintf(batt_buffer,sizeof(batt_buffer),"%d",(int)state.charge_percent);
  text_layer_set_text(s_battery_level,batt_buffer);
}

static void inbox_received_callback(DictionaryIterator *iter, void *context) {

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Got Some: %lu", MESSAGE_KEY_HeartRate);


  // A new message has been successfully received
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Got Some: %lu", iter->cursor->key);

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Got Some: %d", iter->cursor->length);



  // Does this message contain a temperature value?
  Tuple *heartrate_tuple = dict_find(iter, MESSAGE_KEY_HeartRate);
  if(heartrate_tuple) {

    // This value was stored as JS Number, which is stored here as int32_t
    int32_t hr = heartrate_tuple->value->int32;
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Inside: %s", "as");
    static char hr_buf[4];
    snprintf(hr_buf,sizeof(hr_buf),"%lu",hr);
    text_layer_set_text(s_heart_rate_layer,hr_buf);
  }

}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}



static void prv_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  // Heart Rate
  s_heart_rate_layer = text_layer_create(GRect(0, 30, bounds.size.w, 50));
  text_layer_set_text(s_heart_rate_layer, "123");
  text_layer_set_font(s_heart_rate_layer,fonts_get_system_font(FONT_KEY_LECO_42_NUMBERS));
  text_layer_set_text_alignment(s_heart_rate_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_heart_rate_layer));

  // Bottom Section
  s_bottom_section_layer = layer_create(GRect(0,100, bounds.size.w, 100));
  // s_bottom_section_layer = layer_create(bounds);
  layer_set_update_proc(s_bottom_section_layer,bottom_section_layer_update);
  layer_add_child(window_layer,s_bottom_section_layer);


  // Time
  s_time_layer = text_layer_create(GRect(0, 120, bounds.size.w/2, 25));
  text_layer_set_font(s_time_layer,fonts_get_system_font(FONT_KEY_LECO_20_BOLD_NUMBERS));
  text_layer_set_text(s_time_layer, "10:23");
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));


  // Battery Level
  s_battery_level = text_layer_create(GRect(bounds.size.w/2, 120, bounds.size.w/2, 25));
  text_layer_set_font(s_battery_level,fonts_get_system_font(FONT_KEY_ROBOTO_CONDENSED_21));
  text_layer_set_text(s_battery_level, "50%");
  text_layer_set_background_color(s_battery_level,GColorDarkGreen);
  text_layer_set_text_color(s_battery_level,GColorWhite);
  text_layer_set_text_alignment(s_battery_level, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_battery_level));


}

static void prv_window_unload(Window *window) {
  text_layer_destroy(s_time_layer);
  text_layer_destroy(s_heart_rate_layer);
  text_layer_destroy(s_battery_level);
  layer_destroy(s_bottom_section_layer);
}

static void prv_init(void) {
  s_window = window_create();
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = prv_window_load,
    .unload = prv_window_unload,
  });

  // Registering callbacks
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  battery_state_service_subscribe(battery_callback);
  app_message_register_inbox_received(inbox_received_callback);

  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);

  const bool animated = true;
  window_stack_push(s_window, animated);

  // Largest expected inbox and outbox message sizes
  const uint32_t inbox_size = 64;
  const uint32_t outbox_size = 256;

  // Open AppMessage
  app_message_open(inbox_size, outbox_size);

  // For start contents
  update_time();
  battery_callback(battery_state_service_peek());
}

static void prv_deinit(void) {
  window_destroy(s_window);
}

int main(void) {
  prv_init();

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", s_window);

  app_event_loop();
  prv_deinit();
}
