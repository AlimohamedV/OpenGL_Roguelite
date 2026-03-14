#include "journey.h"

void drawJourneyPath(float radius) {
  glDisable(GL_LIGHTING);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE);
  glEnable(GL_LINE_SMOOTH);

  // Outer glow
  glLineWidth(7.0f);
  glBegin(GL_LINE_STRIP);
  for (size_t i = 0; i < journeyOrder.size(); ++i) {
    int idx = journeyOrder[i];
    if (islands[idx].saga > sagaMax) break;
    float x, y, z;
    latLonToXYZ(radius + 0.22f, islands[idx].lat, islands[idx].lon, &x, &y, &z);
    float t = (float)i / (float)journeyOrder.size();
    glColor4f(1.0f, 0.60f + 0.2f * t, 0.12f, 0.18f);
    glVertex3f(x, y, z);
  }
  glEnd();

  // Core path
  glLineWidth(2.5f);
  glBegin(GL_LINE_STRIP);
  for (size_t i = 0; i < journeyOrder.size(); ++i) {
    int idx = journeyOrder[i];
    if (islands[idx].saga > sagaMax) break;
    float x, y, z;
    latLonToXYZ(radius + 0.22f, islands[idx].lat, islands[idx].lon, &x, &y, &z);
    float t = (float)i / (float)journeyOrder.size();
    float pulse = 0.85f + 0.15f * sinf(gTime * 3.0f - t * 15.0f);
    glColor4f(1.0f * pulse, (0.78f + 0.15f * t) * pulse, 0.25f * pulse, 0.9f);
    glVertex3f(x, y, z);
  }
  glEnd();

  glDisable(GL_LINE_SMOOTH);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glLineWidth(1.0f);
  glEnable(GL_LIGHTING);
}

void drawIslandMarker(float x, float y, float z, bool reached, bool special) {
  glPushMatrix();
  glTranslatef(x, y, z);
  if (reached || special) {
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    float gp = 0.7f + 0.3f * sinf(gTime * 2.5f + x * 3.0f);
    if (special) {
      glColor4f(0.2f, 0.9f, 1.0f, 0.15f * gp);
      glutSolidSphere(0.50f, 14, 14);
      glColor4f(0.2f, 0.9f, 1.0f, 0.3f * gp);
      glutSolidSphere(0.30f, 12, 12);
    } else {
      glColor4f(1.0f, 0.8f, 0.2f, 0.12f * gp);
      glutSolidSphere(0.40f, 12, 12);
      glColor4f(1.0f, 0.8f, 0.2f, 0.25f * gp);
      glutSolidSphere(0.22f, 10, 10);
    }
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
  }
  glDisable(GL_LIGHTING);
  if (special) {
    glColor3f(0.3f, 1.0f, 1.0f);
    glutSolidSphere(0.16f, 12, 12);
  } else if (reached) {
    glColor3f(1.0f, 0.85f, 0.3f);
    glutSolidSphere(0.12f, 10, 10);
  } else {
    glColor3f(0.35f, 0.35f, 0.4f);
    glutSolidSphere(0.08f, 8, 8);
  }
  glEnable(GL_LIGHTING);
  glPopMatrix();
}

void drawRegionLabels(int vp[4], float radius) {
  GLdouble model[16], proj[16]; GLint view[4];
  glGetDoublev(GL_MODELVIEW_MATRIX, model);
  glGetDoublev(GL_PROJECTION_MATRIX, proj);
  glGetIntegerv(GL_VIEWPORT, view);
  glDisable(GL_LIGHTING); glDisable(GL_DEPTH_TEST);
  glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity();
  gluOrtho2D(0, vp[2], 0, vp[3]);
  glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();

  struct RL { const char* t; float la, lo; float cr, cg, cb; };
  RL labels[] = {
    {"EAST BLUE",  38.0f,  50.0f,  0.31f, 0.63f, 0.86f},
    {"NORTH BLUE", 38.0f, -50.0f,  0.59f, 0.47f, 0.82f},
    {"WEST BLUE", -38.0f, -50.0f,  0.31f, 0.73f, 0.55f},
    {"SOUTH BLUE",-38.0f,  50.0f,  0.82f, 0.67f, 0.35f},
    {"PARADISE",    0.0f,  90.0f,  0.78f, 0.70f, 0.31f},
    {"NEW WORLD",   0.0f, -90.0f,  0.78f, 0.39f, 0.24f},
  };
  for (auto& lb : labels) {
    float x, y, z;
    latLonToXYZ(radius + 0.3f, lb.la, lb.lo, &x, &y, &z);
    double mx, my, mz;
    gluProject(x, y, z, model, proj, view, &mx, &my, &mz);
    if (mz >= 0 && mz < 0.997 && mx > 0 && mx < vp[2] && my > 0 && my < vp[3]) {
      glColor4f(lb.cr, lb.cg, lb.cb, 0.16f);
      int tw = (int)strlen(lb.t) * 8;
      glRasterPos2i((int)mx - tw/2, (int)my);
      for (const char* c = lb.t; *c; ++c)
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
    }
  }

  glPopMatrix(); glMatrixMode(GL_PROJECTION); glPopMatrix(); glMatrixMode(GL_MODELVIEW);
  glDisable(GL_BLEND); glEnable(GL_DEPTH_TEST); glEnable(GL_LIGHTING);
}

void drawIslandLabels(int vp[4], float radius) {
  GLdouble model[16], proj[16]; GLint view[4];
  glGetDoublev(GL_MODELVIEW_MATRIX, model);
  glGetDoublev(GL_PROJECTION_MATRIX, proj);
  glGetIntegerv(GL_VIEWPORT, view);
  glDisable(GL_LIGHTING); glDisable(GL_DEPTH_TEST);
  glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity();
  gluOrtho2D(0, vp[2], 0, vp[3]);
  glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();

  int drawn = 0;
  for (size_t i = 0; i < journeyOrder.size() && drawn < 20; ++i) {
    int idx = journeyOrder[i];
    if (islands[idx].saga > sagaMax) continue;
    float x, y, z;
    latLonToXYZ(radius + 0.3f, islands[idx].lat, islands[idx].lon, &x, &y, &z);
    double mx, my, mz;
    gluProject(x, y, z, model, proj, view, &mx, &my, &mz);
    if (mz >= 0 && mz < 0.997 && mx > -20 && mx < vp[2]+20 && my > -10 && my < vp[3]+10) {
      if (islands[idx].special) glColor4f(0.4f, 1.0f, 1.0f, 0.9f);
      else glColor4f(1.0f, 0.95f, 0.82f, 0.85f);
      glRasterPos2i((int)mx + 8, (int)my + 2);
      for (const char* c = islands[idx].name; *c && (c - islands[idx].name) < 26; ++c)
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_10, *c);
      drawn++;
    }
  }
  glPopMatrix(); glMatrixMode(GL_PROJECTION); glPopMatrix(); glMatrixMode(GL_MODELVIEW);
  glDisable(GL_BLEND); glEnable(GL_DEPTH_TEST); glEnable(GL_LIGHTING);
}
