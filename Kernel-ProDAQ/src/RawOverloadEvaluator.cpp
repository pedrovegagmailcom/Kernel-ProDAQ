#include "RawOverloadEvaluator.h"

#include <stdlib.h>

void RawOverloadEvaluator::setThresholds(int32_t onCountsAbs, int32_t offCountsAbs) {
  onCountsAbs_ = onCountsAbs;
  offCountsAbs_ = offCountsAbs;
}

void RawOverloadEvaluator::setTiming(uint16_t nOn, uint16_t nOff) {
  nOn_ = nOn;
  nOff_ = nOff;
}

void RawOverloadEvaluator::clearSide(SideState& side) {
  side.active = false;
  side.onCount = 0;
  side.offCount = 0;
}

void RawOverloadEvaluator::updateSide(SideState& side, bool candidate, int32_t absCounts) {
  if (!candidate) {
    side.onCount = 0;
    if (side.active && absCounts <= offCountsAbs_) {
      if (side.offCount < nOff_) {
        ++side.offCount;
      }
      if (side.offCount >= nOff_) {
        side.active = false;
        side.offCount = 0;
      }
    } else {
      side.offCount = 0;
    }
    return;
  }

  side.offCount = 0;
  if (side.active) {
    return;
  }

  if (absCounts >= onCountsAbs_) {
    if (side.onCount < nOn_) {
      ++side.onCount;
    }
    if (side.onCount >= nOn_) {
      side.active = true;
      side.onCount = 0;
    }
  } else {
    side.onCount = 0;
  }
}

RawOverloadResult RawOverloadEvaluator::update(int32_t rawSigned, bool modoCompresometro) {
  if (modoCompresometro) {
    rawSigned = -abs(rawSigned);
  }

  int32_t absCounts = abs(rawSigned);
  bool isTrac = rawSigned >= 0;
  bool isComp = rawSigned < 0;

  updateSide(tracState_, isTrac, absCounts);
  updateSide(compState_, isComp, absCounts);

  if (tracState_.active && compState_.active) {
    if (isTrac) {
      clearSide(compState_);
    } else {
      clearSide(tracState_);
    }
  } else if (tracState_.active) {
    clearSide(compState_);
  } else if (compState_.active) {
    clearSide(tracState_);
  }

  RawOverloadResult result = {};
  result.trac = tracState_.active;
  result.comp = compState_.active;
  return result;
}
