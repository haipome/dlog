# include <stdio.h>
# include <error.h>
# include <errno.h>
# include <unistd.h>
# include <stdlib.h>

# define DLOG_SERVER
# include "dlog.h"

int main()
{
    log_info("system error");

    dlog_t *lp = dlog_init("test", DLOG_SHIFT_BY_DAY | DLOG_NO_TIMESTAMP, 0, 0, 0);
    if (lp == NULL)
        error(1, errno, "dlog_init fail");

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

    return 0;
}

