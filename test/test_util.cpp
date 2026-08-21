#include"sylar.h"
#include<assert.h>
mo1al::Logger::ptr g_logger = SYLAR_LOG_ROOT();
void test1()
{
    SYLAR_LOG_INFO(g_logger)<<mo1al::BacktraceToString(10,2,"    ");
    SYLAR_ASSERT2(false,"xxxxxx");
    // assert(1);
}
int main()
{ 
    test1();
    return 0;
}