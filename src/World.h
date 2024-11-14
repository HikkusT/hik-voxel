#pragma once

#include <string>
#include <glm/vec3.hpp>

namespace cubik {
  class World {
  public:
    virtual ~World() = default;

    virtual size_t calculateSerializedSize() const = 0;

    virtual void serialize(void *target) const = 0;

    virtual const std::string& getCompatibleShader() const = 0;

    virtual int get(glm::ivec3 pos) const = 0;
  };
}
