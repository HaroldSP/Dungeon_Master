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

static int countPips(const uint8_t* gray, int w, int h, int thresh,
                     int& outMinX, int& outMinY, int& outMaxX, int& outMaxY) {
  const int total = w * h;
  uint8_t* bin = (uint8_t*)malloc(total);
  if (!bin) return 0;
  for (int i = 0; i < total; ++i) {
    bin[i] = (gray[i] < thresh) ? 1 : 0;
  }

  const int minArea = (w * h) / 2000;
  const int maxArea = (w * h) / 15;
  int* stackX = (int*)malloc(total * sizeof(int));
  int* stackY = (int*)malloc(total * sizeof(int));
  if (!stackX || !stackY) {
    free(bin);
    if (stackX) free(stackX);
    if (stackY) free(stackY);
    return 0;
  }
  auto idxOf = [w](int x, int y){ return y * w + x; };

  int globalMinX = w, globalMinY = h, globalMaxX = -1, globalMaxY = -1;
  int blobs = 0;
  for (int y = 1; y < h - 1; ++y) {
    for (int x = 1; x < w - 1; ++x) {
      int p = idxOf(x,y);
      if (!bin[p]) continue;
      int top = 0; stackX[top]=x; stackY[top]=y; top++; bin[p]=0;
      int area = 0, minX=x, maxX=x, minY=y, maxY=y;
      while (top) {
        top--;
        int cx = stackX[top], cy = stackY[top];
        area++;
        if (cx < minX) minX=cx; if (cx > maxX) maxX=cx;
        if (cy < minY) minY=cy; if (cy > maxY) maxY=cy;
        const int nx[4]={cx-1,cx+1,cx,cx};
        const int ny[4]={cy,cy,cy-1,cy+1};
        for (int i=0;i<4;++i){
          int qx=nx[i], qy=ny[i];
          if ((unsigned)qx >= (unsigned)w || (unsigned)qy >= (unsigned)h) continue;
          int qi = idxOf(qx,qy);
          if (bin[qi]) { bin[qi]=0; stackX[top]=qx; stackY[top]=qy; top++; }
        }
      }
      if (area < minArea || area > maxArea) continue;
      int bw = maxX - minX + 1, bh = maxY - minY + 1;
      float aspect = (float)bw / (float)bh;
      if (aspect < 0.5f || aspect > 2.0f) continue;
      float fill = (float)area / (float)(bw*bh);
      if (fill < 0.3f || fill > 0.85f) continue;
      blobs++;
      if (minX < globalMinX) globalMinX = minX;
      if (minY < globalMinY) globalMinY = minY;
      if (maxX > globalMaxX) globalMaxX = maxX;
      if (maxY > globalMaxY) globalMaxY = maxY;
      if (blobs > 20) { blobs = 0; y=h; break; }
    }
  }
  free(bin); free(stackX); free(stackY);
  if (blobs >= 1) {
    outMinX = globalMinX;
    outMinY = globalMinY;
    outMaxX = globalMaxX;
    outMaxY = globalMaxY;
    return blobs;
  }
  return 0;
}

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
  if (bestErr > 0.85f) return -1;
  return bestDigit;
}

static int classifyDigitRegion(const uint8_t* gray, int w, int h, int x0, int y0, int x1, int y1) {
  if (x0 < 0 || y0 < 0 || x1 >= w || y1 >= h || x0 > x1 || y0 > y1) return -1;
  const int roiW = x1 - x0 + 1;
  const int roiH = y1 - y0 + 1;
  if (roiW < 4 || roiH < 6) return -1;
  uint8_t* roi = (uint8_t*)malloc((size_t)roiW * roiH);
  if (!roi) return -1;
  for (int yy = 0; yy < roiH; ++yy) {
    memcpy(&roi[yy*roiW], &gray[(y0+yy)*w + x0], roiW);
  }
  int digit = classifyDigit(roi, roiW, roiH);
  free(roi);
  return digit;
}

bool detectDiceFromRGB(const uint8_t* rgb, int width, int height, DiceDetection& out) {
  out.detected = false; out.value = 0; out.x=out.y=out.w=out.h=0;
  if (!rgb || width<32 || height<32) return false;

  const int stepX = max(1, width / 160);
  const int stepY = max(1, height / 120);
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
  int k = max(18, min(40, mean / 4));
  int thresh = (int)mean - k;
  if (thresh < 35) thresh = 35;

  int pipMinX=0, pipMinY=0, pipMaxX=0, pipMaxY=0;
  int pipCount = countPips(gray, w, h, thresh, pipMinX, pipMinY, pipMaxX, pipMaxY);
  if (pipCount >= 1 && pipCount <= 20) {
    out.detected = true;
    out.value = pipCount;
    int margin = 1;
    out.x = clampi(pipMinX * stepX - margin*stepX, 0, width-1);
    out.y = clampi(pipMinY * stepY - margin*stepY, 0, height-1);
    int x2 = clampi((pipMaxX + 1 + margin) * stepX, 0, width-1);
    int y2 = clampi((pipMaxY + 1 + margin) * stepY, 0, height-1);
    out.w = max(1, x2 - out.x);
    out.h = max(1, y2 - out.y);
    free(gray);
    return true;
  }

  int boxMinX = w, boxMinY = h, boxMaxX = -1, boxMaxY = -1;
  for (int y=0; y<h; ++y){
    for (int x=0; x<w; ++x){
      uint8_t val = gray[y*w + x];
      if (val < thresh) {
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

  int value = -1;
  if (boxW > boxH * 1.4f) {
    int searchStart = boxMinX + boxW / 4;
    int searchEnd = boxMaxX - boxW / 4;
    int bestSplit = -1;
    uint32_t bestScore = UINT_MAX;
    for (int cx = searchStart; cx <= searchEnd; ++cx) {
      uint32_t colSum = 0;
      for (int y=boxMinY; y<=boxMaxY; ++y) {
        colSum += (uint32_t)(255 - gray[y*w + cx]);
      }
      if (colSum < bestScore) {
        bestScore = colSum;
        bestSplit = cx;
      }
    }
    if (bestSplit > boxMinX && bestSplit < boxMaxX) {
      int leftDigit = classifyDigitRegion(gray, w, h, boxMinX, boxMinY, bestSplit, boxMaxY);
      int rightDigit = classifyDigitRegion(gray, w, h, bestSplit+1, boxMinY, boxMaxX, boxMaxY);
      if (leftDigit > 0 && rightDigit >= 0) {
        int combined = leftDigit * 10 + rightDigit;
        if (combined >= 1 && combined <= 20) {
          value = combined;
        }
      }
    }
  }

  if (value < 0) {
    int digit = classifyDigitRegion(gray, w, h, boxMinX, boxMinY, boxMaxX, boxMaxY);
    if (digit >= 0) value = digit;
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
    return true;
  }

  free(gray);
  return false;
}
