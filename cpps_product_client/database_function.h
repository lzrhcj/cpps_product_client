#ifndef FUNCTION_H
#define FUNCTION_H

#define DB_CONFIG_DEBUG

#include <QCoreApplication>

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTextCodec>

#include <QDebug>
#include <deque>
#include <QDateTime>

#include "globalVariable.h"

#define DatabaseName "cpps_db"
#define HostName "mysql"
#define Port 3306
#define UserName "cpps"
#define Password "cpps"

using namespace std;

extern QSqlDatabase db;
extern string databaseConnectionName;

extern mutex mutexReadDatabase;

struct SOperationInfo
{
    int operation_id;
    string operation_guid_product;
    string operation_name;
    string operation_status;
};

extern deque<SOperationInfo> d_SOperationInfo;

/*获取数据库的连接状态*/
class CConnectDatabaseThread
{
public:
    CConnectDatabaseThread();
    ~CConnectDatabaseThread();
    static void run();
    void start() { _thread = std::thread(CConnectDatabaseThread::run); };

private:
    std::thread _thread;
};

/*读取数据库里的操作*/
class CReadOperationThread
{
public:
    CReadOperationThread();
    ~CReadOperationThread();
    volatile bool isReadOperation; //isStop是易失性变量，需要用volatile进行申明
    static void run(bool isReadOperation);
    void start() { _thread = std::thread(CReadOperationThread::run, isReadOperation); };

private:
    std::thread _thread;
};

/* 运行逻辑 */
/* 若有一个上料机在working的话，开启线程*/
class CWriteProductStatusThread
{
public:
    CWriteProductStatusThread();
    ~CWriteProductStatusThread();
    volatile bool isWriteProductStatus;
    static void run(bool isWriteProductStatus);
    void start() { _thread = std::thread(CWriteProductStatusThread::run, isWriteProductStatus); };

private:
    std::thread _thread;
};

class CThisProductStatusThread
{
public:
    CThisProductStatusThread();
    ~CThisProductStatusThread();
    volatile bool isThisProductStatus;
    static void run(CThisProductStatusThread *me);
    void start() { _thread = std::thread(CThisProductStatusThread::run, this); };

private:
    std::thread _thread;
    bool is_new_row = true;
    bool flag_operation_finished[10] = {false, false, false, false, false, false, false, false, false, false};
};

enum QUERYTYPE
{
    QUERY_INSERT,
    QUERY_UPDATE,
    QUERY_DELETE,
};

enum TABLENAME
{
    TABLE_EQUIPMENTINFO,
    TABLE_ORDERINFO,
    TABLE_OPERATIONINFO,
    TABLE_PRODUCTINFO,
};

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
string set_query(QUERYTYPE querytype, TABLENAME tablename, initializer_list<string> param_list);

#endif // FUNCTION_H
