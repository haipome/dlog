/*
 * Description: 
 *     History: damonyang@tencent.com, 2013/09/25, create
 */

# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <arpa/inet.h>
# include <netinet/in.h>
# include <error.h>
# include <errno.h>
# include <unistd.h>

# include "dlog.h"

int main(int argc, char *argv[])
{
    if (argc != 3)
        error(1, errno, "Usage: %s ip port", argv[0]);

    struct sockaddr_in log_addr;
    memset(&log_addr, 0, sizeof(log_addr));
    log_addr.sin_family = AF_INET;
    if (inet_aton(argv[1], &log_addr.sin_addr) == 0)
        error(1, errno, "ip: %s in invalid", argv[1]);
    log_addr.sin_port = htons((unsigned short)atoi(argv[2]));

    dlog_t *lp = dlog_init((char *)&log_addr, DLOG_REMOTE_LOG, 0, 0, 0);
    if (lp == NULL)
        error(1, errno, "dlog_init fail");

    int i;
    for (i = 0; i < 100 * 10000; ++i)
    {
        dlog(lp, "hello worldddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd000");
        if ((i % 1000) == 0)
            usleep(500 * 1000);
    }

    return 0;
}

