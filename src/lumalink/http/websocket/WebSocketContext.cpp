#include "WebSocketContext.h"

namespace lumalink::http::websocket
{
    WebSocketContext::WebSocketContext(WebSocketActivationSnapshot activation, WebSocketCallbacks callbacks)
        : activation_(std::move(activation)),
          callbacks_(std::move(callbacks))
    {
    }

    std::map<std::string, std::any> &WebSocketContext::items()
    {
        return activation_.items;
    }

    const std::map<std::string, std::any> &WebSocketContext::items() const
    {
        return activation_.items;
    }

    std::string_view WebSocketContext::methodView() const
    {
        return std::string_view(activation_.method.data(), activation_.method.size());
    }

    std::string_view WebSocketContext::versionView() const
    {
        return std::string_view(activation_.version.data(), activation_.version.size());
    }

    std::string_view WebSocketContext::urlView() const
    {
        return std::string_view(activation_.url.data(), activation_.url.size());
    }

    const HttpHeaderCollection &WebSocketContext::headers() const
    {
        return activation_.headers;
    }

    std::string_view WebSocketContext::remoteAddress() const
    {
        return std::string_view(activation_.remoteAddress.data(), activation_.remoteAddress.size());
    }

    std::uint16_t WebSocketContext::remotePort() const
    {
        return activation_.remotePort;
    }

    std::string_view WebSocketContext::localAddress() const
    {
        return std::string_view(activation_.localAddress.data(), activation_.localAddress.size());
    }

    std::uint16_t WebSocketContext::localPort() const
    {
        return activation_.localPort;
    }

    WebSocketSendResult WebSocketContext::sendText(std::string_view payload)
    {
        if (control_ == nullptr)
        {
            return WebSocketSendResult::InvalidState;
        }

        return control_->sendText(payload);
    }

    WebSocketSendResult WebSocketContext::sendBinary(span<const std::uint8_t> payload)
    {
        if (control_ == nullptr)
        {
            return WebSocketSendResult::InvalidState;
        }

        return control_->sendBinary(payload);
    }

    WebSocketCloseResult WebSocketContext::close(WebSocketCloseCode code, std::string_view reason)
    {
        if (control_ == nullptr)
        {
            return WebSocketCloseResult::InvalidState;
        }

        return control_->close(code, reason);
    }

    bool WebSocketContext::isOpen() const
    {
        return state_ == State::Open;
    }

    bool WebSocketContext::isClosing() const
    {
        return state_ == State::Closing;
    }

    std::optional<std::uint16_t> WebSocketContext::closeCode() const
    {
        return closeCode_;
    }

    std::string_view WebSocketContext::closeReason() const
    {
        return std::string_view(closeReason_.data(), closeReason_.size());
    }

    void WebSocketContext::bindControl(IWebSocketSessionControl *control)
    {
        control_ = control;
    }

    void WebSocketContext::markOpen()
    {
        state_ = State::Open;
    }

    void WebSocketContext::markClosing(std::uint16_t closeCode, std::string_view reason)
    {
        if (state_ == State::Closed)
        {
            return;
        }

        state_ = State::Closing;
        closeCode_ = closeCode;
        closeReason_.assign(reason.data(), reason.size());
    }

    void WebSocketContext::markClosed(std::uint16_t closeCode, std::string_view reason)
    {
        state_ = State::Closed;
        closeCode_ = closeCode;
        closeReason_.assign(reason.data(), reason.size());
    }

    void WebSocketContext::notifyOpen()
    {
        if (openNotified_)
        {
            return;
        }

        openNotified_ = true;
        markOpen();
        if (callbacks_.onOpen)
        {
            callbacks_.onOpen(*this);
        }
    }

    void WebSocketContext::notifyText(std::string_view message)
    {
        if (callbacks_.onText)
        {
            callbacks_.onText(*this, message);
        }
    }

    void WebSocketContext::notifyBinary(span<const std::uint8_t> payload)
    {
        if (callbacks_.onBinary)
        {
            callbacks_.onBinary(*this, payload);
        }
    }

    void WebSocketContext::notifyClose(std::uint16_t closeCode, std::string_view reason)
    {
        if (closeNotified_)
        {
            return;
        }

        closeNotified_ = true;
        markClosed(closeCode, reason);
        if (callbacks_.onClose)
        {
            callbacks_.onClose(*this, closeCode, reason);
        }
    }

    void WebSocketContext::notifyError(std::string_view message)
    {
        if (callbacks_.onError)
        {
            callbacks_.onError(*this, message);
        }
    }
}