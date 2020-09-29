#include "globalVariable.h"
#include "ua_function.h"
#include "database_function.h"
#include <signal.h>
#include <stdlib.h>

volatile UA_StatusCode g_bIsUARunning = -1;
volatile bool g_bIsMainRunning = true;
volatile bool g_bIsMySQLConnected = false;
volatile bool g_bIsUAGetInfo = false;

int g_productFeatureSize=0;

//static void stopHandler(int sig)
static void stopHandler(int sig)
{
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "received ctrl-c");
    g_bIsMainRunning = false;

}


/*--- 需要输入要连接的端口ip和端口号 ---*/

int main(int argc, char *argv[])
{

    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);



    if (argc == 1)
    {
        printf("比如：192.168.1.118");
        printf("请输入ip地址：\n");

        cin >> opcuaIP;

        printf("比如：4844");
        printf("请输入端口号：\n");

        cin >> opcuaPort;

    }

    if (argc == 2)
    {
        opcuaIP = argv[1];

        printf("比如：4844");
        printf("请输入端口号：\n");

        cin >> opcuaPort;

    }

    if (argc == 3)
    {
        opcuaIP = argv[1];
        opcuaPort = argv[2];

    }

    //线程常开，获取数据库的连接状态
    CConnectDatabaseThread MyConnectDatabaseThread;
    MyConnectDatabaseThread.start();

    //线程常开，获取UAServer的连接状态,并获取Server的信息
    CConnectUAThread MyConnectUAThread;
    MyConnectUAThread.start();

    //也先在这里开启线程，只是可能不会一直运行
    //读取操作表线程
    CReadOperationThread MyReadOperationThread;


    //运行产品操作逻辑线程
    CWriteProductStatusThread MyWriteProductStatusThread;


    //****************************************************//

    while (g_bIsMainRunning)
    {
        if(g_bIsUARunning == UA_STATUSCODE_GOOD)
        {
            printf("UAClient连接成功，状态为%i\n",g_bIsUARunning);
            sleep_for(2s);

            if(g_bIsUAGetInfo==false)
            {
                for(int t=0;t<=2;t++)
                {
                    string ProductType[3] = {"Box_Wood","Box_UDisk","Box_Bluetooth"};

                    int ret_getProductType = getProductType(client,ProductType[t]);

                    if(ret_getProductType ==0 )
                    {
                        g_productType = ProductType[t];

                        myPFDBN[7].str_productFeatuerDescriptionBrowseName = g_productType;

                        printf("\n");
                        printf("产品类型为%s\n",g_productType.c_str());
                        printf("\n");

                        break;
                    }
                }
                for(int x=0;x<=9;x++)
                {
                    g_ret_getProductFeatureDescription[x]=getProductFeatureDescription(client,x);

                    if(g_ret_getProductFeatureDescription[x] == 0)
                    {
                        UA_Variant_init(&g_var_productFeatureDescription[x]);
                        UA_Client_readValueAttribute(client,g_nodeID_productFeatureDescriptionBrowsePath[10],&g_var_productFeatureDescription[x]);
                        UA_String *productFeatureDescription = (UA_String*)g_var_productFeatureDescription[x].data;
                        g_str_productFeatureDescription[x] = (char*)productFeatureDescription->data;
                        printf("第%i个特征为：%s\n",x+1,g_str_productFeatureDescription[x].c_str());
                    }
                    else
                    {
                        g_productFeatureSize=x-1;

                        printf("总共有%i个特征\n",g_productFeatureSize+1);
                        break;
                    }

                }

                g_bIsUAGetInfo = true;

                g_bIsFinished = false;

                MyReadOperationThread.isReadOperation = true;
                MyReadOperationThread.start();

                MyWriteProductStatusThread.isWriteProductStatus = true;
                MyWriteProductStatusThread.start();

            }

        }
        else
        {
            //不能在线程运行的时候quit，会报错
            MyReadOperationThread.isReadOperation = false;
            MyWriteProductStatusThread.isWriteProductStatus =false;

            g_bIsUAGetInfo =false;

            clearAllGlobalVariable();

            printf("UAClient连接失败，状态为%i\n",g_bIsUARunning);
            sleep_for(3s);
        }
    }

    UA_Client_disconnect(client);

    UA_Client_delete(client);

    return 0;
}

void clearAllGlobalVariable()
{
    //清零，以免影响下一次
    g_productTypeNum = 0;
    g_productFeatureSize = 0;

    g_productType="";

    for(int i=0;i<=9;i++)
    {
        g_ret_getProductFeatureDescription[i] = 0;
        g_str_productFeatureDescription[i]="";
        UA_NodeId_clear(&g_nodeID_productFeatureDescription[i]);
        UA_Variant_clear(&g_var_productFeatureDescription[i]);
    }

    for(int i=0;i<=10;i++)
    {
        UA_NodeId_clear(&g_nodeID_productFeatureDescriptionBrowsePath[i]);
    }

}
