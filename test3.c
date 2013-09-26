/*
 * Description: 
 *     History: damonyang@tencent.com, 2013/09/26, create
 */

# include <stdio.h>
# include <stdlib.h>
# include <error.h>
# include <errno.h>
# include <unistd.h>

# include "dlog.h"

int main(int argc, char *argv[])
{
    if (argc != 2)
        error(1, 0, "Usage: %s base", argv[0]);

    dlog_t *lp = dlog_init(argv[1], 0, 0, 0, 0);
    if (lp == NULL)
        error(1, 0, "dlog_init fail");

    int i;
    for (i = 0; i < 1000 * 1000; ++i)
    {
        dlog(lp, "hi dlog %d", i);
        if ((i % 1000) == 0)
            usleep(500 * 1000);
    }

    return 0;
}

