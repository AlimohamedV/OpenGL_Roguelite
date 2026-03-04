// ════════════════════════════════════════════════════════════════
//  DUNGEON ROGUELITE — Main Game
//  Build: g++ game/main.cpp -o dungeon.exe -lfreeglut -lopengl32 -lglu32 -O2
// ════════════════════════════════════════════════════════════════

#include <GL/freeglut.h>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

#include "renderer.h"
#include "types.h"
#include "world.h"

// ════════════════════════════════════════════════════════
//  GAME STATE
// ════════════════════════════════════════════════════════

static GameState gameState = STATE_TITLE;
static PlayerState player;
static RunStats stats = {0, 0, 0, 0, 0};
static Enemy enemies[MAX_ENEMIES];
static int numEnemies = 0;
static Projectile projectiles[MAX_PROJECTILES];
static Door doors[MAX_DOORS];
static int numDoors = 0;

// Boon selection
static BoonType boonChoices[3];
static int numBoonChoices = 0;

// Input
static bool keys[256] = {};
static bool specialKeys[256] = {};
static int winW = 1024, winH = 768;
static int centerX, centerY;
static bool warping = false;

// Timing
static int lastTime = 0;
static float gameTime = 0.0f;

// Room transition
static float transitionTimer = 0.0f;
static bool transitioning = false;
static int pendingBoon = -1; // Which boon was picked

// ════════════════════════════════════════════════════════
//  PLAYER INIT & BOONS
// ════════════════════════════════════════════════════════

static void initPlayer(Vec2 startPos) {
  player.pos = startPos;
  player.yaw = -(float)M_PI / 2.0f; // Face north
  player.pitch = 0;
  player.health = 100;
  player.maxHealth = 100;
  player.speed = 3.5f;
  player.damage = 1.0f;
  player.fireRate = 3.0f;
  player.fireCooldown = 0;
  player.dashCooldown = 0;
  player.dashMaxCooldown = 1.2f;
  player.dashTimer = 0;
  player.dashDuration = 0.15f;
  player.dashing = false;
  player.invincible = false;
  player.invTimer = 0;
  player.bobPhase = 0;
  player.pierceCount = 0;
  player.hasSpread = false;
  player.hasLifesteal = false;
  player.lifestealPct = 0;
  player.hasThorns = false;
  player.thornsDamage = 5.0f;
}

static void applyBoon(BoonType b) {
  switch (b) {
  case BOON_DAMAGE_UP:
    player.damage *= 1.25f;
    break;
  case BOON_SPEED_UP:
    player.speed *= 1.2f;
    break;
  case BOON_FIRE_RATE:
    player.fireRate *= 1.3f;
    break;
  case BOON_MAX_HP:
    player.maxHealth += 30;
    player.health += 30;
    break;
  case BOON_HEAL:
    player.health = fminf(player.health + 40, player.maxHealth);
    break;
  case BOON_DASH_CDR:
    player.dashMaxCooldown *= 0.7f;
    break;
  case BOON_PIERCE:
    player.pierceCount++;
    break;
  case BOON_SPREAD:
    player.hasSpread = true;
    break;
  case BOON_LIFESTEAL:
    player.hasLifesteal = true;
    player.lifestealPct += 0.1f;
    break;
  case BOON_THORNS:
    player.hasThorns = true;
    player.thornsDamage += 5.0f;
    break;
  default:
    break;
  }
  stats.boonsCollected++;
}

// ════════════════════════════════════════════════════════
//  GAME INIT
// ════════════════════════════════════════════════════════

static void startNewRun() {
  stats.roomDepth = 0;
  stats.enemiesKilled = 0;
  stats.boonsCollected = 0;
  stats.totalRuns++;

  Vec2 startPos;
  bool isBoss = false;
  generateRoom(0, isBoss, enemies, numEnemies, startPos);
  initPlayer(startPos);

  // Clear projectiles & particles
  memset(projectiles, 0, sizeof(projectiles));
  clearParticles();
  numDoors = 0;
  numBoonChoices = 0;

  fadeAlpha = 1.0f; // Fade in
  gameState = STATE_PLAYING;
}

static void advanceRoom(BoonType reward) {
  applyBoon(reward);
  stats.roomDepth++;

  if (stats.roomDepth > stats.bestDepth)
    stats.bestDepth = stats.roomDepth;

  bool isBoss = ((stats.roomDepth + 1) % 5 == 0);
  Vec2 startPos;
  generateRoom(stats.roomDepth, isBoss, enemies, numEnemies, startPos);
  player.pos = startPos;
  player.yaw = -(float)M_PI / 2.0f;
  player.pitch = 0;

  memset(projectiles, 0, sizeof(projectiles));
  clearParticles();
  numDoors = 0;
  numBoonChoices = 0;

  fadeAlpha = 1.0f;
  gameState = STATE_PLAYING;
}

// ════════════════════════════════════════════════════════
//  PROJECTILE SYSTEM
// ════════════════════════════════════════════════════════

static void spawnProjectile(Vec2 pos, Vec2 dir, ProjectileOwner owner,
                            int pierce) {
  for (int i = 0; i < MAX_PROJECTILES; i++) {
    if (projectiles[i].alive)
      continue;
    projectiles[i].pos = pos;
    projectiles[i].vel = dir * (owner == PROJ_PLAYER ? 12.0f : 5.0f);
    projectiles[i].owner = owner;
    projectiles[i].alive = true;
    projectiles[i].lifetime = 3.0f;
    projectiles[i].pierce = pierce;
    return;
  }
}

static void playerShoot() {
  if (player.fireCooldown > 0)
    return;
  player.fireCooldown = 1.0f / player.fireRate;

  Vec2 dir(cosf(player.yaw), sinf(player.yaw));

  if (player.hasSpread) {
    // 3-shot spread
    for (int s = -1; s <= 1; s++) {
      float angle = player.yaw + s * 0.12f;
      Vec2 d(cosf(angle), sinf(angle));
      spawnProjectile(player.pos + d * 0.3f, d, PROJ_PLAYER,
                      player.pierceCount);
    }
  } else {
    spawnProjectile(player.pos + dir * 0.3f, dir, PROJ_PLAYER,
                    player.pierceCount);
  }

  // Muzzle flash
  flashR = 1;
  flashG = 0.8f;
  flashB = 0.3f;
  flashA = 0.15f;
  screenShake = 0.03f;

  // Muzzle particles
  spawnBurst(player.pos + dir * 0.4f, 0.4f, 3, 1.0f, 0.8f, 0.3f);
}

static void updateProjectiles(float dt) {
  for (int i = 0; i < MAX_PROJECTILES; i++) {
    Projectile &p = projectiles[i];
    if (!p.alive)
      continue;

    p.pos = p.pos + p.vel * dt;
    p.lifetime -= dt;

    // Wall collision
    if (isWall((int)p.pos.x, (int)p.pos.z)) {
      p.alive = false;
      spawnBurst(p.pos, 0.4f, 5, 0.5f, 0.5f, 0.5f);
      continue;
    }

    if (p.lifetime <= 0) {
      p.alive = false;
      continue;
    }

    if (p.owner == PROJ_PLAYER) {
      // Hit enemies
      for (int e = 0; e < numEnemies; e++) {
        if (!enemies[e].alive)
          continue;
        float dist = (enemies[e].pos - p.pos).len();
        if (dist < enemies[e].radius + 0.1f) {
          enemies[e].health -= player.damage;
          enemies[e].hitAnim = 0.2f;
          screenShake = 0.04f;

          // Hit particles
          spawnBurst(enemies[e].pos, 0.35f, 8, 1, 0.5f, 0.2f);

          if (enemies[e].health <= 0) {
            enemies[e].alive = false;
            enemies[e].deathAnim = 1.0f;
            stats.enemiesKilled++;

            // Death explosion
            float cr, cg, cb;
            switch (enemies[e].type) {
            case ENEMY_CHASER:
              cr = 1;
              cg = 0.2f;
              cb = 0.1f;
              break;
            case ENEMY_SHOOTER:
              cr = 0.7f;
              cg = 0.2f;
              cb = 1;
              break;
            case ENEMY_TANK:
              cr = 1;
              cg = 0.6f;
              cb = 0.1f;
              break;
            case ENEMY_BOSS:
              cr = 1;
              cg = 0.1f;
              cb = 0.1f;
              break;
            }
            spawnBurst(enemies[e].pos, 0.35f, 25, cr, cg, cb);
            screenShake = 0.1f;

            // Lifesteal
            if (player.hasLifesteal) {
              player.health = fminf(player.health + enemies[e].maxHealth *
                                                        player.lifestealPct,
                                    player.maxHealth);
            }
          }

          if (p.pierce <= 0) {
            p.alive = false;
          } else {
            p.pierce--;
          }
          break;
        }
      }
    } else {
      // Enemy projectile hits player
      if (!player.invincible && !player.dashing) {
        float dist = (player.pos - p.pos).len();
        if (dist < 0.25f) {
          player.health -= 10.0f;
          p.alive = false;
          flashR = 1;
          flashG = 0;
          flashB = 0;
          flashA = 0.3f;
          screenShake = 0.06f;
          player.invincible = true;
          player.invTimer = 0.3f;

          if (player.health <= 0) {
            player.health = 0;
            gameState = STATE_DEAD;
            if (stats.roomDepth > stats.bestDepth)
              stats.bestDepth = stats.roomDepth;
          }
        }
      }
    }
  }
}

// ════════════════════════════════════════════════════════
//  ENEMY AI
// ════════════════════════════════════════════════════════

static void updateEnemies(float dt) {
  for (int i = 0; i < numEnemies; i++) {
    Enemy &e = enemies[i];
    if (!e.alive) {
      if (e.deathAnim > 0)
        e.deathAnim -= dt * 2.0f;
      continue;
    }
    if (e.hitAnim > 0)
      e.hitAnim -= dt * 5.0f;

    Vec2 toPlayer = player.pos - e.pos;
    float dist = toPlayer.len();
    Vec2 dir = toPlayer.norm();

    // Update facing direction (smooth turn toward player)
    if (dist > 0.3f) {
      float targetAngle = atan2f(dir.x, dir.z);
      // Smooth rotation
      float angleDiff = targetAngle - e.facingAngle;
      // Normalize to [-PI, PI]
      while (angleDiff > (float)M_PI)
        angleDiff -= 2.0f * (float)M_PI;
      while (angleDiff < -(float)M_PI)
        angleDiff += 2.0f * (float)M_PI;
      e.facingAngle += angleDiff * clampf(dt * 8.0f, 0, 1);
    }

    switch (e.type) {
    case ENEMY_CHASER: {
      // Separation: push away from other enemies
      Vec2 sep(0, 0);
      for (int j = 0; j < numEnemies; j++) {
        if (j == i || !enemies[j].alive)
          continue;
        Vec2 away = e.pos - enemies[j].pos;
        float d = away.len();
        if (d < 0.8f && d > 0.01f) {
          sep = sep + away.norm() * (0.8f - d) * 5.0f;
        }
      }

      // Use flow field for pathfinding (navigates around walls)
      if (dist > 0.4f && dist < 12.0f) {
        Vec2 flowDir = getFlowDirection(e.pos.x, e.pos.z);
        // Blend flow field with direct dir when close (smoother interception)
        Vec2 pathDir = (dist < 2.0f) ? dir : flowDir;
        if (pathDir.len() < 0.01f)
          pathDir = dir; // Fallback
        Vec2 moveDir = (pathDir + sep * 0.5f).norm();
        Vec2 newPos = e.pos + moveDir * e.speed * dt;
        if (canMove(newPos.x, e.pos.z, e.radius))
          e.pos.x = newPos.x;
        if (canMove(e.pos.x, newPos.z, e.radius))
          e.pos.z = newPos.z;
      }
      // Melee damage
      if (dist < 0.45f && !player.invincible && !player.dashing) {
        player.health -= e.damage * dt;
        flashR = 1;
        flashG = 0;
        flashB = 0;
        flashA = 0.15f;
        if (player.hasThorns) {
          e.health -= player.thornsDamage * dt;
          if (e.health <= 0) {
            e.alive = false;
            e.deathAnim = 1.0f;
            stats.enemiesKilled++;
            spawnBurst(e.pos, 0.35f, 20, 1, 0.2f, 0.1f);
          }
        }
        if (player.health <= 0) {
          player.health = 0;
          gameState = STATE_DEAD;
          if (stats.roomDepth > stats.bestDepth)
            stats.bestDepth = stats.roomDepth;
        }
      }
      break;
    }

    case ENEMY_SHOOTER: {
      // Keep distance (3-6 tiles), strafe
      float idealDist = 4.5f;
      if (dist < idealDist - 0.5f) {
        Vec2 away = dir * -1.0f;
        Vec2 newPos = e.pos + away * e.speed * dt;
        if (canMove(newPos.x, e.pos.z, e.radius))
          e.pos.x = newPos.x;
        if (canMove(e.pos.x, newPos.z, e.radius))
          e.pos.z = newPos.z;
      } else if (dist > idealDist + 1.0f && dist < 12.0f) {
        Vec2 newPos = e.pos + dir * e.speed * dt;
        if (canMove(newPos.x, e.pos.z, e.radius))
          e.pos.x = newPos.x;
        if (canMove(e.pos.x, newPos.z, e.radius))
          e.pos.z = newPos.z;
      }
      // Shoot
      e.shootTimer -= dt;
      if (e.shootTimer <= 0 && dist < 10.0f) {
        e.shootTimer = 2.0f;
        spawnProjectile(e.pos, dir, PROJ_ENEMY, 0);
        spawnBurst(e.pos + dir * 0.3f, 0.4f, 3, 0.7f, 0.2f, 1.0f);
      }
      break;
    }

    case ENEMY_TANK: {
      // Slow chase using flow field
      if (dist > 0.5f && dist < 10.0f) {
        Vec2 flowDir = getFlowDirection(e.pos.x, e.pos.z);
        Vec2 pathDir = (dist < 2.0f) ? dir : flowDir;
        if (pathDir.len() < 0.01f)
          pathDir = dir;
        Vec2 newPos = e.pos + pathDir.norm() * e.speed * dt;
        if (canMove(newPos.x, e.pos.z, e.radius))
          e.pos.x = newPos.x;
        if (canMove(e.pos.x, newPos.z, e.radius))
          e.pos.z = newPos.z;
      }
      // Heavy melee
      if (dist < 0.55f && !player.invincible && !player.dashing) {
        player.health -= e.damage * dt;
        flashR = 1;
        flashG = 0.3f;
        flashB = 0;
        flashA = 0.2f;
        screenShake = 0.05f;
        if (player.hasThorns) {
          e.health -= player.thornsDamage * dt;
          if (e.health <= 0) {
            e.alive = false;
            e.deathAnim = 1.0f;
            stats.enemiesKilled++;
            spawnBurst(e.pos, 0.35f, 30, 1, 0.6f, 0.1f);
            screenShake = 0.12f;
          }
        }
        if (player.health <= 0) {
          player.health = 0;
          gameState = STATE_DEAD;
          if (stats.roomDepth > stats.bestDepth)
            stats.bestDepth = stats.roomDepth;
        }
      }
      break;
    }

    case ENEMY_BOSS: {
      // Alternate between chase and shoot, with occasional charge
      e.chargeTimer -= dt;

      if (e.charging) {
        // Charge dash
        Vec2 newPos = e.pos + e.chargeDir * 8.0f * dt;
        if (canMove(newPos.x, e.pos.z, e.radius))
          e.pos.x = newPos.x;
        else
          e.charging = false;
        if (canMove(e.pos.x, newPos.z, e.radius))
          e.pos.z = newPos.z;
        else
          e.charging = false;

        e.dashTimer -= dt;
        if (e.dashTimer <= 0)
          e.charging = false;

        // Charge damage
        if (dist < 0.6f && !player.invincible && !player.dashing) {
          player.health -= 25;
          flashR = 1;
          flashG = 0;
          flashB = 0;
          flashA = 0.4f;
          screenShake = 0.15f;
          player.invincible = true;
          player.invTimer = 0.5f;
          e.charging = false;
          if (player.health <= 0) {
            player.health = 0;
            gameState = STATE_DEAD;
            if (stats.roomDepth > stats.bestDepth)
              stats.bestDepth = stats.roomDepth;
          }
        }
      } else {
        // Normal chase
        if (dist > 0.6f && dist < 12.0f) {
          Vec2 newPos = e.pos + dir * e.speed * dt;
          if (canMove(newPos.x, e.pos.z, e.radius))
            e.pos.x = newPos.x;
          if (canMove(e.pos.x, newPos.z, e.radius))
            e.pos.z = newPos.z;
        }

        // Shoot periodically
        e.shootTimer -= dt;
        if (e.shootTimer <= 0 && dist < 10.0f) {
          e.shootTimer = 1.5f;
          // Fire 3 projectiles in spread
          for (int s = -1; s <= 1; s++) {
            float angle = atan2f(dir.z, dir.x) + s * 0.2f;
            Vec2 d(cosf(angle), sinf(angle));
            spawnProjectile(e.pos + d * 0.5f, d, PROJ_ENEMY, 0);
          }
          spawnBurst(e.pos, 0.4f, 5, 0.8f, 0.1f, 0.1f);
        }

        // Charge attack
        if (e.chargeTimer <= 0 && dist < 8.0f && dist > 2.0f) {
          e.charging = true;
          e.chargeDir = dir;
          e.dashTimer = 0.5f;
          e.chargeTimer = 4.0f + randf(0, 2);
          screenShake = 0.08f;
        }
      }

      // Thorns
      if (dist < 0.6f && player.hasThorns && !player.dashing) {
        e.health -= player.thornsDamage * dt;
      }

      if (e.health <= 0 && e.alive) {
        e.alive = false;
        e.deathAnim = 1.0f;
        stats.enemiesKilled++;
        spawnBurst(e.pos, 0.35f, 50, 1, 0.2f, 0.05f);
        screenShake = 0.2f;

        // Boss kill = win (every 5 rooms)
        if ((stats.roomDepth + 1) % 5 == 0) {
          gameState = STATE_WIN;
          if (stats.roomDepth > stats.bestDepth)
            stats.bestDepth = stats.roomDepth;
        }
      }
      break;
    }
    }
  }
}

// ════════════════════════════════════════════════════════
//  CHECK ROOM CLEAR
// ════════════════════════════════════════════════════════

static bool allEnemiesDead() {
  for (int i = 0; i < numEnemies; i++)
    if (enemies[i].alive)
      return false;
  return true;
}

static void checkRoomClear() {
  if (gameState != STATE_PLAYING)
    return;
  if (!allEnemiesDead())
    return;

  gameState = STATE_ROOM_CLEAR;
  numDoors = setupDoors(doors, stats.roomDepth);
}

// ════════════════════════════════════════════════════════
//  UPDATE
// ════════════════════════════════════════════════════════

static void update(float dt) {
  gameTime += dt;

  // Decay screen effects
  if (screenShake > 0)
    screenShake -= dt * 2.0f;
  if (flashA > 0)
    flashA -= dt * 3.0f;
  if (fadeAlpha > 0)
    fadeAlpha -= dt * 2.5f;
  if (fadeAlpha < 0)
    fadeAlpha = 0;

  updateParticles(dt);

  if (gameState == STATE_TITLE)
    return;
  if (gameState == STATE_DEAD || gameState == STATE_WIN)
    return;

  // ── Room transition ──
  if (transitioning) {
    transitionTimer -= dt;
    fadeAlpha = 1.0f; // Keep black during transition
    if (transitionTimer <= 0) {
      transitioning = false;
      if (pendingBoon >= 0 && pendingBoon < numBoonChoices) {
        advanceRoom(boonChoices[pendingBoon]);
      }
    }
    return;
  }

  if (gameState == STATE_BOON_SELECT)
    return; // Wait for input

  // ── Player movement ──
  if (player.fireCooldown > 0)
    player.fireCooldown -= dt;
  if (player.dashCooldown > 0)
    player.dashCooldown -= dt;
  if (player.invTimer > 0) {
    player.invTimer -= dt;
  } else {
    player.invincible = false;
  }

  // Dash update
  if (player.dashing) {
    player.dashTimer -= dt;
    Vec2 dashMove = player.dashDir * 12.0f * dt;
    if (canMove(player.pos.x + dashMove.x, player.pos.z, 0.2f))
      player.pos.x += dashMove.x;
    if (canMove(player.pos.x, player.pos.z + dashMove.z, 0.2f))
      player.pos.z += dashMove.z;
    if (player.dashTimer <= 0) {
      player.dashing = false;
      player.invincible = false;
    }
  } else {
    // Normal movement
    float speed = player.speed * dt;
    float fwdX = cosf(player.yaw), fwdZ = sinf(player.yaw);
    float rightX = cosf(player.yaw + (float)M_PI / 2);
    float rightZ = sinf(player.yaw + (float)M_PI / 2);
    float mx = 0, mz = 0;

    if (keys['w'] || keys['W']) {
      mx += fwdX;
      mz += fwdZ;
    }
    if (keys['s'] || keys['S']) {
      mx -= fwdX;
      mz -= fwdZ;
    }
    if (keys['a'] || keys['A']) {
      mx -= rightX;
      mz -= rightZ;
    }
    if (keys['d'] || keys['D']) {
      mx += rightX;
      mz += rightZ;
    }

    float len = sqrtf(mx * mx + mz * mz);
    if (len > 0.01f) {
      mx = mx / len * speed;
      mz = mz / len * speed;
      if (canMove(player.pos.x + mx, player.pos.z, 0.2f))
        player.pos.x += mx;
      if (canMove(player.pos.x, player.pos.z + mz, 0.2f))
        player.pos.z += mz;
      player.bobPhase += dt * 12.0f;
    }
  }

  // Enemy hit on death anim
  for (int i = 0; i < numEnemies; i++)
    if (!enemies[i].alive && enemies[i].deathAnim > 0)
      enemies[i].deathAnim -= dt * 2.0f;

  // Recompute BFS flow field toward player
  computeFlowField(player.pos.x, player.pos.z);

  updateEnemies(dt);
  updateProjectiles(dt);

  // ── Check door collision (room clear / boon select) ──
  if (gameState == STATE_ROOM_CLEAR) {
    for (int i = 0; i < numDoors; i++) {
      if (!doors[i].active)
        continue;
      float dist = (player.pos - doors[i].pos).len();
      if (dist < 0.6f) {
        // Enter door → pick boon and transition
        // Generate boon choices (just one for this door)
        boonChoices[0] = doors[i].reward;
        numBoonChoices = 1;

        // For multi-door: directly apply the reward and transition
        pendingBoon = 0;
        transitioning = true;
        transitionTimer = 0.5f;
        fadeAlpha = 0;

        break;
      }
    }
  }

  checkRoomClear();
}

// ════════════════════════════════════════════════════════
//  DISPLAY
// ════════════════════════════════════════════════════════

static void display() {
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  if (gameState == STATE_TITLE) {
    // Title screen
    glLoadIdentity();
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, winW, 0, winH);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glClearColor(0.02f, 0.02f, 0.04f, 1);
    float cx = winW / 2.0f, cy = winH / 2.0f;

    glColor3f(0.9f, 0.3f, 0.2f);
    drawText2D(cx - 100, cy + 60, "DUNGEON ROGUELITE");
    glColor3f(0.7f, 0.65f, 0.5f);
    drawText2D(cx - 120, cy + 10, "A Hades-inspired dungeon crawler",
               GLUT_BITMAP_HELVETICA_12);
    glColor3f(0.9f, 0.9f, 0.9f);
    drawText2D(cx - 80, cy - 40, "Press ENTER to start");

    if (stats.totalRuns > 0) {
      char buf[64];
      snprintf(buf, sizeof(buf), "Best: Room %d  |  Runs: %d",
               stats.bestDepth + 1, stats.totalRuns);
      glColor3f(0.5f, 0.5f, 0.5f);
      drawText2D(cx - 90, cy - 80, buf, GLUT_BITMAP_HELVETICA_12);
    }

    glColor3f(0.4f, 0.4f, 0.4f);
    drawText2D(cx - 140, 40,
               "WASD: Move  Mouse: Look  Click: Shoot  Shift: Dash",
               GLUT_BITMAP_HELVETICA_12);

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);

    glutSwapBuffers();
    return;
  }

  glLoadIdentity();

  // ── Camera with shake ──
  float shakeX = 0, shakeY = 0;
  if (screenShake > 0) {
    shakeX = randf(-screenShake, screenShake);
    shakeY = randf(-screenShake, screenShake);
  }
  float eyeY = 0.45f + 0.02f * sinf(player.bobPhase) + shakeY;
  float lookX = player.pos.x + cosf(player.yaw) + shakeX;
  float lookZ = player.pos.z + sinf(player.yaw);
  float lookY = eyeY + sinf(player.pitch);
  gluLookAt(player.pos.x + shakeX, eyeY, player.pos.z, lookX, lookY, lookZ, 0,
            1, 0);

  // ── Torch light ──
  float t = gameTime;
  float flicker = 1.0f + 0.08f * sinf(t * 37.1f) + 0.05f * sinf(t * 71.3f) +
                  0.03f * sinf(t * 113.7f);
  float lightPos[] = {player.pos.x, 0.7f, player.pos.z, 1.0f};
  float lightDif[] = {0.95f * flicker, 0.65f * flicker, 0.3f * flicker, 1};
  float lightAmb[] = {0.035f, 0.025f, 0.015f, 1};
  glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
  glLightfv(GL_LIGHT0, GL_DIFFUSE, lightDif);
  glLightfv(GL_LIGHT0, GL_AMBIENT, lightAmb);
  glLightf(GL_LIGHT0, GL_CONSTANT_ATTENUATION, 0.2f);
  glLightf(GL_LIGHT0, GL_LINEAR_ATTENUATION, 0.3f);
  glLightf(GL_LIGHT0, GL_QUADRATIC_ATTENUATION, 0.15f);

  // Material
  float matDif[] = {1, 1, 1, 1};
  float matAmb[] = {0.15f, 0.15f, 0.15f, 1};
  glMaterialfv(GL_FRONT, GL_DIFFUSE, matDif);
  glMaterialfv(GL_FRONT, GL_AMBIENT, matAmb);
  glMaterialf(GL_FRONT, GL_SHININESS, 10.0f);
  glColor3f(1, 1, 1);

  // ── Draw world ──
  drawWalls();
  drawFloorCeiling();
  if (gameState == STATE_ROOM_CLEAR)
    drawDoors(doors, numDoors, gameTime);
  drawEnemies(enemies, numEnemies, player.yaw, gameTime);
  drawProjectiles(projectiles, MAX_PROJECTILES, player.yaw);
  drawParticles(player.yaw);

  // ── HUD ──
  drawHUD(winW, winH, player, stats, gameState, boonChoices, numBoonChoices,
          doors, numDoors, gameTime);

  glutSwapBuffers();
}

// ════════════════════════════════════════════════════════
//  INPUT
// ════════════════════════════════════════════════════════

static void keyDown(unsigned char key, int, int) {
  keys[key] = true;

  if (key == 27)
    exit(0); // ESC

  if (gameState == STATE_TITLE && (key == 13 || key == ' ')) {
    startNewRun();
    return;
  }

  if ((key == 'r' || key == 'R') &&
      (gameState == STATE_DEAD || gameState == STATE_WIN)) {
    gameState = STATE_TITLE;
    return;
  }

  // Dash (shift is handled via special keys, but also allow space as
  // alternative)
  if (gameState == STATE_PLAYING && !player.dashing &&
      player.dashCooldown <= 0) {
    // Spacebar dash alternative
  }

  // Boon selection with number keys
  if (gameState == STATE_BOON_SELECT) {
    int choice = -1;
    if (key == '1')
      choice = 0;
    if (key == '2')
      choice = 1;
    if (key == '3')
      choice = 2;
    if (choice >= 0 && choice < numBoonChoices) {
      pendingBoon = choice;
      transitioning = true;
      transitionTimer = 0.5f;
      fadeAlpha = 0;
    }
  }
}

static void keyUp(unsigned char key, int, int) { keys[key] = false; }

static void specialDown(int key, int, int) {
  specialKeys[key] = true;

  // Shift = dash
  if ((key == GLUT_KEY_SHIFT_L || key == GLUT_KEY_SHIFT_R) &&
      gameState == STATE_PLAYING && !player.dashing &&
      player.dashCooldown <= 0) {
    player.dashing = true;
    player.invincible = true;
    player.dashTimer = player.dashDuration;
    player.dashCooldown = player.dashMaxCooldown;
    player.dashDir = Vec2(cosf(player.yaw), sinf(player.yaw));

    // Dash particles
    for (int i = 0; i < 8; i++) {
      spawnParticle(player.pos, 0.2f + randf(0, 0.4f),
                    -player.dashDir.x * randf(1, 3), randf(0.5f, 2),
                    -player.dashDir.z * randf(1, 3), 0.3f, 0.5f, 1.0f,
                    randf(0.3f, 0.6f), randf(0.04f, 0.08f));
    }
  }
}

static void specialUp(int key, int, int) { specialKeys[key] = false; }

static void mouseMotion(int x, int y) {
  if (warping)
    return;
  if (gameState != STATE_PLAYING && gameState != STATE_ROOM_CLEAR)
    return;

  int dx = x - centerX;
  int dy = y - centerY;
  if (dx == 0 && dy == 0)
    return;

  player.yaw += dx * 0.003f;
  player.pitch -= dy * 0.003f;
  player.pitch = clampf(player.pitch, -1.2f, 1.2f);

  warping = true;
  glutWarpPointer(centerX, centerY);
  warping = false;
}

static void mouseClick(int button, int state, int, int) {
  if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
    if (gameState == STATE_PLAYING)
      playerShoot();
  }
}

// ════════════════════════════════════════════════════════
//  GLUT CALLBACKS
// ════════════════════════════════════════════════════════

static void idle() {
  int now = glutGet(GLUT_ELAPSED_TIME);
  float dt = (now - lastTime) / 1000.0f;
  if (dt > 0.05f)
    dt = 0.05f;
  lastTime = now;
  update(dt);
  glutPostRedisplay();
}

static void reshape(int w, int h) {
  winW = w;
  winH = h;
  centerX = w / 2;
  centerY = h / 2;
  if (h == 0)
    h = 1;
  glViewport(0, 0, w, h);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(75.0, (double)w / h, 0.05, 50.0);
  glMatrixMode(GL_MODELVIEW);
}

// ════════════════════════════════════════════════════════
//  MAIN
// ════════════════════════════════════════════════════════

int main(int argc, char **argv) {
  srand((unsigned)time(NULL));

  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
  glutInitWindowSize(winW, winH);
  glutCreateWindow("Dungeon Roguelite");

  glEnable(GL_DEPTH_TEST);
  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);
  glEnable(GL_COLOR_MATERIAL);
  glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
  glEnable(GL_NORMALIZE);
  glShadeModel(GL_SMOOTH);
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

  // Fog
  glEnable(GL_FOG);
  float fogCol[] = {0, 0, 0, 1};
  glFogfv(GL_FOG_COLOR, fogCol);
  glFogi(GL_FOG_MODE, GL_EXP2);
  glFogf(GL_FOG_DENSITY, 0.3f);

  initTextures();

  glutSetCursor(GLUT_CURSOR_NONE);

  glutDisplayFunc(display);
  glutReshapeFunc(reshape);
  glutKeyboardFunc(keyDown);
  glutKeyboardUpFunc(keyUp);
  glutSpecialFunc(specialDown);
  glutSpecialUpFunc(specialUp);
  glutPassiveMotionFunc(mouseMotion);
  glutMotionFunc(mouseMotion);
  glutMouseFunc(mouseClick);
  glutIdleFunc(idle);

  centerX = winW / 2;
  centerY = winH / 2;
  glutWarpPointer(centerX, centerY);
  lastTime = glutGet(GLUT_ELAPSED_TIME);

  printf("═══════════════════════════════════\n");
  printf("   DUNGEON ROGUELITE\n");
  printf("═══════════════════════════════════\n");
  printf("WASD: Move      Mouse: Look\n");
  printf("Click: Shoot    Shift: Dash\n");
  printf("1/2/3: Pick boon\n");
  printf("R: Restart      ESC: Quit\n");
  printf("═══════════════════════════════════\n");
  printf("Clear rooms, collect boons, beat the boss!\n\n");

  glutMainLoop();
  return 0;
}
