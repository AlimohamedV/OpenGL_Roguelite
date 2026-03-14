/*
 * One Piece World — 3D Visualizer (Premium Edition)
 * Entry point: GLUT setup, display loop, input handling.
 */
#include "globals.h"
#include "data.h"
#include "stars.h"
#include "globe.h"
#include "journey.h"
#include "hud.h"

// ── Global variable definitions ──
float camDist = 24.0f, camDistTarget = 24.0f;
float camAngleX = 15.0f, camAngleY = 0.0f;
float camAngleXTarget = 15.0f, camAngleYTarget = 0.0f;
bool  autoRotate = true;
float gTime = 0.0f;
bool  mouseDown = false;
int   lastMouseX = 0, lastMouseY = 0;
int   sagaMax = 10;
const char* sagaNames[] = {
  "East Blue","Alabasta","Sky Island","Water 7","Thriller Bark",
  "Summit War","Fish-Man Island","Dressrosa","Whole Cake","Wano","Full Journey"
};
std::vector<Island> islands;
std::vector<int>    journeyOrder;

// ── Display ──
static void display() {
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glLoadIdentity();

  camDist   = lerp(camDist, camDistTarget, 0.08f);
  camAngleX = lerp(camAngleX, camAngleXTarget, 0.1f);
  camAngleY = lerp(camAngleY, camAngleYTarget, 0.1f);

  float eyeX = camDist * cosf(camAngleX*(float)M_PI/180) * sinf(camAngleY*(float)M_PI/180);
  float eyeY = camDist * sinf(camAngleX*(float)M_PI/180);
  float eyeZ = camDist * cosf(camAngleX*(float)M_PI/180) * cosf(camAngleY*(float)M_PI/180);
  gluLookAt(eyeX, eyeY, eyeZ, 0, 0, 0, 0, 1, 0);

  drawStars();

  // Lighting: key (warm sun) + fill (cool) + rim
  float lp0[] = {10,6,8,1};
  float la[]  = {0.14f,0.14f,0.17f,1};
  float ld[]  = {1.0f,0.96f,0.88f,1};  // warm sunlight
  float ls[]  = {1.0f,0.95f,0.85f,1};
  glLightfv(GL_LIGHT0, GL_POSITION, lp0);
  glLightfv(GL_LIGHT0, GL_AMBIENT, la);
  glLightfv(GL_LIGHT0, GL_DIFFUSE, ld);
  glLightfv(GL_LIGHT0, GL_SPECULAR, ls);

  float lp1[] = {-8,-4,-6,1};
  float fd[]  = {0.10f,0.14f,0.22f,1};
  float fa[]  = {0,0,0,1};
  glEnable(GL_LIGHT1);
  glLightfv(GL_LIGHT1, GL_POSITION, lp1);
  glLightfv(GL_LIGHT1, GL_DIFFUSE, fd);
  glLightfv(GL_LIGHT1, GL_AMBIENT, fa);

  // Rim light
  float lp2[] = {0,0,-10,1};
  float rd[]  = {0.05f,0.08f,0.14f,1};
  glEnable(GL_LIGHT2);
  glLightfv(GL_LIGHT2, GL_POSITION, lp2);
  glLightfv(GL_LIGHT2, GL_DIFFUSE, rd);
  glLightfv(GL_LIGHT2, GL_AMBIENT, fa);

  glRotatef(gTime * 3.5f, 0, 1, 0);

  drawAtmosphere(GLOBE_R, eyeX, eyeY, eyeZ);

  glEnable(GL_COLOR_MATERIAL);
  glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
  float sp[] = {0.3f,0.3f,0.4f,1};
  glMaterialfv(GL_FRONT, GL_SPECULAR, sp);
  glMaterialf(GL_FRONT, GL_SHININESS, 18.0f);
  drawWorldSphere(GLOBE_R, 120, 70);
  glDisable(GL_COLOR_MATERIAL);

  drawCloudLayer(GLOBE_R);
  drawGridLines(GLOBE_R);
  drawJourneyPath(GLOBE_R);

  for (size_t i = 0; i < islands.size(); ++i) {
    float x,y,z;
    latLonToXYZ(GLOBE_R + 0.22f, islands[i].lat, islands[i].lon, &x, &y, &z);
    drawIslandMarker(x, y, z, islands[i].saga <= sagaMax, islands[i].special);
  }

  int vp[4]; glGetIntegerv(GL_VIEWPORT, vp);
  drawRegionLabels(vp, GLOBE_R);
  drawIslandLabels(vp, GLOBE_R);
  drawHUD(vp);

  glutSwapBuffers();
}

static void timer(int) {
  gTime += 0.016f;
  if (autoRotate) camAngleYTarget += 3.5f * 0.016f;
  glutPostRedisplay();
  glutTimerFunc(16, timer, 0);
}

static void reshape(int w, int h) {
  if (!h) h = 1;
  glViewport(0,0,w,h);
  glMatrixMode(GL_PROJECTION); glLoadIdentity();
  gluPerspective(45.0, (double)w/h, 0.1, 250.0);
  glMatrixMode(GL_MODELVIEW);
}

static void keyboard(unsigned char key, int, int) {
  switch (key) {
  case 27: exit(0);
  case 'w': case 'W': camAngleXTarget += 3; break;
  case 's': case 'S': camAngleXTarget -= 3; break;
  case 'a': case 'A': camAngleYTarget -= 3; break;
  case 'd': case 'D': camAngleYTarget += 3; break;
  case 'q': case 'Q': camDistTarget += 1.5f; break;
  case 'e': case 'E': camDistTarget -= 1.5f; if (camDistTarget<12) camDistTarget=12; break;
  case 'r': case 'R': autoRotate = !autoRotate; break;
  case '1': sagaMax=0; break; case '2': sagaMax=1; break;
  case '3': sagaMax=2; break; case '4': sagaMax=3; break;
  case '5': sagaMax=4; break; case '6': sagaMax=5; break;
  case '7': sagaMax=6; break; case '8': sagaMax=7; break;
  case '9': sagaMax=8; break; case '0': sagaMax=10; break;
  case '-': sagaMax = sagaMax>0 ? sagaMax-1 : 0; break;
  case '=': sagaMax = sagaMax<10 ? sagaMax+1 : 10; break;
  }
}

static void mouseBtn(int btn, int state, int x, int y) {
  if (btn == GLUT_LEFT_BUTTON) {
    if (state == GLUT_DOWN) { mouseDown=true; lastMouseX=x; lastMouseY=y; autoRotate=false; }
    else mouseDown = false;
  }
  if (btn == 3) { camDistTarget -= 1.5f; if (camDistTarget<12) camDistTarget=12; }
  if (btn == 4) { camDistTarget += 1.5f; if (camDistTarget>60) camDistTarget=60; }
}

static void mouseMotion(int x, int y) {
  if (mouseDown) {
    camAngleYTarget += (x - lastMouseX) * 0.3f;
    camAngleXTarget += (y - lastMouseY) * 0.3f;
    if (camAngleXTarget > 89) camAngleXTarget = 89;
    if (camAngleXTarget < -89) camAngleXTarget = -89;
    lastMouseX = x; lastMouseY = y;
  }
}

int main(int argc, char** argv) {
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH | GLUT_MULTISAMPLE);
  glutInitWindowSize(1280, 800);
  glutCreateWindow("One Piece World - Journey Map");

  glEnable(GL_DEPTH_TEST);
  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);
  glEnable(GL_NORMALIZE);
  glShadeModel(GL_SMOOTH);
  glEnable(GL_MULTISAMPLE);
  glClearColor(0.001f, 0.001f, 0.03f, 1.0f);

  glEnable(GL_FOG);
  float fc[] = {0.001f, 0.001f, 0.03f, 1};
  glFogfv(GL_FOG_COLOR, fc);
  glFogi(GL_FOG_MODE, GL_EXP2);
  glFogf(GL_FOG_DENSITY, 0.004f);

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  initIslands();
  initStars();

  glutDisplayFunc(display);
  glutReshapeFunc(reshape);
  glutKeyboardFunc(keyboard);
  glutMouseFunc(mouseBtn);
  glutMotionFunc(mouseMotion);
  glutTimerFunc(0, timer, 0);

  glutMainLoop();
  return 0;
}
