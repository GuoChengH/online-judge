#pragma once
//这个是mysql版本
/*
编号
标题
难度
描述
时间（内部），空间（内部处理）

两批文件构成
1.question.list：题目列表：不需要出现题目描述
2.需要题目的描述，需要题目的预设置代码（header.cpp）,测试用例代码(tail.cpp)

这两个内容是通过题目的编号，产生关联的
*/
#pragma once
#include "../comm/log.hpp"

#include "../comm/util.hpp"

#include <cassert>
#include <stdio.h>
#include <fstream>
#include <iostream>
#include <stdlib.h>
#include <string>
#include <unordered_map>
#include <vector>
#include<sw/redis++/redis++.h>

#include"include/mysql.h"
// 根据题目list文件，加载所有信息到内存中
// model:主要用来和数据进行交互，对外提供访问数据的接口

namespace ns_model {
using namespace std;
using namespace ns_log;
using namespace ns_util;
class UserCenter
{
public:
  std::string name;
  std::string number;
  std::string password;
  std::string op;
  std::string level;
};
class Question {
public:
  std::string number; // 题目编号
  std::string title;  // 题目的标题
  std::string star;   // 难度：简单中等困难
  int cpu_limit;      // 时间要求 s
  int mem_limit;      // 空间要求 kb
  std::string desc;   // 题目的描述
  std::string header; // 题目预设给用户在线编辑器的代码
  std::string tail; // 题目的测试用例，需要和header拼接形成完整代码
};
///-------------------------------------
const std::string oj_questions ="oj_questions"; 
const std::string oj_user = "oj_user";
const std::string host = "127.0.0.1";
const std::string user = "oj_client";
const std::string passwd = "123456";
const std::string db = "db";
const int port = 3306;

///-------------------------------------------
const std::string redis_path = "tcp://127.0.0.1:6379";
const uint64_t redis_timeout = 10000;
///-------------------------------------------
class Model {
private:
  // 题号：题目细节
  unordered_map<string, Question> questions;
  //初始化的redis，操作该redis
  
  //MYSQL *my = mysql_init(nullptr);
public:
  Model()
  {
 
  }
  bool QueryMySql(const std::string &sql,vector<Question> *out)
  {
    //创建mysql句柄
    MYSQL *my = mysql_init(nullptr);
    //设置超时时间
    int timeout = 120;
    mysql_options(my, MYSQL_OPT_CONNECT_TIMEOUT, &timeout);
    //连接数据库
    if(mysql_real_connect(my,host.c_str(),user.c_str(),passwd.c_str(),db.c_str(),port,nullptr,0) == nullptr){
      LOG(FATAL)<<"连接数据库失败！"<<mysql_errno(my)<<"\n";
      return false;
    }
    //一定要设置该链接的编码格式默认是拉钉的
    mysql_set_character_set(my,"utf8");
    LOG(INFO)<<"连接数据库成功"<<"\n";

    //执行sql语句
    if(0 != mysql_query(my,sql.c_str()))
    {
      LOG(WARNING) << sql <<"execute error!"<<"\n";
      return false;
    }
    MYSQL_RES *res = mysql_store_result(my);
    //分析结果
    int rows = mysql_num_rows(res); //获得行数量
    int cols = mysql_num_fields(res);//获得列数量

    
    Question q;
    for(int i = 0;i<rows;i++)
    {
      MYSQL_ROW row = mysql_fetch_row(res);
      q.number = row[0];
      q.title = row[1];
      q.star = row[2];
      q.desc = row[3];
      q.header = row[4];
      q.tail = row[5];
      q.cpu_limit = atoi(row[6]);
      q.mem_limit = atoi(row[7]);

      out->push_back(q);
    }


    //释放结果空间
    free(res);
    //关闭mysql连接
    mysql_close(my);

    return true;
  }
  bool GetAllQuestion(vector<Question> *out) {
    std::string sql ="select *from ";
    sql += oj_questions;
    return QueryMySql(sql,out);
  }
  bool GetOneQuestion(const std::string &number, Question *q) {
    bool res = false;
    std::string sql = "select *from ";
    sql+=oj_questions;
    sql+= " where number=";
    sql+=number;
    vector<Question> result;
    if(QueryMySql(sql,&result))
    {
      if(result.size() == 1)
      {
        *q = result[0];
        res = true;
      }
    }
    return res;
  }
  
  bool UserRegister(const std::string& in_json,std::string* out_json)
  {
    //这里先对in_json反序列化
    Json::Reader reader;
    Json::Value in_value;
    reader.parse(in_json,in_value);
    std::string number = in_value["number"].asString();
    std::string name = in_value["name"].asString();
    std::string password = in_value["password"].asString();
    int limit = in_value["limit"].asInt();
    int level = in_value["level"].asInt();
    //判断账号密码是否可行
    std::string sql = " select *from ";
    sql+=oj_user;
    sql+=" where number=";
    sql+=number;

    //创建数据库
    MYSQL *my = mysql_init(nullptr);
    //连接数据库
    if(mysql_real_connect(my,host.c_str(),user.c_str(),passwd.c_str(),db.c_str(),port,nullptr,0) == nullptr)
    {
      LOG(WARNING)<<"连接到用户数据库失败"<<mysql_errno(my)<<"\n";
      return false;
    }
    //一定要记得设置该链接的编码格式
    mysql_set_character_set(my,"utf8");
    LOG(INFO)<<"连接到用户数据库成功"<<"\n";

    if(0 != mysql_query(my,sql.c_str())){
      LOG(WARNING)<< sql <<"execute error!"<<"\n";
      return false;
    }
    MYSQL_RES *res = mysql_store_result(my);
    if(mysql_num_rows(res) == 0)//获得行数量
    { 
      //当前输入的数据可以创建用户
      MYSQL_STMT *stmt = mysql_stmt_init(my);
      const char* query = "insert into oj_user values (?,?,?,?,?)";
      if(mysql_stmt_prepare(stmt,query,strlen(query)) != 0){
        LOG(WARNING)<<"stmt出现错误"<<"\n";
        mysql_stmt_close(stmt);
        mysql_close(my);
        return false;
      }
      //下面开始绑定
      MYSQL_BIND bind_params[5];
      memset(bind_params,0,sizeof bind_params);

      bind_params[0].buffer_type = MYSQL_TYPE_STRING;
      bind_params[0].buffer = (char*)number.c_str();
      bind_params[0].buffer_length = number.size();

      bind_params[1].buffer_type = MYSQL_TYPE_STRING;
      bind_params[1].buffer = (char*)name.c_str();
      bind_params[1].buffer_length = name.size();

      bind_params[2].buffer_type = MYSQL_TYPE_STRING;
      bind_params[2].buffer = (char*)password.c_str();
      bind_params[2].buffer_length = password.size();

      bind_params[3].buffer_type = MYSQL_TYPE_LONG;
      bind_params[3].buffer = &limit;
      bind_params[3].is_unsigned = 1;

      bind_params[4].buffer_type = MYSQL_TYPE_LONG;
      bind_params[4].buffer = &level;
      bind_params[4].is_unsigned = 1;
      
      if(mysql_stmt_bind_param(stmt,bind_params) !=0){
        LOG(WARNING) <<"绑定stmt参数出错"<<"\n";
        mysql_stmt_close(stmt);
        mysql_close(my);
        return false;
      }

      //执行插入语句
      if(mysql_stmt_execute(stmt)!=0){
        LOG(WARNING)<<"执行stmt语句的时候出现错误..."<<"\n";
        mysql_stmt_close(stmt);
        mysql_close(my);
        return false;
      }
      
      mysql_stmt_close(stmt);
      mysql_close(my);
      return true;
    }
    else{
      //服务器有重复的用户num ，不允许再创建了
      mysql_close(my);
      return false;
    }
    //保存到服务器

    //这里out_json暂时没有用，没有要返回的值
    mysql_close(my);
    return true;
  }
  bool UserLogin(const std::string& in_json,std::string* out_json)
  {
    Json::Value in_value;
    Json::Reader reader;
    reader.parse(in_json,in_value);
    std::string num = in_value["number"].asString();
    std::string password = in_value["password"].asString();

    MYSQL *my = mysql_init(nullptr);
    if(mysql_real_connect(my,host.c_str(),user.c_str(),passwd.c_str(),db.c_str(),port,nullptr,0) == nullptr)
    {
      LOG(WARNING)<<"连接到用户数据库失败"<<mysql_errno(my)<<"\n";
      return false;
    }
    //一定要记得设置该链接的编码格式
    mysql_set_character_set(my,"utf8");
    LOG(INFO)<<"连接懂啊用户数据库成功"<<"\n";

    std::string sql = " select * from ";
    sql += oj_user;
    sql +=" where number = ";
    sql += num;
    sql += " AND password = ";
    sql += password;
    

    LOG(DEBUG)<<"拼接的sql 语句："<<sql<<"\n";
    //mysql的查询语句
    if(mysql_query(my,sql.c_str()))
    {
      LOG(ERROR)<<"查询失败"<<"\n";
      mysql_close(my);
      return false;
    }
    else{
      //查询语句成功
      MYSQL_RES *result = mysql_store_result(my);;
      LOG(INFO)<<"查询成功"<<"\n";

      if(mysql_num_rows(result) == 0)
      {
        mysql_close(my);
        return false;
      }
      else{
        //查询到当前用户，且当前用户账号密码符合
        //1.生成唯一key
        std::string Skey = LoginSessionUtil::GetOnlySession(num);
        // 2.连接redis
        sw::redis::Redis redis(redis_path);
        redis.set(Skey.c_str(),num);
        redis.expire(Skey.c_str(),redis_timeout);
        LOG(DEBUG)<<"redis 查询结果："<<redis.get(Skey).value()<<"\n";

        mysql_close(my);
        
        return true;
      }
    }
    mysql_close(my);
    return true;
  }

  bool GetUserCenter(const std::string in_cookie,UserCenter &in_user)
  { 
    std::string cookie = in_cookie;
    LOG(DEBUG)<<"进入到getuserCenter函数"<<"\n";
    sw::redis::Redis redis(redis_path);
    LOG(DEBUG)<<"初始化redis函数"<<"\n";
    MYSQL *my = mysql_init(nullptr);

    if(mysql_real_connect(my,host.c_str(),user.c_str(),passwd.c_str(),db.c_str(),port,nullptr,0) == nullptr)
    {
      LOG(WARNING)<<"连接到用户数据库失败"<<mysql_errno(my)<<"\n";
      return false;
    }

    LOG(DEBUG)<<"当前cookie的值："<<cookie<<"\n";

    std::vector<std::string> target;
    StringUtil::SplitString(cookie,&target,"=");
    if(target.size()!=2){
      LOG(ERROR)<<"切分cookie错误"<<"\n";
    }
    auto value1 = redis.get(target[1]);

    if(!value1){
      LOG(WARNING)<<"没找到当前用户"<<"\n";
      return false;
    }
    std::string number = value1.value();
    LOG(DEBUG)<<"查询出用户的number为："<<number<<"\n";

    mysql_set_character_set(my,"utf8");

    std::string sql = "select * from "+ oj_user +" where number = " + number;

    if(mysql_query(my,sql.c_str()) != 0){
      LOG(WARNING)<<"查询语句sql:"<<sql<<"失败"<<"\n";
    }
    LOG(DEBUG)<<"查询语句成功"<<"\n";
    MYSQL_RES *result = mysql_store_result(my);
    if(result == nullptr){
      LOG(INFO)<<"数据库没有当前用户"<<"\n";
      return false;
    }
    int rows = mysql_num_rows(result);
    for(int i=0;i<rows;i++)
    {
      MYSQL_ROW row = mysql_fetch_row(result);
      in_user.number = row[0];
      in_user.name = row[1];
      in_user.password = row[2];
      in_user.op = row[3];
      in_user.level = row[4];
    }
    return true;

  }

  ~Model() {}
  private:
};
} // namespace ns_model