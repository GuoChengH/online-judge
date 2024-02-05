#include"compile_run.hpp"
#include<jsoncpp/json/json.h>
#include"../comm/httplib.h"
using namespace ns_compile_and_run;
using namespace httplib;

//编译服务随时可能被多个人请求,必须保证传递上来的code，形成源文件名称的时候，要具有唯一性，不然影响多个用户
void Usage(std::string proc)
{
    std::cerr <<"Usage:"<<"\n\t"<<proc<<"port"<<std::endl;
}
// ./compiler_server port
int main(int argc,char *argv[])
{
    if(argc!=2){
        Usage(argv[0]);
    }
    Server svr;

    svr.Get("/hello",[](const Request &req,Response &resp)
    {
        resp.set_content("hello httplib,你好httplib","content_type: text/plain");
    });
    //svr.set_base_dir("./wwwroot");
    svr.Post("/compile_and_run",[](const Request &req,Response &resp){  
        //请求服务正文是我们想要的json串
        LOG(DEBUG)<<"调用compile_and_run"<<"\n";
        std::string out_json;
        std::string in_json = req.body;
        if(!in_json.empty()){
            LOG(DEBUG)<<"当前的in_json"<<in_json<<"\n";
            CompileAndRun::Start(in_json,&out_json);
            resp.set_content(out_json,"application/json;charset=utf-8");
        }
    });
    svr.listen("0.0.0.0",atoi(argv[1]));//启动http服务了

    // std::string code = "code";
    // Compiler::Compile(code);
    // Runner::Run(code);


    //0-----------------------测试代码-------------------
    //下面的工作，充当客户端请求的json串
    // std::string in_json;
    // Json::Value in_value;
    // //R"（）" raw string 凡事在这个圆括号里面的东西，就是字符串哪怕有一些特殊的字符串
    // in_value["code"] =R"(#include<iostream>
    // int main(){
    //         std::cout<<"测试成功"<<std::endl;
    //         int a = 10;
    //         a /= 0;
    //         return 0;
    //     })";
    // in_value["input"] ="";
    // in_value["cpu_limit"] = 1;
    // in_value["mem_limit"] = 10240 * 3;

    // Json::FastWriter writer;
    // std::cout<<in_json<<std::endl;
    // in_json = writer.write(in_value);

    // //这个是将来给客户端返回的字符串
    // std::string out_json;
    // CompileAndRun::Start(in_json,&out_json);

    // std::cout<<out_json<<std::endl;
    //0-----------------------------------------------------


    //提供的编译服务，打包新城一个网络服务
    //这次直接用第三方库，cpp-httplib

    return 0;
}