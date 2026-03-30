#pragma once

#include "../../transport/TransportInterfaces.h"

#include <memory>

namespace httpadv::v1::transport {
std::unique_ptr<ITransportFactory> createArduinoWiFiTransportFactory();
} // namespace httpadv::v1::transport