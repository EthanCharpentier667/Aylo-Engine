/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** main
*/

#include <algorithm>
#include <aylo/luau/Function.hpp>
#include <aylo/luau/LibraryBuilder.hpp>
#include <aylo/luau/Runtime.hpp>
#include <aylo/luau/Table.hpp>
#include <aylo/luau/Value.hpp>
#include <aylo/luau/ValuePack.hpp>
#include <cmath>
#include <iostream>

using namespace aylo::luau;

int main() {
    auto runtime = Runtime::create({
        .libs = RuntimeConfig::Libs::kStandard,
        .optimizationLevel = RuntimeConfig::OptimizationLevel::kDisabled,
    });
    if (!runtime) {
        std::cerr << "failed to create Luau runtime: " << runtime.error().message() << std::endl;
        return 1;
    }
    auto script = runtime->load("examples/luau/CustomLibrary/script.luau");
    if (!script) {
        std::cerr << "failed to load script: " << script.error().message() << std::endl;
        return 1;
    }

    // A library is a table of functions. The builder creates the table, and `function` adds a native function to it
    // under the given name: it is bound like any other Function (see the FunctionBinding example). The library owns
    // its functions, so there is no need to keep them: the Library itself must stay alive as long as Luau may call
    // them.
    LibraryBuilder builder{*runtime};

    builder.setName("mathx");

    builder.function("clamp").bind([](const double value, const double min, const double max) {
        ValuePack results{};
        results.add({std::clamp(value, min, max)});
        return results;
    });

    builder.function("divide").bind([](const double dividend, const double divisor) {
        ValuePack results{};
        results.add({std::floor(dividend / divisor)});
        results.add({std::fmod(dividend, divisor)});
        return results;
    });

    const Library library = builder.build();

    // `build` hands over the finished library. Registering it sets its table as a global named after it, which makes
    // it reachable from every script, as `math` or `string` are. `library` must outlive every call made from Luau.
    if (auto result = runtime->globals().registerLibrary(library); !result) {
        std::cerr << "failed to register mathx: " << result.error().message() << std::endl;
        return 1;
    }

    if (auto result = runtime->run(*script); !result) {
        std::cerr << "failed to run script: " << result.error().message() << std::endl;
        return 1;
    }
}
