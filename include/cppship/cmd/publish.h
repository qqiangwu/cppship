#pragma once

#include <optional>

#include "cppship/util/fs.h"

namespace cppship::cmd {

struct PublishOptions {
    fs::path index;
};

int run_publish(const PublishOptions& options);

}
