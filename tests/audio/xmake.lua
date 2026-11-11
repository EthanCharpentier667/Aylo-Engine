-- The audio backends, through their contract: what the engine asks of them, with a device miniaudio opens (its null
-- backend where the machine has no sound card).
target("audio-tests")
    set_kind("binary")
    set_default(false)
    set_group("tests")
    add_deps("audio-miniaudio", "contract-audiodevice")
    add_files("*.cpp", "../main.cpp")
    add_packages("gtest")
    add_tests("default")
