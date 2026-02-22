// src/abstractions/infrastructure/messaging/IEvent.h
#pragma once

#include <domain/events/DomainEvent.h>

namespace winsetup::abstractions {

    using IEvent = domain::DomainEvent;

    template<typename TDerived>
    using EventBase = domain::DomainEventBase<TDerived>;

}
