#ifndef CLIENT_H__ 
#define CLIENT_H__

#define DEFAULT_PLAYERCMD "/usr/bin/mpg123 - > /dev/null"
#define DEFAULT_IF_NAME   "eth0"

struct client_conf_st
{
    char *rcvport;
    char *mgroup;
    char *player_cmd;
};

#endif