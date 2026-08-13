#include <nlohmann/json.hpp>

#include "TestRunner.h"
#include "models/Channel.h"
#include "models/Message.h"
#include "models/Node.h"
#include "models/Source.h"
#include "models/Telemetry.h"
#include "models/Traceroute.h"

using namespace cardmesh::models;

CARDMESH_TEST_MAIN_BEGIN()

    {
        const auto source = Source::fromJson(nlohmann::json{{"id", "default"}, {"name", "Doral Base"}, {"online", true}});
        CARDMESH_CHECK(source.id == "default");
        CARDMESH_CHECK(source.name == "Doral Base");
        CARDMESH_CHECK(source.online);
    }

    {
        // RSSI/SNR/battery are missing -> must not be fabricated as zero.
        const auto node = Node::fromJson(nlohmann::json{{"id", "!a13f829c"}, {"shortName", "MTEDC"}});
        CARDMESH_CHECK(node.id == "!a13f829c");
        CARDMESH_CHECK(!node.rssi.has_value());
        CARDMESH_CHECK(!node.snr.has_value());
        CARDMESH_CHECK(!node.batteryLevel.has_value());
    }

    {
        const auto node = Node::fromJson(nlohmann::json{{"id", "x"}, {"hopCount", 0}, {"rssi", -87.0}});
        CARDMESH_CHECK(node.isDirect());
        CARDMESH_CHECK(node.rssi.has_value() && *node.rssi == -87.0);
    }

    {
        const auto channel = Channel::fromJson(nlohmann::json{{"id", "1"}, {"name", "FloridaMesh"}, {"unreadCount", 4}});
        CARDMESH_CHECK(channel.name == "FloridaMesh");
        CARDMESH_CHECK(channel.unreadCount == 4);
    }

    {
        const auto message = Message::fromJson(
            nlohmann::json{{"id", "m1"}, {"conversationType", "DM"}, {"state", "ACK"}, {"text", "hi"}});
        CARDMESH_CHECK(message.conversationType == ConversationType::DirectMessage);
        CARDMESH_CHECK(message.state == MessageState::Ack);
        CARDMESH_CHECK(message.text == "hi");
    }

    {
        const auto telemetry = Telemetry::fromJson(nlohmann::json{{"nodeId", "x"}, {"batteryLevel", 92}});
        CARDMESH_CHECK(telemetry.batteryLevel.has_value() && *telemetry.batteryLevel == 92);
        CARDMESH_CHECK(!telemetry.temperature.has_value());
    }

    {
        const auto traceroute =
            Traceroute::fromJson(nlohmann::json{{"targetNodeId", "t"}, {"hops", {"a", "b"}}});
        CARDMESH_CHECK(traceroute.hopCount() == 2);
    }

CARDMESH_TEST_MAIN_END()
