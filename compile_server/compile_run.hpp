#pragma once
#include "compiler.hpp"
#include<unistd.h>
#include "runner.hpp"
#include "../comm/log.hpp"
#include "../comm/util.hpp"
#include <jsoncpp/json/json.h>
#include <signal.h>
namespace ns_compile_and_run
{
    using namespace ns_log;
    using namespace ns_util;
    using namespace ns_compiler;
    using namespace ns_runner;
    class CompileAndRun
    {
    public:
        static void RemoveTempFile(const std::string& file_name)
        {
            //清理文件的个数是不确定的，但是有哪些我们是知道的
            std::string _src = PathUtil::Src(file_name);
            if(FileUtil::IsFileExists(_src))unlink(_src.c_str());
            
            std::string _compiler_error = PathUtil::CompilerError(file_name);
            if(FileUtil::IsFileExists(_compiler_error))unlink(_compiler_error.c_str());
            
            std::string _execute = PathUtil::Exe(file_name);
            if(FileUtil::IsFileExists(_execute)) unlink(_execute.c_str());

            std::string _stdin = PathUtil::Stdin(file_name);
            if(FileUtil::IsFileExists(_stdin)) unlink(_stdin.c_str());

            std::string _stdout = PathUtil::Stdout(file_name);
            if(FileUtil::IsFileExists(_stdout)) unlink(_stdout.c_str());

            std::string _stderr = PathUtil::Stderr(file_name);
            if(FileUtil::IsFileExists(_stderr)) unlink(_stderr.c_str());
        }
        static std::string CodeToDesc(int code, std::string file_name) // code >0 <0 ==0
        {
            std::string desc;
            switch (code)
            {
            case 0:
                desc = "编译运行成功";
                break;
            case -1:
                desc = "用户提交的代码是空";
                break;
            case -2:
                desc = "未知错误";
                break;
            case -3:
                // desc = "编译发生报错";
                FileUtil::ReadFile(PathUtil::Stderr(file_name), &desc, true);
                break;
            case -4:
                break;
            case SIGABRT:
                desc = "内存超过范围";
                break;
            case SIGXCPU:
                desc = "CPU信号超时";
                break;
            case SIGFPE:
                desc = "除零错误,浮点数溢出";
                break;
            default:
                desc = "未知：" + std::to_string(code);
                break;
            }
            return desc;
        }
        /*
        输入：
        code：用户提交的代码
        input：用户自己提交的代码，对应的输入-》不做处理
        cpu_limit：时间要求
        mem_limit:空间要求
        输出：
        必填：
        status：状态码
        reason：请求结果
        选填：
        stdout：我的的程序运行完的结果
        stderr：我的程序运行完的错误结构

        参数：
        in_json:{"code":"#include..."."input":"","cpu_limit":1,"mem_limit":10240}
        out_json:{"status":"0","reason":"","stdout":"","stderr":""};
        */
        static void Start(const std::string &in_json, std::string *out_json)
        {
            LOG(INFO)<<"启动compile_and_run"<<"\n";
            Json::Value in_value;
            Json::Reader reader;
            reader.parse(in_json, in_value); // 最后再处理差错问题

            std::string code = in_value["code"].asString();
            std::string input = in_value["input"].asString();
            int cpu_limit = in_value["cpu_limit"].asInt();
            int men_limit = in_value["mem_limit"].asInt();

            int status_code = 0;
            Json::Value out_value;
            int run_result = 0;
            std::string file_name; // 需要内部形成的唯一文件名

            if (code.size() == 0)
            {
                // 说明用户一行代码都没提交
                status_code = -1;
                goto END;
            }

            // 形成的文件名只具有唯一性，没有目录没有后缀
            // 毫秒计时间戳+原子性递增的唯一值：来保证唯一性
            file_name = FileUtil::UniqFileName(); // 形成唯一文件名字
            LOG(DEBUG)<<"调用UniqFileName()形成唯一名字"<<file_name<<"\n";

            run_result = Runner::Run(file_name, cpu_limit, men_limit);
            if (!FileUtil::WriteFile(PathUtil::Src(file_name), code)) // 形成临时src文件.cpp
            {
                status_code = -2; // 未知错误
                goto END;
            }

            if (!Compiler::Compile(file_name))
            {
                // 编译失败
                status_code = -3;
                goto END;
            }

            run_result = Runner::Run(file_name, cpu_limit, men_limit);
            if (run_result < 0)
            {
                // 服务器的内部错误，包括不限于文件打开失败，创建子进程失败等待
                status_code = -2; // 未知错误
                goto END;
            }
            else if (run_result > 0)
            {
                status_code = run_result;
            }
            else
            {
                // 运行成功
                status_code = 0;
            }
        END:
            std::cout<<"到达end语句"<<std::endl;
            // status_code
            out_value["status"] = status_code;
            out_value["reason"] = CodeToDesc(status_code, file_name);
            LOG(DEBUG)<<CodeToDesc(status_code, file_name);
            if (status_code == 0)
            {
                // 整个过程全部成功 , 这时候才需要运行结果
                std::string _stdout;
                FileUtil::ReadFile(PathUtil::Stdout(file_name), &_stdout, true);
                out_value["stdout"] = _stdout; 
            }
            else
            {
                std::string _stderr;
                FileUtil::ReadFile(PathUtil::CompilerError(file_name), &_stderr, true);
                out_value["stderr"] = _stderr;
            }
            
            // 序列化

            Json::StyledWriter writer;
            *out_json = writer.write(out_value);

            //清理所有的临时文件
            RemoveTempFile(file_name);
        }
    };
}