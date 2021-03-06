#ifndef GLOBALVARIABLE_H
#define GLOBALVARIABLE_H


#include "open62541.h"

#include <string>

#include <iostream>
#include <stdio.h>
#include <chrono>
#include <thread>
#include <mutex>

using namespace std;
using namespace std::chrono_literals;
using std::this_thread::sleep_for;


extern string opcuaIP;
extern string opcuaPort;


extern volatile UA_StatusCode g_bIsUARunning;
extern volatile bool g_bIsMySQLConnected;
extern volatile bool g_bIsMainRunning;

extern volatile bool g_bIsUAGetInfo;

extern volatile bool g_bIsFinished;


extern int g_productTypeNum;

extern int g_productFeatureSize;

extern UA_NodeId g_nodeID_productFeatureDescriptionBrowsePath[11];


extern string g_productGuid;
extern string g_productType;
extern int g_ret_getProductFeatureDescription[10];
extern string g_str_productFeatureDescription[10];
extern UA_NodeId g_nodeID_productFeatureDescription[10];
extern UA_Variant g_var_productFeatureDescription[10];


void clearAllGlobalVariable();


#endif // GLOBALVARIABLE_H
