#include "UncompressedGridWorld.h"

namespace cubik {
  UncompressedGridWorld::UncompressedGridWorld(const std::vector<int> &worldData, int worldSize)
    : _worldData(worldData), _worldSize(worldSize) {}

  size_t UncompressedGridWorld::calculateSerializedSize() const {
    return sizeof(_worldSize) + sizeof(int) * _worldData.size();
  }

  void UncompressedGridWorld::serialize(void *target) const {
    char *dataPtr = static_cast<char *>(target);

    memcpy(dataPtr, &_worldSize, sizeof(_worldSize));
    dataPtr += sizeof(_worldSize);
    memcpy(dataPtr, _worldData.data(), sizeof(int) * _worldData.size());
  }

  const std::string& UncompressedGridWorld::getCompatibleShader() const {
    static const std::string compatibleShader = "naiveRayMarcher";
    return compatibleShader;
  }

  int UncompressedGridWorld::get(glm::ivec3 position) const {
    return _worldData[position.x + (position.y * _worldSize) + (position.z * _worldSize * _worldSize)];
  }
}
