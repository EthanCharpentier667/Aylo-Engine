/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** LibraryEntryTests
*/

#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "core/library/LibraryEntry.hpp"

using aylo::core::library::LibraryRegistrar;

TEST(LibraryRegistrar, FindsWhatALibraryProvides) {
    LibraryRegistrar registrar;
    int service = 4;
    EXPECT_EQ(registrar.find<int>("test.service"), nullptr);
    registrar.provide("test.service", service);
    ASSERT_EQ(registrar.find<int>("test.service"), &service);
    EXPECT_EQ(registrar.find<int>("test.other"), nullptr);
}

TEST(LibraryRegistrar, ListsTheNamesOfTheRegistriesThatOfferThem) {
    LibraryRegistrar registrar;
    std::vector<std::string> names{"glfw"};
    EXPECT_TRUE(registrar.listNames().empty());
    registrar.provideNames("test.platforms", [&names] { return names; });
    names.emplace_back("sdl");  // read when listed, not when offered
    const auto listed = registrar.listNames();
    ASSERT_EQ(listed.size(), 1U);
    EXPECT_EQ(listed.at("test.platforms"), (std::vector<std::string>{"glfw", "sdl"}));
}
