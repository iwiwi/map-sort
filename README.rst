概要
----
並列ソートである Map Sort の OpenMP を用いた実装です．
テンプレートで書かれており，比較できる任意の型の配列をソートできます．

整数や浮動小数点数の普通のソートなら，
`並列 Radix Sort <https://github.com/iwiwi/parallel-radix-sort>`_ の方が遥かに高速です．

tbb::parallel_sort を使えばいいのではと言われれば大体そうで，
一応すこし速いのと，tbb を入れるのが面倒なときに役に立つかもしれません．

使い方
------
sample.cc や measure.cc を見ると大体分かると思います．

コンパイル時に -fopenmp を付けないと並列化されないので注意してください．

性能
----
measure.cc で 2 千万要素の int 配列のソートの時間を測ります．

実行例::

  % g++ -O3 -fopenmp measure.cc
  % ./a.out
  N = 20000000
  map_sort::Sort(0): 0.435177 sec
  map_sort::Sort(1): 0.400754 sec
  map_sort::Sort(2): 0.418151 sec
  std::sort(0): 1.728168 sec
  std::sort(1): 1.736658 sec
  std::sort(2): 1.734755 sec

参考文献
--------
* M. Edahiro and Y. Yamashita, "Map Sort: A Scalable Sorting Algorithm for Multi-Core Processors," IPSJ Report (SLDM), No. 27 (Mar., 2007), pp. 19--24, 2007.
