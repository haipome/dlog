# include <stdio.h>
# include <error.h>
# include <errno.h>
# include <unistd.h>

# include "dlog.h"

int main()
{
    dlog_t *t = dlog_init("test", DLOG_SHIFT_BY_DAY, 0, 0, 0);
    if (t == NULL)
        error(1, errno, "create dlog fail");

    int i;
    int ret;
    for (i = 0; i < 1000000; ++i) {
        if ((ret = dlog(t, "hello dlog: %d", i)) < 0)
            error(1, 0, "dlog fail: %d", ret);
    }

    return 0;
}

