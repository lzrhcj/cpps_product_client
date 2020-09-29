
#include "ua_function.h"


string opcuaIP;
string opcuaPort;

int g_productTypeNum;
int g_productFeatureNum;


UA_NodeId g_nodeID_productFeatureDescriptionBrowsePath[11];

string g_productType;
int g_ret_getProductFeatureDescription[10];
string g_str_productFeatureDescription[10];

UA_NodeId g_nodeID_productFeatureDescription[10];
UA_Variant g_var_productFeatureDescription[10];


UA_Client *client;

//typedef struct productFeatuerDescriptionBrowseName
//{
//    int namespaceIndex;
//    UA_UInt32 referenceType;
//    string str_productFeatuerDescriptionBrowseName;
//}PFDBN;


PFDBN myPFDBN[14] =
{
    {0,UA_NS0ID_ROOTFOLDER,"Root"},
    {0,UA_NS0ID_ORGANIZES,"Objects"},
    {2,UA_NS0ID_ORGANIZES,"AMLFiles"},
    {3,UA_NS0ID_ORGANIZES,"amlprojname"},
    {3,UA_NS0ID_HASCOMPONENT,"InstanceHierarchies"},
    {3,UA_NS0ID_HASCOMPONENT,"AssetAdministrationShellInstanceHierarchy"},
    {3,UA_NS0ID_HASCOMPONENT,"Product"},
    {3,UA_NS0ID_HASCOMPONENT,""},//这是产品的类型，Box_Wood、Box_UDisk、Box_Bluetooth三者之一
    {3,UA_NS0ID_HASCOMPONENT,"AAS"},
    {3,UA_NS0ID_HASCOMPONENT,"ProductFeature"},
    {3,UA_NS0ID_HASCOMPONENT,"Feature_"},//这里要加参数
    {3,UA_NS0ID_HASCOMPONENT,"FeatureDescription"},
    {3,UA_NS0ID_HASCOMPONENT,"value"},
    {3,UA_NS0ID_HASPROPERTY,"Value"},
};

CConnectUAThread::CConnectUAThread()
{
    g_bIsUARunning = -1;
}

CConnectUAThread::~CConnectUAThread()
{
    g_bIsUARunning = -1;
}

void CConnectUAThread::run()
{
    //不断循环，要是没有在连接状态的话，新建连接，不断查询连接状态
    //若状态有变的话，删除连接，重新建一个
    client = UA_Client_new();

    UA_ClientConfig_setDefault(UA_Client_getConfig(client));

    while(g_bIsMainRunning)
    {

        string url="opc.tcp://"+ opcuaIP + ":"+ opcuaPort;

        g_bIsUARunning = UA_Client_connect(client, url.c_str());
        UA_Client_run_iterate(client,0);
        sleep_for(2s);

    }

}

UA_StatusCode translateBrowsePathsToNodeIdsRequest(UA_Client *client,
                                                   UA_TranslateBrowsePathsToNodeIdsRequest request,
                                                   UA_NodeId *returnId)
{

    UA_StatusCode ret = UA_STATUSCODE_GOOD;

    UA_TranslateBrowsePathsToNodeIdsResponse response = UA_Client_Service_translateBrowsePathsToNodeIds(client, request);

    if (response.responseHeader.serviceResult == UA_STATUSCODE_GOOD)
    {
        if (response.resultsSize == 1 && response.results[0].targetsSize == 1)
        {
            UA_NodeId_copy(&response.results[0].targets[0].targetId.nodeId, returnId);
        }
        else
        {
            //找不到的话返回-1
            ret = -1;
        }
    }
    else
    {
        printf("Error: %s\n", UA_StatusCode_name(response.responseHeader.serviceResult));
        ret = response.responseHeader.serviceResult;
    }

    //UA_TranslateBrowsePathsToNodeIdsRequest_delete(&request);
    //UA_TranslateBrowsePathsToNodeIdsResponse_delete(&response);

    return ret;
}


int getProductType(UA_Client *client ,string productTypeResult )
{
    myPFDBN[7].str_productFeatuerDescriptionBrowseName = productTypeResult;

    int ret_getProductType = 0;

    for(int i=0;i<=4;i++)
    {
        char *paths[3] = {(char*)myPFDBN[i+1].str_productFeatuerDescriptionBrowseName.c_str(),
                          (char*)myPFDBN[i+2].str_productFeatuerDescriptionBrowseName.c_str(),
                          (char*)myPFDBN[i+3].str_productFeatuerDescriptionBrowseName.c_str()};
        if(i==4)
        {
            paths[2] = (char*)myPFDBN[7].str_productFeatuerDescriptionBrowseName.c_str();
        }

        UA_UInt32 ids[3] = {myPFDBN[i+1].referenceType,
                            myPFDBN[i+2].referenceType,
                            myPFDBN[i+3].referenceType};

        int nsNumOfQualifiedName[3] = {myPFDBN[i+1].namespaceIndex,
                                       myPFDBN[i+2].namespaceIndex,
                                       myPFDBN[i+3].namespaceIndex}; // namespace number of qualified name

        UA_BrowsePath browsePath;
        UA_BrowsePath_init(&browsePath);
        browsePath.relativePath.elements = (UA_RelativePathElement*)UA_Array_new(3, &UA_TYPES[UA_TYPES_RELATIVEPATHELEMENT]);
        browsePath.relativePath.elementsSize = 3;

        //起始节点
        if(i==0)
        {
            browsePath.startingNode = UA_NODEID_NUMERIC(0, UA_NS0ID_ROOTFOLDER);
        }
        if(i==1)
        {
            browsePath.startingNode = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
        }
        if(i==2)
        {
            browsePath.startingNode = UA_NODEID_NUMERIC(2, 5001);
        }
        if(i>=3)
        {
            browsePath.startingNode = g_nodeID_productFeatureDescriptionBrowsePath[i-3];
        }

        for(size_t i = 0; i < 3; ++i)
        {
            UA_RelativePathElement *elem = &browsePath.relativePath.elements[i];
            elem->referenceTypeId = UA_NODEID_NUMERIC(0, ids[i]);
            elem->targetName = UA_QUALIFIEDNAME_ALLOC(nsNumOfQualifiedName[i], paths[i]);
        }

        UA_TranslateBrowsePathsToNodeIdsRequest request;
        UA_TranslateBrowsePathsToNodeIdsRequest_init(&request);
        request.browsePaths = &browsePath;
        request.browsePathsSize = 1;

        int ret_translateBrowsePathsToNodeIdsRequest = 0;

        ret_translateBrowsePathsToNodeIdsRequest =
                translateBrowsePathsToNodeIdsRequest(client,request,&g_nodeID_productFeatureDescriptionBrowsePath[i]);

        if (ret_translateBrowsePathsToNodeIdsRequest == 0)
        {
            if (g_nodeID_productFeatureDescriptionBrowsePath[i].identifierType == UA_NODEIDTYPE_NUMERIC)
            {
                printf("\n");
                printf("==> 路径[%u]的NodeID为: %i\n", i, g_nodeID_productFeatureDescriptionBrowsePath[i].identifier.numeric);
                printf("\n");
            }
            if (g_nodeID_productFeatureDescriptionBrowsePath[i].identifierType == UA_NODEIDTYPE_STRING)
            {
                printf("\n");
                printf("==> 路径[%u]的NodeID为: %s\n", i,g_nodeID_productFeatureDescriptionBrowsePath[i].identifier.string.data);
                printf("\n");
            }
        }
        else
        {
            printf("==> 路径[%u]的NodeID获取失败\n", i);
            ret_getProductType = -1;
        }
    }
    return ret_getProductType;

}

int getProductFeatureDescription(UA_Client *client,int featureSerialNumber)
{
    int ret_getProductType = 0;

    myPFDBN[10].str_productFeatuerDescriptionBrowseName = "Feature_"+ to_string(featureSerialNumber+1);

    for(int i=5;i<=10;i++)
    {
        char *paths[3] = {(char*)myPFDBN[i+1].str_productFeatuerDescriptionBrowseName.c_str(),
                          (char*)myPFDBN[i+2].str_productFeatuerDescriptionBrowseName.c_str(),
                          (char*)myPFDBN[i+3].str_productFeatuerDescriptionBrowseName.c_str()};


        UA_UInt32 ids[3] = {myPFDBN[i+1].referenceType,
                            myPFDBN[i+2].referenceType,
                            myPFDBN[i+3].referenceType};

        int nsNumOfQualifiedName[3] = {myPFDBN[i+1].namespaceIndex,
                                       myPFDBN[i+2].namespaceIndex,
                                       myPFDBN[i+3].namespaceIndex}; // namespace number of qualified name

        UA_BrowsePath browsePath;
        UA_BrowsePath_init(&browsePath);
        browsePath.startingNode = g_nodeID_productFeatureDescriptionBrowsePath[i-3];
        browsePath.relativePath.elements = (UA_RelativePathElement*)UA_Array_new(3, &UA_TYPES[UA_TYPES_RELATIVEPATHELEMENT]);
        browsePath.relativePath.elementsSize = 3;


        for(size_t i = 0; i < 3; ++i)
        {
            UA_RelativePathElement *elem = &browsePath.relativePath.elements[i];
            elem->referenceTypeId = UA_NODEID_NUMERIC(0, ids[i]);
            elem->targetName = UA_QUALIFIEDNAME_ALLOC(nsNumOfQualifiedName[i], paths[i]);
        }

        UA_TranslateBrowsePathsToNodeIdsRequest request;
        UA_TranslateBrowsePathsToNodeIdsRequest_init(&request);
        request.browsePaths = &browsePath;
        request.browsePathsSize = 1;

        int ret_translateBrowsePathsToNodeIdsRequest = 0;

        ret_translateBrowsePathsToNodeIdsRequest =
                translateBrowsePathsToNodeIdsRequest(client,request,&g_nodeID_productFeatureDescriptionBrowsePath[i]);

        if (ret_translateBrowsePathsToNodeIdsRequest == 0)
        {
            if (g_nodeID_productFeatureDescriptionBrowsePath[i].identifierType == UA_NODEIDTYPE_NUMERIC)
            {
                printf("\n");
                printf("==> 路径[%u]的NodeID为: %i\n", i, g_nodeID_productFeatureDescriptionBrowsePath[i].identifier.numeric);
                printf("\n");
            }
            if (g_nodeID_productFeatureDescriptionBrowsePath[i].identifierType == UA_NODEIDTYPE_STRING)
            {
                printf("\n");
                printf("==> 路径[%u]的NodeID为: %s\n", i,g_nodeID_productFeatureDescriptionBrowsePath[i].identifier.string.data);
                printf("\n");
            }
        }
        else
        {
            printf("==> 路径[%u]的NodeID获取失败\n", i);
            ret_getProductType = -1;
        }
    }

    return ret_getProductType;
}

