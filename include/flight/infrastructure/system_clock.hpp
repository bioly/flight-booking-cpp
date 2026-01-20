#pragma once

#include "flight/application/clock.hpp"

namespace flight::infrastructure {

class SystemClock final : public flight::application::IClock {
public:
  std::chrono::system_clock::time_point now() const override {
    return std::chrono::system_clock::now();
  }
};

} // namespace flight::infrastructure
