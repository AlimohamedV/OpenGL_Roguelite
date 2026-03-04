// ════════════════════════════════════════════════════════════════
//  DUNGEON ROGUELITE — World (implementation)
// ════════════════════════════════════════════════════════════════

#include "world.h"
#include <cmath>
#include <cstring>

// ── Global definitions ──
char roomMap[ROOM_H][ROOM_W + 1];
Vec2 flowField[ROOM_H][ROOM_W];
int flowDist[ROOM_H][ROOM_W];

// ── Internal state ──
static bool flowVisited[ROOM_H][ROOM_W];
struct BfsNode {
  int x, z;
};
static BfsNode bfsQueue[ROOM_W * ROOM_H];

// ════════════════════════════════════════
//  MAP QUERIES
// ════════════════════════════════════════

bool isWall(int x, int z) {
  if (x < 0 || x >= ROOM_W || z < 0 || z >= ROOM_H)
    return true;
  return roomMap[z][x] == '#';
}

bool canMove(float x, float z, float r) {
  int x0 = (int)floorf(x - r), x1 = (int)floorf(x + r);
  int z0 = (int)floorf(z - r), z1 = (int)floorf(z + r);
  for (int gz = z0; gz <= z1; gz++)
    for (int gx = x0; gx <= x1; gx++)
      if (isWall(gx, gz))
        return false;
  return true;
}

// ════════════════════════════════════════
//  BFS FLOW FIELD
// ════════════════════════════════════════

void computeFlowField(float playerX, float playerZ) {
  int px = (int)playerX, pz = (int)playerZ;
  if (px < 0)
    px = 0;
  if (px >= ROOM_W)
    px = ROOM_W - 1;
  if (pz < 0)
    pz = 0;
  if (pz >= ROOM_H)
    pz = ROOM_H - 1;

  for (int z = 0; z < ROOM_H; z++)
    for (int x = 0; x < ROOM_W; x++) {
      flowVisited[z][x] = false;
      flowDist[z][x] = 9999;
      flowField[z][x] = Vec2(0, 0);
    }

  int head = 0, tail = 0;
  flowVisited[pz][px] = true;
  flowDist[pz][px] = 0;
  bfsQueue[tail++] = {px, pz};

  const int dx8[] = {0, 1, 1, 1, 0, -1, -1, -1};
  const int dz8[] = {-1, -1, 0, 1, 1, 1, 0, -1};

  while (head < tail) {
    BfsNode cur = bfsQueue[head++];
    int curDist = flowDist[cur.z][cur.x];
    for (int d = 0; d < 8; d++) {
      int nx = cur.x + dx8[d], nz = cur.z + dz8[d];
      if (nx < 0 || nx >= ROOM_W || nz < 0 || nz >= ROOM_H)
        continue;
      if (flowVisited[nz][nx])
        continue;
      if (isWall(nx, nz))
        continue;
      if (dx8[d] != 0 && dz8[d] != 0) {
        if (isWall(cur.x + dx8[d], cur.z) || isWall(cur.x, cur.z + dz8[d]))
          continue;
      }
      flowVisited[nz][nx] = true;
      flowDist[nz][nx] = curDist + 1;
      float fdx = (float)(cur.x - nx), fdz = (float)(cur.z - nz);
      float len = sqrtf(fdx * fdx + fdz * fdz);
      if (len > 0.01f)
        flowField[nz][nx] = Vec2(fdx / len, fdz / len);
      bfsQueue[tail++] = {nx, nz};
    }
  }
}

Vec2 getFlowDirection(float wx, float wz) {
  int gx = (int)wx, gz = (int)wz;
  if (gx < 0 || gx >= ROOM_W || gz < 0 || gz >= ROOM_H)
    return Vec2(0, 0);
  return flowField[gz][gx];
}

// ════════════════════════════════════════
//  CELLULAR AUTOMATA ROOM GENERATION
// ════════════════════════════════════════

static char tempMap[ROOM_H][ROOM_W];

static int countWallNeighbors(int x, int z) {
  int count = 0;
  for (int dz = -1; dz <= 1; dz++)
    for (int dx = -1; dx <= 1; dx++) {
      if (dx == 0 && dz == 0)
        continue;
      int nx = x + dx, nz = z + dz;
      if (nx < 0 || nx >= ROOM_W || nz < 0 || nz >= ROOM_H)
        count++;
      else if (roomMap[nz][nx] == '#')
        count++;
    }
  return count;
}

static void smoothPass() {
  for (int z = 0; z < ROOM_H; z++)
    for (int x = 0; x < ROOM_W; x++)
      tempMap[z][x] = (countWallNeighbors(x, z) >= 5) ? '#' : '.';
  for (int z = 1; z < ROOM_H - 1; z++)
    for (int x = 1; x < ROOM_W - 1; x++)
      roomMap[z][x] = tempMap[z][x];
}

static bool floodVisitedCA[ROOM_H][ROOM_W];

static int floodFillFrom(int sx, int sz) {
  if (sx < 0 || sx >= ROOM_W || sz < 0 || sz >= ROOM_H)
    return 0;
  if (roomMap[sz][sx] == '#')
    return 0;
  memset(floodVisitedCA, 0, sizeof(floodVisitedCA));
  BfsNode queue[ROOM_W * ROOM_H];
  int head = 0, tail = 0;
  queue[tail++] = {sx, sz};
  floodVisitedCA[sz][sx] = true;
  int count = 0;
  while (head < tail) {
    BfsNode cur = queue[head++];
    count++;
    const int d4x[] = {0, 1, 0, -1}, d4z[] = {-1, 0, 1, 0};
    for (int d = 0; d < 4; d++) {
      int nx = cur.x + d4x[d], nz = cur.z + d4z[d];
      if (nx < 0 || nx >= ROOM_W || nz < 0 || nz >= ROOM_H)
        continue;
      if (floodVisitedCA[nz][nx] || roomMap[nz][nx] == '#')
        continue;
      floodVisitedCA[nz][nx] = true;
      queue[tail++] = {nx, nz};
    }
  }
  return count;
}

static void removeDisconnected(int sx, int sz) {
  floodFillFrom(sx, sz);
  for (int z = 1; z < ROOM_H - 1; z++)
    for (int x = 1; x < ROOM_W - 1; x++)
      if (roomMap[z][x] == '.' && !floodVisitedCA[z][x])
        roomMap[z][x] = '#';
}

static void clearCircle(int cx, int cz, int radius) {
  for (int dz = -radius; dz <= radius; dz++)
    for (int dx = -radius; dx <= radius; dx++) {
      if (dx * dx + dz * dz > radius * radius)
        continue;
      int x = cx + dx, z = cz + dz;
      if (x > 0 && x < ROOM_W - 1 && z > 0 && z < ROOM_H - 1)
        roomMap[z][x] = '.';
    }
}

static void carveTunnel(int x1, int z1, int x2, int z2) {
  int x = x1, z = z1;
  while (x != x2 || z != z2) {
    if (x > 0 && x < ROOM_W - 1 && z > 0 && z < ROOM_H - 1) {
      roomMap[z][x] = '.';
      if (x + 1 < ROOM_W - 1)
        roomMap[z][x + 1] = '.';
      if (z + 1 < ROOM_H - 1)
        roomMap[z + 1][x] = '.';
    }
    if (x != x2)
      x += (x2 > x) ? 1 : -1;
    else if (z != z2)
      z += (z2 > z) ? 1 : -1;
  }
}

// ════════════════════════════════════════
//  ENEMY SPAWNING
// ════════════════════════════════════════

static int spawnEnemies(Enemy enemies[], int depth, bool isBoss) {
  int count = 0;

  if (isBoss) {
    enemies[count].pos = Vec2(ROOM_W / 2.0f, ROOM_H / 2.0f);
    enemies[count].type = ENEMY_BOSS;
    enemies[count].maxHealth = 15.0f + depth * 3.0f;
    enemies[count].health = enemies[count].maxHealth;
    enemies[count].alive = true;
    enemies[count].hitAnim = 0;
    enemies[count].deathAnim = 0;
    enemies[count].shootTimer = 2.0f;
    enemies[count].chargeTimer = 5.0f;
    enemies[count].dashTimer = 0;
    enemies[count].charging = false;
    enemies[count].radius = 0.45f;
    enemies[count].speed = 1.5f;
    enemies[count].damage = 20.0f;
    enemies[count].facingAngle = 0;
    count++;

    int extras = 1 + depth / 5;
    if (extras > 4)
      extras = 4;
    for (int i = 0; i < extras && count < MAX_ENEMIES; i++) {
      // Find walkable position
      for (int att = 0; att < 30; att++) {
        float ex = randf(2, ROOM_W - 2), ez = randf(2, ROOM_H / 2.0f);
        if (canMove(ex, ez, 0.3f)) {
          enemies[count].pos = Vec2(ex, ez);
          break;
        }
      }
      enemies[count].type = ENEMY_CHASER;
      enemies[count].maxHealth = 2.0f;
      enemies[count].health = 2.0f;
      enemies[count].alive = true;
      enemies[count].hitAnim = 0;
      enemies[count].deathAnim = 0;
      enemies[count].shootTimer = 0;
      enemies[count].chargeTimer = 0;
      enemies[count].dashTimer = 0;
      enemies[count].charging = false;
      enemies[count].radius = 0.25f;
      enemies[count].speed = 2.5f;
      enemies[count].damage = 8.0f;
      enemies[count].facingAngle = 0;
      count++;
    }
    return count;
  }

  // Normal room
  int numChasers = 4 + depth;
  int numShooters = depth >= 1 ? 1 + depth / 2 : 0;
  int numTanks = depth >= 3 ? 1 + (depth - 3) / 3 : 0;
  int total = numChasers + numShooters + numTanks;
  if (total > MAX_ENEMIES - 2)
    total = MAX_ENEMIES - 2;

  for (int i = 0; i < total && count < MAX_ENEMIES; i++) {
    Enemy &e = enemies[count];
    e.pos = Vec2(ROOM_W / 2.0f, ROOM_H / 2.0f);
    for (int att = 0; att < 40; att++) {
      float ex = randf(2, ROOM_W - 2);
      float ez = randf(2, ROOM_H * 0.7f);
      if (canMove(ex, ez, 0.3f)) {
        e.pos = Vec2(ex, ez);
        break;
      }
    }

    if (i < numChasers) {
      e.type = ENEMY_CHASER;
      e.maxHealth = 2.0f + depth * 0.3f;
      e.radius = 0.25f;
      e.speed = 2.2f + depth * 0.08f;
      e.damage = 10.0f + depth * 0.5f;
    } else if (i < numChasers + numShooters) {
      e.type = ENEMY_SHOOTER;
      e.maxHealth = 2.0f + depth * 0.2f;
      e.radius = 0.25f;
      e.speed = 1.2f;
      e.damage = 12.0f;
    } else {
      e.type = ENEMY_TANK;
      e.maxHealth = 5.0f + depth * 0.5f;
      e.radius = 0.35f;
      e.speed = 0.8f;
      e.damage = 15.0f + depth;
    }
    e.health = e.maxHealth;
    e.alive = true;
    e.hitAnim = 0;
    e.deathAnim = 0;
    e.shootTimer = randf(1.0f, 3.0f);
    e.chargeTimer = 0;
    e.dashTimer = 0;
    e.charging = false;
    e.facingAngle = 0;
    count++;
  }
  return count;
}

// ════════════════════════════════════════
//  ROOM GENERATION (main entry)
// ════════════════════════════════════════

void generateRoom(int depth, bool isBoss, Enemy enemies[], int &numEnemies,
                  Vec2 &playerStart) {
  // Fill with walls + null terminators
  for (int z = 0; z < ROOM_H; z++) {
    for (int x = 0; x < ROOM_W; x++)
      roomMap[z][x] = '#';
    roomMap[z][ROOM_W] = '\0';
  }

  // Random noise (wall density varies by depth)
  float wallChance = isBoss ? 0.35f : (0.38f + depth * 0.008f);
  if (wallChance > 0.46f)
    wallChance = 0.46f;

  for (int z = 1; z < ROOM_H - 1; z++)
    for (int x = 1; x < ROOM_W - 1; x++)
      roomMap[z][x] = (randf(0, 1) < wallChance) ? '#' : '.';

  // Cellular automata smoothing
  int passes = isBoss ? 3 : (4 + randi(0, 1));
  for (int p = 0; p < passes; p++)
    smoothPass();

  // Key locations
  int spawnX = ROOM_W / 2, spawnZ = ROOM_H - 3;
  int doorZ = 2;

  // Clear spawn and door areas
  clearCircle(spawnX, spawnZ, 2);
  clearCircle(ROOM_W / 2, doorZ, 2);
  if (isBoss)
    clearCircle(ROOM_W / 2, ROOM_H / 2, 4);

  // Guarantee connectivity
  carveTunnel(spawnX, spawnZ, ROOM_W / 2, doorZ);
  removeDisconnected(spawnX, spawnZ);

  // Ensure enough floor space
  int floorCount = 0;
  for (int z = 1; z < ROOM_H - 1; z++)
    for (int x = 1; x < ROOM_W - 1; x++)
      if (roomMap[z][x] == '.')
        floorCount++;

  if (floorCount < 60) {
    for (int i = 0; i < 3; i++) {
      int cx = randi(3, ROOM_W - 4), cz = randi(3, ROOM_H - 4);
      clearCircle(cx, cz, 2);
      carveTunnel(spawnX, spawnZ, cx, cz);
    }
    removeDisconnected(spawnX, spawnZ);
  }

  playerStart = Vec2(spawnX + 0.5f, spawnZ + 0.5f);
  numEnemies = spawnEnemies(enemies, depth, isBoss);
}

// ════════════════════════════════════════
//  DOOR SETUP
// ════════════════════════════════════════

int setupDoors(Door doors[], int depth) {
  int numDoors = (depth % 5 == 4) ? 1 : randi(2, 3);
  if (numDoors > MAX_DOORS)
    numDoors = MAX_DOORS;

  BoonType used[MAX_DOORS] = {};
  for (int i = 0; i < numDoors; i++) {
    BoonType b;
    bool unique;
    do {
      b = (BoonType)(rand() % BOON_COUNT);
      unique = true;
      for (int j = 0; j < i; j++)
        if (used[j] == b)
          unique = false;
    } while (!unique);
    used[i] = b;
    doors[i].reward = b;
    doors[i].active = true;

    // Place doors along upper wall area, find walkable spots
    float spacing = (float)(ROOM_W - 4) / (numDoors + 1);
    float dx = 2.0f + spacing * (i + 1);
    // Find closest walkable cell near door position
    int bestX = (int)dx, bestZ = 2;
    for (int dz = 1; dz < 5; dz++)
      for (int ddx = -2; ddx <= 2; ddx++) {
        int tx = (int)dx + ddx, tz = dz;
        if (tx > 0 && tx < ROOM_W - 1 && tz > 0 && tz < ROOM_H - 1) {
          if (roomMap[tz][tx] == '.') {
            bestX = tx;
            bestZ = tz;
            dz = 99;
            break; // Found it
          }
        }
      }
    doors[i].pos = Vec2(bestX + 0.5f, bestZ + 0.5f);
    doors[i].wallSide = 0;
  }
  return numDoors;
}
