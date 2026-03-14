#pragma once
#include <GL/freeglut.h>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#ifndef GL_MULTISAMPLE
#define GL_MULTISAMPLE 0x809D
#endif

// ── Camera ──
extern float camDist, camDistTarget;
extern float camAngleX, camAngleY;
extern float camAngleXTarget, camAngleYTarget;
extern bool  autoRotate;
extern float gTime;

// ── Mouse ──
extern bool mouseDown;
extern int  lastMouseX, lastMouseY;

// ── Spoiler ──
extern int sagaMax;
extern const char* sagaNames[];

// ── Globe constants ──
const float GLOBE_R         = 10.0f;
const float RED_LINE_LON    = 0.0f;
const float RED_LINE_WIDTH  = 8.0f;
const float GRAND_LINE_W    = 7.0f;
const float CALM_BELT_W     = 10.0f;

// ── Island data ──
struct Island {
  const char* name;
  float lat, lon;
  int saga;
  bool special;
};
extern std::vector<Island> islands;
extern std::vector<int>    journeyOrder;

// ── Helpers ──
inline float lerp(float a, float b, float t) { return a + (b - a) * t; }
inline float smoothstep(float lo, float hi, float x) {
  float t = fmaxf(0.0f, fminf(1.0f, (x - lo) / (hi - lo)));
  return t * t * (3.0f - 2.0f * t);
}
inline void latLonToXYZ(float r, float lat, float lon, float* x, float* y, float* z) {
  float phi   = (90.0f - lat) * (float)M_PI / 180.0f;
  float theta = lon * (float)M_PI / 180.0f;
  *x = r * sinf(phi) * cosf(theta);
  *y = r * cosf(phi);
  *z = r * sinf(phi) * sinf(theta);
}

// Region classification
inline bool inRedLine(float lon) {
  float d = fabsf(lon - RED_LINE_LON);
  if (d > 180.0f) d = 360.0f - d;
  return d < RED_LINE_WIDTH;
}
inline bool inGrandLine(float lat) { return fabsf(lat) < GRAND_LINE_W; }
inline bool inCalmBeltN(float lat) { return lat > GRAND_LINE_W && lat < GRAND_LINE_W + CALM_BELT_W; }
inline bool inCalmBeltS(float lat) { return lat < -GRAND_LINE_W && lat > -GRAND_LINE_W - CALM_BELT_W; }
inline bool inParadise(float lon)  { return (lon > 0 && lon < 180) || lon < -90; }
