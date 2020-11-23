//  This file is distributed under the BSD 3-Clause License. See LICENSE for details.

#include "bitwidth_range.hpp"

#include "fmt/format.h"
#include "iassert.hpp"

Lconst Bitwidth_range::to_lconst(bool overflow, int64_t val) {
  if (val == 0)
    return Lconst(0);

  if (!overflow) {
    return Lconst(val);
  }

  if (val > 0) {
    return Lconst(1).lsh_op(val) - 1;
  }

  return Lconst(0) - (Lconst(1).lsh_op(-val));
}

Bitwidth_range::Bitwidth_range(const Lconst &val) {
  if (val.is_i()) {
    overflow = false;
    max      = val.to_i();
    min      = val.to_i();
  } else {
    overflow  = true;
    auto bits = val.get_bits();

    if (val.is_negative()) {
      max = -bits;
      min = -bits;
    } else {
      max = bits;
      min = bits;
    }
  }
}

void Bitwidth_range::set_range(const Lconst &min_val, const Lconst &max_val) {
  I(max_val >= min_val);

  if (max_val.is_i() && min_val.is_i()) {
    overflow = false;
    max      = max_val.to_i();
    min      = min_val.to_i();
  } else {
    overflow = true;
    if (max_val == 0) {
      max = 0;
    } else {
      auto bits = max_val.get_bits();
      if (max_val.is_negative()) {
        max = -bits;
      } else {
        max = bits;
      }
    }

    if (min_val == 0) {
      min = 0;
    } else {
      auto bits = min_val.get_bits();
      if (min_val.is_negative()) {
        min = -bits;
      } else {
        min = bits;
      }
    }
  }
}

Bitwidth_range::Bitwidth_range(const Lconst &min_val, const Lconst &max_val) {
  set_range(min_val, max_val);
}

void Bitwidth_range::set_narrower_range(const Lconst &min_val, const Lconst &max_val) {
  if (max_val.is_i() && min_val.is_i()) {
    I(max>= max_val.to_i());
    I(min<= min_val.to_i());
  }
  set_range(min_val, max_val);
}

Bitwidth_range::Bitwidth_range(Bits_t bits, bool _sign) {
  if (_sign)
    set_sbits(bits);
  else
    set_ubits(bits);
}

Bitwidth_range::Bitwidth_range(Bits_t bits) { set_ubits(bits); }

void Bitwidth_range::set_sbits(Bits_t size) {
  I(size < Bits_max);

  if (size == 0) {
    overflow = true;
    max      = 326768;
    min      = -32768;
    return;
  }

  if (size > 63) {
    overflow = true;
    max      = size - 1;     // Use bits in overflow mode
    min      = -(size - 1);  // Use bits
  } else {
    overflow = false;
    max      = (1UL << (size - 1)) - 1;
    min      = -(1UL << (size - 1));
  }
}

void Bitwidth_range::set_ubits(Bits_t size) {
  I(size < Bits_max);

  if (size == 0) {
    overflow = true;
    max      = 326768;
    min      = 0;
    return;
  }
  assert(size);

  min = 0;

  if (size > 63) {
    overflow = true;
    max      = size;  // Use bits in overflow mode
  } else {
    overflow = false;
    max      = (1UL << size) - 1;
  }
}

// FIXME->sh: the following might be a false conclusion!!!!!
// Note: I change the semantic to -> get the least bits needed to represent both max/min together,
//           only when the min is too negative that the max_bits cannot represent, we need to increase 
//           one bit, this could perfectly avoid the Tposs extra-1-bit ripple problem.
//           e.g. (max, min) = (15, -1) ---> bits 4
//                (max, min) = (15, -8) ---> bits 4
//                (max, min) = (15, -9) ---> bits 5! since -9 needs 5sbits
// Note: the only node affected by this semantic change is DP-assign, where the mask need to mask all 
//       possible value
//               
Bits_t Bitwidth_range::get_bits() const {
  if (overflow) {
    Bits_t bits = max;
    if (min < 0)
      bits++;
    if (bits >= Bits_max)
      return 0;                          // To indicate overflow (unable to compute)
    return bits;
  }

  Bits_t bits = 1;
  if (max) {
    auto abs_max = abs(max);
    bits = (sizeof(uint64_t) * 8 - __builtin_clzll(abs_max));
  }
  
  //
  // original code     
  if (min < 0)
    bits++;
  /* if (min < -pow(2, ceil(log2(max)))) */
  /*   bits++; */


  I(bits < Bits_max);

  return bits;
}

void Bitwidth_range::dump() const { fmt::print("max:{} min:{} {}\n", max, min, overflow ? "overflow" : ""); }
