#include "SvoWorld.h"
#include "spdlog/spdlog.h"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

namespace cubik {
  OctreeNode::OctreeNode(std::optional<int> value)
    : _value(value) {

  }

  size_t OctreeNode::calculateSize() {
    size_t size = 1;
    if (_value.has_value()) return size;

    for (const auto & child : children) {
      size += child->calculateSize();
    }
    return size;
  }

  const std::string& SvoWorld::getCompatibleShader() const {
    static const std::string compatibleShader = "svoRayMarcher";
    return compatibleShader;
  }

  SvoWorld::SvoWorld(const std::vector<int> &worldData, int worldSize)
    : _worldSize(worldSize) {
    _svo = buildSvo(worldData, glm::ivec3(0), worldSize);
    buildLinearizedSvo(*_svo);
  }

  std::unique_ptr<OctreeNode> SvoWorld::buildSvo(const std::vector<int> &worldData, glm::ivec3 position, int size) {
    if (size == 1) {
      return std::make_unique<OctreeNode>(std::optional<int>(worldData[position.x + (position.y * _worldSize) + (position.z * _worldSize * _worldSize)]));
    }

    int halfSize = size / 2;
    auto node = std::make_unique<OctreeNode>();

    for (int i = 0; i < 8; ++i) {
      int offsetX = (i & 1) ? halfSize : 0;
      int offsetY = (i & 2) ? halfSize : 0;
      int offsetZ = (i & 4) ? halfSize : 0;
      // 0 -> (0, 0, 0)
      // 1 -> (1, 0, 0)
      // 2 -> (0, 1, 0)
      // 3 -> (1, 1, 0)
      // 4 -> (0, 0, 1)
      // 5 -> (1, 0, 1)
      // 6 -> (0, 1, 1)
      // 7 -> (1, 1, 1)

      // Recursively build each child octant
      node->children[i] = buildSvo(worldData, position + glm::ivec3(offsetX, offsetY, offsetZ), halfSize);
    }

    std::optional<int> childValue = node->children[0]->_value;
    for (const auto & child : node->children) {
      if (child->_value != childValue) return node;
    }

    if (childValue.has_value()) {
      node->_value = childValue;
    }

    return node;
  }

  size_t SvoWorld::calculateSerializedSize() const {
    return sizeof(_worldSize) + sizeof(LinearOctreeNode) * _linearizedSvo.size();
  }

  void SvoWorld::serialize(void *target) const {
    char *dataPtr = static_cast<char *>(target);

    memcpy(dataPtr, &_worldSize, sizeof(_worldSize));
    dataPtr += sizeof(_worldSize);
    memcpy(dataPtr, _linearizedSvo.data(), sizeof(LinearOctreeNode) * _linearizedSvo.size());
  }

  void SvoWorld::print() {
    for (int i = 0; i < _linearizedSvo.size(); i++) {
      spdlog::info("Node {} got mask {} and offsets: 1: {}  2: {}  3: {}  4: {}  5: {}  6: {}  7: {}  8: {}", i, _linearizedSvo[i].LeafMask,
                   _linearizedSvo[i].childrenOffsets[0], _linearizedSvo[i].childrenOffsets[1], _linearizedSvo[i].childrenOffsets[2],
                   _linearizedSvo[i].childrenOffsets[3], _linearizedSvo[i].childrenOffsets[4], _linearizedSvo[i].childrenOffsets[5],
                   _linearizedSvo[i].childrenOffsets[6], _linearizedSvo[i].childrenOffsets[7]);
    }
  }

  int SvoWorld::buildLinearizedSvo(OctreeNode& nodeToLinearize) {
    LinearOctreeNode node{
      .LeafMask = 0
    };
    _linearizedSvo.push_back(node);
    int currentNodeIndex = static_cast<int>(_linearizedSvo.size()) - 1;

    // This should only happen for the root node. Actually, now that I think about this, this is probably unnecessary
//    if (nodeToLinearize._value.has_value()) {
//      node.Value = nodeToLinearize._value.value();
//      return numberOfAllocatedNodes;
//    }

    int numberOfAllocatedNodes = 1;
    for (int i = 0; i < 8; i++) {
      if (nodeToLinearize.children[i]->_value.has_value()) {
        node.LeafMask |= 1 << i;
        node.childrenOffsets[i] = nodeToLinearize.children[i]->_value.value();
      } else {
        node.childrenOffsets[i] = numberOfAllocatedNodes;
        numberOfAllocatedNodes += buildLinearizedSvo(*nodeToLinearize.children[i]);
      }
    }

    _linearizedSvo[currentNodeIndex] = node;
    return numberOfAllocatedNodes;
  }

  int SvoWorld::get(glm::ivec3 position) const {
    auto currentSearch = glm::ivec3(0);
    int currentSize = _worldSize;
    int currentLinearIndex = 0;

    while (currentSize > 1) {
      currentSize = currentSize / 2;
      glm::ivec3 offset = position - currentSearch;

      int index = 0;
      if (offset.x >= currentSize) index |= 1; // 1st bit (X axis)
      if (offset.y >= currentSize) index |= 2; // 2nd bit (Y axis)
      if (offset.z >= currentSize) index |= 4; // 3rd bit (Z axis)

      if (_linearizedSvo[currentLinearIndex].LeafMask & (1 << index)) {
        return _linearizedSvo[currentLinearIndex].childrenOffsets[index];
      }

      // Move to the child node's position
      currentSearch += glm::ivec3(
        (index & 1) ? currentSize : 0,
        (index & 2) ? currentSize : 0,
        (index & 4) ? currentSize : 0
      );
      currentLinearIndex += _linearizedSvo[currentLinearIndex].childrenOffsets[index];
    }

    spdlog::error("Failed to get value for {}", glm::to_string(position));
    abort();
  }
}
