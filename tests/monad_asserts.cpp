//
// Created by david on 2026. 09. 28..
//
#include <memory>
#include <string>
#if __cplusplus >= 202302l
#include <optional>
#include <expected>
#include <vector>
#endif
#include "monad.hpp"

using move_only_type = std::unique_ptr<int[1024]>;

static_assert(ext::monadic<std::optional<move_only_type>>);
static_assert(ext::monadic<std::optional<int>>);
static_assert(ext::monadic<std::optional<std::string>>);
#if __cplusplus >= 202302l
static_assert(ext::error_monad<std::expected<double, int>>);
static_assert(ext::error_monad<std::expected<std::vector<int>, std::string>>);
static_assert(ext::error_monad<std::expected<move_only_type, std::string>>);
static_assert(ext::error_monad<std::expected<std::tuple<float, float, std::u8string>, std::string>>);
#endif

int main() {}
