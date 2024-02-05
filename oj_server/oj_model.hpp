#pragma once
//文件版本
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
#include <fstream>
#include <iostream>
#include <stdlib.h>
#include <string>
#include <unordered_map>
#include <vector>
// 根据题目list文件，加载所有信息到内存中
// model:主要用来和数据进行交互，对外提供访问数据的接口

namespace ns_model {
using namespace std;
using namespace ns_log;
using namespace ns_util;
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
const std::string question_list = "./questions/questions.list";
const std::string question_path = "./questions/";
class Model {
private:
  // 题号：题目细节
  unordered_map<string, Question> questions;

public:
  Model() { assert(LoadQuestionList(question_list)); }
  bool LoadQuestionList(const std::string &question_list) {
    // 加载配置文件questions/question.list + 题目编号文件
    ifstream in(question_list);
    if (!in.is_open()) {
      LOG(FATEL) << "加载题库失败，请检查是否存在题库文件"
                 << "\n";
      return false;
    }
    std::string line;
    while (getline(in, line)) {
      vector<string> tokens;
      StringUtil::SplitString(line, &tokens, " ");
      if (tokens.size() != 5) {

        LOG(WARNING) << "加载部分题目失败，请检查文件格式"
                     << "\n";
        continue;
      }
      Question q;
      q.number = tokens[0];
      q.title = tokens[1];
      q.star = tokens[2];
      q.cpu_limit = atoi(tokens[3].c_str());
      q.mem_limit = atoi(tokens[4].c_str());

      std::string path = question_path;
      path += q.number;
      path += "/";
      FileUtil::ReadFile(path + "desc.txt", &(q.desc), true);
      FileUtil::ReadFile(path + "header.cpp", &(q.header), true);
      FileUtil::ReadFile(path + "tail.cpp", &(q.tail), true);

      questions.insert({q.number, q});
    }
    LOG(INFO) << "加载题库成功！"
              << "\n";
    in.close();
    return true;
  }
  bool GetAllQuestion(vector<Question> *out) {
    if (questions.size() == 0) {
      LOG(ERROR) << "用户获取题库失败"
                 << "\n";
      return false;
    }
    for (const auto &q : questions) {
      out->push_back(q.second); // fir是key' sec是value
    }
    return true;
  }
  bool GetOneQuestion(const std::string &number, Question *q) {
    const auto &iter = questions.find(number);
    if (iter == questions.end()) {
      LOG(ERROR) << "用户获取题目失败：" << number << "\n";

      return false;
    }
    (*q) = iter->second;
    return true;
  }
  ~Model() {}
};
} // namespace ns_model