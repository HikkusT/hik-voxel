#pragma once

#include "World.h"
#include <vector>

namespace cubik {
  class UncompressedGridWorld : public World {
  public:
    UncompressedGridWorld(const std::vector<int> &worldData, int worldSize);

    // Returns the serialized size of the world data
    [[nodiscard]] size_t calculateSerializedSize() const override;

    // Serializes the world data into the provided buffer
    void serialize(void *target) const override;

  private:
    std::vector<int> _worldData;
    int _worldSize;
  };
}
