#include "lvgl.h"
#include <cstring>
#include <vector>
#include <cstdlib>
#include <ctime>

#define PIN_PF10 PF10 // Haut
#define PIN_PF6 PF6   // Droite
#define PIN_PI2 PI2   // Gauche
#define PIN_PG7 PG7   // Bas

#ifdef ARDUINO

#include "lvglDrivers.h"
void start_snake_game(); 

static int32_t size = 0;
static bool size_dec = false;
static lv_obj_t *direction_label = NULL;
static lv_timer_t *hide_timer = NULL;
static lv_timer_t *snake_timer = NULL;

// Pour Snake
static bool game_started = false;
static lv_obj_t *snake_dot = NULL;
static int snake_x = 100, snake_y = 100;
static int dx = 0, dy = 0;
static lv_obj_t* score_label = NULL;


struct SnakePart
{
  int x;
  int y;
  lv_obj_t *obj;
};

std::vector<SnakePart> snake;
static lv_obj_t *apple = NULL;
static int score = 0;

static const char *sequence[] = {"Haut"}; //{"Haut", "Bas", "Gauche", "Droite"};
static int seq_index = 0;

bool is_on_snake(int x, int y)
{
  for (auto &part : snake)
  {
    if (part.x == x && part.y == y)
      return true;
  }
  return false;
}

static void event_handler(lv_event_t *e)
{
  lv_event_code_t code = lv_event_get_code(e);

  if (code == LV_EVENT_CLICKED)
  {
    LV_LOG_USER("Clicked");
  }
  else if (code == LV_EVENT_VALUE_CHANGED)
  {
    LV_LOG_USER("Toggled");
  }
}

static void timer_cb(lv_timer_t *timer)
{
  lv_obj_invalidate((lv_obj_t *)lv_timer_get_user_data(timer));
  if (size_dec)
    size--;
  else
    size++;

  if (size == 50)
    size_dec = true;
  else if (size == 0)
    size_dec = false;
}

static void event_cb(lv_event_t *e)
{
  lv_obj_t *obj = lv_event_get_target_obj(e);
  lv_draw_task_t *draw_task = lv_event_get_draw_task(e);
  lv_draw_dsc_base_t *base_dsc = (lv_draw_dsc_base_t *)lv_draw_task_get_draw_dsc(draw_task);
  if (base_dsc->part == LV_PART_MAIN)
  {
    lv_draw_rect_dsc_t draw_dsc;
    lv_draw_rect_dsc_init(&draw_dsc);
    draw_dsc.bg_color = lv_color_hex(0xffaaaa);
    draw_dsc.radius = LV_RADIUS_CIRCLE;
    draw_dsc.border_color = lv_color_hex(0xff5555);
    draw_dsc.border_width = 2;
    draw_dsc.outline_color = lv_color_hex(0xff0000);
    draw_dsc.outline_pad = 3;
    draw_dsc.outline_width = 2;

    lv_area_t a;
    a.x1 = 0;
    a.y1 = 0;
    a.x2 = size;
    a.y2 = size;
    lv_area_t obj_coords;
    lv_obj_get_coords(obj, &obj_coords);
    lv_area_align(&obj_coords, &a, LV_ALIGN_CENTER, 0, 0);

    lv_draw_rect(base_dsc->layer, &draw_dsc, &a);
  }
}
void create_start_button()
{
    lv_obj_t *btn = lv_btn_create(lv_screen_active());
    lv_obj_align(btn, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_size(btn, 150, 50);
    lv_obj_add_event_cb(btn, [](lv_event_t *e) {
    start_snake_game();  
    }, LV_EVENT_CLICKED, NULL);

    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, "Demarrer Snake");
    lv_obj_center(label);
}
void lv_example_event_draw(void)
{
  lv_obj_t *cont = lv_obj_create(lv_screen_active());
  lv_obj_set_size(cont, 200, 200);
  lv_obj_center(cont);
  lv_obj_add_event_cb(cont, event_cb, LV_EVENT_DRAW_TASK_ADDED, NULL);
  lv_obj_add_flag(cont, LV_OBJ_FLAG_SEND_DRAW_TASK_EVENTS);
  lv_timer_create(timer_cb, 30, cont);
}

void lv_score(void)
{
    /*Change the active screen's background color*/
    lv_obj_set_style_bg_color(lv_screen_active(), lv_color_hex(0x003a57), LV_PART_MAIN);

    /*Create a white label, set its text*/
    lv_obj_t *label = lv_label_create(lv_screen_active());
    lv_label_set_text(label, "Vers le haut ou bouton pour lancer Snake");
    lv_obj_set_style_text_color(lv_screen_active(), lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_align(label, LV_ALIGN_TOP_LEFT, 0, 0);

    create_start_button();
}


void affichepos(const char *txt)
{
  if (direction_label == NULL)
  {
    direction_label = lv_label_create(lv_screen_active());
    lv_obj_set_style_text_color(direction_label, lv_color_hex(0xff00ff), LV_PART_MAIN);
    lv_obj_align(direction_label, LV_ALIGN_TOP_MID, 0, 20);
  }

  lv_label_set_text(direction_label, txt);


}

void spawn_apple()
{
  if (apple)
  {
    lv_obj_del(apple);
  }

  int ax, ay;
  do
  {
    ax = (rand()%48) * 10;
    ay = (rand()%27) * 10;

  } while (is_on_snake(ax, ay)); // éviter le corps

  apple = lv_obj_create(lv_screen_active());
  lv_obj_set_size(apple, 10, 10);
  lv_obj_set_style_bg_color(apple, lv_color_hex(0xFF0000), LV_PART_MAIN);
  lv_obj_clear_flag(apple, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_pos(apple, ax, ay);
}


void game_over()
{
  Serial.println("Game Over !");
  affichepos("Game Over!");
  game_started = false;

  for (auto &part : snake)
  {
    if (part.obj)
    {
      lv_obj_del(part.obj);
    }
  }
  snake.clear();

  if (apple)
  {
    lv_obj_del(apple);
    apple = NULL;
  }

  if (snake_timer)
  {
    lv_timer_del(snake_timer);
    snake_timer = NULL;
  }

  // Réinitialiser la position
  snake_x = 100;
  snake_y = 100;
  dx = dy = 0;
  score = 0;

  // Ajouter un délai avant retour à l'écran principal
  lv_timer_create([](lv_timer_t *t) {
    // Nettoyer l'écran
    lv_obj_clean(lv_screen_active());

    // Afficher l'écran d'accueil
    lv_score();

  
  }, 2000, NULL); // 2000 ms = 2 secondes
}




void update_score() {
  if (!score_label) return;

  char buf[32];
  snprintf(buf, sizeof(buf), "Score: %d", score);
  lv_label_set_text(score_label, buf);
}

void start_snake_game()
{
  game_started = true;
  lv_obj_clean(lv_screen_active()); // Clear screen

  dx = 10; // Direction initiale vers la droite
  dy = 0;

  SnakePart head = {snake_x, snake_y, lv_obj_create(lv_screen_active())};
  lv_obj_set_size(head.obj, 10, 10);
  lv_obj_set_style_bg_color(head.obj, lv_color_hex(0x00FF00), LV_PART_MAIN);
  lv_obj_set_pos(head.obj, snake_x, snake_y);
  lv_obj_clear_flag(head.obj, LV_OBJ_FLAG_SCROLLABLE);

  snake.push_back(head);

  spawn_apple();

  // Supprimer ancien timer si existant
  if (snake_timer)
  {
    lv_timer_del(snake_timer);
    snake_timer = NULL;
  }

  // Timer de déplacement
  snake_timer = lv_timer_create([](lv_timer_t *timer)
{
  if (!game_started || snake.empty())
    return;

  int new_x = snake[0].x + dx;
  int new_y = snake[0].y + dy;

  // Collision murs
  if (new_x < 0 || new_x >= 480 || new_y < 0 || new_y >= 270)
  {
    game_over();
    return;
  }

  // Sécurité : position doit rester alignée sur une grille 10x10
  if (new_x % 10 != 0 || new_y % 10 != 0)
  {
    game_over();
    return;
  }

  // Collision avec soi-même
  for (size_t i = 1; i < snake.size(); i++)
  {
    if (snake[i].x == new_x && snake[i].y == new_y)
    {
      game_over();
      return;
    }
  }

  // Nouvelle tête
  lv_obj_t *new_part = lv_obj_create(lv_screen_active());
  lv_obj_set_size(new_part, 10, 10);
  lv_obj_set_style_bg_color(new_part, lv_color_hex(0x00FF00), LV_PART_MAIN);
  lv_obj_clear_flag(new_part, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_pos(new_part, new_x, new_y);

  snake.insert(snake.begin(), {new_x, new_y, new_part});

  // Si on mange la pomme
  if (apple && new_x == lv_obj_get_x(apple) && new_y == lv_obj_get_y(apple))
  {
    score++;
    update_score();
    spawn_apple();

  }
  else
  {
    if (snake.back().obj)
    {
      lv_obj_del(snake.back().obj);
    }
    snake.pop_back();
  }
},
200, NULL);

  // Label du score
  score_label = lv_label_create(lv_screen_active());
  lv_obj_set_style_text_color(score_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
  lv_obj_align(score_label, LV_ALIGN_TOP_RIGHT, -10, 10);
  update_score(); // Affiche "Score: 0"


}



void mySetup()
{
  // Initialisation LVGL
  lv_score();
  // lv_example_event_draw();
  // testLvgl();

  // Configuration des pins en entrée
  pinMode(PIN_PF10, INPUT); // Haut
  pinMode(PIN_PF6, INPUT);  // Droite
  pinMode(PIN_PI2, INPUT);  // Gauche
  pinMode(PIN_PG7, INPUT);  // Bas

  Serial.begin(115200);
  srand(time(NULL));
}

void loop()
{
}

void myTask(void *pvParameters)
{
  //TickType_t xLastWakeTime = xTaskGetTickCount();
  while (1)
  {
    try
    {
      int valPF10 = digitalRead(PIN_PF10); // Haut
      int valPF6 = digitalRead(PIN_PF6);   // Droite
      int valPI2 = digitalRead(PIN_PI2);   // Gauche
      int valPG7 = digitalRead(PIN_PG7);   // Bas

      if (!game_started)
      {
        if (valPF10)
        {
          Serial.println("affichepos(Haut);");
          affichepos("Haut");
          Serial.println("start_snake_game();");
          start_snake_game();
          Serial.println("passé");
        }
      }
      else
      {
        // Contrôle du Snake avec sécurité : pas de demi-tour direct
        if (valPF10 && dy == 0)
        {
          dx = 0;
          dy = -10;
        }
        else if (valPG7 && dy == 0)
        {
          dx = 0;
          dy = 10;
        }
        else if (valPI2 && dx == 0)
        {
          dx = -10;
          dy = 0;
        }
        else if (valPF6 && dx == 0)
        {
          dx = 10;
          dy = 0;
        }
      }
Serial.println("délai");
      Serial.println("passé");
    }
    catch (const std::exception &e)
    {
      Serial.println(e.what());
    }
    catch (...) {
      Serial.println("Erreur !");
    }
  }
}

#else

#include "lvgl.h"
#include "app_hal.h"
#include <cstdio>

int main(void)
{
  printf("LVGL Simulator\n");
  fflush(stdout);

  lv_init();
  hal_setup();

  testLvgl();

  hal_loop();
  return 0;
}

#endif
