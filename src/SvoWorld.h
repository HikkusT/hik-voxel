#pragma once

#include "World.h"
#include <vector>
#include <memory>
#include <optional>
#include <glm/vec3.hpp>

namespace cubik {
  class OctreeNode {
  public:
    std::optional<int> _value;

    std::unique_ptr<OctreeNode> children[8];

    OctreeNode(std::optional<int> value = std::nullopt);

    size_t calculateSize();
  };

  struct LinearOctreeNode {
  public:
    int LeafMask;

    int childrenOffsets[8];
  };

  class SvoWorld : public World {
  public:
    SvoWorld(const std::vector<int> &worldData, int worldSize);

    // Returns the serialized size of the world data
    [[nodiscard]] size_t calculateSerializedSize() const override;

    // Serializes the world data into the provided buffer
    void serialize(void *target) const override;

    void print();

    const std::string& getCompatibleShader() const override;

    int get(glm::ivec3 position) const override;


  private:
    std::unique_ptr<OctreeNode> _svo;
    std::vector<LinearOctreeNode> _linearizedSvo;
    int _worldSize;

    std::unique_ptr<OctreeNode> buildSvo(const std::vector<int> &worldData, glm::ivec3 position, int size);
    int buildLinearizedSvo(OctreeNode& nodeToLinearize);
  };
}
