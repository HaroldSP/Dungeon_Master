#include "dice_detection.h"
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <float.h>

static inline uint8_t luminance(uint8_t r, uint8_t g, uint8_t b) {
  return (uint8_t)((30 * r + 59 * g + 11 * b) / 100);
}

const uint8_t kDigitFont5x7[10][7] = {
  {0x1E,0x11,0x11,0x11,0x11,0x11,0x1E}, //0
  {0x04,0x0C,0x14,0x04,0x04,0x04,0x1F}, //1
  {0x1E,0x01,0x01,0x1E,0x10,0x10,0x1F}, //2
  {0x1E,0x01,0x01,0x0E,0x01,0x01,0x1E}, //3
  {0x02,0x06,0x0A,0x12,0x1F,0x02,0x02}, //4
  {0x1F,0x10,0x10,0x1E,0x01,0x01,0x1E}, //5
  {0x0E,0x10,0x10,0x1E,0x11,0x11,0x1E}, //6
  {0x1F,0x01,0x02,0x04,0x08,0x08,0x08}, //7
  {0x1E,0x11,0x11,0x1E,0x11,0x11,0x1E}, //8
  {0x1E,0x11,0x11,0x1F,0x01,0x01,0x1E}  //9
};

static inline int clampi(int v, int lo, int hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

static inline int max(int a, int b) { return (a > b) ? a : b; }
static inline int min(int a, int b) { return (a < b) ? a : b; }

// Pip detection removed - only detecting numbers 1-20 now

static int classifyDigit(const uint8_t* gray, int w, int h) {
  if (w < 3 || h < 5) return -1;
  const int cells = 3;
  int cellW = max(1, w / cells);
  int cellH = max(1, h / cells);
  uint32_t feats[cells*cells] = {0};
  for (int cy=0; cy<cells; ++cy){
    for (int cx=0; cx<cells; ++cx){
      uint32_t sum = 0;
      int xEnd = min(w, (cx+1)*cellW);
      int yEnd = min(h, (cy+1)*cellH);
      for (int y=cy*cellH; y<yEnd; ++y){
        for (int x=cx*cellW; x<xEnd; ++x){
          sum += (uint32_t)(255 - gray[y*w + x]);
        }
      }
      feats[cy*cells+cx] = sum;
    }
  }

  const float refs[10][9] = {
    {0.90f,0.90f,0.90f, 0.70f,0.10f,0.70f, 0.90f,0.90f,0.90f}, //0
    {0.20f,0.30f,0.20f, 0.80f,0.80f,0.80f, 0.30f,0.30f,0.30f}, //1
    {0.90f,0.90f,0.90f, 0.20f,0.80f,0.80f, 0.90f,0.90f,0.20f}, //2
    {0.90f,0.90f,0.90f, 0.20f,0.80f,0.80f, 0.90f,0.90f,0.90f}, //3
    {0.70f,0.10f,0.70f, 0.70f,0.90f,0.70f, 0.20f,0.20f,0.70f}, //4
    {0.90f,0.90f,0.90f, 0.90f,0.90f,0.20f, 0.90f,0.90f,0.90f}, //5
    {0.80f,0.80f,0.80f, 0.80f,0.90f,0.20f, 0.90f,0.90f,0.90f}, //6
    {0.90f,0.90f,0.90f, 0.20f,0.80f,0.40f, 0.20f,0.80f,0.40f}, //7
    {0.90f,0.90f,0.90f, 0.90f,0.90f,0.90f, 0.90f,0.90f,0.90f}, //8
    {0.90f,0.90f,0.90f, 0.90f,0.90f,0.90f, 0.90f,0.90f,0.30f}, //9
  };

  float f[9];
  uint32_t total=0;
  for (int i=0;i<9;++i) total+=feats[i];
  for (int i=0;i<9;++i) f[i] = total ? (float)feats[i]/(float)total : 0.0f;
  int bestDigit = -1;
  float bestErr = FLT_MAX;
  for (int d=0; d<10; ++d){
    float err=0;
    for (int i=0;i<9;++i){
      float diff=f[i]-refs[d][i];
      err += diff*diff;
    }
    if (err<bestErr){bestErr=err; bestDigit=d;}
  }
  if (bestErr > 1.2f) return -1;  // Relaxed threshold for better detection
  return bestDigit;
}

static int classifyDigitRegion(const uint8_t* gray, int w, int h,
                               int x0, int y0, int x1, int y1,
                               bool invertPixels) {
  if (x0 < 0 || y0 < 0 || x1 < 0 || y1 < 0 ||
      x0 >= w || x1 >= w || y0 >= h || y1 >= h ||
      x0 > x1 || y0 > y1) {
    return -1;
  }
  const int roiW = x1 - x0 + 1;
  const int roiH = y1 - y0 + 1;
  if (roiW < 4 || roiH < 6) return -1;
  uint8_t* roi = (uint8_t*)malloc((size_t)roiW * roiH);
  if (!roi) return -1;
  for (int yy = 0; yy < roiH; ++yy) {
    uint8_t* dst = &roi[yy*roiW];
    const uint8_t* src = &gray[(y0+yy)*w + x0];
    if (!invertPixels) {
      memcpy(dst, src, roiW);
    } else {
      for (int xx = 0; xx < roiW; ++xx) {
        dst[xx] = (uint8_t)(255 - src[xx]);
      }
    }
  }
  int digit = classifyDigit(roi, roiW, roiH);
  free(roi);
  return digit;
}

bool detectDiceFromRGB(const uint8_t* rgb, int width, int height, DiceDetection& out) {
  out.detected = false; out.value = 0; out.x=out.y=out.w=out.h=0; out.confidence = 0.0f;
  if (!rgb || width<32 || height<32) return false;

  const int stepX = max(1, width / 120);
  const int stepY = max(1, height / 90);
  const int w = width / stepX;
  const int h = height / stepY;
  const int total = w * h;
  uint8_t* gray = (uint8_t*)malloc((size_t)w*h);
  if (!gray) return false;

  size_t idx=0;
  uint32_t sum = 0;
  for (int y=0;y<height;y+=stepY){
    const int rowOff = y*width*3;
    for (int x=0;x<width;x+=stepX){
      const int off=rowOff + x*3;
      uint8_t r=rgb[off+0], g=rgb[off+1], b=rgb[off+2];
      uint8_t lum = luminance(r,g,b);
      gray[idx++] = lum;
      sum += lum;
    }
  }

  const uint8_t mean = (uint8_t)(sum / (total ? total : 1));
  int offset = max(20, min(60, (int)mean / 2));
  int threshDark = (int)mean - offset;
  if (threshDark < 25) threshDark = 25;
  int threshBright = (int)mean + offset;
  if (threshBright > 230) threshBright = 230;

  int darkPixels = 0;
  int brightPixels = 0;
  for (int i = 0; i < total; ++i) {
    uint8_t v = gray[i];
    if (v < threshDark) darkPixels++;
    if (v > threshBright) brightPixels++;
  }
  bool useBright = (brightPixels > 0) && (brightPixels > darkPixels);

  // Find bounding box of target regions (numbers)
  int boxMinX = w, boxMinY = h, boxMaxX = -1, boxMaxY = -1;
  for (int y=0; y<h; ++y){
    for (int x=0; x<w; ++x){
      uint8_t val = gray[y*w + x];
      bool mask = useBright ? (val > threshBright) : (val < threshDark);
      if (mask) {
        if (x < boxMinX) boxMinX = x;
        if (x > boxMaxX) boxMaxX = x;
        if (y < boxMinY) boxMinY = y;
        if (y > boxMaxY) boxMaxY = y;
      }
    }
  }
  if (boxMaxX < boxMinX || boxMaxY < boxMinY) {
    free(gray);
    return false;
  }

  int boxW = boxMaxX - boxMinX + 1;
  int boxH = boxMaxY - boxMinY + 1;
  int maskPixels = useBright ? brightPixels : darkPixels;
  if (maskPixels < (total / 800) || maskPixels > (total * 3) / 5) {
    free(gray);
    return false;
  }

  int value = -1;
  if (boxW > boxH * 1.4f) {
    int searchStart = boxMinX + boxW / 4;
    int searchEnd = boxMaxX - boxW / 4;
    int bestSplit = -1;
    uint32_t bestScore = UINT_MAX;
    for (int cx = searchStart; cx <= searchEnd; ++cx) {
      uint32_t colSum = 0;
      for (int y=boxMinY; y<=boxMaxY; ++y) {
        uint8_t v = gray[y*w + cx];
        colSum += useBright ? v : (uint32_t)(255 - v);
      }
      if (colSum < bestScore) {
        bestScore = colSum;
        bestSplit = cx;
      }
    }
    if (bestSplit > boxMinX && bestSplit < boxMaxX) {
      int leftDigit = classifyDigitRegion(gray, w, h, boxMinX, boxMinY, bestSplit, boxMaxY, useBright);
      int rightDigit = classifyDigitRegion(gray, w, h, bestSplit+1, boxMinY, boxMaxX, boxMaxY, useBright);
      if (leftDigit > 0 && rightDigit >= 0) {
        int combined = leftDigit * 10 + rightDigit;
        if (combined >= 1 && combined <= 20) {
          value = combined;
        }
      }
    }
  }

  if (value < 0) {
    int digit = classifyDigitRegion(gray, w, h, boxMinX, boxMinY, boxMaxX, boxMaxY, useBright);
    if (digit >= 1 && digit <= 9) value = digit;  // Only single digits 1-9
  }

  if (value >= 1 && value <= 20) {
    int margin = 2;
    int minX = clampi(boxMinX - margin, 0, w-1);
    int minY = clampi(boxMinY - margin, 0, h-1);
    int maxX = clampi(boxMaxX + margin, 0, w-1);
    int maxY = clampi(boxMaxY + margin, 0, h-1);

    out.detected = true;
    out.value = value;
    out.x = clampi(minX * stepX, 0, width-1);
    out.y = clampi(minY * stepY, 0, height-1);
    int x2 = clampi((maxX + 1) * stepX, 0, width-1);
    int y2 = clampi((maxY + 1) * stepY, 0, height-1);
    out.w = max(1, x2 - out.x);
    out.h = max(1, y2 - out.y);
    free(gray);
    out.confidence = 1.0f;
    return true;
  }

  free(gray);
  return false;
}
