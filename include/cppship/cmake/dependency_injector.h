#pragma once

#include <ostream>

#include "cppship/core/manifest.h"
#include "cppship/util/class.h"

namespace cppship::cmake {

class DependencyInjector {
public:
    virtual ~DependencyInjector() = default;

    DECLARE_UNCOPYABLE(DependencyInjector);

    virtual void inject(std::ostream& out, const Manifest& manifest) = 0;
};

class CmakeDependencyInjector : public DependencyInjector {
public:
    void inject(std::ostream& out, const Manifest& manifest) override;
};

}