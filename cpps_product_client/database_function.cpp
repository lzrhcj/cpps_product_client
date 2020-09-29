#include "database_function.h"


deque<SOperationInfo> d_SOperationInfo(500);

mutex mutexReadDatabase;

string databaseConnectionName = "public_cpps";

string thisProductGuid = "34848b50-0c40-11ea-8d72-02004c4f4f50";

MYSQL *db = NULL;

volatile bool g_bIsFinished = false;

int eqpt_num;


CConnectDatabaseThread::CConnectDatabaseThread()
{
    g_bIsMySQLConnected = false;
}

CConnectDatabaseThread::~CConnectDatabaseThread()
{
    g_bIsMySQLConnected = false;
}

void CConnectDatabaseThread::run()
{
    mysql_init(db);
if (!mysql_real_connect(db, HostName, UserName, Password, DatabaseName, Port, NULL, 0))
	{
		cout << "连接数据库失败"<< endl;
		cout << mysql_error(db) << endl;
        g_bIsMySQLConnected = false;
	}
    else
    {
        printf((char*)"连接数据库成功\n");
        g_bIsMySQLConnected = true;
    }

    while(g_bIsMainRunning)
    {
        g_bIsMySQLConnected = (db == NULL);
        sleep_for(5s); //每5秒查询连接状态

        if(!g_bIsMySQLConnected)
        {
            for(int i=1;i<=10;i++)
            {
                printf((char*)"重连数据库第%i次\n",i);
                g_bIsMySQLConnected = mysql_real_connect(db, HostName, UserName, Password, DatabaseName, Port, NULL, 0);//建立数据库连接
            }
        }
        else
        {
            g_bIsMySQLConnected = true;
        }
    }
}


CReadOperationThread::CReadOperationThread()
{
    isReadOperation = true;
}

CReadOperationThread::~CReadOperationThread()
{
    isReadOperation = false;
}

void CReadOperationThread::run(bool isReadOperation)
{
    while(g_bIsMainRunning == true)
    {
        while ((isReadOperation == true)&&(g_bIsUARunning == UA_STATUSCODE_GOOD))
        {
            mutexReadDatabase.lock();

            mysql_query(db, "SELECT * FROM cpps_db.tab_operationinfo;");
            auto result = mysql_store_result(db);

            int oper_num = mysql_num_rows(result);

#ifdef DB_CONFIG_DEBUG
            printf("操作表总共有%i行\n",oper_num);
#endif
            // int new_size = deque_equipment_ip_port.size();

            for (int i = 0; i <= oper_num; i++)
            {
				auto sql_row = mysql_fetch_row(result);

                deque<SOperationInfo>::iterator d_SOperationInfo_iterator = d_SOperationInfo.begin();

                advance(d_SOperationInfo_iterator, i);//迭代器前进i个元素，注意i是从0开始

                d_SOperationInfo_iterator->operation_id = stoi(sql_row[0]); //读取第1列字段：id
                d_SOperationInfo_iterator->operation_guid_product = sql_row[1]; //读取第2列字段：guid_product
                d_SOperationInfo_iterator->operation_name = sql_row[2]; //读取第3列字段：operation_name
                d_SOperationInfo_iterator->operation_status = sql_row[3];//读取第5列字段：operation_status

#ifdef DB_CONFIG_DEBUG
                // qDebug()<<d_SOperationInfo_iterator->operation_id;
                // qDebug()<<d_SOperationInfo_iterator->operation_guid_product.c_str();
                // qDebug()<<d_SOperationInfo_iterator->operation_name.c_str();
                // qDebug()<<d_SOperationInfo_iterator->operation_status.c_str();
#endif
            }

            mutexReadDatabase.unlock();



            if(g_bIsFinished==true)
            {
                isReadOperation = false;
            }

            sleep_for(1s);
        }
        sleep_for(1s);
    }
}


CWriteProductStatusThread::CWriteProductStatusThread()
{
    isWriteProductStatus =true;
}

CWriteProductStatusThread::~CWriteProductStatusThread()
{
    isWriteProductStatus=false;
}

void CWriteProductStatusThread::run(bool isWriteProductStatus)
{
    while(g_bIsMainRunning == true)
    {
        while((isWriteProductStatus == true)&&(g_bIsUARunning == UA_STATUSCODE_GOOD))
        {
            //TODO 这里有错误
            CThisProductStatusThread MyThisProductStatusThread;

            for(int y=0;y<=499;y++)
            {
                //当上料机开始操作这个产品的时候，开始线程
                if ((d_SOperationInfo.at(y).operation_guid_product == thisProductGuid)&&
                        (d_SOperationInfo.at(y).operation_name=="Operation_LB")&&
                        (d_SOperationInfo.at(y).operation_status=="working"))
                {

                    printf("开始了！开始了！");

                    MyThisProductStatusThread.start();

                    while((g_bIsFinished==false)&&(g_bIsUARunning == UA_STATUSCODE_GOOD))
                    {
                        sleep_for(1s);
                    }


                    MyThisProductStatusThread.isThisProductStatus = false;
                    isWriteProductStatus =false;

                    //等一会
                    sleep_for(2s);

                    //完成了就把线程删了
                }
            }

            sleep_for(1s);
        }

        sleep_for(1s);
    }

}

CThisProductStatusThread::CThisProductStatusThread()
{
    isThisProductStatus =true;
    is_new_row = false;

    for(int u=0;u<=9;u++)
    {
        flag_operation_finished[u]=false;
    }


}

CThisProductStatusThread::~CThisProductStatusThread()
{
    isThisProductStatus=false;
}

void CThisProductStatusThread::run(CThisProductStatusThread* me)
{
    while(me->isThisProductStatus)
    {
        if (me->is_new_row==false)
        {

            string str_insertProduct_query=set_query(QUERY_INSERT,TABLE_PRODUCTINFO,
            {"Guid_product","product_status",
             "Start_time",
             "feature_1","feature_1_status",

             thisProductGuid,"working",
             "NOW()",
             g_str_productFeatureDescription[0],"working",} );

            printf("%s\n",str_insertProduct_query.c_str());

            printf("%s\n",thisProductGuid.c_str());

            mutexReadDatabase.lock();


            mysql_query(db, str_insertProduct_query.c_str());

            mutexReadDatabase.unlock();

            for(int u=2;u<=g_productFeatureSize+1;u++)
            {
                string str_updateProduct_query_part_1 = "feature_"+to_string(u);

                string str_updateProduct_query_part_2 = "feature_"+to_string(u)+"_status";

                //UPDATE `cpps_db`.`tab_productinfo` SET `feature_1_status` = '11111' WHERE (`id` = '1');

                string str_updateProduct_query=set_query(QUERY_UPDATE,TABLE_PRODUCTINFO,
                {str_updateProduct_query_part_1,g_str_productFeatureDescription[u-1],
                 str_updateProduct_query_part_2,"waiting",
                 "Guid_product",thisProductGuid});

                printf("%s\n",str_updateProduct_query.c_str());

                mutexReadDatabase.lock();

                mysql_query(db, str_updateProduct_query.c_str());

                mutexReadDatabase.unlock();
            }

            me->is_new_row =true;

        }

        for(int i=0;i<=499;i++)
        {

            if (me->flag_operation_finished[0]==false)
            {

                if((d_SOperationInfo.at(i).operation_guid_product==thisProductGuid)&&
                        (d_SOperationInfo.at(i).operation_name=="Operation_LB")&&
                        (d_SOperationInfo.at(i).operation_status=="done"))
                {
                    string str_updateProduct_query=set_query(QUERY_UPDATE,TABLE_PRODUCTINFO,
                    {"feature_1_status","done",
                     "Guid_product",thisProductGuid});

                    printf("%s\n",str_updateProduct_query.c_str());

                    mutexReadDatabase.lock();

                    mysql_query(db, str_updateProduct_query.c_str());

                    mutexReadDatabase.unlock();

                    me->flag_operation_finished[0]= true;
                }
            }

            if (me->flag_operation_finished[1]==false)
            {
                if((d_SOperationInfo.at(i).operation_guid_product==thisProductGuid)&&
                        (d_SOperationInfo.at(i).operation_name=="Operation_LM")&&
                        (d_SOperationInfo.at(i).operation_status=="working"))
                {
                    string str_updateProduct_query=set_query(QUERY_UPDATE,TABLE_PRODUCTINFO,
                    {"feature_2_status","working",
                     "Guid_product",thisProductGuid});


                    printf("%s\n",str_updateProduct_query.c_str());

                    mutexReadDatabase.lock();

                    mysql_query(db, str_updateProduct_query.c_str());

                    mutexReadDatabase.unlock();

                }

                if((d_SOperationInfo.at(i).operation_guid_product==thisProductGuid)&&
                        (d_SOperationInfo.at(i).operation_name=="Operation_LM")&&
                        (d_SOperationInfo.at(i).operation_status=="done"))
                {
                    string str_updateProduct_query=set_query(QUERY_UPDATE,TABLE_PRODUCTINFO,
                    {"feature_2_status","done",
                     "Guid_product",thisProductGuid});


                    printf("%s\n",str_updateProduct_query.c_str());

                    mutexReadDatabase.lock();

                    mysql_query(db, str_updateProduct_query.c_str());

                    mutexReadDatabase.unlock();

                    me->flag_operation_finished[1]= true;
                }
            }

            if(me->flag_operation_finished[2]==false)
            {

                if((d_SOperationInfo.at(i).operation_guid_product==thisProductGuid)&&
                        (d_SOperationInfo.at(i).operation_name=="Operation_machine")&&
                        (d_SOperationInfo.at(i).operation_status=="working"))
                {
                    string str_updateProduct_query=set_query(QUERY_UPDATE,TABLE_PRODUCTINFO,
                    {"feature_3_status","working",
                     "Guid_product",thisProductGuid});

                    printf("%s\n",str_updateProduct_query.c_str());

                    mutexReadDatabase.lock();

                    mysql_query(db, str_updateProduct_query.c_str());

                    mutexReadDatabase.unlock();

                }

                if((d_SOperationInfo.at(i).operation_guid_product==thisProductGuid)&&
                        (d_SOperationInfo.at(i).operation_name=="Operation_laser")&&
                        (d_SOperationInfo.at(i).operation_status=="working"))
                {
                    string str_updateProduct_query=set_query(QUERY_UPDATE,TABLE_PRODUCTINFO,
                    {"feature_3_status","working",
                     "Guid_product",thisProductGuid});

                    printf("%s\n",str_updateProduct_query.c_str());

                    mutexReadDatabase.lock();

                    mysql_query(db, str_updateProduct_query.c_str());

                    mutexReadDatabase.unlock();

                }

                if((d_SOperationInfo.at(i).operation_guid_product==thisProductGuid)&&
                        (d_SOperationInfo.at(i).operation_name=="Operation_machine")&&
                        (d_SOperationInfo.at(i).operation_status=="done"))
                {
                    string str_updateProduct_query=set_query(QUERY_UPDATE,TABLE_PRODUCTINFO,
                    {"feature_3_status","done",
                     "Guid_product",thisProductGuid});

                    printf("%s\n",str_updateProduct_query.c_str());

                    mutexReadDatabase.lock();

                    mysql_query(db, str_updateProduct_query.c_str());

                    mutexReadDatabase.unlock();

                    me->flag_operation_finished[2]= true;
                }

                if((d_SOperationInfo.at(i).operation_guid_product==thisProductGuid)&&
                        (d_SOperationInfo.at(i).operation_name=="Operation_laser")&&
                        (d_SOperationInfo.at(i).operation_status=="done"))
                {
                    string str_updateProduct_query=set_query(QUERY_UPDATE,TABLE_PRODUCTINFO,
                    {"feature_3_status","done",
                     "Guid_product",thisProductGuid});

                    printf("%s\n",str_updateProduct_query.c_str());

                    mutexReadDatabase.lock();

                    mysql_query(db, str_updateProduct_query.c_str());

                    mutexReadDatabase.unlock();

                    me->flag_operation_finished[2]= true;
                }
            }

            if (me->flag_operation_finished[3]==false)
            {
                if((d_SOperationInfo.at(i).operation_guid_product==thisProductGuid)&&
                        (d_SOperationInfo.at(i).operation_name=="Operation_package")&&
                        (d_SOperationInfo.at(i).operation_status=="working"))
                {
                    string str_updateProduct_query=set_query(QUERY_UPDATE,TABLE_PRODUCTINFO,
                    {"feature_4_status","working",
                     "Guid_product",thisProductGuid});


                    printf("%s\n",str_updateProduct_query.c_str());

                    mutexReadDatabase.lock();

                    mysql_query(db, str_updateProduct_query.c_str());

                    mutexReadDatabase.unlock();

                }

                if((d_SOperationInfo.at(i).operation_guid_product==thisProductGuid)&&
                        (d_SOperationInfo.at(i).operation_name=="Operation_package")&&
                        (d_SOperationInfo.at(i).operation_status=="done"))
                {
                    string str_updateProduct_query=set_query(QUERY_UPDATE,TABLE_PRODUCTINFO,
                    {"feature_4_status","done",
                     "Guid_product",thisProductGuid});


                    printf("%s\n",str_updateProduct_query.c_str());

                    mutexReadDatabase.lock();

                    mysql_query(db, str_updateProduct_query.c_str());

                    mutexReadDatabase.unlock();

                    me->flag_operation_finished[3]= true;
                }
            }

            if (me->flag_operation_finished[4]==false)
            {
                if((d_SOperationInfo.at(i).operation_guid_product==thisProductGuid)&&
                        (d_SOperationInfo.at(i).operation_name=="Operation_LC")&&
                        (d_SOperationInfo.at(i).operation_status=="working"))
                {
                    string str_updateProduct_query=set_query(QUERY_UPDATE,TABLE_PRODUCTINFO,
                    {"feature_5_status","working",
                     "Guid_product",thisProductGuid});


                    printf("%s\n",str_updateProduct_query.c_str());

                    mutexReadDatabase.lock();

                    mysql_query(db, str_updateProduct_query.c_str());

                    mutexReadDatabase.unlock();
                }

                if((d_SOperationInfo.at(i).operation_guid_product==thisProductGuid)&&
                        (d_SOperationInfo.at(i).operation_name=="Operation_LC")&&
                        (d_SOperationInfo.at(i).operation_status=="done"))
                {
                    string str_updateProduct_query=set_query(QUERY_UPDATE,TABLE_PRODUCTINFO,
                    {"feature_5_status","done",
                     "Guid_product",thisProductGuid});


                    printf("%s\n",str_updateProduct_query.c_str());

                    mutexReadDatabase.lock();

                    mysql_query(db, str_updateProduct_query.c_str());

                    mutexReadDatabase.unlock();

                    me->flag_operation_finished[4]= true;
                }
            }

            if (me->flag_operation_finished[5]==false)
            {
                if((d_SOperationInfo.at(i).operation_guid_product==thisProductGuid)&&
                        (d_SOperationInfo.at(i).operation_name=="Operation_LC")&&
                        (d_SOperationInfo.at(i).operation_status=="working"))
                {
                    string str_updateProduct_query=set_query(QUERY_UPDATE,TABLE_PRODUCTINFO,
                    {"feature_6_status","working",
                     "Guid_product",thisProductGuid});

                    printf("%s\n",str_updateProduct_query.c_str());

                    mutexReadDatabase.lock();

                    mysql_query(db, str_updateProduct_query.c_str());

                    mutexReadDatabase.unlock();

                }


                if((d_SOperationInfo.at(i).operation_guid_product==thisProductGuid)&&
                        (d_SOperationInfo.at(i).operation_name=="Operation_LC")&&
                        (d_SOperationInfo.at(i).operation_status=="done"))
                {
                    string str_updateProduct_query=set_query(QUERY_UPDATE,TABLE_PRODUCTINFO,
                    {"feature_6_status","done",
                     "Guid_product",thisProductGuid});


                    printf("%s\n",str_updateProduct_query.c_str());

                    mutexReadDatabase.lock();

                    mysql_query(db, str_updateProduct_query.c_str());

                    mutexReadDatabase.unlock();

                    me->flag_operation_finished[5]= true;
                }
            }
        }

        if( (me->flag_operation_finished[0]==true)&&
                 (me->flag_operation_finished[1]==true)&&
                 (me->flag_operation_finished[2]==true)&&
                 (me->flag_operation_finished[3]==true)&&
                 (me->flag_operation_finished[4]==true)&&
                 (me->flag_operation_finished[5]==true))
        {
            string str_updateProduct_query=set_query(QUERY_UPDATE,TABLE_PRODUCTINFO,
            {"product_status","done",
             "End_time",
             "NOW()",
             "Guid_product",thisProductGuid});

            printf("%s\n",str_updateProduct_query.c_str());

            mutexReadDatabase.lock();

            mysql_query(db, str_updateProduct_query.c_str());

            mutexReadDatabase.unlock();

            me->isThisProductStatus=false;

            g_bIsFinished = true;

        }
        sleep_for(1s);
    }
}


/*由这个函数统一生成query语句
 * QUERYTYPE querytype 对数据库操作类型，插入、更新、删除
 * TABLENAME tablename 操作数据库表的名字，table_equipmentinfo、table_orderinfo、table_operationinfo
 * initializer_list<string>param_list 一系列可变参数数组
 * param_list 对INSERT输入的格式为，{列1名字，列2名字...列n名字，列1的值，列2的值...列n的值}
 * param_list 对UPDATE输入的格式为，{列1名字，列1的值，列2名字，列2的值，列n名字...列n的值，该行序号}
 * param_list 对DELETE输入的格式为，{该行序号}
 * 使用范例：
 * string query1;
 * string query2;
 * string query3;
 * query1 = set_query(QUERY_INSERT, TABLE_EQUIPMENTINFO,{ "equipment_id","IP_port","equipment_type","1","192.168.1.1","plc"});
 * query2 = set_query(QUERY_UPDATE, TABLE_EQUIPMENTINFO,{ "equipment_id","2","IP_port","192.168.1.103","equipment_type","plc","2"});
 * query3 = set_query(QUERY_DELETE, TABLE_EQUIPMENTINFO, {"2"});
 * "INSERT INTO `cpps_db`.`table_equipmentinfo` (`equipment_id`, `IP_port`, `equipment_guid`, `equipment_type`, `equipment_type_subno`, `equipment_name`, `skill`, `reach area`, `connected`, `available`) VALUES ('19', 'opc.tcp://192.168.137.203:4840', 'opc.tcp://192.168.137.203:4840', '0', '1', '0', '00', '0', '0', '0');"
 * "UPDATE `cpps_db`.`table_equipmentinfo` SET `IP_port` = '电话费', `equipment_guid` = '发的广泛地', `equipment_type` = '电饭锅', `equipment_type_subno` = '梵蒂冈', `equipment_name` = '地方', `skill` = '电饭锅', `reach area` = '电饭锅', `connected` = '大改', `available` = '电饭锅' WHERE (`equipment_id` = '1453');"
 * "DELETE FROM `cpps_db`.`table_equipmentinfo` WHERE (`equipment_id` = '1453');"
 */
string set_query(QUERYTYPE querytype, TABLENAME tablename, initializer_list<string> param_list)
{
    string query_part1;
    string query_part2;
    string query_part3;
    string query;

    int size_param_list;
    size_param_list = param_list.size();

    switch (querytype)
    {
    case QUERY_INSERT:
    {
        query_part1 = "INSERT INTO `cpps_db`.`";
        switch (tablename)
        {
        case TABLE_EQUIPMENTINFO:
            query_part2 = "tab_equipmentinfo` (`";
            break;
        case TABLE_ORDERINFO:
            query_part2 = "tab_orderinfo` (`";
            break;
        case TABLE_OPERATIONINFO:
            query_part2 = "tab_operationinfo` (`";
            break;
        case TABLE_PRODUCTINFO:
            query_part2 = "tab_productinfo` (`";
            break;
        }

        for (int i = 1; i <= size_param_list / 2; i++)
        {
            initializer_list<string>::iterator param_list_iterator = param_list.begin();
            advance(param_list_iterator, i - 1);
            query_part3 += *param_list_iterator;
            if (i != size_param_list / 2)
            {
                query_part3 += "`, `";
            }
        }

        query_part3 += "`) VALUES ('";

        for (int i = (size_param_list / 2) + 1; i <= size_param_list; i++)
        {
            initializer_list<string>::iterator param_list_iterator = param_list.begin();
            advance(param_list_iterator, i - 1);
            query_part3 += *param_list_iterator;
            if (i != size_param_list)
            {
                query_part3 += "', '";
            }
        }
    }
        break;

    case QUERY_UPDATE:
    {
        query_part1 = "UPDATE `cpps_db`.`";
        switch (tablename)
        {
        case TABLE_EQUIPMENTINFO:
            query_part2 = "tab_equipmentinfo` SET `";
            break;
        case TABLE_ORDERINFO:
            query_part2 = "tab_orderinfo` SET `";
            break;
        case TABLE_OPERATIONINFO:
            query_part2 = "tab_operationinfo` SET `";
            break;
        case TABLE_PRODUCTINFO:
            query_part2 = "tab_productinfo` SET `";
            break;
        }

        for (int i = 1; i <= (size_param_list - 2); i++)
        {
            initializer_list<string>::iterator param_list_iterator = param_list.begin();
            advance(param_list_iterator, i - 1);
            query_part3 += *param_list_iterator;

            if (i % 2 == 0)
            {
                //i是偶数时
                if (i != (size_param_list - 2))
                {
                    query_part3 += "', `";
                }
            }
            else
            {
                //i是奇数时
                query_part3 += "` = '";
            }
        }

        query_part3 += "' WHERE (`";

        //WHERE (`equipment_id` = '1453');"

        for (int i = (size_param_list - 1); i <= size_param_list; i++)
        {
            initializer_list<string>::iterator param_list_iterator = param_list.begin();
            advance(param_list_iterator, i - 1);
            query_part3 += *param_list_iterator;

            if(i == (size_param_list - 1))
            {
                query_part3 += "` = '";
            }

        }

    }
        break;

    case QUERY_DELETE:
    {
        query_part1 = "DELETE FROM `cpps_db`.`";

        switch (tablename)
        {
        case TABLE_EQUIPMENTINFO:
            query_part2 = "tab_equipmentinfo` WHERE (`";
            break;
        case TABLE_ORDERINFO:
            query_part2 = "tab_orderinfo` WHERE (`";
            break;
        case TABLE_OPERATIONINFO:
            query_part2 = "tab_operationinfo` WHERE (`";
            break;
        case TABLE_PRODUCTINFO:
            query_part2 = "tab_productinfo` WHERE (`";
            break;
        }

        query_part3 += *param_list.begin();
        query_part3 += "` = '";
        query_part3 += *param_list.end();

    }
        break;

    }

    query_part3 += "');";

    query = query_part1 + query_part2 + query_part3;

    return query;

}


