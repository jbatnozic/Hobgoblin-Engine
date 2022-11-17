#ifndef UHOBGOBLIN_RMLUI_PRIVATE_RMLUI_SFML_SYSTEM_HPP
#define UHOBGOBLIN_RMLUI_PRIVATE_RMLUI_SFML_SYSTEM_HPP

#include <RmlUi/Core/SystemInterface.h>
#include <SFML/Window.hpp>
#include <SFML/System.hpp>

#include <Hobgoblin/Private/Pmacro_define.hpp>
HOBGOBLIN_NAMESPACE_BEGIN
namespace rml {
namespace detail {

class RmlUiSFMLSystem : public Rml::SystemInterface
{
public:
    ~RmlUiSFMLSystem() override;

    double GetElapsedTime() override;

    bool LogMessage(Rml::Log::Type aType, const Rml::String& aMessage) override;

    void SetClipboardText(const Rml::String& aText) override;

    void GetClipboardText(Rml::String& aText) override;

private:
    sf::Clock _clock;
};

} // namespace detail
} // namespace rml
HOBGOBLIN_NAMESPACE_END
#include <Hobgoblin/Private/Pmacro_undef.hpp>
#include <Hobgoblin/Private/Short_namespace.hpp>

#endif // !UHOBGOBLIN_RMLUI_PRIVATE_RMLUI_SFML_SYSTEM_HPP
