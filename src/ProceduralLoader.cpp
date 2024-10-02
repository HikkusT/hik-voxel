#include "ProceduralLoader.h"

namespace cubik {
  std::vector<int> loadStaircase(int size) {
    std::vector<int> voxelData(size * size * size, 0);
    for (int i = 0; i < size; i++) {
      for (int j = 0; j < size; j++) {
        for (int k = 0 ; k < size; k++) {
          voxelData[i * size * size + j * size + k] = k < j ? 1 : 0;
        }
      }
    }

    return voxelData;
  }
}
