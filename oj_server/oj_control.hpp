#pragma once

#include<iostream>
#include<string>
#include<cassert>
#include<algorithm>
#include<fstream>
#include<jsoncpp/json/json.h>
#include<vector>
#include<mutex>
#include"oj_view.hpp" 
//#include"oj_model.hpp"
#include"oj_model2.hpp"
#include"../comm/log.hpp"
#include"../comm/util.hpp"
#include"../comm/httplib.h"
namespace ns_control
{
    using namespace std;
    using namespace httplib;
    using namespace ns_log;
    using namespace ns_util;
    using namespace ns_model; 
    using namespace ns_view;
    //提供服务的主机的内容
    class Machine
    {
    public:
        std::string ip; //编译服务器的ip
        int port;       //编译服务器的端口
        uint64_t load;  //编译服务器负载
        std::mutex *mtx;//mutex是禁止拷贝的,使用指针来完成
    public:
        Machine():ip(""),port(0),load(0),mtx(nullptr)
        {}
        ~Machine()
        {}
    public:
    void ResetLoad()
    {
        if(mtx)mtx->lock();
        load = 0;
        LOG(DEBUG)<<"当前ip:"<<ip<<"端口："<<port<<"的load已经清除load = "<<load<<"\n";
        if(mtx)mtx->unlock();
    }
    //提升主机负载
        void IncLoad()
        {
            if(mtx) mtx->lock();
            ++load;
            if(mtx) mtx->unlock();
        }
    //减少主机负载
        void DecLoad()
        {
            if(mtx) mtx->lock();
            --load;
            if(mtx) mtx->unlock();
        }

        //获取主机负载,没有太大的意义，只是为了同一接口
        uint64_t Load()
        {
            uint64_t _load = 0;
            if(mtx) mtx->lock();
            _load = load;
            if(mtx) mtx->unlock();
            return _load;
        }
    };
    const std::string service_machine = "./conf/service_machine.conf";
    //负载均衡模块
    class LoadBalance
    { 
    private:
        //可以给我们提供编译服务的所有的主机
        //每一台主机都有自己的下标，充当当前主机的id
        std::vector<Machine> machines; 
        //所有在线的主机
        std::vector<int> online;
        //所有离线主机的id
        std::vector<int> offline;
        //保证选择主机上的这个东西要保证数据安全
        std::mutex mtx;
    public:
        LoadBalance(){
            assert(LoadConf(service_machine));
            LOG(INFO)<<"加载"<<service_machine <<"成功"<<"\n";
        }
        ~LoadBalance(){}
    public:
        bool LoadConf(const std::string &machine_cof)
        {
            std::ifstream in(machine_cof);
            if(!in.is_open())\
            {
                LOG(FATAL) <<"加载："<<machine_cof<<"失败"<<"\n";
                return false;
            }
            std::string line;
            while (getline(in,line))
            {
                std::vector<std::string> tokens;
                StringUtil::SplitString(line,&tokens,":");
                if(tokens.size()!=2)
                {
                    LOG(WARNING) <<"切分"<<line<<"失败"<<"\n";
                    std::cout<<tokens[0]<<":"<<tokens[1]<<std::endl;
                    continue;
                }
                //LOG(INFO) <<"切分"<<tokens[0]<<tokens[1]<<"成功"<<"\n";
                Machine m;
                m.ip = tokens[0];
                m.port = atoi(tokens[1].c_str());
                m.load = 0;
                m.mtx = new std::mutex();

                online.push_back(machines.size());
                machines.push_back(m);
            }
            
            in.close();
            return true;
        }
        //id:是一个输出型参数
        //m:是一个输出型参数
        bool SmartChoice(int *id,Machine **m)
        {
            //1.使用选择好的主机(更新该主机的负载)
            //2.我们需要可能离线该主机
            mtx.lock();
            //选择主机
            //一般的负载均衡的算法
            //1.随机数法 + hash
            //2.轮询 + hash
            int online_num = online.size();//在线主机的个数
            if(online_num == 0){
                mtx.unlock();
                LOG(FATAL) << "所有的后端编译主机已经全部离线，请后端的尽快重启"<<"\n";
                return false;
            }
            LOG(DEBUG)<<"online:"<<online.size()<<"\n";
            //通过编译，找到负载最小的机器
            *id = online[0];
            *m = &machines[online[0]];
            uint64_t min_load = machines[online[0]].Load();
            for(int i = 1;i<online_num;i++)
            {
                uint64_t curr_load = machines[online[i]].Load();
                if(min_load > curr_load){
                    min_load = curr_load;
                    *id = online[i];
                    *m = &machines[online[i]];
                }
            }
            
            mtx.unlock();
            return true;
        }
        void OfflineMachine(int which)
        {
            mtx.lock();
            for(auto iter = online.begin();iter!=online.end();iter++)
            {
                if(*iter == which){
                    //要离线的主机找到了
                    machines[which].ResetLoad();
                    LOG(DEBUG)<<"当前离线主机的负载更改为："<<machines[which].load;
                    online.erase(iter);
                    offline.push_back(which);
                    break;//因为break的存在，所以暂时不考虑迭代器失效的问题
                }
            }
            mtx.unlock();
        }
        void OnlineMachine()
        {
            //我们统一上线,后面统一解决
            mtx.lock();
            
            online.insert(online.end(),offline.begin(),offline.end());
            offline.erase(offline.begin(),offline.end());
            mtx.unlock();
            LOG(INFO)<<"所有的主机又上线了"<<"\n";
            LOG(INFO)<<"online:"<<online.size()<<"offline"<<offline.size()<<"\n";
            

        }
        void ShowMachines()
        {
            mtx.lock();
            LOG(INFO)<<"online:"<<online.size()<<"offline"<<offline.size()<<"\n";
            mtx.unlock();
        }
    };

    
    //这是我们核心业务逻辑的控制器
    class Control
    {
    private:
        Model model_;//提供后台数据
        View view_; //提供网页渲染功能
        LoadBalance load_blance_; //核心负载均衡器
        
    public:
        void RecoveryMachine()
        {
            load_blance_.OnlineMachine();
        }
        //根据题目数据构建网页
        //html：输出型参数
        bool AllQuestions(string *html)
        {
            bool ret = true;
            vector<Question> all;
            if(model_.GetAllQuestion(&all))
            {
                sort(all.begin(),all.end(),[](const Question &q1,const Question &q2){
                    return atoi(q1.number.c_str()) < atoi(q2.number.c_str());
                });

                //获取题目信息 成功，将所有的题目数据构建成网页
                view_.AllExpandHtml(all,html);
            }
            else
            {
                *html="获取题目失败，形成题目列表失败";
                ret = false;
            }
            return ret;
        }
        bool OneQuestion(const string &number,string *html)
        {
            Question q;
            bool ret = true;
            if(model_.GetOneQuestion(number,&q))
            {
                //获取指定信息的题目成功，构建程网页
                view_.OneExpandHtml(q,html);
            }
            else
            {
                *html="获取指定题目题目失败，形成题目列表失败";
                ret = false;
            }
            return ret;
        }
        void Login(const std::string in_json,std::string *out_json)
        {
            //in_json是发送过来的请求数据，用户的账号等待

            //返回渲染的登录界面
            view_.LoginExpandHtml(out_json);

        }
        void Register(const std::string in_json,std::string *out_json)
        {
            if(view_.RegisterExpandHtml(out_json)){
                LOG(INFO)<<"插入成功"<<"\n";
            }
            else{
                LOG(INFO)<<"插入失败，可能是重复的用户"<<"\n";
            }
            
        }
        bool PersonCenter(const std::string in_cookie,std::string *out_json)
        {
            //1.检测是否有redis对应的key model2去做
            UserCenter user;
            
            //根据in_cookie 获取 user数据
            model_.GetUserCenter(in_cookie,user);
            
            //渲染是让view去做
            return view_.PersonCenterExpandHtml(user,out_json);
        }
        bool UserRegister(const std::string in_json,std::string *out_json)
        {
            return model_.UserRegister(in_json,out_json);
        }
        bool UserLogin(const std::string in_json,std::string *out_json)
        {
            return model_.UserLogin(in_json,out_json);
        }
        
        //id:: 100 
        //code:include
        //input:
        void Judge(const std::string &number,const std::string in_json,std::string *out_json)
        {
            // LOG(INFO)<<"调用Judge功能"<<"\n";
            // LOG(DEBUG)<<in_json<<"\nnumber:"<<number<<"\n";
            //0.根据题目编号，拿到题目细节
            Question q;
            model_.GetOneQuestion(number,&q);
            //1.in_json反序列化 ，得到题目的id，得到源代码，input
            Json::Reader reader;
            Json::Value in_value;
            reader.parse(in_json,in_value);
            std::string code = in_value["code"].asString();
            //2.重新拼接用户代码+测试用例代码，形成新的代码
            Json::Value compile_value;
            compile_value["input"] = in_value["input"].asString();
            compile_value["code"] = code + q.tail;
            compile_value["cpu_limit"] = q.cpu_limit;
            compile_value["mem_limit"] = q.mem_limit;
            Json::FastWriter writer;
            std::string compile_string = writer.write(compile_value);
            //3.选择负载最低的主机，然后发起HTTP请求得到结果
            //规则：一直选择，直到主机可用，否则就是全部挂掉
            while(true)
            {
                int id = 0;
                Machine *m = nullptr;
                if(!load_blance_.SmartChoice(&id,&m))
                {   
                    break;
                }
                 //4.*out_json = 将结果复制给out_json
                Client cli(m->ip,m->port);
                m->IncLoad();
                LOG(DEBUG)<<"选择主机成功，主机id:"<<id<<"\n详情："<<m->ip<<":"<<m->port<<"当前主机负载："<<m->Load()<<"\n";
                if(auto res = cli.Post("/compile_and_run",compile_string,"application/json;charset=utf-8"))
                {
                    //将我们的结果返回给out_json
                    if(res->status == 200)
                    {
                        *out_json = res->body;
                        m->DecLoad();
                        LOG(INFO)<<"请求编译和运行服务成功..."<<"\n";
                        break;
                    }                        
                    m->DecLoad();

                }
                else
                {
                    //请求失败
                    LOG(ERROR)<<"选择主机失败，主机id:"<<id<<"详情："<<m->ip<<":"<<m->port<<"可能已经离线"<<"\n";
                    load_blance_.OfflineMachine(id);
                    load_blance_.ShowMachines();//仅仅为了调试
                    
                }
                //m->DecLoad();
            }
           

        }
        Control(){}
        ~Control(){}
    };
}