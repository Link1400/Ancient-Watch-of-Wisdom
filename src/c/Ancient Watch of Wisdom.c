#include <pebble.h>

static Window *logo;
static Window *s_window;
static Window *s_submenu_window;
static MenuLayer *s_menu_layer;
static MenuLayer *s_submenu_layer;
static BitmapLayer *s_mqlogo;
static GBitmap *s_logoimg;
static GFont s_mason_sans;
static uint16_t s_selected_menu;

#if defined(PBL_PLATFORM_EMERY)
#define LOGO_RESOURCE RESOURCE_ID_IMAGE_ICON_EMERY
#elif defined(PBL_ROUND)
#define LOGO_RESOURCE RESOURCE_ID_IMAGE_ICON_ROUND
#else
#define LOGO_RESOURCE RESOURCE_ID_IMAGE_ICON
#endif

static void prv_logo_select_click_handler(ClickRecognizerRef recognizer,
                                          void *context) {
  window_stack_push(s_window, true);
}

static void prv_logo_click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT,
                                prv_logo_select_click_handler);
}

static uint16_t prv_menu_get_num_rows(MenuLayer *menu_layer,
                                      uint16_t section_index,
                                      void *context) {
  return 3;
}

static const uint16_t s_submenu_counts[3] = { 13, 5, 3 };

static const char *s_menu_titles[2][13] = {
  {
    "Lightning", "Quest Two", "Quest Three", "Quest Four",
    "Quest Five", "Quest Six", "Quest Seven", "Quest Eight",
    "Quest Nine", "Quest Ten", "Quest Eleven", "Quest Twelve",
    "Quest Thirteen"
  },
  {
    "Adventure One", "Adventure Two", "Adventure Three",
    "Adventure Four", "Adventure Five"
  }
};

static uint16_t prv_submenu_get_num_rows(MenuLayer *menu_layer,
                                         uint16_t section_index,
                                         void *context) {
  return s_submenu_counts[s_selected_menu];
}

static void prv_draw_menu_row(GContext *ctx,
                              const Layer *cell_layer,
                              const char *title) {
  const bool highlighted = menu_cell_layer_is_highlighted(cell_layer);
  const GRect bounds = layer_get_bounds(cell_layer);

  graphics_context_set_fill_color(ctx,
                                  highlighted ? GColorBlack : GColorWhite);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  graphics_context_set_text_color(ctx,
                                  highlighted ? GColorWhite : GColorBlack);
  graphics_draw_text(ctx, title, s_mason_sans, bounds,
                     GTextOverflowModeTrailingEllipsis,
                     GTextAlignmentCenter, NULL);
}

static void prv_menu_draw_row(GContext *ctx,
                              const Layer *cell_layer,
                              MenuIndex *cell_index,
                              void *context) {
  const char *titles[] = { "Quests", "Adventures"};
  prv_draw_menu_row(ctx, cell_layer, titles[cell_index->row]);
}

static void prv_menu_select(MenuLayer *menu_layer,
                            MenuIndex *cell_index,
                            void *context) {
  s_selected_menu = cell_index->row;
  window_stack_push(s_submenu_window, true);
}

static void prv_submenu_draw_row(GContext *ctx,
                                 const Layer *cell_layer,
                                 MenuIndex *cell_index,
                                 void *context) {
  prv_draw_menu_row(ctx, cell_layer,
                    s_menu_titles[s_selected_menu][cell_index->row]);
}

static void prv_logowindow_load(Window *logo){
  Layer *logoLayer = window_get_root_layer(logo);
  GRect bounds = layer_get_bounds(logoLayer);

  s_logoimg = gbitmap_create_with_resource(LOGO_RESOURCE);
  s_mqlogo = bitmap_layer_create(bounds);
  
  bitmap_layer_set_bitmap(s_mqlogo, s_logoimg);
  bitmap_layer_set_alignment(s_mqlogo, GAlignCenter);

  layer_add_child(logoLayer, bitmap_layer_get_layer(s_mqlogo));
}

static void prv_logowindow_unload(Window *logo){
  bitmap_layer_destroy(s_mqlogo);
  gbitmap_destroy(s_logoimg);

  s_mqlogo = NULL;
  s_logoimg = NULL;
}

static void prv_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_menu_layer = menu_layer_create(bounds);
  menu_layer_set_callbacks(s_menu_layer, NULL, (MenuLayerCallbacks) {
    .get_num_rows = prv_menu_get_num_rows,
    .draw_row = prv_menu_draw_row,
    .select_click = prv_menu_select,
  });
  menu_layer_set_click_config_onto_window(s_menu_layer, window);
  layer_add_child(window_layer, menu_layer_get_layer(s_menu_layer));
}

static void prv_window_unload(Window *window) {
  menu_layer_destroy(s_menu_layer);
  s_menu_layer = NULL;
}

static void prv_submenu_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_submenu_layer = menu_layer_create(bounds);
  menu_layer_set_callbacks(s_submenu_layer, NULL, (MenuLayerCallbacks) {
    .get_num_rows = prv_submenu_get_num_rows,
    .draw_row = prv_submenu_draw_row,
  });
  menu_layer_set_click_config_onto_window(s_submenu_layer, window);
  layer_add_child(window_layer,
                  menu_layer_get_layer(s_submenu_layer));
}

static void prv_submenu_unload(Window *window) {
  menu_layer_destroy(s_submenu_layer);
  s_submenu_layer = NULL;
}

static void prv_init(void) {
  s_mason_sans = fonts_load_custom_font(
  resource_get_handle(RESOURCE_ID_FONT_MASON_SAN_REGULAR_24));

  logo = window_create();
  window_set_click_config_provider(logo, prv_logo_click_config_provider);
  window_set_window_handlers(logo, (WindowHandlers) {
    .load = prv_logowindow_load,
    .unload = prv_logowindow_unload,
  });

  s_window = window_create();
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = prv_window_load,
    .unload = prv_window_unload,
  });

  s_submenu_window = window_create();
  window_set_window_handlers(s_submenu_window, (WindowHandlers) {
    .load = prv_submenu_load,
    .unload = prv_submenu_unload,
  });

  const bool animated = true;
  window_stack_push(logo, animated);
}

static void prv_deinit(void) {
  fonts_unload_custom_font(s_mason_sans);
  window_destroy(logo);
  window_destroy(s_window);
  window_destroy(s_submenu_window);
}

int main(void) {
  prv_init();

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", s_window);

  app_event_loop();
  prv_deinit();
}
