#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <glob.h>
#include <syslog.h>
#include <string.h>
#include <errno.h>
#include <error.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "server_conf.h"
#include "medialib.h"
#include "../Include/proto.h"
// #include "proto.h"
#include "mytbf.h"

#define PATHSIZE 1024
#define LINEBUFSIZE 1024

struct channel_context_st
{
    chnid_t chnid;
    char *desc;
    glob_t mp3glob;
    int pos;
    int fd;
    off_t offset;
    mytbf_t *tbf;
};

static struct channel_context_st channel[MAXCHNID + 1];
static chnid_t curr_id = MINCHNID;

static int open_next(chnid_t chnid)
{
    // 最多尝试这么多次
    for (int i = 0; i < channel[chnid].mp3glob.gl_pathc; ++i)
    {
        ++channel[chnid].pos;
        if (channel[chnid].pos == channel[chnid].mp3glob.gl_pathc)
        {
            channel[chnid].pos = 0;
        }
        close(channel[chnid].fd);
        channel[chnid].fd = open(channel[chnid].mp3glob.gl_pathv[channel[chnid].pos], O_RDONLY);
        // 如果打开还是失败
        if (channel[chnid].fd < 0)
        {
            syslog(LOG_WARNING, "open(%s):%s", channel[chnid].mp3glob.gl_pathv[chnid], strerror(errno));
        }
        else // success
        {
            channel[chnid].offset = 0;
            return 0;
        }
    }
    syslog(LOG_ERR, "None of mp3 in channel %d is available.", chnid);
    return -1;
}

static struct channel_context_st *path2entry(const char *path)
{
    char pathstr[PATHSIZE] = {'\0'};
    char linebuf[LINEBUFSIZE];
    FILE *fp;
    struct channel_context_st *me;

    // 解析描述文件
    strncpy(pathstr, path, PATHSIZE);
    strncat(pathstr, DESC_FNAME, PATHSIZE);
    fp = fopen(pathstr, "r");
    if (fp == NULL)
    {
        syslog(LOG_INFO, "%s is not a channel dir(can't find desc.txt)", path);
        return NULL;
    }

    if (fgets(linebuf, LINEBUFSIZE, fp) == NULL)
    {
        syslog(LOG_INFO, "%s is not a channel dir(cant get the desc.txt)", path);
        fclose(fp);
        return NULL;
    }
    fclose(fp);

    me = malloc(sizeof(*me));
    if (me == NULL)
    {
        syslog(LOG_ERR, "malloc():%s", strerror(errno));
        return NULL;
    }

    me->tbf = mytbf_init(MP3_BITRATE / 8 * 3, MP3_BITRATE / 8 * 10);
    if (me->tbf == NULL)
    {
        syslog(LOG_ERR, "mytbf_init():%s", strerror(errno));
        free(me);
        return NULL;
    }
    me->desc = strdup(linebuf);

    // 解析MP3文件集合
    strncpy(pathstr, path, PATHSIZE);
    strncat(pathstr, MP3_PARTERN, PATHSIZE);

    if (glob(pathstr, 0, NULL, &me->mp3glob) != 0)
    {
        syslog(LOG_ERR, "%s is not a channel dir(can not find mp3 files)", path);
        mytbf_destroy(me->tbf);
        free(me);
        return NULL;
    }
    me->pos = 0;
    me->offset = 0;
    me->fd = open(me->mp3glob.gl_pathv[me->pos], O_RDONLY);

    if (me->fd < 0)
    {
        syslog(LOG_WARNING, "%s open failed.", me->mp3glob.gl_pathv[me->pos]);
        mytbf_destroy(me->tbf);
        free(me);
        return NULL;
    }
    me->chnid = curr_id;
    curr_id++;
    return me;
}

int mlib_getchnlist(struct mlib_listentry_st **result, int *resnum)
{
    for (int i = 0; i < MAXCHNID + 1; i++)
    {
        channel[i].chnid = -1;
    }
    char path[PATHSIZE];
    snprintf(path, PATHSIZE, "%s/*", server_conf.media_dir);
    glob_t globres;
    if (glob(path, 0, NULL, &globres))
    {
        return -1;
    }
    struct mlib_listentry_st *ptr;
    ptr = malloc(sizeof(struct mlib_listentry_st) * globres.gl_pathc);
    if (ptr == NULL)
    {
        syslog(LOG_ERR, "malloc() error.");
        exit(1);
    }
    int num = 0;
    struct channel_context_st *res;
    for (int i = 0; i < globres.gl_pathc; ++i)
    {
        res = path2entry(globres.gl_pathv[i]);
        if (res != NULL)
        {
            syslog(LOG_DEBUG, "Channel: %d desc: %s", res->chnid, res->desc);
            // res回填到channel数组中
            memcpy(channel + res->chnid, res, sizeof(*res));
            ptr[num].chnid = res->chnid;
            ptr[num].desc = strdup(res->desc);
            num++;
        }
    }
    *result = realloc(ptr, sizeof(struct mlib_listentry_st) * num);
    if (*result == NULL)
    {
        syslog(LOG_ERR, "realloc() failed.");
    }

    *resnum = num;

    return 0;
}

int mlib_freechnlist(struct mlib_listentry_st *ptr)
{
    free(ptr);
    return 0;
}

size_t mlib_readchn(chnid_t chnid, void *buf, size_t size)
{
    int tbfsize = mytbf_fetchtoken(channel[chnid].tbf, size);
    int len;
    while (1)
    {
        len = pread(channel[chnid].fd, buf, tbfsize, channel[chnid].offset);
        if (len < 0)
        {
            // 当前这首歌可能有问题，读取下一首歌
            syslog(LOG_WARNING, "media file %s pread():%s", channel[chnid].mp3glob.gl_pathv[channel[chnid].pos], strerror(errno));
            open_next(chnid);
        }
        else if (len == 0)
        {
            syslog(LOG_DEBUG, "media %s file is over", channel[chnid].mp3glob.gl_pathv[channel[chnid].pos]);
            open_next(chnid);
        }
        else /*len > 0*/ // 真正读取到了数据
        {
            channel[chnid].offset += len;
            break;
        }
    }
    mytbf_returntoken(channel[chnid].tbf, tbfsize - len);
    return len;
}