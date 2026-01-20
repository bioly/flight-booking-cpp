#pragma once

#include <chrono>

namespace flight::application {

class IClock {
public:
  virtual ~IClock() = default;
  virtual std::chrono::system_clock::time_point now() const = 0;
};

} // namespace flight::application
