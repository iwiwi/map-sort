// Copyright 2010, Takuya Akiba
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Takuya Akiba nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// Usage:
//   % g++ -O3 map_sort_test.cc -lgtest -lgtest_main -fopenmp
//   % ./a.out

#include "map_sort.h"
#include <stdint.h>
#include <cmath>
#include <algorithm>
#include <string>
#include <iostream>
#include <gtest/gtest.h>

using testing::Types;

namespace {
uint8_t Random8bit() {
  return rand() & 0xFF;
}

template<typename T> T Random() {
  T r(0);
  for (size_t i = 0; i < sizeof(T); ++i) {
    r |= static_cast<T>(Random8bit()) << (i * 8);
  }
  return r;
}

template<> float Random() {
  for (;;) {
    union {
      uint32_t u;
      float f;
    } uf;
    uf.u = Random<uint32_t>();
    if (isnanf(uf.f)) continue;
    return uf.f;
  }
}

template<> double Random() {
  for (;;) {
    union {
      uint64_t u;
      double f;
    } uf;
    uf.u = Random<uint64_t>();
    if (isnan(uf.f)) continue;
    return uf.f;
  }
}

template<> std::string Random() {
  static const double kLastingProbability = 0.9;
  std::string res;
  do {
    res += 'a' + rand() % 26;
  } while (rand() / (double)RAND_MAX < kLastingProbability);
  return res;
};

template<> std::pair<int, int> Random() {
  return std::make_pair(rand() % 10, rand());
}

template<typename T> void FillRandom(T *a, size_t n) {
  for (size_t i = 0; i < n; ++i) {
    a[i] = Random<T>();
  }
}

template<typename T> void SortAndCheck(map_sort::MapSort<T> &map_sort,
                                       T *dat, size_t num_elems, int num_threads) {
  T *ans = new T[num_elems];
  ASSERT_NE(reinterpret_cast<T*>(NULL), ans);

  std::partial_sort_copy(dat, dat + num_elems, ans, ans + num_elems);
  map_sort.Sort(dat, num_elems, num_threads);

  for (size_t i = 0; i < num_elems; ++i) {
    // We don't use |ASSERT_EQ| because it requires |operator<<| for output
    ASSERT_TRUE(ans[i] == dat[i]);
  }
  delete [] ans;
}
}  // namespace

typedef Types<
  int,
  double,
  std::string,
  std::pair<int, int>
> SortingTypes;

template<typename T>
class MapSortTest : public testing::Test {};
TYPED_TEST_CASE(MapSortTest, SortingTypes);

TYPED_TEST(MapSortTest, small) {
  const size_t kMaxNumElems = 500;
  const int kMaxNumThreads = 32;
  const int kNumTrials = 100;

  TypeParam *dat = new TypeParam[kMaxNumElems];
  ASSERT_NE(reinterpret_cast<TypeParam*>(NULL), dat);
  map_sort::MapSort<TypeParam> map_sort;
  map_sort.Init(kMaxNumElems, kMaxNumThreads);

  for (int t = 0; t < kNumTrials; ++t) {
    size_t num_elems = 1 + rand() % kMaxNumElems;
    int num_threads = 1 + rand() % kMaxNumThreads;
    FillRandom(dat, num_elems);
    SortAndCheck(map_sort, dat, num_elems, num_threads);
  }
  delete [] dat;
}

TYPED_TEST(MapSortTest, large) {
  const size_t kMaxNumElems = 100000;
  const int kMaxNumThreads = 32;
  const int kNumTrials = 10;

  TypeParam *dat = new TypeParam[kMaxNumElems];
  ASSERT_NE(reinterpret_cast<TypeParam*>(NULL), dat);
  map_sort::MapSort<TypeParam> map_sort;
  map_sort.Init(kMaxNumElems, kMaxNumThreads);

  for (int t = 0; t < kNumTrials; ++t) {
    size_t num_elems = 1 + rand() % kMaxNumElems;
    int num_threads = 1 + rand() % kMaxNumThreads;
    FillRandom(dat, num_elems);
    SortAndCheck(map_sort, dat, num_elems, num_threads);
  }
  delete [] dat;
}

TYPED_TEST(MapSortTest, reuse) {
  const int kNumTrials = 10;
  const int kNumReuse = 10;
  const size_t kMaxMaxNumElems = 1000;
  const int kMaxMaxNumThreads = 20;

  for (int t = 0; t < kNumTrials; ++t) {
    map_sort::MapSort<TypeParam> map_sort;

    for (int r = 0; r < kNumReuse; ++r) {
      size_t max_num_elems = 1 + rand() % kMaxMaxNumElems;
      int max_num_threads = 1 + rand() % kMaxMaxNumThreads;

      TypeParam *dat = new TypeParam[max_num_elems];
      ASSERT_NE(reinterpret_cast<TypeParam*>(NULL), dat);
      FillRandom(dat, max_num_elems);

      map_sort.Init(max_num_elems, max_num_threads);
      SortAndCheck(map_sort, dat, max_num_elems, max_num_threads);

      delete [] dat;
    }
  }
}
