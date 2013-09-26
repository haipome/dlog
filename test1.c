/*
 * Description: 
 *     History: damonyang@tencent.com, 2013/00/00, create
 */


# include "dlog.h"

int main()
{
    dlog_t *lp = dlog_init("test", DLOG_SHIFT_BY_DAY, 0, 0, 0);
    if (lp == NULL)
        error(1, 0, "init fail");

    int i;
    for (i = 0; i < 1000000; ++i)
    {
        dlog(lp, "hello dlog: %d", i);
    }

    return 0;
}

