#include <pebble.h>

static Window *logo;
static Window *s_window;
static Window *s_submenu_window;
static Window *s_timeline_window;
static MenuLayer *s_menu_layer;
static MenuLayer *s_submenu_layer;
static MenuLayer *s_timeline_layer;
static BitmapLayer *s_mqlogo;
static GBitmap *s_logoimg;
static GFont s_mason_sans;
static GFont s_mason_sans_small;
static uint16_t s_selected_menu;
static uint16_t s_selected_item;
static bool s_timeline_completed[6];

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

static const uint16_t s_submenu_counts[3] = { 16, 6, 3 };

static const char *s_menu_titles[2][16] = {
  {
    "Lightning", "Dazzle", "Enchant", "Healing",
    "Portal", "Protection", "Freeze", "Ice Arrow",
    "Distract", "Reveal", "Celestial", "Great Wolf",
    "Master Magi", "Fire", "Potion", "Shield"
  },
  {
    "Pixie", "Goblin", "Ursa Major",
    "Dragon", "Ice Dragon", "Silver Dragon"
  }
};

static uint16_t prv_submenu_get_num_rows(MenuLayer *menu_layer,
                                         uint16_t section_index,
                                         void *context) {
  return s_submenu_counts[s_selected_menu];
}

static const char *s_quest_steps[16][6] = {
  { "Codebook", "Shield", "Armor",
    "Sword", "Mist/Gargoyle"},
  { "Seek five crystals", "Gather the colors", "Find the Rune",
    "Return to the castle", "Claim the reward", "Master Dazzle" },
  { "Find the enchanted creature", "Cast the enchantment",
    "Recover the trophy", "Return to the castle", "Claim the reward",
    "Master Enchant" },
  { "Find moss and knotweed", "Find the pine tree", "Gather the fungus",
    "Brew the healing potion", "Learn the Healing Rune", "Master Healing" },
  { "Find the Portal Rune", "Seek the Castle", "Touch the portal",
    "Collect the pieces", "Return to the castle", "Master Portal" },
  { "Find the green garment", "Find the white wizard", "Find the tree",
    "Find the fairy painting", "Return to the castle", "Master Freeze" },
  { "Find the gold pieces", "Collect three crystals", "Find the Glass",
    "Guard the castle", "Claim the reward", "Master Protection" },
  { "Read the Master's tale", "Find the three skulls", "Find the snowflakes",
    "Find the ticket", "Return to the castle", "Master Ice Arrow" },
  { "Find the hidden clue", "Follow the trail", "Complete the task",
    "Return with proof", "Claim the reward", "Master Distract" },
  { "Learn the secret sign", "Find the hidden mark", "Reveal the clue",
    "Return with the answer", "Claim the reward", "Master Reveal" },
  { "Find the celestial clue", "Follow the star trail", "Gather the sign",
    "Return to the castle", "Claim the reward", "Master Celestial" },
  { "Find the wolf's trail", "Follow the tracks", "Find the great wolf",
    "Return with proof", "Claim the reward", "Master Great Wolf" },
  { "Complete the first trial", "Complete the second trial",
    "Complete the third trial", "Complete the fourth trial",
    "Complete the final trial", "Become a Master Magi" },
  { "Find the flame", "Gather the fire sign", "Practice the spell",
    "Complete the fire trial", "Claim the reward", "Master Fire" },
  { "Find the ingredients", "Gather the potion sign", "Brew the potion",
    "Complete the potion trial", "Claim the reward", "Master Potion" },
  { "Find the shield sign", "Gather the protection clues", "Practice the ward",
    "Complete the shield trial", "Claim the reward", "Master Shield" },
};

static const char *s_adventure_steps[6][6] = {
  { "Meet the Pixie", "Find the first clue", "Explore the trail",
    "Solve the Pixie's task", "Return with proof", "Complete Pixie" },
  { "Meet the Goblin", "Find the first clue", "Explore the trail",
    "Solve the Goblin's task", "Return with proof", "Complete Goblin" },
  { "Find the Ursa clue", "Follow the star trail", "Explore the signs",
    "Solve the constellation task", "Return with proof", "Complete Ursa Major" },
  { "Meet the Dragon", "Find the first clue", "Explore the lair",
    "Solve the Dragon's task", "Return with proof", "Complete Dragon" },
  { "Meet the Ice Dragon", "Find the frozen clue", "Explore the lair",
    "Solve the Ice Dragon's task", "Return with proof", "Complete Ice Dragon" },
  { "Meet the Silver Dragon", "Find the silver clue", "Explore the lair",
    "Solve the Silver Dragon's task", "Return with proof", "Complete Silver Dragon" },
};

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

static void prv_submenu_select(MenuLayer *menu_layer,
                                MenuIndex *cell_index,
                                void *context) {
  s_selected_item = cell_index->row;
  window_stack_push(s_timeline_window, true);
}

static uint16_t prv_timeline_get_num_rows(MenuLayer *menu_layer,
                                          uint16_t section_index,
                                          void *context) {
  return 6;
}

static int16_t prv_timeline_get_cell_height(MenuLayer *menu_layer,
                                            MenuIndex *cell_index,
                                            void *context) {
#if defined(PBL_PLATFORM_EMERY) || defined(PBL_ROUND)
  return 52;
#else
  return 42;
#endif
}

static void prv_timeline_draw_row(GContext *ctx,
                                  const Layer *cell_layer,
                                  MenuIndex *cell_index,
                                  void *context) {
  const bool highlighted = menu_cell_layer_is_highlighted(cell_layer);
  const GRect bounds = layer_get_bounds(cell_layer);
  const int16_t center_y = bounds.origin.y + bounds.size.h / 2;
  const int16_t marker_x = 18;
  const GColor foreground = highlighted ? GColorWhite : GColorBlack;
  const char *step_label = s_selected_menu == 0
    ? s_quest_steps[s_selected_item][cell_index->row]
    : s_adventure_steps[s_selected_item][cell_index->row];

  graphics_context_set_fill_color(ctx,
                                  highlighted ? GColorBlack : GColorWhite);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  graphics_context_set_stroke_color(ctx, foreground);
  graphics_context_set_stroke_width(ctx, 2);
  graphics_draw_line(ctx,
                     GPoint(marker_x, bounds.origin.y),
                     GPoint(marker_x, bounds.origin.y + bounds.size.h));

  graphics_context_set_fill_color(ctx,
                                  s_timeline_completed[cell_index->row]
                                    ? foreground : GColorWhite);
  graphics_fill_circle(ctx, GPoint(marker_x, center_y), 6);
  graphics_context_set_stroke_color(ctx, foreground);
  graphics_draw_circle(ctx, GPoint(marker_x, center_y), 6);

  graphics_context_set_text_color(ctx, foreground);
  GRect text_bounds = GRect(34, bounds.origin.y,
                            bounds.size.w - 38, bounds.size.h);
  graphics_draw_text(ctx, step_label, s_mason_sans_small,
                     text_bounds, GTextOverflowModeTrailingEllipsis,
                     GTextAlignmentLeft, NULL);
}

static void prv_timeline_select(MenuLayer *menu_layer,
                                MenuIndex *cell_index,
                                void *context) {
  s_timeline_completed[cell_index->row] =
    !s_timeline_completed[cell_index->row];
  menu_layer_reload_data(menu_layer);
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
    .select_click = prv_submenu_select,
  });
  menu_layer_set_click_config_onto_window(s_submenu_layer, window);
  layer_add_child(window_layer,
                  menu_layer_get_layer(s_submenu_layer));
}

static void prv_submenu_unload(Window *window) {
  menu_layer_destroy(s_submenu_layer);
  s_submenu_layer = NULL;
}

static void prv_timeline_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  memset(s_timeline_completed, 0, sizeof(s_timeline_completed));
#if defined(PBL_ROUND)
  bounds = GRect(12, 20, bounds.size.w - 24, bounds.size.h - 40);
#endif
  s_timeline_layer = menu_layer_create(bounds);
  menu_layer_set_callbacks(s_timeline_layer, NULL, (MenuLayerCallbacks) {
    .get_num_rows = prv_timeline_get_num_rows,
    .get_cell_height = prv_timeline_get_cell_height,
    .draw_row = prv_timeline_draw_row,
    .select_click = prv_timeline_select,
  });
  menu_layer_set_center_focused(s_timeline_layer, false);
  menu_layer_set_click_config_onto_window(s_timeline_layer, window);
  layer_add_child(window_layer,
                  menu_layer_get_layer(s_timeline_layer));
}

static void prv_timeline_unload(Window *window) {
  menu_layer_destroy(s_timeline_layer);
  s_timeline_layer = NULL;
}

static void prv_init(void) {
  s_mason_sans = fonts_load_custom_font(
  resource_get_handle(RESOURCE_ID_FONT_MASON_SAN_REGULAR_24));
  s_mason_sans_small = fonts_load_custom_font(
  resource_get_handle(RESOURCE_ID_FONT_MASON_SAN_REGULAR_18));

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

  s_timeline_window = window_create();
  window_set_window_handlers(s_timeline_window, (WindowHandlers) {
    .load = prv_timeline_load,
    .unload = prv_timeline_unload,
  });

  const bool animated = true;
  window_stack_push(logo, animated);
}

static void prv_deinit(void) {
  fonts_unload_custom_font(s_mason_sans);
  fonts_unload_custom_font(s_mason_sans_small);
  window_destroy(logo);
  window_destroy(s_window);
  window_destroy(s_submenu_window);
  window_destroy(s_timeline_window);
}

int main(void) {
  prv_init();

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", s_window);

  app_event_loop();
  prv_deinit();
}
