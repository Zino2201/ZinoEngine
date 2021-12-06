#include "engine/module/module.hpp"
#include <boost/locale.hpp>

namespace ze
{

Module::Module() : handle(nullptr)
{
    const boost::locale::generator generator;
    const std::locale locale = generator.generate(std::locale(), "");
    std::locale::global(locale);
}

}