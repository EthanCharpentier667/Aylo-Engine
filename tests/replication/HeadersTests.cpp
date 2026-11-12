/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** HeadersTests
*/

// Includes every public header of the module.
#include <gtest/gtest.h>

#include "aylo/replication/Export.hpp"
#include "aylo/replication/MessageType.hpp"
#include "aylo/replication/Replication.hpp"
#include "aylo/replication/ReplicationPlugin.hpp"
#include "aylo/replication/ReplicationSettings.hpp"
#include "aylo/replication/component/NetworkId.hpp"
#include "aylo/replication/component/Owner.hpp"
#include "aylo/replication/component/PlayerInput.hpp"
#include "aylo/replication/event/ClientConnected.hpp"
#include "aylo/replication/event/ClientDisconnected.hpp"
#include "aylo/replication/exceptions/ReplicationExceptions.hpp"
#include "aylo/replication/input/InputBuffer.hpp"
#include "aylo/replication/input/InputKind.hpp"
#include "aylo/replication/input/InputLayout.hpp"
#include "aylo/replication/input/InputMessage.hpp"
#include "aylo/replication/manifest/Manifest.hpp"
#include "aylo/replication/manifest/ManifestMessages.hpp"
#include "aylo/replication/manifest/ManifestReceiver.hpp"
#include "aylo/replication/packet/PacketCalls.hpp"
#include "aylo/replication/packet/PacketCodec.hpp"
#include "aylo/replication/packet/PacketSchema.hpp"
#include "aylo/replication/serialization/EntityMapper.hpp"
#include "aylo/replication/serialization/FieldSerializer.hpp"

TEST(Headers, LibraryLinksAndIsNamed) { EXPECT_EQ(aylo::replication::libraryName(), "aylo-replication"); }
