#include "globe.h"
#include "noise.h"

/* ── 4-layer atmosphere (matching reference's addAtmoLayer approach) ── */
static void drawAtmoLayer(float r, float red, float grn, float blu, float opacity, int sl, int st) {
  glBegin(GL_QUADS);
  for (int i = 0; i < st; ++i) {
    float p0 = (float)M_PI * i / st;
    float p1 = (float)M_PI * (i + 1) / st;
    for (int j = 0; j < sl; ++j) {
      float t0 = 2.0f * (float)M_PI * j / sl;
      float t1 = 2.0f * (float)M_PI * (j + 1) / sl;
      auto v = [&](float p, float t) {
        float nx = sinf(p)*cosf(t), ny = cosf(p), nz = sinf(p)*sinf(t);
        glColor4f(red, grn, blu, opacity);
        glVertex3f(r*nx, r*ny, r*nz);
      };
      v(p0,t0); v(p0,t1); v(p1,t1); v(p1,t0);
    }
  }
  glEnd();
}

void drawAtmosphere(float radius, float eyeX, float eyeY, float eyeZ) {
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE);
  glDisable(GL_LIGHTING);
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_FOG);

  // 4 layers matching reference: thin haze, inner glow, outer bloom, space limb
  drawAtmoLayer(radius * 1.005f, 0.27f, 0.53f, 0.80f, 0.06f, 64, 32);
  drawAtmoLayer(radius * 1.012f, 0.20f, 0.40f, 0.67f, 0.08f, 64, 32);
  drawAtmoLayer(radius * 1.04f,  0.13f, 0.33f, 0.67f, 0.04f, 48, 24);
  drawAtmoLayer(radius * 1.08f,  0.07f, 0.20f, 0.67f, 0.018f, 48, 24);

  glEnable(GL_FOG);
  glEnable(GL_DEPTH_TEST);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_LIGHTING);
}

/* ── Ocean coloring with FBM noise (matching reference's paintOcean) ── */
static void colorForRegion(float lat, float lon, float phi, float theta) {
  // Noise for organic variation
  float nx = phi * 5.0f, ny = theta * 5.0f;
  float n = Noise::fbm(nx, ny, 3) * 0.5f + 0.5f;

  float absLat = fabsf(lat);
  float eqDist = absLat / 90.0f;

  // Zone strengths (smooth transitions like reference's smoothstep)
  float calmStr = smoothstep(10.0f, 13.0f, absLat) * (1.0f - smoothstep(17.0f, 20.0f, absLat));
  float blueStr = smoothstep(17.0f, 22.0f, absLat);
  float glStr   = 1.0f - smoothstep(9.0f, 13.0f, absLat);

  // Red Line — separate
  float redDist = fabsf(lon - RED_LINE_LON);
  if (redDist > 180.0f) redDist = 360.0f - redDist;
  if (redDist < RED_LINE_WIDTH) {
    float dc = 1.0f - redDist / RED_LINE_WIDTH;
    float shade = 0.6f + n * 0.55f;
    float r, g, b;
    if (dc > 0.82f) { // Peak — bright crimson
      r = (0.85f + n * 0.1f) * shade;
      g = (0.12f + n * 0.04f) * shade;
      b = (0.06f + n * 0.02f) * shade;
    } else if (dc > 0.5f) { // Mid-slopes — vivid red
      r = (0.70f + n * 0.1f) * shade;
      g = (0.15f + n * 0.04f) * shade;
      b = (0.08f + n * 0.03f) * shade;
    } else if (dc > 0.14f) { // Lower — dark red
      r = (0.52f + n * 0.08f) * shade;
      g = (0.14f + n * 0.04f) * shade;
      b = (0.08f + n * 0.03f) * shade;
    } else { // Base edge — reddish brown, fade
      float fade = dc / 0.14f;
      r = lerp(0.10f, 0.38f + n * 0.06f, fade) * shade;
      g = lerp(0.25f, 0.12f + n * 0.03f, fade) * shade;
      b = lerp(0.50f, 0.07f + n * 0.02f, fade) * shade;
    }
    // Snow caps at high latitudes on peaks
    if (dc > 0.55f && absLat > 45.0f) {
      float s = fminf(1.0f, (absLat - 45.0f) / 25.0f) * dc;
      r = lerp(r, 0.96f, s);
      g = lerp(g, 0.93f, s);
      b = lerp(b, 0.91f, s);
    }
    glColor3f(r, g, b);
    return;
  }

  // Base ocean color: deep navy → rich blue → teal near equator (matching reference)
  float r = (4.0f + eqDist * 12.0f + n * 8.0f) / 255.0f;
  float g = (28.0f + (1.0f - eqDist) * 28.0f + n * 14.0f) / 255.0f;
  float b = (58.0f + (1.0f - eqDist) * 30.0f + n * 18.0f) / 255.0f;

  // Calm Belt tinting (matching reference)
  if (calmStr > 0) {
    float s = calmStr * (0.18f + n * 0.07f);
    float cR = (28.0f + n * 12.0f) / 255.0f;
    float cG = (68.0f + n * 14.0f) / 255.0f;
    float cB = (72.0f + n * 10.0f) / 255.0f;
    r = r * (1 - s) + cR * s;
    g = g * (1 - s) + cG * s;
    b = b * (1 - s) + cB * s;
  }

  // Four Blues tinting (matching reference zone colors)
  if (blueStr > 0) {
    float tR, tG, tB;
    bool east = (lon >= 0 && lon <= 180) || (lon < -180);
    bool north = lat > 0;
    if (north && east) { // East Blue — cerulean
      tR = (8+n*6)/255.f; tG = (42+n*14)/255.f; tB = (108+n*18)/255.f;
    } else if (north) { // North Blue — indigo-purple
      tR = (24+n*10)/255.f; tG = (20+n*10)/255.f; tB = (88+n*18)/255.f;
    } else if (!east) { // West Blue — teal-green
      tR = (6+n*5)/255.f; tG = (55+n*16)/255.f; tB = (65+n*14)/255.f;
    } else { // South Blue — amber-turquoise
      tR = (38+n*14)/255.f; tG = (48+n*12)/255.f; tB = (52+n*10)/255.f;
    }
    float s = blueStr * (0.24f + n * 0.06f);
    r = r*(1-s) + tR*s;
    g = g*(1-s) + tG*s;
    b = b*(1-s) + tB*s;
  }

  // Grand Line tinting
  if (glStr > 0) {
    float tR, tG, tB, s;
    if (lon < -5) { // New World — storm-reddish purple
      tR = (38+n*10)/255.f; tG = (10+n*6)/255.f; tB = (36+n*10)/255.f;
      s = glStr * (0.28f + n * 0.08f);
    } else if (lon > 5) { // Paradise — warm indigo
      tR = (18+n*8)/255.f; tG = (28+n*10)/255.f; tB = (58+n*10)/255.f;
      s = glStr * (0.22f + n * 0.06f);
    } else { s = 0; tR = tG = tB = 0; }
    if (s > 0) {
      r = r*(1-s) + tR*s;
      g = g*(1-s) + tG*s;
      b = b*(1-s) + tB*s;
    }
  }

  glColor3f(r, g, b);
}

void drawWorldSphere(float radius, int slices, int stacks) {
  glBegin(GL_QUADS);
  for (int i = 0; i < stacks; ++i) {
    float phi0 = (float)M_PI * i / stacks;
    float phi1 = (float)M_PI * (i + 1) / stacks;
    for (int j = 0; j < slices; ++j) {
      float theta0 = 2.0f * (float)M_PI * j / slices;
      float theta1 = 2.0f * (float)M_PI * (j + 1) / slices;
      float lat0 = 90.0f - phi0 * 180.0f / (float)M_PI;
      float lat1 = 90.0f - phi1 * 180.0f / (float)M_PI;
      float lon0 = theta0 * 180.0f / (float)M_PI;
      float lon1 = theta1 * 180.0f / (float)M_PI;
      if (lon0 > 180.0f) lon0 -= 360.0f;
      if (lon1 > 180.0f) lon1 -= 360.0f;

      auto vert = [radius](float ph, float th, float la, float lo) {
        colorForRegion(la, lo, ph, th);
        float nx = sinf(ph)*cosf(th), ny = cosf(ph), nz = sinf(ph)*sinf(th);
        glNormal3f(nx, ny, nz);
        // Red Line raised mountain (FBM-based jagged profile like reference)
        float extra = 0.0f;
        float d = fabsf(lo - RED_LINE_LON);
        if (d > 180.0f) d = 360.0f - d;
        if (d < RED_LINE_WIDTH) {
          float dc = 1.0f - d / RED_LINE_WIDTH;
          float profile = powf(fmaxf(0.0f, 1.0f - dc*dc), 0.45f);
          // Use noise for jagged peaks (matching reference)
          float n1 = Noise::fbm(ph*8.0f + 50, th*6.0f, 4) * 0.5f + 0.5f;
          float ridge = fabsf(Noise::fbm(ph*2.5f, th*2.5f + 300, 3));
          float peaks = powf(Noise::fbm(ph*20.0f+100, th*20.0f, 3)*0.5f+0.5f, 0.7f);
          float spikes = powf(fabsf(Noise::fbm(ph*40.0f, th*40.0f, 2)), 0.6f) * 0.2f;
          extra = 0.5f * profile * (n1*0.3f + ridge*0.25f + peaks*0.3f + spikes);
          // Gap at equator (Reverse Mountain / Fish-Man Island)
          if (fabsf(la) < 8.0f) {
            float gap = 1.0f - fabsf(la) / 8.0f;
            extra *= (1.0f - gap * 0.7f);
          }
        }
        float r = radius + extra;
        glVertex3f(r*nx, r*ny, r*nz);
      };

      vert(phi0, theta0, lat0, lon0);
      vert(phi0, theta1, lat0, lon1);
      vert(phi1, theta1, lat1, lon1);
      vert(phi1, theta0, lat1, lon0);
    }
  }
  glEnd();
}

void drawGridLines(float radius) {
  glDisable(GL_LIGHTING);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  float gridR = radius + 0.05f;

  // Lat lines
  glLineWidth(0.8f);
  glColor4f(0.4f, 0.6f, 0.9f, 0.06f);
  for (float lat = -75.0f; lat <= 75.0f; lat += 15.0f) {
    glBegin(GL_LINE_LOOP);
    for (int j = 0; j < 120; ++j) {
      float lon = -180.0f + 360.0f * j / 120.0f;
      float x, y, z;
      latLonToXYZ(gridR, lat, lon, &x, &y, &z);
      glVertex3f(x, y, z);
    }
    glEnd();
  }
  // Lon lines
  for (float lon = -180.0f; lon < 180.0f; lon += 30.0f) {
    glBegin(GL_LINE_STRIP);
    for (int j = 0; j <= 60; ++j) {
      float lat = -90.0f + 180.0f * j / 60.0f;
      float x, y, z;
      latLonToXYZ(gridR, lat, lon, &x, &y, &z);
      glVertex3f(x, y, z);
    }
    glEnd();
  }
  // Grand Line dashed & Calm Belt borders
  glColor4f(0.78f, 0.70f, 0.31f, 0.10f);
  glLineWidth(1.2f);
  glEnable(GL_LINE_STIPPLE);
  glLineStipple(3, 0xAAAA);
  for (float lat : {(float)GRAND_LINE_W, (float)-GRAND_LINE_W}) {
    glBegin(GL_LINE_LOOP);
    for (int j = 0; j < 180; ++j) {
      float lon = -180.0f + 360.0f * j / 180.0f;
      float x, y, z;
      latLonToXYZ(gridR + 0.02f, lat, lon, &x, &y, &z);
      glVertex3f(x, y, z);
    }
    glEnd();
  }
  glDisable(GL_LINE_STIPPLE);
  glLineWidth(1.0f);
  glDisable(GL_BLEND);
  glEnable(GL_LIGHTING);
}

/* ── Thin cloud layer sphere with noise-based alpha ── */
void drawCloudLayer(float radius) {
  float cloudR = radius * 1.008f;
  int sl = 60, st = 30;
  glDisable(GL_LIGHTING);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glDepthMask(GL_FALSE);

  glBegin(GL_QUADS);
  for (int i = 0; i < st; ++i) {
    float p0 = (float)M_PI * i / st, p1 = (float)M_PI * (i+1) / st;
    for (int j = 0; j < sl; ++j) {
      float t0 = 2.0f*(float)M_PI*j/sl, t1 = 2.0f*(float)M_PI*(j+1)/sl;
      auto cv = [&](float p, float t) {
        float nx = sinf(p)*cosf(t), ny = cosf(p), nz = sinf(p)*sinf(t);
        // Cloud noise — shift with time for slow drift
        float cn = Noise::fbm(p*3.0f + gTime*0.003f + 50, t*3.0f + 50, 4);
        float cloud = fmaxf(0.0f, cn * 2.5f);
        float alpha = fminf(0.18f, cloud * 0.12f);
        glColor4f(1.0f, 1.0f, 1.0f, alpha);
        glNormal3f(nx, ny, nz);
        glVertex3f(cloudR*nx, cloudR*ny, cloudR*nz);
      };
      cv(p0,t0); cv(p0,t1); cv(p1,t1); cv(p1,t0);
    }
  }
  glEnd();
  glDepthMask(GL_TRUE);
  glDisable(GL_BLEND);
  glEnable(GL_LIGHTING);
}
