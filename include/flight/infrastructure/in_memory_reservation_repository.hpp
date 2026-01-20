#pragma once

#include "flight/application/reservation_repository.hpp"

#include <shared_mutex>
#include <unordered_map>
#include <vector>

namespace flight::infrastructure {

class InMemoryReservationRepository final : public flight::application::IReservationRepository {
public:
  void add(flight::domain::Reservation reservation) override;
  std::optional<flight::domain::Reservation> get(flight::domain::ReservationId id) const override;
  std::vector<flight::domain::Reservation> list_by_order(flight::domain::OrderId order_id) const override;

private:
  mutable std::shared_mutex mu_;
  std::unordered_map<flight::domain::ReservationId::value_type, flight::domain::Reservation> reservations_;
};

} // namespace flight::infrastructure
