# include <stdio.h>
# include <error.h>
# include <errno.h>
# include <unistd.h>
# include <stdlib.h>

# define DLOG_SERVER
# include "dlog.h"

int main()
{
    dlog_t *lp = dlog_init("test", DLOG_SHIFT_BY_DAY | DLOG_USE_FORK, 0, 0, 0);
    if (lp == NULL)
        error(1, errno, "dlog_init fail");

    dlog_t *lp2 = dlog_init("marvin", DLOG_SHIFT_BY_HOUR, 0, 0, 7);
    if (lp2 == NULL)
        error(1, errno, "dlog_init fail");

    int i;
    for (i = 0; i < 1000000; ++i)
    {
        dlog(lp, "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890abcedfghijklmnoiqasdkjfasdhfaskdjfabcedf");
        //usleep(100);
    }

# if 0
    while (1)
    {
        dlog(lp, "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890");
    }

    dlog(lp2, "hi");
    char s[65 * 1024];
    for (i = 0; i < sizeof(s); ++i)
        s[i] = 'a';
    s[sizeof(s) - 1] = 0;
    dlog(lp2, s);
    dlog(lp2, "hi");

    default_dlog = lp;
    default_dlog_flag = dlog_read_flag("fatal, error     debug, info, USER1");

    printf("%#x\n", default_dlog_flag);

    log_vip("vip");
    log_fatal("fatal");
    log_error("error");
    log_warn("warn");
    log_info("info");
    log_notice("notice");
    log_debug("debug");
    log_user1("hello");
    log_user2("world");

    int *a = NULL;
    *a = 1;
# endif

    dlog_check(NULL, NULL);

    return 0;
}

