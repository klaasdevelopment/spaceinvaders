#include "raylib.h"
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define WIDTH 800
#define HEIGHT 720
#define ROWS 5
#define COLS 11
#define CELL 4
#define SHIELD_W 20
#define SHIELD_H 12
#define SHIELDS 4
#define PLAYER_Y 640

typedef struct { float x, y; bool active; } Bullet;
typedef struct {
  bool aliens[ROWS][COLS];
  bool shield[SHIELDS][SHIELD_H][SHIELD_W];
  Bullet shot, bombs[3];
  float player, fleetX, fleetY, march, fire, ufoX, ufoTimer, hurt, transition;
  int direction, alive, score, high, lives, wave, frame, beat, shots;
  bool started, over, paused, ufo, bonus;
} Game;

typedef struct { Sound shoot, hit, death, saucer, march[4]; bool ready; } Audio;
static const Color green = {102, 242, 115, 255};
static const unsigned short sprites[3][2][8] = {
  {{0x060,0x0f0,0x1f8,0x36c,0x3fc,0x090,0x108,0x204},
   {0x060,0x0f0,0x1f8,0x36c,0x3fc,0x108,0x204,0x108}},
  {{0x108,0x090,0x1f8,0x36c,0x7fe,0x5fa,0x408,0x090},
   {0x108,0x490,0x5fa,0x76e,0x3fc,0x1f8,0x108,0x204}},
  {{0x0f0,0x3fc,0x7fe,0x666,0x7fe,0x198,0x30c,0x606},
   {0x0f0,0x3fc,0x7fe,0x666,0x7fe,0x198,0x264,0x108}}
};

static Sound Tone(float start, float end, float seconds, bool noise) {
  const int rate = 22050;
  int count = (int)(rate*seconds);
  short *samples = malloc((size_t)count*sizeof(*samples));
  if (!samples) return (Sound){0};
  float phase = 0;
  uint32_t random = 0x12345678;
  for (int i = 0; i < count; i++) {
    float t = (float)i/count;
    phase += (start + (end-start)*t)/rate;
    random ^= random << 13; random ^= random >> 17; random ^= random << 5;
    float value = noise ? ((float)(random & 65535)/32768 - 1) :
                         (fmodf(phase, 1) < 0.5f ? 1.0f : -1.0f);
    samples[i] = (short)(value*6500*(1-t)*fminf(1, i/80.0f));
  }
  Wave wave = { (unsigned int)count, rate, 16, 1, samples };
  Sound sound = LoadSoundFromWave(wave);
  free(samples);
  return sound;
}

static Audio LoadAudio(void) {
  Audio a = {0};
  InitAudioDevice();
  a.ready = IsAudioDeviceReady();
  if (!a.ready) return a;
  a.shoot = Tone(1700, 140, .16f, false);
  a.hit = Tone(220, 50, .12f, true);
  a.death = Tone(100, 20, .65f, true);
  a.saucer = Tone(450, 850, .18f, false);
  for (int i = 0; i < 4; i++) a.march[i] = Tone(110-i*15, 70-i*10, .09f, false);
  return a;
}

static void Play(Audio *a, Sound sound) {
  if (a->ready && IsSoundValid(sound)) PlaySound(sound);
}

static void UnloadAudio(Audio *a) {
  if (!a->ready) return;
  UnloadSound(a->shoot); UnloadSound(a->hit); UnloadSound(a->death);
  UnloadSound(a->saucer);
  for (int i = 0; i < 4; i++) UnloadSound(a->march[i]);
  CloseAudioDevice();
}

static Rectangle AlienRect(const Game *g, int row, int col) {
  return (Rectangle){g->fleetX + col*52, g->fleetY + row*42, 33, 24};
}

static void NewWave(Game *g) {
  memset(g->aliens, 1, sizeof(g->aliens));
  g->alive = ROWS*COLS;
  g->fleetX = 100; g->fleetY = 145 + ((g->wave-1)%5)*18;
  g->direction = 1; g->march = 0; g->fire = .9f;
  g->shot.active = false;
  memset(g->bombs, 0, sizeof(g->bombs));
  g->ufo = false; g->ufoTimer = 15;
  for (int s = 0; s < SHIELDS; s++)
    for (int y = 0; y < SHIELD_H; y++)
      for (int x = 0; x < SHIELD_W; x++)
        g->shield[s][y][x] = !(y < 3 && (x < 3-y || x >= SHIELD_W-3+y)) &&
                             !(y >= 7 && x >= 6 && x < 14);
}

static void NewGame(Game *g) {
  int high = g->high;
  *g = (Game){.high=high, .lives=3, .wave=1, .player=WIDTH/2,
              .started=true};
  NewWave(g);
}

static Rectangle ShieldRect(int s, int x, int y) {
  return (Rectangle){100+s*175+x*CELL, 550+y*CELL, CELL, CELL};
}

static bool HitShield(Game *g, Rectangle bullet) {
  for (int s = 0; s < SHIELDS; s++)
    for (int y = 0; y < SHIELD_H; y++)
      for (int x = 0; x < SHIELD_W; x++) {
        if (!g->shield[s][y][x] || !CheckCollisionRecs(bullet, ShieldRect(s,x,y))) continue;
        for (int dy = -2; dy <= 2; dy++)
          for (int dx = -2; dx <= 2; dx++)
            if (x+dx >= 0 && x+dx < SHIELD_W && y+dy >= 0 && y+dy < SHIELD_H &&
                dx*dx+dy*dy <= 5) g->shield[s][y+dy][x+dx] = false;
        return true;
      }
  return false;
}

static void AddScore(Game *g, int points) {
  g->score += points;
  if (!g->bonus && g->score >= 1500) { g->lives++; g->bonus = true; }
  if (g->score > g->high) g->high = g->score;
}

static void UpdateGame(Game *g, Audio *a, float dt) {
  if ((!g->started || g->over) && IsKeyPressed(KEY_ENTER)) { NewGame(g); return; }
  if (!g->started || g->over) return;
  if (IsKeyPressed(KEY_P)) g->paused = !g->paused;
  if (g->paused) return;
  if (g->hurt > 0) { g->hurt -= dt; return; }
  if (g->transition > 0) {
    g->transition -= dt;
    if (g->transition <= 0) { g->wave++; NewWave(g); }
    return;
  }
  g->player += (IsKeyDown(KEY_RIGHT)-IsKeyDown(KEY_LEFT))*280*dt;
  g->player = fmaxf(36, fminf(WIDTH-36, g->player));
  if (IsKeyDown(KEY_SPACE) && !g->shot.active) {
    g->shots++;
    g->shot = (Bullet){g->player, PLAYER_Y-12, true}; Play(a, a->shoot);
  }
  if (g->shot.active) {
    // Small steps prevent fast shots from tunneling through shield pixels.
    int steps = (int)ceilf(620*dt/3);
    for (int step = 0; step < steps && g->shot.active; step++) {
      g->shot.y -= 620*dt/steps;
      Rectangle shot = {g->shot.x-2, g->shot.y, 4, 12};
      if (g->shot.y < 70 || HitShield(g, shot)) { g->shot.active = false; break; }
      if (g->ufo && CheckCollisionRecs(shot, (Rectangle){g->ufoX, 95, 42, 18})) {
        static const int awards[15] = {50,50,100,150,100,100,50,300,100,100,100,50,150,100,100};
        g->ufo = false; g->shot.active = false;
        AddScore(g,awards[(g->shots-1)%15]); Play(a,a->hit); break;
      }
      for (int r = ROWS-1; r >= 0 && g->shot.active; r--)
        for (int c = 0; c < COLS && g->shot.active; c++)
          if (g->aliens[r][c] && CheckCollisionRecs(shot,AlienRect(g,r,c))) {
            g->aliens[r][c] = false; g->alive--; g->shot.active = false;
            AddScore(g,r == 0 ? 30 : r < 3 ? 20 : 10); Play(a,a->hit);
          }
    }
  }
  if (g->alive == 0) { g->transition = 1.2f; return; }
  g->march -= dt;
  if (g->march <= 0) {
    g->march = fmaxf(.055f, .09f + .65f*g->alive/(ROWS*COLS) - (g->wave-1)*.025f);
    bool edge = false;
    for (int r = 0; r < ROWS; r++) for (int c = 0; c < COLS; c++) {
      Rectangle alien = AlienRect(g,r,c);
      if (g->aliens[r][c] && (alien.x + g->direction*12 < 28 ||
          alien.x+alien.width+g->direction*12 > WIDTH-28)) edge = true;
    }
    if (edge) { g->direction *= -1; g->fleetY += 22; }
    else g->fleetX += g->direction*12;
    g->frame ^= 1; Play(a,a->march[g->beat++%4]);
    for (int r = 0; r < ROWS; r++) for (int c = 0; c < COLS; c++) {
      if (!g->aliens[r][c]) continue;
      Rectangle alien = AlienRect(g,r,c);
      if (alien.y+alien.height >= PLAYER_Y) { g->over = true; Play(a,a->death); }
      for (int s = 0; s < SHIELDS; s++) for (int y = 0; y < SHIELD_H; y++)
        for (int x = 0; x < SHIELD_W; x++)
          if (g->shield[s][y][x] && CheckCollisionRecs(alien,ShieldRect(s,x,y)))
            g->shield[s][y][x] = false;
    }
  }
  g->fire -= dt;
  if (g->fire <= 0) {
    g->fire = fmaxf(.23f, .85f - g->wave*.04f - (ROWS*COLS-g->alive)*.009f);
    int start = GetRandomValue(0,COLS-1);
    bool fired = false;
    for (int offset = 0; offset < COLS && !fired; offset++) {
      int c = (start+offset)%COLS;
      for (int r = ROWS-1; r >= 0; r--) if (g->aliens[r][c]) {
        for (int b = 0; b < 3; b++) if (!g->bombs[b].active) {
          Rectangle alien = AlienRect(g,r,c);
          g->bombs[b] = (Bullet){alien.x+16,alien.y+24,true}; fired = true; break;
        }
        break;
      }
    }
  }
  for (int i = 0; i < 3; i++) {
    Bullet *b = &g->bombs[i];
    if (!b->active) continue;
    b->y += fminf(340, 210+g->wave*12)*dt;
    Rectangle bomb = {b->x-2,b->y,5,12};
    if (b->y > PLAYER_Y+30 || HitShield(g,bomb)) b->active = false;
    else if (CheckCollisionRecs(bomb,(Rectangle){g->player-18,PLAYER_Y,36,20})) {
      g->lives--; g->hurt = 1.2f; g->over = g->lives <= 0;
      memset(g->bombs,0,sizeof(g->bombs)); g->shot.active = false;
      g->player = WIDTH/2; Play(a,a->death); break;
    }
  }
  g->ufoTimer -= dt;
  if (!g->ufo && g->ufoTimer <= 0) {
    g->ufo = true; g->ufoX = -45; g->ufoTimer = (float)GetRandomValue(18,25);
  }
  if (g->ufo) {
    g->ufoX += 110*dt;
    if (a->ready && !IsSoundPlaying(a->saucer)) Play(a,a->saucer);
    if (g->ufoX > WIDTH) g->ufo = false;
  }
}

static void CenterText(const char *text, int y, int size, Color color) {
  DrawText(text,(WIDTH-MeasureText(text,size))/2,y,size,color);
}

static void DrawShip(int x, int y, Color color) {
  DrawRectangle(x-18,y+8,36,12,color);
  DrawRectangle(x-12,y+3,24,8,color);
  DrawRectangle(x-2,y-3,4,9,color);
}

static void DrawGame(const Game *g) {
  ClearBackground((Color){8,10,18,255});
  DrawText(TextFormat("SCORE %05d",g->score),28,25,24,RAYWHITE);
  DrawText(TextFormat("HI %05d",g->high),330,25,24,green);
  DrawText(TextFormat("WAVE %02d",g->wave),640,25,24,RAYWHITE);
  DrawLine(28,65,WIDTH-28,65,(Color){45,65,65,255});
  if (g->started) {
    for (int r = 0; r < ROWS; r++) for (int c = 0; c < COLS; c++) {
      if (!g->aliens[r][c]) continue;
      Rectangle rect = AlienRect(g,r,c);
      for (int y = 0; y < 8; y++) for (int x = 0; x < 11; x++)
        if (sprites[(r+1)/2][g->frame][y] & (1 << (10-x)))
          DrawRectangle((int)rect.x+x*3,(int)rect.y+y*3,3,3,RAYWHITE);
    }
    for (int s = 0; s < SHIELDS; s++) for (int y = 0; y < SHIELD_H; y++)
      for (int x = 0; x < SHIELD_W; x++)
        if (g->shield[s][y][x]) DrawRectangleRec(ShieldRect(s,x,y),green);
    if (g->hurt <= 0 || ((int)(g->hurt*12)%2)) DrawShip((int)g->player,PLAYER_Y,green);
    if (g->shot.active) DrawRectangle((int)g->shot.x-2,(int)g->shot.y,4,12,RAYWHITE);
    for (int i = 0; i < 3; i++) if (g->bombs[i].active) {
      int x = (int)g->bombs[i].x, y = (int)g->bombs[i].y;
      DrawLineEx((Vector2){x-2,y},(Vector2){x+2,y+6},3,ORANGE);
      DrawLineEx((Vector2){x+2,y+6},(Vector2){x-2,y+12},3,ORANGE);
    }
    if (g->ufo) {
      int x = (int)g->ufoX;
      DrawRectangle(x+12,95,18,4,RED); DrawRectangle(x+5,99,32,5,RED);
      DrawRectangle(x,104,42,5,RED);
      for (int i = 0; i < 3; i++) DrawRectangle(x+7+i*12,109,4,4,RED);
    }
  }
  DrawLine(28,674,WIDTH-28,674,green);
  DrawText(TextFormat("LIVES %d",g->lives),28,690,18,green);
  DrawText("ARROWS  MOVE     SPACE  FIRE     P  PAUSE",220,690,16,GRAY);
  if (!g->started || g->over || g->paused || g->transition > 0) {
    DrawRectangle(65,245,670,230,(Color){8,10,18,240});
    CenterText(!g->started ? "SPACE INVADERS" : g->over ? "GAME OVER" :
               g->paused ? "PAUSED" : "WAVE CLEARED",270,42,green);
    if (!g->started) {
      CenterText("INVADERS  30 / 20 / 10 POINTS",335,18,RAYWHITE);
      CenterText("MYSTERY SHIP  50 - 300    BONUS LIFE  1500",368,20,RED);
    }
    CenterText(g->paused ? "PRESS P TO RESUME" : g->transition > 0 ? "GET READY" :
               "PRESS ENTER TO PLAY",425,22,RAYWHITE);
  }
}

int main(void) {
  InitWindow(WIDTH,HEIGHT,"Space Invaders");
  SetTargetFPS(60);
  Audio audio = LoadAudio();
  Game game = {.lives=3,.wave=1};
  while (!WindowShouldClose()) {
    UpdateGame(&game,&audio,fminf(GetFrameTime(),1.0f/30));
    BeginDrawing(); DrawGame(&game); EndDrawing();
  }
  UnloadAudio(&audio);
  CloseWindow();
  return 0;
}
