#pragma once

#include "flight/application/id_generator.hpp"

#include <atomic>

namespace flight::infrastructure {

class AtomicIdGenerator final : public flight::application::IIdGenerator {
public:
  flight::domain::FlightId next_flight_id() override {
    return flight::domain::FlightId{flight_id_.fetch_add(1, std::memory_order_relaxed)};
  }
  flight::domain::ReservationId next_reservation_id() override {
    return flight::domain::ReservationId{reservation_id_.fetch_add(1, std::memory_order_relaxed)};
  }
  flight::domain::OrderId next_order_id() override {
    return flight::domain::OrderId{order_id_.fetch_add(1, std::memory_order_relaxed)};
  }

private:
  std::atomic<std::uint64_t> flight_id_{1};
  std::atomic<std::uint64_t> reservation_id_{1};
  std::atomic<std::uint64_t> order_id_{1};
};

} // namespace flight::infrastructure
