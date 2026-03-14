#include "hud.h"

static void drawPanel(float x, float y, float w, float h, float alpha) {
  glColor4f(0.05f, 0.07f, 0.14f, alpha);
  glBegin(GL_QUADS);
  glVertex2f(x, y); glVertex2f(x+w, y); glVertex2f(x+w, y+h); glVertex2f(x, y+h);
  glEnd();
  glColor4f(0.25f, 0.40f, 0.65f, alpha * 0.7f);
  glBegin(GL_LINE_LOOP);
  glVertex2f(x, y); glVertex2f(x+w, y); glVertex2f(x+w, y+h); glVertex2f(x, y+h);
  glEnd();
}

static void bstr(float x, float y, const char* s, void* f) {
  glRasterPos2f(x, y);
  for (const char* c = s; *c; ++c) glutBitmapCharacter(f, *c);
}

void drawHUD(int vp[4]) {
  glDisable(GL_LIGHTING); glDisable(GL_DEPTH_TEST);
  glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity();
  gluOrtho2D(0, vp[2], 0, vp[3]);
  glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();

  float W = (float)vp[2], H = (float)vp[3];

  // Title panel
  drawPanel(6, H-52, 580, 46, 0.65f);
  glColor4f(1.0f, 0.85f, 0.35f, 1.0f);
  bstr(16, H-22, "ONE PIECE WORLD", GLUT_BITMAP_HELVETICA_18);
  glColor4f(0.7f, 0.82f, 1.0f, 0.85f);
  bstr(200, H-22, "Straw Hat Journey Map", GLUT_BITMAP_HELVETICA_12);
  glColor4f(0.5f, 0.55f, 0.65f, 0.7f);
  bstr(16, H-42, "Drag: orbit | Scroll: zoom | WASD/QE | R: rotate | 1-0: Saga | ESC: quit", GLUT_BITMAP_HELVETICA_10);

  // Spoiler panel
  drawPanel(6, H-88, 280, 30, 0.55f);
  glColor4f(0.6f, 0.75f, 1.0f, 0.9f);
  bstr(16, H-78, "Spoiler Shield:", GLUT_BITMAP_HELVETICA_12);
  glColor4f(1.0f, 0.9f, 0.35f, 1.0f);
  bstr(130, H-78, sagaNames[sagaMax], GLUT_BITMAP_HELVETICA_12);

  // Legend
  drawPanel(6, 6, 220, 115, 0.55f);
  struct LI { float r,g,b; const char* l; };
  LI legend[] = {
    {0.78f,0.22f,0.12f,"Red Line"},
    {0.15f,0.50f,0.88f,"Grand Line (Paradise)"},
    {0.12f,0.42f,0.82f,"Grand Line (New World)"},
    {0.24f,0.20f,0.38f,"Calm Belts"},
    {0.12f,0.35f,0.72f,"Four Blues"},
  };
  float ly = 100.0f;
  for (auto& item : legend) {
    glColor4f(item.r, item.g, item.b, 0.95f);
    glBegin(GL_QUADS);
    glVertex2f(16,ly); glVertex2f(28,ly); glVertex2f(28,ly+10); glVertex2f(16,ly+10);
    glEnd();
    glColor4f(0.85f, 0.88f, 0.95f, 0.85f);
    bstr(34, ly+2, item.l, GLUT_BITMAP_HELVETICA_10);
    ly -= 18.0f;
  }

  // Journey timeline
  int vis = 0;
  for (size_t i = 0; i < journeyOrder.size(); ++i)
    if (islands[journeyOrder[i]].saga <= sagaMax) vis++;
  if (vis > 0) {
    float tlW = 320, tlX = W - tlW - 12, tlY = 8;
    drawPanel(tlX-6, tlY-4, tlW+12, 46, 0.55f);
    glColor4f(1.0f, 0.75f, 0.25f, 0.9f);
    bstr(tlX, tlY+30, "Journey Timeline", GLUT_BITMAP_HELVETICA_12);
    char buf[32]; snprintf(buf, 32, "%d islands", vis);
    glColor4f(0.6f, 0.7f, 0.85f, 0.7f);
    bstr(tlX+tlW-80, tlY+30, buf, GLUT_BITMAP_HELVETICA_10);
    float sp = vis > 1 ? tlW/(float)(vis-1) : 0;
    int d = 0;
    for (size_t i = 0; i < journeyOrder.size(); ++i) {
      if (islands[journeyOrder[i]].saga > sagaMax) continue;
      float dx = tlX + d*sp, dy = tlY + 10;
      float p = 0.6f + 0.4f * sinf(gTime*2.0f - d*0.3f);
      glColor4f(1,0.7f,0.2f, 0.2f*p);
      glBegin(GL_TRIANGLE_FAN); glVertex2f(dx,dy);
      for (int a=0;a<=12;++a){ float an=2*(float)M_PI*a/12; glVertex2f(dx+5*cosf(an),dy+5*sinf(an)); }
      glEnd();
      glColor4f(1,0.75f,0.2f,0.95f);
      glBegin(GL_TRIANGLE_FAN); glVertex2f(dx,dy);
      for (int a=0;a<=12;++a){ float an=2*(float)M_PI*a/12; glVertex2f(dx+2.5f*cosf(an),dy+2.5f*sinf(an)); }
      glEnd();
      d++;
    }
  }

  glPopMatrix(); glMatrixMode(GL_PROJECTION); glPopMatrix(); glMatrixMode(GL_MODELVIEW);
  glDisable(GL_BLEND); glEnable(GL_DEPTH_TEST); glEnable(GL_LIGHTING);
}
