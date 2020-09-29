#ifndef UA_FUNCTION_H
#define UA_FUNCTION_H

#include "open62541.h"

#include "globalVariable.h"

typedef struct productFeatuerDescriptionBrowseName
{
    int namespaceIndex;
    UA_UInt32 referenceType;
    string str_productFeatuerDescriptionBrowseName;
} PFDBN;

extern PFDBN myPFDBN[14];
extern UA_Client *client;

/*获取UA的连接状态*/
class CConnectUAThread
{
public:
    CConnectUAThread();
    ~CConnectUAThread();
    static void run();
    void start() { _thread = std::thread(run); };

private:
    std::thread _thread;
};

UA_StatusCode translateBrowsePathsToNodeIdsRequest(UA_Client *client,
                                                   UA_TranslateBrowsePathsToNodeIdsRequest request,
                                                   UA_NodeId *returnId);

int getProductType(UA_Client *client, string productTypeResult);

int getProductFeatureDescription(UA_Client *client, int featureSerialNumber);

#endif // UA_FUNCTION_H
