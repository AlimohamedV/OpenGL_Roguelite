#include "data.h"

void initIslands() {
  islands.clear();
  journeyOrder.clear();
  auto add = [](const char* name, float lat, float lon, int saga, bool special = false) {
    int i = (int)islands.size();
    islands.push_back(Island{ name, lat, lon, saga, special });
    journeyOrder.push_back(i);
  };
  // East Blue (saga 0)
  add("Dawn Island (Foosha)", 32.0f, 145.0f, 0);
  add("Goat Island",          28.0f, 152.0f, 0);
  add("Shells Town",          26.0f, 158.0f, 0);
  add("Orange Town",          22.0f, 165.0f, 0);
  add("Syrup Village",        18.0f, 172.0f, 0);
  add("Baratie",              12.0f, 178.0f, 0);
  add("Conomi Islands",        8.0f,-175.0f, 0);
  add("Loguetown",             5.0f,-168.0f, 0);
  // Paradise (sagas 1-5)
  add("Reverse Mountain",      0.0f, 180.0f, 1, true);
  add("Whisky Peak",           2.0f,-170.0f, 1);
  add("Little Garden",         1.0f,-155.0f, 1);
  add("Drum Island",          -3.0f,-140.0f, 1);
  add("Alabasta",             -2.0f,-125.0f, 1);
  add("Jaya",                  0.0f,-108.0f, 2);
  add("Skypiea",               5.0f,-105.0f, 2);
  add("Long Ring Long Land",  -1.0f, -85.0f, 3);
  add("Water 7",               0.0f, -70.0f, 3);
  add("Enies Lobby",          -2.0f, -65.0f, 3);
  add("Thriller Bark",         0.0f, -45.0f, 4);
  add("Sabaody Archipelago",   0.0f, -25.0f, 5);
  add("Mariejois",             0.0f,   0.0f, 5, true);
  add("Fish-Man Island",      -1.0f,   5.0f, 6, true);
  // New World (sagas 7-10)
  add("Punk Hazard",           3.0f,  15.0f, 7);
  add("Dressrosa",             0.0f,  28.0f, 7);
  add("Green Bit",             1.0f,  30.0f, 7);
  add("Zou",                   5.0f,  55.0f, 8);
  add("Whole Cake Island",    -4.0f,  72.0f, 8);
  add("Wano Country",          8.0f,  95.0f, 9);
  add("Egghead",               0.0f, 118.0f, 10);
  add("Elbaf",                12.0f, 130.0f, 10);
}
