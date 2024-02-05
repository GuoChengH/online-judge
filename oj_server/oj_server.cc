
#include "../comm/httplib.h"
#include "login.hpp"
#include <iostream>
#include <signal.h>
#include"oj_control.hpp"
using namespace httplib;
using namespace ns_control;
const std::string login_path = "../oj_login/wwwroot/";
static Control *ctrl_ptr = nullptr;
void Recovery(int signo)
{
  ctrl_ptr->RecoveryMachine();
}
int main() {
  signal(SIGQUIT,Recovery);
  // 用户请求的服务路由功能
  
  

  Server svr;
  Control ctrl;
  Login login;
  ctrl_ptr = &ctrl;
  /*
  1获取所有的题目列表

  */
  svr.Get(R"(/all_questions)", [&ctrl](const Request &req, Response &resp) {
    std::string html;
    ctrl.AllQuestions(&html);
    resp.set_content(html, "text/html;charset=utf-8");
  });

  //   2用户要根据题目编号来选择题目
  // 这里的\d是正则表达式 + 是匹配数字
  // R"()"保持原始字符串不会被特殊字符影响比如\d \r \n之类的不需要做相关的转义
  svr.Get(R"(/question/(\d+))", [&ctrl](const Request &req, Response &resp) {
    std::string number = req.matches[1];
    std::string html;
    ctrl.OneQuestion(number,&html);
    resp.set_content(html,"text/html;charset=utf-8");
  });

  // 3用户提交代码，使用我们的判题功能（1.没道题目的测试用例 2.compile_and_run)
  svr.Post(R"(/judge/(\d+))",[&ctrl](const Request &req, Response &resp){
    std::string number = req.matches[1];
    // resp.set_content("这是指定的一道题目的判题：" + number,
    //                  "text/plain;charset=utf-8");
    std::string result_json;
    ctrl.Judge(number,req.body,&result_json);
    resp.set_content(result_json,"application/json;charset=utf-8");
  });
  svr.Post(R"(/dealregister)",[&ctrl](const Request &req, Response &resp){
    int status = 1;
    std::string in_json = req.body;
    std::string out_json;
    if(!ctrl.UserRegister(in_json,&out_json)){
      status = 0;
    }
    LOG(INFO)<<"用户注册成功!!"<<"\n";
    Json::Value tmp;
    tmp["status"] = status;
    Json::FastWriter writer;
    std::string res = writer.write(tmp);
    
    resp.set_content(res,"application/json;charset=utf-8");
  });
  svr.Post(R"(/deallogin)",[&ctrl](const Request &req, Response &resp){
    int status = 1;
    Json::Value tmp;
    Json::FastWriter writer;
    std::string in_json = req.body;
    std::string out_json;
    if(!ctrl.UserLogin(in_json,&out_json))
    {
      status = 0;
      LOG(INFO)<<"用户登录失败!!"<<"\n";
    }
    tmp["status"] = status;

    Json::Value cookie;
    Json::Reader reader;
  
    reader.parse(in_json,cookie);
    std::string RNumber = cookie["number"].asString();
    std::string id = LoginSessionUtil::GetOnlySession(RNumber);
    std::string SetCookie = "id="+ id;
    resp.set_header("Set-Cookie",SetCookie.c_str());
    std::string res = writer.write(tmp);
    resp.set_content(res,"application/json;charset=utf-8");
  });

  svr.Get(R"(/my_login)",[&login,&ctrl](const Request &req,Response &resp){
    //直接跳转到静态的html
    std::string html;
    ctrl.Login(req.body,&html);
    // resp.set_header()
    resp.set_content(html, "text/html;charset=utf-8");
  });

  svr.Get(R"(/register)",[&login,&ctrl](const Request &req,Response &resp){
    //直接跳转到静态的html
    std::string html;
    ctrl.Register(req.body,&html);
    resp.set_content(html, "text/html;charset=utf-8");
  });

  svr.Get(R"(/personcer)",[&ctrl](const Request &req,Response& resp){
    std::string html;
    std::string cookie = req.get_header_value("Cookie");
    // std::cout<<"当前获得的cookie"<<cookie<<"\n";
    LOG(DEBUG)<<"当前的cookie："<<cookie<<"\n";

    ctrl.PersonCenter(cookie,&html);
    resp.set_content(html,"text/html;charset=utf-8");
  });
  
  svr.set_base_dir("./wwwroot");
  svr.listen("0.0.0.0", 8008);
  return 0;
}