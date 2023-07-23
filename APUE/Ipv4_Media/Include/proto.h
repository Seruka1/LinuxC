#ifndef PROTO1_H__
#define PROTO1_H__

#include "site_type.h"

#define DEFAULT_MGROUP "224.2.2.2"
#define DEFAULT_RCVPORT "1989"

#define CHNNR 100
#define LISTCHNID 0
#define MINCHNID 1
#define MAXCHNID (CHNNR + MINCHNID - 1)

#define MSG_CHANNEL_MAX (65536 - 20 - 8) // tcp报头为20字节，udp报头为8字节
#define MAX_DATA (MSG_CHANNEL_MAX - sizeof(chnid_t))

#define MSG_LIST_MAX (65536 - 20 - 8)
#define MAX_ENTRY (MSG_LIST_MAX - sizeof(chnid_t))

// 频道
struct msg_channel_st
{
    chnid_t chnid;
    uint8_t data[0];
} __attribute__((packed));

struct msg_listentry_st
{
    chnid_t chnid;
    uint16_t len;
    uint8_t desc[0];
} __attribute__((packed));

struct msg_list_st
{
    chnid_t chnid;
    struct msg_listentry_st entry[0];
} __attribute__((packed));

#endif