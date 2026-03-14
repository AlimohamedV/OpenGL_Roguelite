#pragma once
#include <cmath>
#include <cstdlib>

/* ── Value-noise FBM (ported from reference site) ── */
namespace Noise {

inline float hashf(float x, float y) {
  float v = sinf(x * 127.1f + y * 311.7f) * 43758.5453f;
  return v - floorf(v);
}

inline float valuenoise(float x, float y) {
  float ix = floorf(x), iy = floorf(y);
  float fx = x - ix, fy = y - iy;
  // smoothstep
  fx = fx * fx * (3.0f - 2.0f * fx);
  fy = fy * fy * (3.0f - 2.0f * fy);
  float a = hashf(ix, iy);
  float b = hashf(ix + 1, iy);
  float c = hashf(ix, iy + 1);
  float d = hashf(ix + 1, iy + 1);
  return a + (b - a) * fx + (c - a) * fy + (a - b - c + d) * fx * fy;
}

inline float fbm(float x, float y, int octaves = 3) {
  float v = 0, amp = 0.5f, freq = 1.0f;
  for (int i = 0; i < octaves; ++i) {
    v += amp * valuenoise(x * freq, y * freq);
    amp *= 0.5f;
    freq *= 2.0f;
  }
  return v;
}

} // namespace Noise
