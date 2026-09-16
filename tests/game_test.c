#define main game_main
#include "../test.c"
#undef main
#include <assert.h>
#include <stdio.h>

int main(void) {
  Game g = {.high=500};
  NewGame(&g);
  assert(g.alive == 55 && g.lives == 3 && g.high == 500);
  int count = 0;
  for (int r = 0; r < ROWS; r++) for (int c = 0; c < COLS; c++) count += g.aliens[r][c];
  assert(count == 55);
  AddScore(&g,1490);
  assert(g.lives == 3);
  AddScore(&g,10);
  assert(g.lives == 4 && g.bonus && g.high == 1500);
  AddScore(&g,1500);
  assert(g.lives == 4);
  assert(g.shield[0][4][4]);
  assert(HitShield(&g,ShieldRect(0,4,4)));
  assert(!g.shield[0][4][4]);
  assert(!g.shield[0][10][10]);
  assert(!HitShield(&g,(Rectangle){0,0,4,12}));
  g.wave++;
  g.shot.active = true; g.bombs[0].active = true;
  NewWave(&g);
  assert(g.score == 3000 && g.lives == 4);
  assert(!g.shot.active && !g.bombs[0].active && g.shield[0][4][4]);
  NewGame(&g);
  assert(g.score == 0 && g.high == 3000 && g.lives == 3 && !g.bonus);
  puts("Game state, shields, scoring, bonus life, wave and restart checks passed.");
  return 0;
}
