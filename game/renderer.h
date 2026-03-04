// ════════════════════════════════════════════════════════════════
//  DUNGEON ROGUELITE — Renderer (declarations)
// ════════════════════════════════════════════════════════════════
#pragma once

#include "types.h"
#include <GL/freeglut.h>

// ── Screen effects (shared with main.cpp) ──
extern float screenShake;
extern float flashR, flashG, flashB, flashA;
extern float fadeAlpha;

// ── Textures ──
void initTextures();

// ── Particles ──
void spawnParticle(Vec2 pos, float y, float vx, float vy, float vz, float r,
                   float g, float b, float life, float size);
void spawnBurst(Vec2 pos, float y, int count, float r, float g, float b);
void updateParticles(float dt);
void drawParticles(float camYaw);
void clearParticles();

// ── 3D World ──
void drawWalls();
void drawFloorCeiling();
void drawDoors(Door doors[], int numDoors, float time);
void drawEnemies(Enemy enemies[], int numEnemies, float camYaw, float time);
void drawProjectiles(Projectile projs[], int count, float camYaw);

// ── HUD ──
void drawText2D(float x, float y, const char *str,
                void *font = GLUT_BITMAP_HELVETICA_18);
void drawHUD(int winW, int winH, PlayerState &player, RunStats &stats,
             GameState state, BoonType boonChoices[], int numBoonChoices,
             Door doors[], int numDoors, float time);
