#pragma once

#include <string>
#include <vector>

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

namespace cppship::util {

template <template <class...> class Container, class Input, class Pred> inline auto split_to(Input&& input, Pred pred)
{
    Container<std::string> result;

    boost::split(result, std::forward<Input>(input), pred, boost::token_compress_on);

    return result;
}

template <class Input, class Pred> inline auto split(Input&& input, Pred pred)
{
    return split_to<std::vector>(std::forward<Input>(input), pred);
}

inline bool contains(std::string_view corpus, std::string_view patten)
{
    return corpus.find(patten) != std::string_view::npos;
}

}
