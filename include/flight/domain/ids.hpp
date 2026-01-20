#pragma once

#include "flight/util/strong_id.hpp"

namespace flight::domain {

struct FlightIdTag {};
struct OrderIdTag {};
struct ReservationIdTag {};

using FlightId = flight::util::StrongId<FlightIdTag>;
using OrderId = flight::util::StrongId<OrderIdTag>;
using ReservationId = flight::util::StrongId<ReservationIdTag>;

} // namespace flight::domain
