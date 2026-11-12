/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** HeadersTests
*/

// Includes every public header of the module. network-tests has no Asio include path, so this file fails to
// build if one of them includes Asio.
#include <gtest/gtest.h>

#include "aylo/network/Export.hpp"
#include "aylo/network/NetworkSettings.hpp"
#include "aylo/network/RandomSource.hpp"
#include "aylo/network/Types.hpp"
#include "aylo/network/protocol/Constants.hpp"
#include "aylo/network/protocol/Sequence.hpp"
#include "aylo/network/protocol/SequenceBuffer.hpp"

TEST(Headers, PublicHeadersCompileWithoutAsio) { SUCCEED(); }
