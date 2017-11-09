#include "timeusage.h"
#include <thread>

void func_1(int a) {
    MYBLOCK_TIME_CONSUMING(BlockTimeUsageStat::instance(), FILE_LINE);

    {
        BLOCK_TIME_CONSUMING("func_1.block1");
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

void func_2(int a, int b) {
    BLOCK_TIME_CONSUMING(FILE_LINE);

    std::this_thread::sleep_for(std::chrono::milliseconds(1));
}

void test_function() {

    BlockTimeUsageStat::instance()->setOpen(true);

    for (int i = 0; i < 555; ++i) {
        func_1(i);
        func_2(0, 0);
    }

    std::cout << BlockTimeStat::dumpHeaderAsString() << std::endl;
    BlockTimeUsageStat::instance()->dump();
}

int main()
{
    test_function();
}