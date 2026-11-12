add_requires("tl_expected")

target("ecs")
    set_kind("shared")
    set_basename("aylo-ecs")
    add_files("**.cpp")
    add_headerfiles("**.hpp", "**.tpp")
    -- Public: the type headers include core/hash/NameHash.hpp.
    add_deps("core", {public = true})
    add_defines("AYLO_ECS_BUILD", {public = false})
    -- Headers are included as "aylo/ecs/...".
    add_includedirs("$(projectdir)/src", {public = true})
    -- Public: Result (tl::expected) appears in the public headers.
    add_packages("tl_expected", {public = true})
    -- AYLO_ECS_COMPONENT relies on __VA_OPT__, which MSVC only supports with its conforming preprocessor.
    -- Public: the macro expands in the files of every module that declares components.
    add_cxflags("/Zc:preprocessor", {tools = {"cl"}, public = true})
