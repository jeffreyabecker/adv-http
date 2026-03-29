#pragma once

#include "../TransportInterfaces.h"

#include <memory>

namespace HttpServerAdvanced {
std::unique_ptr<ITransportFactory> createNativeSocketTransportFactory();
}