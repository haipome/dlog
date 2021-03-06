# include <stdio.h>
# include <error.h>
# include <errno.h>
# include <unistd.h>
# include <stdlib.h>

# include "dlog.h"

int func_b()
{
    log_exception("process fail");
    return -1;
}

int func_a()
{
    return func_b();
}

int main()
{
    log_info("system error");

    dlog_t *lp = dlog_init("test", DLOG_SHIFT_BY_DAY, 0, 0, 0);
    if (lp == NULL)
        error(1, errno, "dlog_init fail");

    default_dlog = lp;
    default_dlog_flag = dlog_read_flag("fatal, error debug, info, USER1");

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
    func_a();

    return 0;
}

