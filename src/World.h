#pragma once

namespace cubik {
  class World {
  public:
    virtual ~World() = default;

    virtual size_t calculateSerializedSize() const = 0;

    virtual void serialize(void *target) const = 0;
  };
}
