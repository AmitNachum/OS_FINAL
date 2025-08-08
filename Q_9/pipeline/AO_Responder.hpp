#pragma once
#include <functional>
#include "ActiveObject.hpp"
#include "Response.hpp"

namespace Q9 {

// AO_Responder is the pipeline sink that sends responses to clients.
// It accepts Outgoing payloads and calls the provided send function.
class AO_Responder : public ActiveObject<Outgoing> {
public:
    using SendFunc = std::function<void(int /*fd*/, const std::string&)>;

    AO_Responder(BlockingQueue<Outgoing>& in_q, SendFunc sender)
        : ActiveObject<Outgoing>(in_q), m_sender(std::move(sender)) {}

protected:
    void process(Outgoing&& out) override {
        if (out.client_fd < 0) return;
        if (m_sender) m_sender(out.client_fd, out.payload);
    }

private:
    SendFunc m_sender;
};

} // namespace Q9
