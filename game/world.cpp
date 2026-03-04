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
      int nx = cur.x + dx8[d];
      int nz = cur.z + dz8[d];

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
      float fdx = (float)(cur.x - nx);
      float fdz = (float)(cur.z - nz);
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
//  ROOM GENERATION
// ════════════════════════════════════════

static void fillWalls() {
  for (int z = 0; z < ROOM_H; z++)
    for (int x = 0; x < ROOM_W; x++)
      roomMap[z][x] = '#';
  for (int z = 0; z < ROOM_H; z++)
    roomMap[z][ROOM_W] = '\0';
}

static void carveArena(int depth) {
  int pad = 3 - depth / 3;
  if (pad < 1)
    pad = 1;
  for (int z = pad; z < ROOM_H - pad; z++)
    for (int x = pad; x < ROOM_W - pad; x++)
      roomMap[z][x] = '.';
}

static void placeObstacles(int count, int pad) {
  for (int i = 0; i < count; i++) {
    int cx = randi(pad + 2, ROOM_W - pad - 3);
    int cz = randi(pad + 2, ROOM_H - pad - 3);
    int shape = randi(0, 4);

    switch (shape) {
    case 0:
      roomMap[cz][cx] = '#';
      break;
    case 1:
      roomMap[cz][cx] = '#';
      roomMap[cz + 1][cx] = '#';
      roomMap[cz][cx + 1] = '#';
      break;
    case 2:
      roomMap[cz][cx] = '#';
      roomMap[cz][cx + 1] = '#';
      roomMap[cz][cx - 1] = '#';
      roomMap[cz + 1][cx] = '#';
      break;
    case 3:
      for (int dx = 0; dx < randi(2, 3); dx++)
        if (cx + dx < ROOM_W - pad - 1)
          roomMap[cz][cx + dx] = '#';
      break;
    case 4:
      for (int dz = 0; dz < randi(2, 3); dz++)
        if (cz + dz < ROOM_H - pad - 1)
          roomMap[cz + dz][cx] = '#';
      break;
    }
  }

  // Clear player start area
  for (int dz = -1; dz <= 1; dz++)
    for (int dx = -1; dx <= 1; dx++) {
      int x = ROOM_W / 2 + dx;
      int z = ROOM_H - pad - 2 + dz;
      if (x > 0 && x < ROOM_W - 1 && z > 0 && z < ROOM_H - 1)
        roomMap[z][x] = '.';
    }

  // Clear door area
  for (int dx = -3; dx <= 3; dx++) {
    int x = ROOM_W / 2 + dx;
    if (x > pad && x < ROOM_W - pad)
      roomMap[pad][x] = '.';
    if (x > pad && x < ROOM_W - pad)
      roomMap[pad + 1][x] = '.';
  }
}

static int spawnEnemies(Enemy enemies[], int depth, bool isBoss, int pad) {
  int count = 0;

  if (isBoss) {
    enemies[count].pos = Vec2(ROOM_W / 2.0f, ROOM_H / 2.0f - 2.0f);
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
      enemies[count].pos =
          Vec2(randf(pad + 1, ROOM_W - pad - 1), randf(pad + 1, ROOM_H / 2.0f));
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

  int numChasers = 4 + depth;
  int numShooters = depth >= 1 ? 1 + depth / 2 : 0;
  int numTanks = depth >= 3 ? 1 + (depth - 3) / 3 : 0;
  int total = numChasers + numShooters + numTanks;
  if (total > MAX_ENEMIES - 2)
    total = MAX_ENEMIES - 2;

  for (int i = 0; i < total && count < MAX_ENEMIES; i++) {
    Enemy &e = enemies[count];
    e.pos = Vec2(ROOM_W / 2.0f, ROOM_H / 2.0f);
    for (int attempt = 0; attempt < 30; attempt++) {
      float ex = randf(pad + 1, ROOM_W - pad - 1);
      float ez = randf(pad + 1, ROOM_H * 0.65f);
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

void generateRoom(int depth, bool isBoss, Enemy enemies[], int &numEnemies,
                  Vec2 &playerStart) {
  fillWalls();

  int pad = 3 - depth / 3;
  if (pad < 1)
    pad = 1;
  if (isBoss)
    pad = 1;

  carveArena(depth);

  int obstacles = isBoss ? randi(3, 5) : randi(4, 6 + depth / 2);
  if (obstacles > 10)
    obstacles = 10;
  placeObstacles(obstacles, pad);

  playerStart = Vec2(ROOM_W / 2.0f, ROOM_H - pad - 1.5f);
  numEnemies = spawnEnemies(enemies, depth, isBoss, pad);
}

int setupDoors(Door doors[], int depth) {
  int numDoors = (depth % 5 == 4) ? 1 : randi(2, 3);
  if (numDoors > MAX_DOORS)
    numDoors = MAX_DOORS;

  int pad = 3 - depth / 3;
  if (pad < 1)
    pad = 1;

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

    float roomLeft = (float)(pad + 1);
    float roomRight = (float)(ROOM_W - pad - 1);
    float spacing = (roomRight - roomLeft) / (numDoors + 1);
    float dx = roomLeft + spacing * (i + 1);
    doors[i].pos = Vec2(dx, (float)(pad) + 0.5f);
    doors[i].wallSide = 0;
  }
  return numDoors;
}
