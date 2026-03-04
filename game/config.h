// ════════════════════════════════════════════════════════════════
//  DUNGEON ROGUELITE — Configuration System
//  Loads game data from simple key=value config files
// ════════════════════════════════════════════════════════════════
#pragma once

#include <cstdio>
#include <cstdlib>
#include <cstring>


// Max config entries
#define MAX_CONFIG_ENTRIES 128
#define MAX_KEY_LEN 64
#define MAX_VAL_LEN 128

struct ConfigEntry {
  char key[MAX_KEY_LEN];
  char value[MAX_VAL_LEN];
};

struct Config {
  ConfigEntry entries[MAX_CONFIG_ENTRIES];
  int count;
};

// Load a config file (key=value format, # for comments)
static bool loadConfig(Config &cfg, const char *filepath) {
  cfg.count = 0;
  FILE *f = fopen(filepath, "r");
  if (!f) {
    printf("[Config] Could not open: %s (using defaults)\n", filepath);
    return false;
  }

  char line[256];
  while (fgets(line, sizeof(line), f) && cfg.count < MAX_CONFIG_ENTRIES) {
    // Strip newline
    char *nl = strchr(line, '\n');
    if (nl)
      *nl = '\0';
    char *cr = strchr(line, '\r');
    if (cr)
      *cr = '\0';

    // Skip empty lines and comments
    if (line[0] == '\0' || line[0] == '#' || line[0] == ';')
      continue;

    // Find '=' separator
    char *eq = strchr(line, '=');
    if (!eq)
      continue;

    *eq = '\0';
    char *key = line;
    char *val = eq + 1;

    // Trim leading whitespace
    while (*key == ' ' || *key == '\t')
      key++;
    while (*val == ' ' || *val == '\t')
      val++;

    // Trim trailing whitespace from key
    char *kend = key + strlen(key) - 1;
    while (kend > key && (*kend == ' ' || *kend == '\t')) {
      *kend = '\0';
      kend--;
    }

    strncpy(cfg.entries[cfg.count].key, key, MAX_KEY_LEN - 1);
    cfg.entries[cfg.count].key[MAX_KEY_LEN - 1] = '\0';
    strncpy(cfg.entries[cfg.count].value, val, MAX_VAL_LEN - 1);
    cfg.entries[cfg.count].value[MAX_VAL_LEN - 1] = '\0';
    cfg.count++;
  }

  fclose(f);
  printf("[Config] Loaded %d entries from %s\n", cfg.count, filepath);
  return true;
}

// Query helpers
static const char *cfgStr(const Config &cfg, const char *key,
                          const char *fallback) {
  for (int i = 0; i < cfg.count; i++)
    if (strcmp(cfg.entries[i].key, key) == 0)
      return cfg.entries[i].value;
  return fallback;
}

static float cfgFloat(const Config &cfg, const char *key, float fallback) {
  const char *v = cfgStr(cfg, key, nullptr);
  return v ? (float)atof(v) : fallback;
}

static int cfgInt(const Config &cfg, const char *key, int fallback) {
  const char *v = cfgStr(cfg, key, nullptr);
  return v ? atoi(v) : fallback;
}
