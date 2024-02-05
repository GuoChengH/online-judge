
#ifndef COMPILER_ONLINE
#include"header.cpp"
#endif
//上面的宏只是为了编写代码的时候有语法提醒，在编译的时候直接-D COMPILER_ONLINE就可以去掉
void Test1()
{
    //通过定义临时对象，来完成方法的调用
    bool ret = Solution().isPalindrome(121);
    if(ret){
        std::cout<<"通过用例1，测试121通过.."<<std::endl;
    }
    else{
        std::cout<<"没有通过用例1，测试121..."<<std::endl;
    }
}
void Test2()
{
    //通过定义临时对象，来完成方法的调用
    bool ret = Solution().isPalindrome(-121);
    if(!ret){
        std::cout<<"通过用例2，测试-121通过.."<<std::endl;
    }
    else{
        std::cout<<"没有通过用例2，测试-121..."<<std::endl;
    }
}
int main()
{
    Test1();
    Test2();

    return 0;
}