#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <syslog.h>

static int si_init_flag = 0;
static int s_conf_fd = -1;
static struct sockaddr_un s_conf_addr;
static int s_buf_index = 0;
static char s_conf_buf[4][(16384)];
static pthread_mutex_t s_atomic;
static char CONF_LOCAL_NAME[16] = "/tmp/cfgClient";

inline int cfgSockSetLocal(const char *pLocalName)
{
    if (pLocalName)
    {
        snprintf(CONF_LOCAL_NAME, 15, "%s", pLocalName);
        return 0;
    }
    else
        return -1;
}

int cfgSockInit()
{
    if (si_init_flag)
        return 0;

    s_conf_fd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (s_conf_fd < 0)
    {
        return -1;
    }

    if (0 != pthread_mutex_init(&s_atomic, 0))
    {
        close(s_conf_fd);
        s_conf_fd = -1;

        return -1;
    }

    unlink(CONF_LOCAL_NAME);

    bzero(&s_conf_addr, sizeof(s_conf_addr));
    s_conf_addr.sun_family = AF_UNIX;

    snprintf(s_conf_addr.sun_path, sizeof(s_conf_addr.sun_path), "%s", CONF_LOCAL_NAME);

    bind(s_conf_fd, (struct sockaddr *)&s_conf_addr, sizeof(s_conf_addr));

    bzero(&s_conf_addr, sizeof(s_conf_addr));
    s_conf_addr.sun_family = AF_UNIX;

    snprintf(s_conf_addr.sun_path, sizeof(s_conf_addr.sun_path), "/tmp/cfgServer");

    si_init_flag = 1;

    return 0;
}

int cfgSockUninit()
{
    if (!si_init_flag)
        return 0;

    cfgSockSaveFiles();

    if (s_conf_fd > 0)
    {
        close(s_conf_fd);
    }

    s_conf_fd = -1;

    unlink(CONF_LOCAL_NAME);

    pthread_mutex_destroy(&s_atomic);

    si_init_flag = 0;

    return 0;
}

static int cfgSockSend(const char *command)
{
    int ret = -1;

    int addrlen = sizeof(s_conf_addr);
    int len = strlen(command);

    if (!si_init_flag)
        return -1;

    ret = sendto(s_conf_fd, command, len, 0, (struct sockaddr *)&s_conf_addr, addrlen);

    if (ret != len)
    {
        close(s_conf_fd);
        s_conf_fd = -1;

        ret = -1;

        si_init_flag = 0;
        syslog(LOG_ERR, "send conf message failed, ret = %d, errno %d\n", ret, errno);
    }

    return ret;
}

static int cfgSockRecv(char *buf, int len)
{
    int ret;
    if (!si_init_flag)
        return -1;

    ret = recv(s_conf_fd, buf, len - 1, 0);
    if (ret > 0)
        buf[ret] = '\0';
    else
        buf[0] = '\0';

    return ret;
}

static int cfgSockRequest(const char *command, char *recvbuf, int recvlen)
{
    int ret;

    if (!command || !recvbuf || !recvlen)
        return -1;

    ret = cfgSockSend(command);
    if (ret >= 0)
    {
        ret = cfgSockRecv(recvbuf, recvlen);
    }

    return ret;
}

const char *cfgSockGetValue(const char *a_pSection, const char *a_pKey, const char *a_pDefault)
{
    int nRet = -1;
    char *conf_buf;

    if (!si_init_flag || (!a_pSection && !a_pKey))
        return 0;

    pthread_mutex_lock(&s_atomic);
    conf_buf = s_conf_buf[s_buf_index];

    s_buf_index = (s_buf_index + 1) & 3;

    if (a_pSection && a_pKey)
    {
        if (!a_pDefault)
            snprintf(conf_buf, (16384), "R %s.%s %s", a_pSection, a_pKey, "NULL");
        else
            snprintf(conf_buf, (16384), "R %s.%s %s", a_pSection, a_pKey, a_pDefault);
    }
    else
    {
        const char *key = (a_pSection) ? a_pSection : a_pKey;

        if (!a_pDefault)
            snprintf(conf_buf, (16384), "R %s %s", key, "NULL");
        else
            snprintf(conf_buf, (16384), "R %s %s", key, a_pDefault);
    }

    cfgSockRequest(conf_buf, conf_buf, (16384));

    pthread_mutex_unlock(&s_atomic);

    nRet = strncmp(conf_buf, "NULL", 4);
    if (0 == nRet)
    {
        return a_pDefault;
    }

    return conf_buf;
}

int cfgSockSetValue(const char *a_pSection, const char *a_pKey, const char *a_pValue)
{
    char *conf_buf;

    if (!si_init_flag || (!a_pSection && !a_pKey) || !a_pValue)
        return -1;

    pthread_mutex_lock(&s_atomic);
    conf_buf = s_conf_buf[s_buf_index];

    s_buf_index = (s_buf_index + 1) & 3;

    if (a_pSection && a_pKey)
    {
        snprintf(conf_buf, (16384), "W %s.%s %s", a_pSection, a_pKey, a_pValue);
    }
    else
    {
        const char *key = (a_pSection) ? a_pSection : a_pKey;

        snprintf(conf_buf, (16384), "W %s %s", key, a_pValue);
    }

    cfgSockRequest(conf_buf, conf_buf, (16384));

    pthread_mutex_unlock(&s_atomic);

    return 0;
}

int cfgSockSaveFiles()
{
    char *conf_buf;
    if (!si_init_flag)
        return -1;

    pthread_mutex_lock(&s_atomic);
    conf_buf = s_conf_buf[s_buf_index];

    s_buf_index = (s_buf_index + 1) & 3;

    cfgSockRequest("s", conf_buf, (16384));

    pthread_mutex_unlock(&s_atomic);

    return 0;
}

