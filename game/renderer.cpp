// ════════════════════════════════════════════════════════════════
//  DUNGEON ROGUELITE — Renderer (implementation)
// ════════════════════════════════════════════════════════════════

#include "renderer.h"
#include "world.h"
#include <cmath>
#include <cstdio>

// ── Global definitions ──
float screenShake = 0.0f;
float flashR = 0, flashG = 0, flashB = 0, flashA = 0;
float fadeAlpha = 1.0f;

// ── Internal state ──
static GLuint texWall = 0, texFloor = 0, texCeil = 0, texDoor = 0;
static Particle particles[MAX_PARTICLES];
static int particleCount = 0;

static unsigned int texRng = 12345;
static unsigned int texRand() {
  texRng = texRng * 1103515245 + 12345;
  return (texRng >> 16) & 0x7FFF;
}

// ════════════════════════════
//  TEXTURE GENERATION
// ════════════════════════════

static GLuint uploadTex(int w, int h, unsigned char *data) {
  GLuint id;
  glGenTextures(1, &id);
  glBindTexture(GL_TEXTURE_2D, id);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE,
               data);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                  GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGB, w, h, GL_RGB, GL_UNSIGNED_BYTE,
                    data);
  return id;
}

void initTextures() {
  const int W = 128, H = 128;
  unsigned char data[W * H * 3];

  // ── Brick wall ──
  for (int y = 0; y < H; y++)
    for (int x = 0; x < W; x++) {
      int i = (y * W + x) * 3;
      texRng = x * 31 + y * 57 + 777;
      int brickH = 16, brickW = 32;
      int row = y / brickH;
      int ox = (row % 2) * (brickW / 2);
      int bx = (x + ox) % brickW, by = y % brickH;
      bool mortar = (bx < 2 || by < 2);
      if (mortar) {
        int v = 45 + (int)(texRand() % 15);
        data[i] = v;
        data[i + 1] = v - 3;
        data[i + 2] = v - 5;
      } else {
        int n = (int)(texRand() % 20) - 10;
        texRng = ((x + ox) / brickW) * 137 + row * 251 + 42;
        int brickVar = (int)(texRand() % 30) - 15;
        data[i] = (unsigned char)clampf(105 + n + brickVar, 50, 160);
        data[i + 1] = (unsigned char)clampf(52 + n / 2 + brickVar / 2, 30, 90);
        data[i + 2] = (unsigned char)clampf(38 + n / 3 + brickVar / 3, 20, 70);
      }
    }
  texWall = uploadTex(W, H, data);

  // ── Stone floor ──
  for (int y = 0; y < H; y++)
    for (int x = 0; x < W; x++) {
      int i = (y * W + x) * 3;
      texRng = x * 17 + y * 43 + 999;
      int tile = 32;
      bool grout = (x % tile < 2) || (y % tile < 2);
      if (grout) {
        data[i] = 28;
        data[i + 1] = 25;
        data[i + 2] = 22;
      } else {
        int n = (int)(texRand() % 20) - 10;
        texRng = (x / tile) * 111 + (y / tile) * 197 + 77;
        int tileVar = (int)(texRand() % 15) - 7;
        data[i] = (unsigned char)clampf(60 + n + tileVar, 30, 90);
        data[i + 1] = (unsigned char)clampf(56 + n + tileVar, 28, 85);
        data[i + 2] = (unsigned char)clampf(50 + n + tileVar, 25, 80);
      }
    }
  texFloor = uploadTex(W, H, data);

  // ── Dark ceiling ──
  for (int y = 0; y < H; y++)
    for (int x = 0; x < W; x++) {
      int i = (y * W + x) * 3;
      texRng = x * 13 + y * 37 + 555;
      int n = (int)(texRand() % 12) - 6;
      data[i] = (unsigned char)clampf(35 + n, 15, 55);
      data[i + 1] = (unsigned char)clampf(32 + n, 12, 50);
      data[i + 2] = (unsigned char)clampf(30 + n, 10, 48);
    }
  texCeil = uploadTex(W, H, data);

  // ── Door texture ──
  for (int y = 0; y < H; y++)
    for (int x = 0; x < W; x++) {
      int i = (y * W + x) * 3;
      texRng = x * 7 + y * 53 + 321;
      int plankW = 22;
      bool gap = (x % plankW < 2);
      int n = (int)(texRand() % 15) - 7;
      int grain = (int)(texRand() % 8) - 4;
      if (gap) {
        data[i] = 25;
        data[i + 1] = 18;
        data[i + 2] = 10;
      } else {
        data[i] = (unsigned char)clampf(90 + n + grain, 50, 130);
        data[i + 1] = (unsigned char)clampf(60 + n / 2 + grain, 30, 85);
        data[i + 2] = (unsigned char)clampf(30 + n / 3, 15, 50);
      }
    }
  texDoor = uploadTex(W, H, data);
}

// ════════════════════════════
//  PARTICLES
// ════════════════════════════

void spawnParticle(Vec2 pos, float y, float vx, float vy, float vz, float r,
                   float g, float b, float life, float size) {
  for (int i = 0; i < MAX_PARTICLES; i++) {
    Particle &p = particles[i];
    if (p.alive)
      continue;
    p.alive = true;
    p.pos = pos;
    p.y = y;
    p.vx = vx;
    p.vy = vy;
    p.vz = vz;
    p.r = r;
    p.g = g;
    p.b = b;
    p.a = 1.0f;
    p.life = life;
    p.maxLife = life;
    p.size = size;
    if (particleCount < MAX_PARTICLES)
      particleCount++;
    return;
  }
}

void spawnBurst(Vec2 pos, float y, int count, float r, float g, float b) {
  for (int i = 0; i < count; i++) {
    float angle = randf(0, 2.0f * (float)M_PI);
    float speed = randf(1.0f, 4.0f);
    float vy = randf(1.0f, 5.0f);
    spawnParticle(pos, y, cosf(angle) * speed, vy, sinf(angle) * speed,
                  r + randf(-0.1f, 0.1f), g + randf(-0.1f, 0.1f),
                  b + randf(-0.1f, 0.1f), randf(0.5f, 1.2f),
                  randf(0.03f, 0.08f));
  }
}

void updateParticles(float dt) {
  for (int i = 0; i < MAX_PARTICLES; i++) {
    Particle &p = particles[i];
    if (!p.alive)
      continue;
    p.pos.x += p.vx * dt;
    p.pos.z += p.vz * dt;
    p.y += p.vy * dt;
    p.vy -= 9.8f * dt;
    p.life -= dt;
    p.a = p.life / p.maxLife;
    if (p.life <= 0 || p.y < -0.5f)
      p.alive = false;
  }
}

void clearParticles() {
  memset(particles, 0, sizeof(particles));
  particleCount = 0;
}

void drawParticles(float camYaw) {
  glDisable(GL_LIGHTING);
  glDisable(GL_TEXTURE_2D);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE);

  for (int i = 0; i < MAX_PARTICLES; i++) {
    Particle &p = particles[i];
    if (!p.alive)
      continue;
    glPushMatrix();
    glTranslatef(p.pos.x, p.y, p.pos.z);
    float mv[16];
    glGetFloatv(GL_MODELVIEW_MATRIX, mv);
    mv[0] = 1;
    mv[1] = 0;
    mv[2] = 0;
    mv[4] = 0;
    mv[5] = 1;
    mv[6] = 0;
    mv[8] = 0;
    mv[9] = 0;
    mv[10] = 1;
    glLoadMatrixf(mv);
    glColor4f(p.r, p.g, p.b, p.a);
    float s = p.size * (1.0f + (1.0f - p.a) * 0.5f);
    glBegin(GL_QUADS);
    glVertex3f(-s, -s, 0);
    glVertex3f(s, -s, 0);
    glVertex3f(s, s, 0);
    glVertex3f(-s, s, 0);
    glEnd();
    glPopMatrix();
  }

  glDisable(GL_BLEND);
  glEnable(GL_LIGHTING);
}

// ════════════════════════════
//  3D WORLD RENDERING
// ════════════════════════════

void drawWalls() {
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, texWall);
  glColor3f(1, 1, 1);

  for (int z = 0; z < ROOM_H; z++)
    for (int x = 0; x < ROOM_W; x++) {
      if (!isWall(x, z))
        continue;
      float fx = (float)x, fz = (float)z;

      if (!isWall(x, z - 1)) {
        glNormal3f(0, 0, -1);
        glBegin(GL_QUADS);
        glTexCoord2f(0, 0);
        glVertex3f(fx, 0, fz);
        glTexCoord2f(1, 0);
        glVertex3f(fx + 1, 0, fz);
        glTexCoord2f(1, 1);
        glVertex3f(fx + 1, WALL_H, fz);
        glTexCoord2f(0, 1);
        glVertex3f(fx, WALL_H, fz);
        glEnd();
      }
      if (!isWall(x, z + 1)) {
        glNormal3f(0, 0, 1);
        glBegin(GL_QUADS);
        glTexCoord2f(0, 0);
        glVertex3f(fx + 1, 0, fz + 1);
        glTexCoord2f(1, 0);
        glVertex3f(fx, 0, fz + 1);
        glTexCoord2f(1, 1);
        glVertex3f(fx, WALL_H, fz + 1);
        glTexCoord2f(0, 1);
        glVertex3f(fx + 1, WALL_H, fz + 1);
        glEnd();
      }
      if (!isWall(x - 1, z)) {
        glNormal3f(-1, 0, 0);
        glBegin(GL_QUADS);
        glTexCoord2f(0, 0);
        glVertex3f(fx, 0, fz + 1);
        glTexCoord2f(1, 0);
        glVertex3f(fx, 0, fz);
        glTexCoord2f(1, 1);
        glVertex3f(fx, WALL_H, fz);
        glTexCoord2f(0, 1);
        glVertex3f(fx, WALL_H, fz + 1);
        glEnd();
      }
      if (!isWall(x + 1, z)) {
        glNormal3f(1, 0, 0);
        glBegin(GL_QUADS);
        glTexCoord2f(0, 0);
        glVertex3f(fx + 1, 0, fz);
        glTexCoord2f(1, 0);
        glVertex3f(fx + 1, 0, fz + 1);
        glTexCoord2f(1, 1);
        glVertex3f(fx + 1, WALL_H, fz + 1);
        glTexCoord2f(0, 1);
        glVertex3f(fx + 1, WALL_H, fz);
        glEnd();
      }
    }
  glDisable(GL_TEXTURE_2D);
}

void drawFloorCeiling() {
  glEnable(GL_TEXTURE_2D);
  glColor3f(1, 1, 1);

  for (int z = 0; z < ROOM_H; z++)
    for (int x = 0; x < ROOM_W; x++) {
      if (isWall(x, z))
        continue;
      float fx = (float)x, fz = (float)z;

      glBindTexture(GL_TEXTURE_2D, texFloor);
      glNormal3f(0, 1, 0);
      glBegin(GL_QUADS);
      glTexCoord2f(0, 0);
      glVertex3f(fx, 0, fz);
      glTexCoord2f(1, 0);
      glVertex3f(fx + 1, 0, fz);
      glTexCoord2f(1, 1);
      glVertex3f(fx + 1, 0, fz + 1);
      glTexCoord2f(0, 1);
      glVertex3f(fx, 0, fz + 1);
      glEnd();

      glBindTexture(GL_TEXTURE_2D, texCeil);
      glNormal3f(0, -1, 0);
      glBegin(GL_QUADS);
      glTexCoord2f(0, 0);
      glVertex3f(fx, WALL_H, fz + 1);
      glTexCoord2f(1, 0);
      glVertex3f(fx + 1, WALL_H, fz + 1);
      glTexCoord2f(1, 1);
      glVertex3f(fx + 1, WALL_H, fz);
      glTexCoord2f(0, 1);
      glVertex3f(fx, WALL_H, fz);
      glEnd();
    }
  glDisable(GL_TEXTURE_2D);
}

void drawDoors(Door doors[], int numDoors, float time) {
  for (int i = 0; i < numDoors; i++) {
    if (!doors[i].active)
      continue;
    Door &d = doors[i];

    float pulse = 0.7f + 0.3f * sinf(time * 3.0f + i * 2.0f);

    glPushMatrix();
    glTranslatef(d.pos.x, 0, d.pos.z);

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texDoor);
    glNormal3f(0, 0, 1);
    glColor3f(1, 1, 1);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0);
    glVertex3f(-0.4f, 0, 0);
    glTexCoord2f(1, 0);
    glVertex3f(0.4f, 0, 0);
    glTexCoord2f(1, 1);
    glVertex3f(0.4f, WALL_H * 0.85f, 0);
    glTexCoord2f(0, 1);
    glVertex3f(-0.4f, WALL_H * 0.85f, 0);
    glEnd();
    glDisable(GL_TEXTURE_2D);

    float br, bg, bb;
    boonColor(d.reward, br, bg, bb);
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glColor4f(br, bg, bb, 0.4f * pulse);
    float gs = 0.35f;
    glBegin(GL_QUADS);
    glVertex3f(-gs, WALL_H * 0.55f, 0.01f);
    glVertex3f(gs, WALL_H * 0.55f, 0.01f);
    glVertex3f(gs, WALL_H * 0.55f + gs * 2, 0.01f);
    glVertex3f(-gs, WALL_H * 0.55f + gs * 2, 0.01f);
    glEnd();
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);

    // Boon name label
    glDisable(GL_LIGHTING);
    glColor3f(br, bg, bb);
    const char *name = boonName(d.reward);
    int textLen = 0;
    for (const char *c = name; *c; c++)
      textLen++;
    float textOffset = textLen * 0.02f;
    glRasterPos3f(-textOffset, WALL_H * 0.92f, 0.02f);
    for (const char *c = name; *c; c++)
      glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);
    glEnable(GL_LIGHTING);

    glPopMatrix();
  }
}

void drawEnemies(Enemy enemies[], int numEnemies, float camYaw, float time) {
  glDisable(GL_TEXTURE_2D);

  for (int i = 0; i < numEnemies; i++) {
    Enemy &e = enemies[i];
    if (!e.alive && e.deathAnim <= 0)
      continue;

    float scale = e.alive ? 1.0f : clampf(e.deathAnim, 0, 1);
    float bob = e.alive ? sinf(time * 4.0f + i * 1.5f) * 0.03f : 0;
    float spin = time * 90.0f + i * 45.0f;
    float flashMix = e.hitAnim > 0 ? clampf(e.hitAnim * 5.0f, 0, 1) : 0;

    glPushMatrix();
    glTranslatef(e.pos.x, bob, e.pos.z);
    glScalef(scale, scale, scale);
    glRotatef(e.facingAngle * 180.0f / (float)M_PI, 0, 1, 0);
    glEnable(GL_LIGHTING);

    switch (e.type) {
    case ENEMY_CHASER: {
      float r = lerpf(0.9f, 1.0f, flashMix), g = lerpf(0.12f, 1.0f, flashMix),
            b = lerpf(0.08f, 1.0f, flashMix);
      float c[] = {r, g, b, 1};
      glMaterialfv(GL_FRONT, GL_DIFFUSE, c);
      glColor3f(r, g, b);
      glPushMatrix();
      glTranslatef(0, 0.25f, 0);
      glutSolidSphere(0.18, 12, 8);
      glPopMatrix();
      glPushMatrix();
      glTranslatef(-0.08f, 0.42f, 0);
      glRotatef(-20, 0, 0, 1);
      glRotatef(-90, 1, 0, 0);
      glutSolidCone(0.04, 0.15, 6, 2);
      glPopMatrix();
      glPushMatrix();
      glTranslatef(0.08f, 0.42f, 0);
      glRotatef(20, 0, 0, 1);
      glRotatef(-90, 1, 0, 0);
      glutSolidCone(0.04, 0.15, 6, 2);
      glPopMatrix();
      float ec[] = {1, 1, 0.4f, 1};
      glMaterialfv(GL_FRONT, GL_DIFFUSE, ec);
      glColor3f(1, 1, 0.4f);
      glPushMatrix();
      glTranslatef(-0.07f, 0.30f, 0.14f);
      glutSolidSphere(0.035, 6, 4);
      glPopMatrix();
      glPushMatrix();
      glTranslatef(0.07f, 0.30f, 0.14f);
      glutSolidSphere(0.035, 6, 4);
      glPopMatrix();
      break;
    }
    case ENEMY_SHOOTER: {
      float r = lerpf(0.6f, 1.0f, flashMix), g = lerpf(0.15f, 1.0f, flashMix),
            b = lerpf(0.95f, 1.0f, flashMix);
      float c[] = {r, g, b, 1};
      glMaterialfv(GL_FRONT, GL_DIFFUSE, c);
      glColor3f(r, g, b);
      glPushMatrix();
      glTranslatef(0, 0.4f, 0);
      glutSolidSphere(0.15, 12, 8);
      float rc[] = {r * 0.6f, g * 0.6f, b * 0.6f, 1};
      glMaterialfv(GL_FRONT, GL_DIFFUSE, rc);
      glColor3f(r * 0.6f, g * 0.6f, b * 0.6f);
      glRotatef(spin * 2.0f, 0, 1, 0);
      glutSolidTorus(0.02, 0.22, 6, 16);
      glPopMatrix();
      float ec[] = {1, 0.8f, 1, 1};
      glMaterialfv(GL_FRONT, GL_DIFFUSE, ec);
      glColor3f(1, 0.8f, 1);
      glPushMatrix();
      glTranslatef(0, 0.42f, 0.14f);
      glutSolidSphere(0.04, 6, 4);
      glPopMatrix();
      break;
    }
    case ENEMY_TANK: {
      float r = lerpf(0.95f, 1.0f, flashMix), g = lerpf(0.55f, 1.0f, flashMix),
            b = lerpf(0.08f, 1.0f, flashMix);
      float c[] = {r, g, b, 1};
      glMaterialfv(GL_FRONT, GL_DIFFUSE, c);
      glColor3f(r, g, b);
      glPushMatrix();
      glTranslatef(0, 0.25f, 0);
      glScalef(0.3f, 0.35f, 0.25f);
      glutSolidCube(1.0);
      glPopMatrix();
      float hc[] = {r * 0.8f, g * 0.7f, b * 0.5f, 1};
      glMaterialfv(GL_FRONT, GL_DIFFUSE, hc);
      glColor3f(r * 0.8f, g * 0.7f, b * 0.5f);
      glPushMatrix();
      glTranslatef(0, 0.52f, 0);
      glutSolidSphere(0.12, 10, 6);
      glPopMatrix();
      glMaterialfv(GL_FRONT, GL_DIFFUSE, c);
      glColor3f(r, g, b);
      glPushMatrix();
      glTranslatef(-0.22f, 0.22f, 0);
      glScalef(0.08f, 0.2f, 0.08f);
      glutSolidSphere(1.0, 8, 6);
      glPopMatrix();
      glPushMatrix();
      glTranslatef(0.22f, 0.22f, 0);
      glScalef(0.08f, 0.2f, 0.08f);
      glutSolidSphere(1.0, 8, 6);
      glPopMatrix();
      float ec[] = {1, 0.3f, 0.1f, 1};
      glMaterialfv(GL_FRONT, GL_DIFFUSE, ec);
      glColor3f(1, 0.3f, 0.1f);
      glPushMatrix();
      glTranslatef(-0.05f, 0.55f, 0.10f);
      glutSolidSphere(0.025, 6, 4);
      glPopMatrix();
      glPushMatrix();
      glTranslatef(0.05f, 0.55f, 0.10f);
      glutSolidSphere(0.025, 6, 4);
      glPopMatrix();
      break;
    }
    case ENEMY_BOSS: {
      float r = lerpf(0.75f, 1.0f, flashMix), g = lerpf(0.05f, 1.0f, flashMix),
            b = lerpf(0.05f, 1.0f, flashMix);
      float c[] = {r, g, b, 1};
      glMaterialfv(GL_FRONT, GL_DIFFUSE, c);
      glColor3f(r, g, b);
      glPushMatrix();
      glTranslatef(0, 0.35f, 0);
      glutSolidSphere(0.35, 16, 12);
      glPopMatrix();
      float hc[] = {r * 0.7f, g * 0.3f, b * 0.3f, 1};
      glMaterialfv(GL_FRONT, GL_DIFFUSE, hc);
      glColor3f(r * 0.7f, g * 0.3f, b * 0.3f);
      glPushMatrix();
      glTranslatef(0, 0.72f, 0);
      glutSolidSphere(0.18, 12, 8);
      glPopMatrix();
      glMaterialfv(GL_FRONT, GL_DIFFUSE, c);
      glColor3f(r, g, b);
      glPushMatrix();
      glTranslatef(-0.15f, 0.85f, 0);
      glRotatef(-30, 0, 0, 1);
      glRotatef(-90, 1, 0, 0);
      glutSolidCone(0.06, 0.25, 8, 3);
      glPopMatrix();
      glPushMatrix();
      glTranslatef(0.15f, 0.85f, 0);
      glRotatef(30, 0, 0, 1);
      glRotatef(-90, 1, 0, 0);
      glutSolidCone(0.06, 0.25, 8, 3);
      glPopMatrix();
      glPushMatrix();
      glTranslatef(-0.4f, 0.3f, 0);
      glScalef(0.12f, 0.25f, 0.12f);
      glutSolidSphere(1.0, 8, 6);
      glPopMatrix();
      glPushMatrix();
      glTranslatef(0.4f, 0.3f, 0);
      glScalef(0.12f, 0.25f, 0.12f);
      glutSolidSphere(1.0, 8, 6);
      glPopMatrix();
      glDisable(GL_LIGHTING);
      glEnable(GL_BLEND);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE);
      float pulse = 0.5f + 0.5f * sinf(time * 5.0f);
      glColor4f(1, 0.3f, 0.1f, 0.4f * pulse);
      glPushMatrix();
      glTranslatef(0, 0.35f, 0);
      glutSolidSphere(0.38, 12, 8);
      glPopMatrix();
      glDisable(GL_BLEND);
      glEnable(GL_LIGHTING);
      float ec[] = {1, 1, 0.2f, 1};
      glMaterialfv(GL_FRONT, GL_DIFFUSE, ec);
      glColor3f(1, 1, 0.2f);
      glPushMatrix();
      glTranslatef(-0.08f, 0.76f, 0.15f);
      glutSolidSphere(0.04, 6, 4);
      glPopMatrix();
      glPushMatrix();
      glTranslatef(0.08f, 0.76f, 0.15f);
      glutSolidSphere(0.04, 6, 4);
      glPopMatrix();
      break;
    }
    }

    // Health bar (billboarded)
    if (e.alive && e.health < e.maxHealth) {
      glDisable(GL_LIGHTING);
      glEnable(GL_BLEND);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      float barY = (e.type == ENEMY_BOSS) ? 1.15f : 0.7f;
      glPushMatrix();
      glTranslatef(0, barY, 0);
      float mv[16];
      glGetFloatv(GL_MODELVIEW_MATRIX, mv);
      mv[0] = 1;
      mv[1] = 0;
      mv[2] = 0;
      mv[4] = 0;
      mv[5] = 1;
      mv[6] = 0;
      mv[8] = 0;
      mv[9] = 0;
      mv[10] = 1;
      glLoadMatrixf(mv);
      float barW = (e.type == ENEMY_BOSS) ? 0.4f : 0.2f, barH = 0.03f;
      float hp = e.health / e.maxHealth;
      glColor4f(0, 0, 0, 0.7f);
      glBegin(GL_QUADS);
      glVertex3f(-barW, 0, 0);
      glVertex3f(barW, 0, 0);
      glVertex3f(barW, barH, 0);
      glVertex3f(-barW, barH, 0);
      glEnd();
      glColor4f(1, 0.1f, 0.1f, 0.9f);
      glBegin(GL_QUADS);
      glVertex3f(-barW, 0, 0);
      glVertex3f(-barW + barW * 2 * hp, 0, 0);
      glVertex3f(-barW + barW * 2 * hp, barH, 0);
      glVertex3f(-barW, barH, 0);
      glEnd();
      glPopMatrix();
      glDisable(GL_BLEND);
      glEnable(GL_LIGHTING);
    }

    float white[] = {1, 1, 1, 1};
    glMaterialfv(GL_FRONT, GL_DIFFUSE, white);
    glPopMatrix();
  }
}

void drawProjectiles(Projectile projs[], int count, float camYaw) {
  glDisable(GL_LIGHTING);
  glDisable(GL_TEXTURE_2D);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE);

  for (int i = 0; i < count; i++) {
    Projectile &p = projs[i];
    if (!p.alive)
      continue;

    glPushMatrix();
    glTranslatef(p.pos.x, 0.4f, p.pos.z);
    float mv[16];
    glGetFloatv(GL_MODELVIEW_MATRIX, mv);
    mv[0] = 1;
    mv[1] = 0;
    mv[2] = 0;
    mv[4] = 0;
    mv[5] = 1;
    mv[6] = 0;
    mv[8] = 0;
    mv[9] = 0;
    mv[10] = 1;
    glLoadMatrixf(mv);

    float sz = 0.06f;
    if (p.owner == PROJ_PLAYER) {
      glColor4f(1, 0.9f, 0.4f, 0.6f);
      glBegin(GL_QUADS);
      glVertex3f(-sz * 2, -sz * 2, 0);
      glVertex3f(sz * 2, -sz * 2, 0);
      glVertex3f(sz * 2, sz * 2, 0);
      glVertex3f(-sz * 2, sz * 2, 0);
      glEnd();
      glColor4f(1, 1, 0.8f, 1);
    } else {
      glColor4f(1, 0.2f, 0.1f, 0.6f);
      glBegin(GL_QUADS);
      glVertex3f(-sz * 2, -sz * 2, 0);
      glVertex3f(sz * 2, -sz * 2, 0);
      glVertex3f(sz * 2, sz * 2, 0);
      glVertex3f(-sz * 2, sz * 2, 0);
      glEnd();
      glColor4f(1, 0.4f, 0.3f, 1);
    }
    glBegin(GL_QUADS);
    glVertex3f(-sz, -sz, 0);
    glVertex3f(sz, -sz, 0);
    glVertex3f(sz, sz, 0);
    glVertex3f(-sz, sz, 0);
    glEnd();

    glPopMatrix();
  }

  glDisable(GL_BLEND);
  glEnable(GL_LIGHTING);
}

// ════════════════════════════
//  HUD
// ════════════════════════════

void drawText2D(float x, float y, const char *str, void *font) {
  glRasterPos2f(x, y);
  for (const char *c = str; *c; c++)
    glutBitmapCharacter(font, *c);
}

void drawHUD(int winW, int winH, PlayerState &player, RunStats &stats,
             GameState state, BoonType boonChoices[], int numBoonChoices,
             Door doors[], int numDoors, float time) {
  glDisable(GL_LIGHTING);
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_TEXTURE_2D);
  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  gluOrtho2D(0, winW, 0, winH);
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();

  float cx = winW / 2.0f, cy = winH / 2.0f;
  char buf[64];

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  // Dash cooldown ring
  float dashPct =
      1.0f - clampf(player.dashCooldown / player.dashMaxCooldown, 0, 1);
  int segments = 24;
  float ringR = 16.0f;
  if (dashPct < 1.0f)
    glColor4f(0.3f, 0.5f, 1.0f, 0.4f);
  else
    glColor4f(0.3f, 1.0f, 0.5f, 0.5f);
  glLineWidth(2);
  glBegin(GL_LINE_STRIP);
  int segsToShow = (int)(segments * dashPct);
  for (int i = 0; i <= segsToShow; i++) {
    float a = -(float)M_PI / 2.0f + 2.0f * (float)M_PI * i / segments;
    glVertex2f(cx + cosf(a) * ringR, cy + sinf(a) * ringR);
  }
  glEnd();

  // Crosshair
  float chAlpha = player.dashing ? 0.3f : 0.8f;
  glColor4f(1, 1, 1, chAlpha);
  glLineWidth(2);
  glBegin(GL_LINES);
  glVertex2f(cx - 12, cy);
  glVertex2f(cx - 5, cy);
  glVertex2f(cx + 5, cy);
  glVertex2f(cx + 12, cy);
  glVertex2f(cx, cy - 12);
  glVertex2f(cx, cy - 5);
  glVertex2f(cx, cy + 5);
  glVertex2f(cx, cy + 12);
  glEnd();

  // Health bar
  float barW = 220, barH = 18;
  float barX = 20, barY = 20;
  float hp = clampf(player.health / player.maxHealth, 0, 1);

  glColor4f(0.15f, 0.0f, 0.0f, 0.8f);
  glBegin(GL_QUADS);
  glVertex2f(barX - 2, barY - 2);
  glVertex2f(barX + barW + 2, barY - 2);
  glVertex2f(barX + barW + 2, barY + barH + 2);
  glVertex2f(barX - 2, barY + barH + 2);
  glEnd();

  float hr = hp > 0.5f ? 2.0f * (1.0f - hp) : 1.0f;
  float hg = hp > 0.5f ? 1.0f : 2.0f * hp;
  glColor3f(hr, hg, 0);
  glBegin(GL_QUADS);
  glVertex2f(barX, barY);
  glVertex2f(barX + barW * hp, barY);
  glVertex2f(barX + barW * hp, barY + barH);
  glVertex2f(barX, barY + barH);
  glEnd();

  glColor3f(0.6f, 0.6f, 0.6f);
  glLineWidth(1);
  glBegin(GL_LINE_LOOP);
  glVertex2f(barX, barY);
  glVertex2f(barX + barW, barY);
  glVertex2f(barX + barW, barY + barH);
  glVertex2f(barX, barY + barH);
  glEnd();

  snprintf(buf, sizeof(buf), "%.0f / %.0f", player.health, player.maxHealth);
  glColor3f(1, 1, 1);
  drawText2D(barX + 5, barY + 2, buf, GLUT_BITMAP_HELVETICA_12);

  // Room depth
  snprintf(buf, sizeof(buf), "Room %d", stats.roomDepth + 1);
  glColor3f(0.9f, 0.85f, 0.6f);
  drawText2D(winW / 2.0f - 25, winH - 30.0f, buf);

  if ((stats.roomDepth + 1) % 5 == 0) {
    glColor3f(1.0f, 0.3f, 0.2f);
    drawText2D(winW / 2.0f - 20, winH - 50.0f, "BOSS",
               GLUT_BITMAP_HELVETICA_12);
  }

  // Kills
  snprintf(buf, sizeof(buf), "Kills: %d", stats.enemiesKilled);
  glColor3f(0.7f, 0.7f, 0.7f);
  drawText2D(20, barY + barH + 12, buf, GLUT_BITMAP_HELVETICA_12);

  // Boon selection
  if (state == STATE_BOON_SELECT && numBoonChoices > 0) {
    glColor4f(0, 0, 0, 0.6f);
    glBegin(GL_QUADS);
    glVertex2f(0, 0);
    glVertex2f((float)winW, 0);
    glVertex2f((float)winW, (float)winH);
    glVertex2f(0, (float)winH);
    glEnd();

    glColor3f(0.9f, 0.85f, 0.6f);
    drawText2D(cx - 80, cy + 130, "CHOOSE A BOON");

    float cardW = 180, cardH = 80;
    float totalW = numBoonChoices * cardW + (numBoonChoices - 1) * 20;
    float startX = cx - totalW / 2;

    for (int i = 0; i < numBoonChoices; i++) {
      float tx = startX + i * (cardW + 20);
      float ty = cy - cardH / 2;

      float br, bg, bb;
      boonColor(boonChoices[i], br, bg, bb);

      float pulse = 0.7f + 0.15f * sinf(time * 3.0f + i * 2.0f);
      glColor4f(br * 0.2f, bg * 0.2f, bb * 0.2f, 0.85f);
      glBegin(GL_QUADS);
      glVertex2f(tx, ty);
      glVertex2f(tx + cardW, ty);
      glVertex2f(tx + cardW, ty + cardH);
      glVertex2f(tx, ty + cardH);
      glEnd();

      glColor4f(br * pulse, bg * pulse, bb * pulse, 1);
      glLineWidth(2);
      glBegin(GL_LINE_LOOP);
      glVertex2f(tx, ty);
      glVertex2f(tx + cardW, ty);
      glVertex2f(tx + cardW, ty + cardH);
      glVertex2f(tx, ty + cardH);
      glEnd();

      snprintf(buf, sizeof(buf), "[%d]", i + 1);
      glColor4f(1, 1, 1, 0.8f);
      drawText2D(tx + cardW / 2 - 8, ty + cardH - 25, buf);

      glColor3f(br, bg, bb);
      drawText2D(tx + 10, ty + 15, boonName(boonChoices[i]),
                 GLUT_BITMAP_HELVETICA_12);
    }
  }

  // Room clear
  if (state == STATE_ROOM_CLEAR) {
    glColor3f(0.2f, 1.0f, 0.4f);
    drawText2D(cx - 70, cy + 40, "ROOM CLEARED!");
    glColor3f(0.7f, 0.7f, 0.7f);
    drawText2D(cx - 100, cy + 10, "Walk to a door to continue",
               GLUT_BITMAP_HELVETICA_12);
  }

  // Dead
  if (state == STATE_DEAD) {
    glColor4f(0.2f, 0, 0, 0.75f);
    glBegin(GL_QUADS);
    glVertex2f(0, 0);
    glVertex2f((float)winW, 0);
    glVertex2f((float)winW, (float)winH);
    glVertex2f(0, (float)winH);
    glEnd();
    glColor3f(1, 0.2f, 0.15f);
    drawText2D(cx - 60, cy + 40, "YOU DIED");
    snprintf(buf, sizeof(buf), "Reached Room %d  |  %d kills",
             stats.roomDepth + 1, stats.enemiesKilled);
    glColor3f(0.8f, 0.8f, 0.8f);
    drawText2D(cx - 100, cy, buf, GLUT_BITMAP_HELVETICA_12);
    snprintf(buf, sizeof(buf), "Best: Room %d  |  Total Runs: %d",
             stats.bestDepth + 1, stats.totalRuns);
    glColor3f(0.6f, 0.6f, 0.6f);
    drawText2D(cx - 100, cy - 25, buf, GLUT_BITMAP_HELVETICA_12);
    glColor3f(0.9f, 0.9f, 0.9f);
    drawText2D(cx - 80, cy - 60, "Press R to restart",
               GLUT_BITMAP_HELVETICA_12);
  }

  // Win
  if (state == STATE_WIN) {
    glColor4f(0, 0.1f, 0, 0.75f);
    glBegin(GL_QUADS);
    glVertex2f(0, 0);
    glVertex2f((float)winW, 0);
    glVertex2f((float)winW, (float)winH);
    glVertex2f(0, (float)winH);
    glEnd();
    glColor3f(0.2f, 1, 0.4f);
    drawText2D(cx - 80, cy + 40, "BOSS DEFEATED!");
    snprintf(buf, sizeof(buf), "Cleared %d rooms  |  %d kills",
             stats.roomDepth + 1, stats.enemiesKilled);
    glColor3f(0.8f, 0.8f, 0.8f);
    drawText2D(cx - 100, cy, buf, GLUT_BITMAP_HELVETICA_12);
    glColor3f(0.9f, 0.9f, 0.9f);
    drawText2D(cx - 100, cy - 40, "Press R for new run",
               GLUT_BITMAP_HELVETICA_12);
  }

  // Controls hint
  if (state == STATE_PLAYING) {
    glColor4f(0.5f, 0.5f, 0.5f, 0.4f);
    drawText2D(10, winH - 20.0f,
               "WASD: Move  Mouse: Look  Click: Shoot  Shift: Dash  ESC: Quit",
               GLUT_BITMAP_HELVETICA_12);
  }

  // Screen flash
  if (flashA > 0.01f) {
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(flashR, flashG, flashB, flashA);
    glBegin(GL_QUADS);
    glVertex2f(0, 0);
    glVertex2f((float)winW, 0);
    glVertex2f((float)winW, (float)winH);
    glVertex2f(0, (float)winH);
    glEnd();
  }

  // Fade
  if (fadeAlpha > 0.01f) {
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0, 0, 0, fadeAlpha);
    glBegin(GL_QUADS);
    glVertex2f(0, 0);
    glVertex2f((float)winW, 0);
    glVertex2f((float)winW, (float)winH);
    glVertex2f(0, (float)winH);
    glEnd();
  }

  glDisable(GL_BLEND);
  glPopMatrix();
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_LIGHTING);
}
