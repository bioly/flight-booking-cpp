#pragma once

#include "flight/domain/ids.hpp"
#include "flight/domain/reservation.hpp"

#include <optional>
#include <vector>

namespace flight::application {

class IReservationRepository {
public:
  virtual ~IReservationRepository() = default;

  virtual void add(flight::domain::Reservation reservation) = 0;
  virtual std::optional<flight::domain::Reservation> get(flight::domain::ReservationId id) const = 0;
  virtual std::vector<flight::domain::Reservation> list_by_order(flight::domain::OrderId order_id) const = 0;
};

} // namespace flight::application
