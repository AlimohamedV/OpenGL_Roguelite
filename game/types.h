// ════════════════════════════════════════════════════════════════
//  DUNGEON ROGUELITE — Shared Types, Constants, Math Utilities
// ════════════════════════════════════════════════════════════════
#pragma once

#include <cmath>
#include <cstdint>
#include <cstdlib>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ── Map ──
#define ROOM_W 16
#define ROOM_H 16
#define TILE 1.0f
#define WALL_H 1.0f

// ── Limits ──
#define MAX_ENEMIES 24
#define MAX_PROJECTILES 64
#define MAX_PARTICLES 256
#define MAX_BOONS 12
#define MAX_DOORS 3

// ── Enums ──

enum GameState {
  STATE_TITLE,
  STATE_PLAYING,
  STATE_ROOM_CLEAR,
  STATE_BOON_SELECT,
  STATE_ROOM_TRANSITION,
  STATE_BOSS_INTRO,
  STATE_DEAD,
  STATE_WIN,
};

enum EnemyType {
  ENEMY_CHASER,  // Red — runs at player, melee
  ENEMY_SHOOTER, // Purple — keeps distance, shoots
  ENEMY_TANK,    // Orange — slow, high HP, high damage
  ENEMY_BOSS,    // Dark red — big, shoots + charges
};

enum BoonType {
  BOON_DAMAGE_UP, // +25% damage
  BOON_SPEED_UP,  // +20% move speed
  BOON_FIRE_RATE, // +30% fire rate
  BOON_MAX_HP,    // +30 max HP
  BOON_HEAL,      // Restore 40 HP
  BOON_DASH_CDR,  // -30% dash cooldown
  BOON_PIERCE,    // Projectiles pierce 1 extra enemy
  BOON_SPREAD,    // Fire 3 projectiles in a spread
  BOON_LIFESTEAL, // 10% damage healed on kill
  BOON_THORNS,    // Enemies take damage on contact
  BOON_COUNT
};

enum ProjectileOwner {
  PROJ_PLAYER,
  PROJ_ENEMY,
};

// ── Structs ──

struct Vec2 {
  float x, z;
  Vec2() : x(0), z(0) {}
  Vec2(float x_, float z_) : x(x_), z(z_) {}
  Vec2 operator+(Vec2 o) const { return {x + o.x, z + o.z}; }
  Vec2 operator-(Vec2 o) const { return {x - o.x, z - o.z}; }
  Vec2 operator*(float s) const { return {x * s, z * s}; }
  float len() const { return sqrtf(x * x + z * z); }
  Vec2 norm() const {
    float l = len();
    return l > 0.001f ? Vec2(x / l, z / l) : Vec2(0, 0);
  }
  float dot(Vec2 o) const { return x * o.x + z * o.z; }
};

struct Enemy {
  Vec2 pos;
  EnemyType type;
  float health, maxHealth;
  bool alive;
  float hitAnim;
  float deathAnim;
  float shootTimer;
  float chargeTimer; // Boss charge cooldown
  float dashTimer;   // Boss charge duration
  bool charging;
  Vec2 chargeDir;
  float radius;
  float speed;
  float damage;
  float facingAngle; // Radians, 0 = +Z direction
};

struct Projectile {
  Vec2 pos, vel;
  ProjectileOwner owner;
  bool alive;
  float lifetime;
  int pierce; // How many enemies it can still pass through
};

struct Particle {
  Vec2 pos;
  float y, vy;  // Vertical position/velocity
  float vx, vz; // Horizontal velocity
  float r, g, b, a;
  float life, maxLife;
  float size;
  bool alive;
};

struct Boon {
  BoonType type;
  bool active;
};

struct Door {
  Vec2 pos;
  BoonType reward; // What boon this door offers
  bool active;
  int wallSide; // 0=north, 1=south, 2=west, 3=east
};

struct PlayerState {
  Vec2 pos;
  float yaw, pitch;
  float health, maxHealth;
  float speed;
  float damage;
  float fireRate;
  float fireCooldown;
  float dashCooldown;
  float dashMaxCooldown;
  float dashTimer; // >0 = currently dashing
  float dashDuration;
  Vec2 dashDir;
  bool dashing;
  bool invincible; // i-frames
  float invTimer;
  float bobPhase;
  int pierceCount;
  bool hasSpread;
  bool hasLifesteal;
  bool hasThorns;
  float thornsDamage;
  float lifestealPct;
};

struct RunStats {
  int roomDepth;
  int enemiesKilled;
  int boonsCollected;
  int bestDepth; // Persistent across runs
  int totalRuns;
};

// ── Utility ──

inline float randf() { return (float)rand() / (float)RAND_MAX; }
inline float randf(float lo, float hi) { return lo + randf() * (hi - lo); }
inline int randi(int lo, int hi) { return lo + rand() % (hi - lo + 1); }
inline float clampf(float v, float lo, float hi) {
  return v < lo ? lo : (v > hi ? hi : v);
}
inline float lerpf(float a, float b, float t) { return a + (b - a) * t; }

inline const char *boonName(BoonType t) {
  switch (t) {
  case BOON_DAMAGE_UP:
    return "Fury (+25% DMG)";
  case BOON_SPEED_UP:
    return "Swiftness (+20% SPD)";
  case BOON_FIRE_RATE:
    return "Haste (+30% Fire Rate)";
  case BOON_MAX_HP:
    return "Vitality (+30 Max HP)";
  case BOON_HEAL:
    return "Restoration (Heal 40)";
  case BOON_DASH_CDR:
    return "Agility (-30% Dash CD)";
  case BOON_PIERCE:
    return "Penetration (Pierce +1)";
  case BOON_SPREAD:
    return "Scatter (Triple Shot)";
  case BOON_LIFESTEAL:
    return "Vampirism (Heal on Kill)";
  case BOON_THORNS:
    return "Thorns (Dmg on Contact)";
  default:
    return "???";
  }
}

inline void boonColor(BoonType t, float &r, float &g, float &b) {
  switch (t) {
  case BOON_DAMAGE_UP:
    r = 1.0f;
    g = 0.3f;
    b = 0.2f;
    break;
  case BOON_SPEED_UP:
    r = 0.3f;
    g = 0.9f;
    b = 1.0f;
    break;
  case BOON_FIRE_RATE:
    r = 1.0f;
    g = 0.8f;
    b = 0.2f;
    break;
  case BOON_MAX_HP:
    r = 0.3f;
    g = 1.0f;
    b = 0.4f;
    break;
  case BOON_HEAL:
    r = 0.4f;
    g = 1.0f;
    b = 0.6f;
    break;
  case BOON_DASH_CDR:
    r = 0.5f;
    g = 0.5f;
    b = 1.0f;
    break;
  case BOON_PIERCE:
    r = 1.0f;
    g = 0.5f;
    b = 0.0f;
    break;
  case BOON_SPREAD:
    r = 0.9f;
    g = 0.2f;
    b = 0.9f;
    break;
  case BOON_LIFESTEAL:
    r = 0.8f;
    g = 0.0f;
    b = 0.3f;
    break;
  case BOON_THORNS:
    r = 0.6f;
    g = 0.6f;
    b = 0.2f;
    break;
  default:
    r = 0.5f;
    g = 0.5f;
    b = 0.5f;
    break;
  }
}
