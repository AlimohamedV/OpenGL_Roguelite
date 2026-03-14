#include "stars.h"

#define NUM_STARS 2000
static float starPos[NUM_STARS][3];
static float starBright[NUM_STARS];
static float starSz[NUM_STARS];
static float starPh[NUM_STARS];

void initStars() {
  for (int i = 0; i < NUM_STARS; ++i) {
    float th = ((float)rand() / RAND_MAX) * 2.0f * (float)M_PI;
    float ph = acosf(2.0f * ((float)rand() / RAND_MAX) - 1.0f);
    float r  = 60.0f + ((float)rand() / RAND_MAX) * 60.0f;
    starPos[i][0] = r * sinf(ph) * cosf(th);
    starPos[i][1] = r * sinf(ph) * sinf(th);
    starPos[i][2] = r * cosf(ph);
    starBright[i] = 0.5f + 0.5f * ((float)rand() / RAND_MAX);
    starSz[i]     = 1.0f + 1.8f * ((float)rand() / RAND_MAX);
    starPh[i]     = ((float)rand() / RAND_MAX) * 6.28f;
  }
}

void drawStars() {
  glDisable(GL_LIGHTING);
  glDisable(GL_FOG);
  glEnable(GL_POINT_SMOOTH);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE);
  for (int i = 0; i < NUM_STARS; ++i) {
    float tw = 0.5f + 0.5f * sinf(gTime * (1.2f + (i % 9) * 0.25f) + starPh[i]);
    float b  = starBright[i] * tw;
    // Blue-white-yellow variation like reference
    float t = starPh[i] / 6.28f;
    float cr = (0.8f + t * 0.2f) * b;
    float cg = (0.85f + t * 0.15f) * b;
    float cb = b;
    glPointSize(starSz[i] * (0.7f + 0.3f * tw));
    glBegin(GL_POINTS);
    glColor4f(cr, cg, cb, 1.0f);
    glVertex3fv(starPos[i]);
    glEnd();
  }
  glDisable(GL_POINT_SMOOTH);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_FOG);
  glEnable(GL_LIGHTING);
}
