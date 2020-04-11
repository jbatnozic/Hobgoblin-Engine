#ifndef UHOBGOBLIN_RN_EVENT_HPP
#define UHOBGOBLIN_RN_EVENT_HPP

#include <Hobgoblin/Common.hpp>
#include <Hobgoblin/Utility/Visitor.hpp>

#include <optional>
#include <string>
#include <utility>
#include <variant>

#include <Hobgoblin/Private/Pmacro_define.hpp>

HOBGOBLIN_NAMESPACE_START
namespace rn {

struct RN_Event {

    struct BadPassphrase {
        std::optional<PZInteger> clientIndex; // Not used on client side
        std::string incorrectPassphrase;
    };

    struct ConnectAttemptFailed {
        enum class Reason {
            Error, TimedOut
        };
        Reason reason;
    };

    struct Connected {
        std::optional<PZInteger> clientIndex; // Not used on client side
    };

    struct Disconnected {
        enum class Reason {
            Error, TimedOut
        };
        std::optional<PZInteger> clientIndex; // Not used on client side
        Reason reason;
        std::string message;
    };

    using EventVariant = std::variant<
        BadPassphrase,
        ConnectAttemptFailed,
        Connected,
        Disconnected>;

    RN_Event() = default;

    template <class T>
    RN_Event(T&& arg)
        : eventVariant{std::forward<T>(arg)}
    {
    }

    std::optional<EventVariant> eventVariant;

    template <class ...Callables>
    void visit(Callables&&... callables) {
        std::visit(util::MakeVisitor(std::forward<Callables>(callables)...), eventVariant.value());
    }

    template <class ...Callables>
    void visit(Callables&&... callables) const {
        std::visit(util::MakeVisitor(std::forward<Callables>(callables)...), eventVariant.value());
    }
};

class RN_Node;

namespace detail {
    
class EventFactory {
public:
    explicit EventFactory(RN_Node& node);
    explicit EventFactory(RN_Node& node, PZInteger clientIndex);

    void createBadPassphrase(std::string incorrectPassphrase) const;
    void createConnectAttemptFailed(RN_Event::ConnectAttemptFailed::Reason reason) const;
    void createConnected() const;
    void createDisconnected(RN_Event::Disconnected::Reason reason, std::string message) const;

private:
    RN_Node& _node;
    std::optional<PZInteger> _clientIndex;
};

} // namespace detail

} // namespace rn
HOBGOBLIN_NAMESPACE_END

#include <Hobgoblin/Private/Pmacro_undef.hpp>
#include <Hobgoblin/Private/Short_namespace.hpp>

#endif // !UHOBGOBLIN_RN_EVENT_HPP