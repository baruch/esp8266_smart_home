#ifndef FILTERS_H
#define FILTERS_H

template <typename T, int S> class FilterMedian {
public:
  void reset(void) { value = cur = 0; for (int i = 0; i < S; i++) {data[i] = 0;}}
  void input(T val) { data[cur] = val; cur++; if (cur == S) cur = 0; calc_value(); }
  T output(void) { return value; }

private:
  void calc_value(void) {
    int i;
    T ldata[S];

    // Copy data to local array for processing
    for (i = 0; i < S; i++)
      ldata[i] = data[i];

    // Sort the local array (bubble sort)
    for (i = 0; i < S-1; i++) {
      for (int j = i+1; j < S; j++) {
        if (ldata[i] > ldata[j]) {
          int tmp = ldata[i];
          ldata[i] = ldata[j];
          ldata[j] = tmp;
        }
      }
    }

    // Calculate the middle point
    value = ldata[S/2+1];
  }

  T value;
  T data[S];
  int cur;
};

#endif
