// ════════════════════════════════════════════════════════════════
//  DUNGEON ROGUELITE — World (declarations)
// ════════════════════════════════════════════════════════════════
#pragma once

#include "types.h"

// ── Room map ──
extern char roomMap[ROOM_H][ROOM_W + 1];

// ── Flow field (BFS pathfinding) ──
extern Vec2 flowField[ROOM_H][ROOM_W];
extern int flowDist[ROOM_H][ROOM_W];

// ── Map queries ──
bool isWall(int x, int z);
bool canMove(float x, float z, float r);

// ── Pathfinding ──
void computeFlowField(float playerX, float playerZ);
Vec2 getFlowDirection(float wx, float wz);

// ── Room generation ──
void generateRoom(int depth, bool isBoss, Enemy enemies[], int &numEnemies,
                  Vec2 &playerStart);
int setupDoors(Door doors[], int depth);
