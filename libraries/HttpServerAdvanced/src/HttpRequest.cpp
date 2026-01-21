#include "./HttpRequest.h"

namespace HttpServerAdvanced {

IPAddress HttpRequest::remoteIP() {
    return remoteIP_;
}

uint16_t HttpRequest::remotePort() {
    return remotePort_;
}

IPAddress HttpRequest::localIP() {
    return localIP_;
}

uint16_t HttpRequest::localPort() {
    return localPort_;
}

} // namespace HttpServerAdvanced
