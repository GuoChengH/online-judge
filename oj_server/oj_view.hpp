#pragma once

#include<iostream>
#include<string>
#include<ctemplate/template.h>
#include<sw/redis++/redis++.h>
//#include"oj_model.hpp"
#include"oj_model2.hpp"



namespace ns_view
{
    using namespace ns_model;

    const std::string template_path ="./template_html/";
    const std::string login_path = "./login_html/";
    const std::string pcenter_path = "./login_html/";
    class View
    {

    public:
        View(){}
        ~View(){}
        bool RegisterExpandHtml(std::string *html)
        {
            //新城路径
            std::string src_html = login_path + "register.html";
            //形成数据字典
            ctemplate::TemplateDictionary root("register");
            //获取渲染的网页
            ctemplate::Template *tpl = ctemplate::Template::GetTemplate(src_html,ctemplate::DO_NOT_STRIP);
            //开始渲染
            tpl->Expand(html,&root);
            return true;
        }
        void LoginExpandHtml(std::string *html)
        {
            //形成路径
            std::string src_html = login_path + "login.html";
            //形成数据字典
            ctemplate::TemplateDictionary root("my_login");
            //获取渲染网页
            ctemplate::Template *tpl = ctemplate::Template::GetTemplate(src_html,ctemplate::DO_NOT_STRIP);
            //开始渲染
            tpl->Expand(html,&root);
        }
        void AllExpandHtml(const vector<Question> &questions,std::string *html)
        {
            // 题目的编号 题目的标题 题目的难度
            // 推荐使用表格显示
            //1。形成路径
            std::string src_html = template_path + "all_questions.html";
            LOG(INFO)<<"形成路径成功:"<< src_html <<"\n";
            //2.形成数据字典
            ctemplate::TemplateDictionary root("all_questions");
            for(const auto& q:questions)
            {
                ctemplate::TemplateDictionary *sub = root.AddSectionDictionary("question_list");
                sub->SetValue("number",q.number);
                sub->SetValue("title",q.title);
                sub->SetValue("star",q.star);
            }
            //3.获取被渲染的网页html
            ctemplate::Template *tpl = ctemplate::Template::GetTemplate(src_html,ctemplate::DO_NOT_STRIP);
            LOG(INFO)<<"获取渲染网页的html成功"<<"\n";

            //4.开始完成渲染功能
            tpl->Expand(html,&root);
            LOG(INFO)<<"渲染成功"<<"\n";

        }
        void OneExpandHtml(const Question &q,std::string *html)
        {
            //形成路径
            std::string src_html = template_path + "one_question.html";
            LOG(DEBUG)<<"one expand html :"<<src_html<<"\n";
            //q.desc
            //形成数字典
            ctemplate::TemplateDictionary root("one_question");
            root.SetValue("number",q.number);
            root.SetValue("title",q.title);
            root.SetValue("star",q.star);
            root.SetValue("desc",q.desc);
            root.SetValue("pre_code",q.header);
            //获取被渲染的html
            ctemplate::Template *tpl = ctemplate::Template::GetTemplate(src_html,ctemplate::DO_NOT_STRIP);
            //开始渲染功能
            tpl->Expand(html,&root);
        }
        bool PersonCenterExpandHtml(UserCenter& user , std::string *html)
        {
            
            //1.形成路径
            std::string src_path = pcenter_path + "PersonCer.html";
            //2.形成数据字典
            ctemplate::TemplateDictionary root("PersonCer");
            root.SetValue("name",user.name);
            root.SetValue("number",user.number);
            root.SetValue("passwd",user.password);
            root.SetValue("op",user.op);
            root.SetValue("level",user.level);
            //3.获取被渲染的html
            ctemplate::Template *tpl = ctemplate::Template::GetTemplate(src_path,ctemplate::DO_NOT_STRIP);

            //4.开始渲染
            tpl->Expand(html,&root);
            
            return true;
        }
    };
}