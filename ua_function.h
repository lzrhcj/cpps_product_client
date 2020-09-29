#ifndef UA_FUNCTION_H
#define UA_FUNCTION_H

#include "open62541.h"

#include "globalVariable.h"

#include <QThread>
#include <QMutex>



typedef struct productFeatuerDescriptionBrowseName
{
    int namespaceIndex;
    UA_UInt32 referenceType;
    string str_productFeatuerDescriptionBrowseName;
}PFDBN;


extern PFDBN myPFDBN[14];
extern UA_Client *client;


/*获取UA的连接状态*/
class CConnectUAThread : public QThread
{
public:
    CConnectUAThread();
    ~CConnectUAThread();
    virtual void run();

protected:

private:

};


UA_StatusCode translateBrowsePathsToNodeIdsRequest(UA_Client *client,
                                                   UA_TranslateBrowsePathsToNodeIdsRequest request,
                                                   UA_NodeId *returnId);


int getProductType(UA_Client *client,string productTypeResult);


int getProductFeatureDescription(UA_Client *client,int featureSerialNumber);






#endif // UA_FUNCTION_H
