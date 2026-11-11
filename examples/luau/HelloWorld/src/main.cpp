/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** main
*/

#include <aylo/luau/Runtime.hpp>
#include <fstream>
#include <iostream>

int main() {
    // ReSharper disable once CppTooWideScopeInitStatement
    constexpr aylo::luau::RuntimeConfig config{
        .libs = aylo::luau::RuntimeConfig::Libs::kStandard,
        .optimizationLevel = aylo::luau::RuntimeConfig::OptimizationLevel::kDisabled,
    };
    const auto runtime = aylo::luau::Runtime::create(config);

    if (!runtime) {
        std::cerr << "failed to create Luau runtime: " << runtime.error().message() << std::endl;
        return 1;
    }

    auto script = runtime->load("examples/luau/HelloWorld/script.luau");

    if (!script) {
        std::cerr << "failed to load script: " << script.error().message() << std::endl;
        return 1;
    }

    if (auto result = runtime->run(*script); !result) {
        std::cerr << "failed to run script: " << result.error().message() << std::endl;
        return 1;
    }
}
