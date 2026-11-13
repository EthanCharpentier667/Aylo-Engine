add_requires("asio 1.36.x", {alias = "asio"})

target("network")
    -- Static, linked into the executable: no plugin links it (aylo.static_modules enforces it).
    set_kind("static")
    set_basename("aylo-network")
    add_files("**.cpp")
    add_headerfiles("**.hpp", "**.tpp")
    add_defines("AYLO_NETWORK_BUILD", {public = false})
    add_defines("AYLO_NETWORK_STATIC", {public = true})
    -- Headers are included as "aylo/network/...".
    add_includedirs("$(projectdir)/src", {public = true})
    -- Private: no public header includes Asio or libsodium, so their users never see them. libsodium encrypts the
    -- transport (crypto/, docs/network/protocol.md).
    add_packages("asio", "libsodium", {public = false})
    if is_plat("windows") then
        add_defines("_WIN32_WINNT=0x0A00", {public = false})
        add_syslinks("ws2_32", "mswsock", {public = false})
    end
