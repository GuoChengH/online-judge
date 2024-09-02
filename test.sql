
create database db;

use db;
CREATE TABLE oj_questions (
    `number` VARCHAR(255),       -- 题目编号
    `title` VARCHAR(255),        -- 题目的标题
    `star` VARCHAR(255),         -- 难度：简单、中等、困难
    `cpu_limit` INT,             -- 时间要求，以秒为单位
    `mem_limit` INT,             -- 空间要求，以KB为单位
    `desc` TEXT,                 -- 题目的描述
    `header` TEXT,               -- 题目预设给用户在线编辑器的代码
    `tail` TEXT,                 -- 题目的测试用例，需要和header拼接形成完整代码
    PRIMARY KEY (`number`)       -- 将题目编号设置为主键
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;