#include "TestRunner.h"
#include "api/MeshMonitorClient.h"

using cardmesh::api::MeshMonitorClient;

CARDMESH_TEST_MAIN_BEGIN()

    CARDMESH_CHECK(MeshMonitorClient::sourcePath("default", "/nodes") == "/api/v1/sources/default/nodes");
    CARDMESH_CHECK(MeshMonitorClient::sourcePath("default", "/messages") ==
                    "/api/v1/sources/default/messages");
    CARDMESH_CHECK(MeshMonitorClient::sourcePath("doral base", "/nodes") ==
                    "/api/v1/sources/doral%20base/nodes");

CARDMESH_TEST_MAIN_END()
