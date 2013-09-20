/*
 * Description: a log lib with cache
 *     History: yang@haipo.me, 2013/05/20, created
 */

# undef  _FILE_OFFSET_BITS
# define _FILE_OFFSET_BITS 64

# include <stdio.h>
# include <stdint.h>
# include <string.h>
# include <stdlib.h>
# include <stdarg.h>
# include <fcntl.h>
# include <time.h>
# include <errno.h>
# include <limits.h>
# include <unistd.h>
# include <inttypes.h>
# include <sys/types.h>
# include <sys/stat.h>
# include <sys/time.h>

# include "dlog.h"

dlog_t *default_dlog = NULL;
int     default_dlog_flag = 0;

# define WRITE_INTERVAL_IN_USEC (10 * 1000)     /* 100 ms */
# define WRITE_BUFFER_LEN       (32 * 1024)     /* 32 KB */

# ifdef DEBUG
static int write_times = 0;
# endif

/* all opened log is in a list, vist by lp_list_head */
static dlog_t *lp_list_head = NULL;

/* use to make sure dlog_atexit only call once */
static int init_flag = 0;

static char *log_suffix(int type, time_t sec, int i)
{
    static char str[30];

    if (type == DLOG_SHIFT_BY_SIZE)
    {
        if (i)
            snprintf(str, sizeof(str), "%d.log", i);
        else
            snprintf(str, sizeof(str), ".log");

        return str;
    }

    struct tm *t = localtime(&sec);
    ssize_t n = 0;

    switch (type)
    {
        case DLOG_SHIFT_BY_MIN:
            n = snprintf(str, sizeof(str), "_%04d%02d%02d%02d%02d",
                    t->tm_year + 1900, t->tm_mon + 1,
                    t->tm_mday, t->tm_hour, t->tm_min);
            
            break;
        case DLOG_SHIFT_BY_HOUR:
            n = snprintf(str, sizeof(str), "_%04d%02d%02d%02d",
                    t->tm_year + 1900, t->tm_mon + 1,
                    t->tm_mday, t->tm_hour);

            break;
        case DLOG_SHIFT_BY_DAY:
            n = snprintf(str, sizeof(str), "_%04d%02d%02d",
                    t->tm_year + 1900, t->tm_mon + 1, t->tm_mday);
            break;
        default:
            str[0] = 0;

            break;
    }

    if (i)
        snprintf(str + n, sizeof(str) - n, "_%d.log", i);
    else
        snprintf(str + n, sizeof(str) - n, ".log");

    return str;
}

static uint64_t timeval_diff(struct timeval *old, struct timeval *new)
{
    return (new->tv_sec - old->tv_sec) * (1000 * 1000) -
        (old->tv_usec - new->tv_usec);
}

static int _unlink_expire(dlog_t *lp, time_t expire_time)
{
    char path[PATH_MAX];
    int  num = 0;
    int  i;

    if (lp->log_num == 0)
    {
        for (i = 0;; ++i)
        {
            snprintf(path, PATH_MAX, "%s%s", lp->base_name,
                    log_suffix(lp->shift_type, expire_time, i));
            if (access(path, F_OK) == 0)
                ++num;
            else
                break;
        }
    }
    else
    {
        num = lp->log_num;
    }

    for (i = 0; i < num; ++i)
    {
        snprintf(path, PATH_MAX, "%s%s", lp->base_name,
                log_suffix(lp->shift_type, expire_time, i));

        unlink(path);
    }

    return 0;
}

static int unlink_expire(dlog_t *lp, struct timeval *now)
{
    if (lp->shift_type == DLOG_SHIFT_BY_SIZE || lp->keep_time == 0)
        return 0;

    int expire = 0;
    time_t expire_time;

    switch (lp->shift_type)
    {
        case DLOG_SHIFT_BY_MIN:
            if (now->tv_sec - lp->last_unlink > 60)
            {
                expire = 1;
                expire_time = now->tv_sec - 60 * lp->keep_time;
            }

            break;
        case DLOG_SHIFT_BY_HOUR:
            if (now->tv_sec - lp->last_unlink > 3600)
            {
                expire = 1;
                expire_time = now->tv_sec - 3600 * lp->keep_time;
            }

            break;
        case DLOG_SHIFT_BY_DAY:
            if (now->tv_sec - lp->last_unlink > 86400)
            {
                expire = 1;
                expire_time = now->tv_sec - 86400 * lp->keep_time;
            }

            break;
    }

    if (expire)
    {
# ifdef DEBUG
        struct timeval start, end;
        gettimeofday(&start, NULL);
# endif
        if (lp->use_fork)
        {
            if (fork() == 0)
            {
                _unlink_expire(lp, expire_time);
                _exit(0);
            }
        }
        else
        {
            _unlink_expire(lp, expire_time);
        }
# ifdef DEBUG
        gettimeofday(&end, NULL);
        printf("unlink time: %"PRIu64"\n", timeval_diff(&start, &end));
# endif
        lp->last_unlink = now->tv_sec;
    }

    return 0;
}

static char *log_name(dlog_t *lp, struct timeval *now)
{
    sprintf(lp->name, "%s%s", lp->base_name,
            log_suffix(lp->shift_type, now->tv_sec, 0));

    return lp->name;
}

static int _shift_log(dlog_t *lp, struct timeval *now)
{
    if (lp->log_num == 1)
    {
        unlink(lp->name);

        return 0;
    }

    char path[PATH_MAX];
    char new_path[PATH_MAX];
    int  num = 0;
    int  i;

    if (lp->log_num == 0)
    {
        for (i = 0;; ++i)
        {
            snprintf(path, PATH_MAX, "%s%s", lp->base_name,
                    log_suffix(lp->shift_type, now->tv_sec, i));
            if (access(path, F_OK) == 0)
                ++num;
            else
                break;
        }
    }
    else
    {
        num = lp->log_num - 1;
    }

    for (i = num - 1; i >= 0; --i)
    {
        snprintf(path, PATH_MAX, "%s%s", lp->base_name,
                log_suffix(lp->shift_type, now->tv_sec, i));

        if (access(path, F_OK) == 0)
        {
            snprintf(new_path, PATH_MAX, "%s%s", lp->base_name,
                    log_suffix(lp->shift_type, now->tv_sec, i + 1));

            rename(path, new_path);
        }
    }

    return 0;
}

static int shift_log(dlog_t *lp, struct timeval *now)
{
    if (lp->max_size == 0)
        return 0;

    struct stat fs;
    if (stat(lp->name, &fs) < 0)
    {
        if (errno == ENOENT)
            return 0;
        else
            return -1;
    }

    int ret = 0;

    if (fs.st_size >= lp->max_size)
    {
# ifdef DEBUG
        struct timeval start, end;
        gettimeofday(&start, NULL);
# endif
        if (lp->use_fork)
        {
            if (fork() == 0)
            {
                _shift_log(lp, now);
                _exit(0);
            }
        }
        else
        {
            _shift_log(lp, now);
        }
# ifdef DEBUG
        gettimeofday(&end, NULL);
        printf("shift time: %"PRIu64"\n", timeval_diff(&start, &end));
# endif
    }

    return ret;
}

static ssize_t xwrite(int fd, const void *buf, size_t len)
{
    ssize_t nr;
    while (1)
    {
        nr = write(fd, buf, len);
        if ((nr < 0) && (errno == EAGAIN || errno == EINTR))
            continue;

        return nr;
    }
}

static ssize_t write_in_full(int fd, const void *buf, size_t count)
{
    const char *p = buf;
    ssize_t total = 0;

    while (count > 0)
    {
        ssize_t written = xwrite(fd, p, count);
        if (written < 0)
            return -1;
        if (!written)
        {
            errno = ENOSPC;
            return -1;
        }
        count -= written;
        p += written;
        total += written;
    }

    return total;
}

static int flush_log(dlog_t *lp, struct timeval *now)
{
    log_name(lp, now);

    shift_log(lp, now);
    unlink_expire(lp, now);

    ssize_t n = 0;

    if (lp->w_len)
    {
        int fd = open(lp->name, O_WRONLY | O_APPEND | O_CREAT, 0664);
        if (fd < 0)
            return -1;

        n = write_in_full(fd, lp->buf, lp->w_len);
        lp->w_len = 0;

        close(fd);
# ifdef DEBUG
        ++write_times;
# endif
        lp->last_write = *now;
    }

    if (n < 0)
        return -1;

    return 0;
}

static void dlog_atexit()
{
    dlog_t *lp = lp_list_head;

    while (lp)
    {
        dlog_t *_lp = lp;
        lp = (dlog_t *)lp->next;
        dlog_fini(_lp);
    }
}

dlog_t *dlog_init(char *base_name, int shift_type,
        size_t max_size, int log_num, int keep_time)
{
    if (base_name == NULL)
        return NULL;

    int use_fork = shift_type & DLOG_USE_FORK;
    shift_type &= ~DLOG_USE_FORK;

    int no_cache = shift_type & DLOG_NO_CACHE;
    shift_type &= ~DLOG_NO_CACHE;

    if (shift_type < DLOG_SHIFT_BY_SIZE || shift_type > DLOG_SHIFT_BY_DAY)
        return NULL;

    dlog_t *lp = calloc(1, sizeof(dlog_t));
    if (lp == NULL)
        return NULL;

    lp->base_name = malloc(strlen(base_name) + 1);
    lp->name = malloc(strlen(base_name) + 30);
    lp->buf_len = WRITE_BUFFER_LEN * 2;
    lp->buf = malloc(lp->buf_len);

    if (lp->base_name == NULL || lp->name == NULL || lp->buf == NULL)
    {
        if (lp->base_name)
            free(lp->base_name);
        if (lp->name)
            free(lp->name);
        if (lp->buf)
            free(lp->buf);

        free(lp);

        return NULL;
    }

    strcpy(lp->base_name, base_name);

    lp->shift_type = shift_type;
    lp->use_fork   = use_fork;
    lp->no_cache   = no_cache;
    lp->max_size   = max_size;
    lp->log_num    = log_num;
    lp->keep_time  = keep_time;

    struct timeval now;
    gettimeofday(&now, NULL);
    lp->last_write = now;

    if (init_flag == 0)
    {
        atexit(dlog_atexit);
        init_flag = 1;
    }
    
    int fd = open(log_name(lp, &now), O_WRONLY | O_APPEND | O_CREAT, 0664);
    if (fd < 0)
    {
        free(lp->base_name);
        free(lp->name);
        free(lp->buf);
        free(lp);

        return NULL;
    }

    close(fd);

    if (lp_list_head == NULL)
    {
        lp_list_head = lp;
    }
    else
    {
        dlog_t *_lp = lp_list_head;
        while (_lp->next)
            _lp = (dlog_t *)_lp->next;
        _lp->next = (void *)lp;
    }

    return lp;
}

static void _dlog_check(dlog_t *lp, struct timeval *now)
{
    if ((timeval_diff(&lp->last_write, now) >= WRITE_INTERVAL_IN_USEC) ||
            (lp->w_len >= WRITE_BUFFER_LEN))
    {
        flush_log(lp, now);
    }
}

void dlog_check(dlog_t *lp, struct timeval *tv)
{
    struct timeval now;

    if (tv == NULL)
    {
        gettimeofday(&now, NULL);
        tv = &now;
    }

    if (lp == NULL)
    {
        lp = lp_list_head;
        while (lp)
        {
            _dlog_check(lp, tv);
            lp = (dlog_t *)lp->next;
        }
    }
    else
    {
        _dlog_check(lp, tv);
    }
}

static char *timeval_str(struct timeval *tv)
{
    static char str[64];

    struct tm *t = localtime(&tv->tv_sec);
    snprintf(str, sizeof(str), "%04d-%02d-%02d %02d:%02d:%02d.%.6d",
            t->tm_year + 1990, t->tm_mon + 1, t->tm_mday, t->tm_hour,
            t->tm_min, t->tm_sec, (int)tv->tv_usec);

    return str;
}

int dlog(dlog_t *lp, char *fmt, ...)
{
    if (!lp || !fmt)
        return -1;

    struct timeval now;
    gettimeofday(&now, NULL);
    char *timestmap = timeval_str(&now);

    ssize_t n  = 0;
    ssize_t ret;

    char *p = lp->buf + lp->w_len;
    size_t len = lp->buf_len - lp->w_len;

    ret = snprintf(p, len, "[%s] ", timestmap);
    if (ret < 0)
        return -1;

    p += ret;
    len -= ret;
    n += ret;

    va_list args;
    va_start(args, fmt);
    ret = vsnprintf(p, len, fmt, args);
    va_end(args);
    
    if (ret < 0)
    {
        return -1;
    }
    else if (ret >= len)
    {
        flush_log(lp, &now);

        FILE *fp = fopen(log_name(lp, &now), "a+");
        if (fp == NULL)
            return -2;

        fprintf(fp, "[%s] ", timestmap);
        va_start(args, fmt);
        vfprintf(fp, fmt, args);
        va_end(args);
        fprintf(fp, "\n");

        fclose(fp);
    }
    else
    {
        n += ret;
        lp->w_len += n;

        lp->buf[lp->w_len] = '\n';
        lp->w_len += 1;

        if (lp->no_cache)
            flush_log(lp, &now);
        else
            _dlog_check(lp, &now);
    }

    return 0;
}

int dlog_fini(dlog_t *lp)
{
    if (lp == lp_list_head)
    {
        lp_list_head = (dlog_t *)lp->next;
    }
    else
    {
        dlog_t *_lp = lp_list_head;
        while (_lp)
        {
            if (_lp->next == lp)
            {
                _lp->next = lp->next;

                break;
            }
            else
            {
                _lp = (dlog_t *)_lp->next;
            }
        }

        return -1; /* not found */
    }

    struct timeval now;
    gettimeofday(&now, NULL);

    flush_log(lp, &now);

    free(lp->base_name);
    free(lp->name);
    free(lp->buf);
    free(lp);

# ifdef DEBUG
    printf("write %d times\n", write_times);
# endif

    return 0;
}

static char *strtolower(char *str)
{
    char *s = str;

    while (*s)
    {
        if (*s >= 'A' && *s <= 'Z')
            *s += ('a' - 'A');

        ++s;
    }

    return str;
}

int dlog_read_flag(char *str)
{
    if (str == NULL)
        return 0;

    char *s   = strdup(str);
    int  flag = 0;

    char *f   = strtok(s, "\r\n\t ,");
    while (f != NULL)
    {
        strtolower(f);

        if (strcmp(f, "fatal") == 0)
            flag |= DLOG_FATAL;
        else if (strcmp(f, "error") == 0)
            flag |= DLOG_ERROR;
        else if (strcmp(f, "warn") == 0)
            flag |= DLOG_WARN;
        else if (strcmp(f, "info") == 0)
            flag |= DLOG_INFO;
        else if (strcmp(f, "notice") == 0)
            flag |= DLOG_NOTICE;
        else if (strcmp(f, "debug") == 0)
            flag |= DLOG_DEBUG;
        else if (strcmp(f, "user1") == 0)
            flag |= DLOG_USER1;
        else if (strcmp(f, "user2") == 0)
            flag |= DLOG_USER2;
        else
            ;

        f = strtok(NULL, "\r\n\t ,");
    }

    return flag;
}

