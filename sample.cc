#include "map_sort.h"

#include <cstdio>
#include <iostream>

int main() {
  // Sorting integers
  {
    int data[5] = {-1, 2, 0, -2, 1};

    map_sort::Sort(data, 5);

    for (int i = 0; i < 5; ++i) std::cout << data[i] << " ";
    std::cout << std::endl;
  }

  // Sorting strings
  {
    std::string data[5];
    data[0] = "hoge";
    data[1] = "piyo";
    data[2] = "fuga";
    data[3] = "foo";
    data[4] = "bar";

    map_sort::Sort(data, 5);

    for (int i = 0; i < 5; ++i) std::cout << data[i] << " ";
    std::cout << std::endl;
  }

  // You can specify the number of threads.
  // (Otherwise default value given by OpenMP would be used.)
  {
    int data[5] = {-1, 2, 0, -2, 1};

    map_sort::Sort(data, 5, 4);  // 4 thread

    for (int i = 0; i < 5; ++i) std::cout << data[i] << " ";
    std::cout << std::endl;
  }

  // When you perform sorting more than once, you can avoid
  // the cost of initialization using |MapSort| class
  {
    int data[5] = {-1, 2, 0, -2, 1};

    map_sort::MapSort<int> map_sort;
    map_sort.Init(5);
    map_sort.Sort(data, 5);

    for (int i = 0; i < 5; ++i) std::cout << data[i] << " ";
    std::cout << std::endl;
  }

  return 0;
}
