#pragma once

#define DECLARE_UNCOPYABLE(cls)                                                                                        \
    cls() = default;                                                                                                   \
    cls(const cls&) = delete;                                                                                          \
    cls(cls&&) = delete;                                                                                               \
    cls& operator=(const cls&) = delete;                                                                               \
    cls& operator=(cls&&) = delete
