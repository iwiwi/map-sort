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

#ifndef MAP_SORT_H_
#define MAP_SORT_H_

#ifdef _OPENMP
#include <omp.h>
#endif

#include <stdint.h>
#include <cstring>
#include <cassert>
#include <climits>
#include <algorithm>
#include <utility>

namespace map_sort {
namespace utility {
// Return the number of threads that would be executed in parallel regions
int GetMaxThreads() {
#ifdef _OPENMP
  return omp_get_max_threads();
#else
  return 1;
#endif
}

// Set the number of threads that would be executed in parallel regions
void SetNumThreads(int num_threads) {
#ifdef _OPENMP
  omp_set_num_threads(num_threads);
#else
  if (num_threads != 1) {
    assert(!"compile with -fopenmp");
  }
#endif
}

// Return the thread number, which lies in [0, the number of threads)
int GetThreadId() {
#ifdef _OPENMP
  return omp_get_thread_num();
#else
  return 0;
#endif
}
}  // namespace utility

template<typename Type, size_t NumIntervals = 256>
class MapSort {
public:
  MapSort();
  ~MapSort();

  void Init(size_t max_elems, int max_threads = -1);

  void Sort(Type *src, size_t num_elems, int num_threads = -1);
private:
  static const size_t kNumPivots = NumIntervals - 1;

  size_t max_elems_;
  int max_threads_;

  Type *tmp_;
  size_t *rng_;

  Type piv_[kNumPivots];
  size_t pos_[NumIntervals + 1];

  size_t **histo_;

  int num_threads_;
  int *pos_bgn_, *pos_end_;

  void DeleteAll();

  // Compute |pos_bgn_| and |pos_end_| (associated ranges for each threads)
  void ComputeRanges(size_t num_elems);

  // First step of sorting
  // Compute the histogram of |src| using randomly sampled pivots
  void ComputeHistogram(Type *src, size_t num_elems);

  // Second step of sorting
  // Scatter elements of |src| to |tmp_| using the histogram
  void Scatter(Type *src);

  // Third step of sorting
  // Sort each intervals of |tmp_| divided by the pivots
  void SortIntervals();
};

template<typename Type, size_t NumIntervals>
MapSort<Type, NumIntervals>::MapSort()
  : max_elems_(0), max_threads_(0), tmp_(NULL), rng_(NULL), histo_(NULL),
    pos_bgn_(NULL), pos_end_(NULL) {}

template<typename Type, size_t NumIntervals>
MapSort<Type, NumIntervals>::~MapSort() {
  DeleteAll();
}

template<typename Type, size_t NumIntervals>
void MapSort<Type, NumIntervals>::DeleteAll() {
  delete [] tmp_;
  tmp_ = NULL;

  delete [] rng_;
  rng_ = NULL;

  for (int i = 0; i < max_threads_; ++i) delete [] histo_[i];
  delete [] histo_;
  histo_ = NULL;

  delete [] pos_bgn_;
  delete [] pos_end_;
  pos_bgn_ = pos_end_ = NULL;

  max_elems_ = 0;
  max_threads_ = 0;
}

template<typename Type, size_t kNumIntervals>
void MapSort<Type, kNumIntervals>::Init(size_t max_elems, int max_threads) {
  DeleteAll();

  max_elems_ = max_elems;

  if (max_threads == -1) {
    max_threads = utility::GetMaxThreads();
  }
  assert(max_threads >= 1);
  max_threads_ = max_threads;

  tmp_ = new Type[max_elems];
  rng_ = new size_t[max_elems];

  histo_ = new size_t*[max_threads];
  for (int i = 0; i < max_threads; ++i) {
    histo_[i] = new size_t[kNumIntervals];
  }

  pos_bgn_ = new int[max_threads];
  pos_end_ = new int[max_threads];
}

template<typename Type, size_t kNumIntervals>
void MapSort<Type, kNumIntervals>
::Sort(Type *src, size_t num_elems, int num_threads) {
  assert(num_elems <= max_elems_);

  if (num_threads == -1) {
    num_threads = utility::GetMaxThreads();
  }
  assert(1 <= num_threads && num_threads <= max_threads_);
  utility::SetNumThreads(num_threads);
  assert(utility::GetMaxThreads() == num_threads);
  num_threads_ = num_threads;

  // Compute |pos_bgn_| and |pos_end_|
  ComputeRanges(num_elems);

  ComputeHistogram(src, num_elems);
  Scatter(src);
  SortIntervals();

  for (size_t i = 0; i < num_elems; ++i) {
    src[i] = tmp_[i];
  }
}

template<typename Type, size_t kNumIntervals>
void MapSort<Type, kNumIntervals>
::ComputeRanges(size_t num_elems) {
  pos_bgn_[0] = 0;
  for (int i = 0; i < num_threads_ - 1; ++i) {
    const size_t t = (num_elems - pos_bgn_[i]) / (num_threads_ - i);
    pos_bgn_[i + 1] = pos_end_[i] = pos_bgn_[i] + t;
  }
  pos_end_[num_threads_ - 1] = num_elems;
}

template<typename Type, size_t kNumIntervals>
void MapSort<Type, kNumIntervals>
::ComputeHistogram(Type *src, size_t num_elems) {
  // Selecting pivots
  for (size_t i = 0; i < kNumPivots; ++i) {
    piv_[i] = src[rand() % num_elems];
  }
  std::sort(piv_, piv_ + kNumPivots);

  // Compute local histogram
  #ifdef _OPENMP
  #pragma omp parallel
  #endif
  {
    const int my_id = utility::GetThreadId();
    const size_t my_bgn = pos_bgn_[my_id];
    const size_t my_end = pos_end_[my_id];
    size_t *my_histo = histo_[my_id];

    memset(my_histo, 0, sizeof(size_t) * kNumIntervals);
    for (size_t i = my_bgn; i < my_end; ++i) {
      int k = std::lower_bound(piv_, piv_ + kNumPivots, src[i]) - piv_;
      rng_[i] = k;
      ++my_histo[k];
    }
  }

  // Compute global histogram
  size_t s = 0;
  for (size_t i = 0; i < kNumIntervals; ++i) {
    pos_[i] = s;
    for (int j = 0; j < num_threads_; ++j) {
      const size_t t = s + histo_[j][i];
      histo_[j][i] = s;
      s = t;
    }
  }
  pos_[kNumIntervals] = num_elems;
}

template<typename Type, size_t kNumIntervals>
void MapSort<Type, kNumIntervals>
::Scatter(Type *src) {
  #ifdef _OPENMP
  #pragma omp parallel
  #endif
  {
    const int my_id = utility::GetThreadId();
    const size_t my_bgn = pos_bgn_[my_id];
    const size_t my_end = pos_end_[my_id];
    size_t *my_histo = histo_[my_id];

    for (size_t i = my_bgn; i < my_end; ++i) {
      int k = rng_[i];
      tmp_[my_histo[k]++] = src[i];
    }
  }
}

template<typename Type, size_t kNumIntervals>
void MapSort<Type, kNumIntervals>
::SortIntervals() {
  #ifdef _OPENMP
  #pragma omp parallel for schedule(static, 1)
  #endif
  for (size_t i = 0; i < kNumIntervals; ++i) {
    std::sort(tmp_ + pos_[i], tmp_ + pos_[i + 1]);
  }
}

template<typename Type>
void Sort(Type *data, size_t num_elems, int num_threads = -1) {
  MapSort<Type> map_sort;
  map_sort.Init(num_elems, num_threads);
  map_sort.Sort(data, num_elems, num_threads);
}
};  // namespace map sort

#endif  // MAP_SORT_H_
