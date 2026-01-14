#pragma once

#include <stdint.h>

struct RawOverloadResult {
  bool trac;
  bool comp;
};

class RawOverloadEvaluator {
 public:
  void setThresholds(int32_t onCountsAbs, int32_t offCountsAbs);
  void setTiming(uint16_t nOn, uint16_t nOff);

  RawOverloadResult update(int32_t rawSigned, bool modoCompresometro);

 private:
  struct SideState {
    bool active = false;
    uint16_t onCount = 0;
    uint16_t offCount = 0;
  };

  void updateSide(SideState& side, bool candidate, int32_t absCounts);
  void clearSide(SideState& side);

  int32_t onCountsAbs_ = 0;
  int32_t offCountsAbs_ = 0;
  uint16_t nOn_ = 1;
  uint16_t nOff_ = 1;

  SideState tracState_;
  SideState compState_;
};
