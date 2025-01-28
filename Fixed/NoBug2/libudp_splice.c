#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <errno.h>

struct udp_splice_handle {
    pthread_mutex_t lock;
    int sock;
    uint16_t id;
};

static int udp_splice_get_family_id(int sock) {
    struct {
        struct nlmsghdr nl;
        char buf[4096];
    } buf;
    struct genlmsghdr *genl;
    struct rtattr *rta;
    struct sockaddr_nl addr = {
        .nl_family = AF_NETLINK,
    };
    int len;

    memset(&buf.nl, 0, sizeof(buf.nl));
    buf.nl.nlmsg_len = NLMSG_LENGTH(GENL_HDRLEN);
    buf.nl.nlmsg_flags = NLM_F_REQUEST;
    buf.nl.nlmsg_type = GENL_ID_CTRL;

    genl = (struct genlmsghdr *)buf.buf;
    memset(genl, 0, sizeof(*genl));
    genl->cmd = CTRL_CMD_GETFAMILY;

    rta = (struct rtattr *)(genl + 1);
    rta->rta_type = CTRL_ATTR_FAMILY_NAME;
    rta->rta_len = RTA_LENGTH(sizeof(UDP_SPLICE_GENL_NAME));
    memcpy(RTA_DATA(rta), UDP_SPLICE_GENL_NAME, sizeof(UDP_SPLICE_GENL_NAME));
    buf.nl.nlmsg_len += rta->rta_len;

    if (sendto(sock, &buf, buf.nl.nlmsg_len, 0, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("sendto failed");
        return -1;
    }

    len = recv(sock, &buf, sizeof(buf), 0);
    if (len < 0) {
        perror("recv failed");
        return -1;
    }
    if (len < sizeof(buf.nl) || buf.nl.nlmsg_len != len) {
        errno = EBADMSG;
        return -1;
    }
    if (buf.nl.nlmsg_type == NLMSG_ERROR) {
        struct nlmsgerr *errmsg = (struct nlmsgerr *)buf.buf;
        errno = -errmsg->error;
        return -1;
    }

    len -= sizeof(buf.nl) + sizeof(*genl);
    while (RTA_OK(rta, len)) {
        if (rta->rta_type == CTRL_ATTR_FAMILY_ID) {
            return *(uint16_t *)RTA_DATA(rta);
        }
        rta = RTA_NEXT(rta, len);
    }

    errno = EBADMSG;
    return -1;
}

void *udp_splice_open(void) {
    struct udp_splice_handle *h;
    int retval;
    struct sockaddr_nl addr;

    h = malloc(sizeof(*h));
    if (!h) {
        perror("malloc failed");
        return NULL;
    }

    retval = pthread_mutex_init(&h->lock, NULL);
    if (retval) {
        errno = retval;
        free(h);
        return NULL;
    }

    h->sock = socket(PF_NETLINK, SOCK_DGRAM | SOCK_NONBLOCK | SOCK_CLOEXEC, NETLINK_GENERIC);
    if (h->sock < 0) {
        perror("socket creation failed");
        pthread_mutex_destroy(&h->lock);
        free(h);
        return NULL;
    }

    memset(&addr, 0, sizeof(addr));
    addr.nl_family = AF_NETLINK;
    if (bind(h->sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind failed");
        close(h->sock);
        pthread_mutex_destroy(&h->lock);
        free(h);
        return NULL;
    }

    retval = udp_splice_get_family_id(h->sock);
    if (retval < 0) {
        close(h->sock);
        pthread_mutex_destroy(&h->lock);
        free(h);
        return NULL;
    }

    h->id = retval;
    return h;
}

int udp_splice_add(void *handle, int sock, int sock2, uint32_t timeout) {
    struct {
        struct nlmsghdr nl;
        struct genlmsghdr genl;
        char attrs[RTA_LENGTH(4) * 3];
    } req;
    struct {
        struct nlmsghdr nl;
        struct nlmsgerr err;
    } res;
    struct rtattr *rta;
    struct sockaddr_nl addr = { .nl_family = AF_NETLINK };
    int len;
    struct udp_splice_handle *h = handle;

    memset(&req, 0, sizeof(req.nl) + sizeof(req.genl));
    req.nl.nlmsg_len = NLMSG_LENGTH(GENL_HDRLEN);
    req.nl.nlmsg_flags = NLM_F_REQUEST | NLM_F_ACK;
    req.nl.nlmsg_type = h->id;

    req.genl.cmd = UDP_SPLICE_CMD_ADD;

    rta = (struct rtattr *)req.attrs;
    rta->rta_type = UDP_SPLICE_ATTR_SOCK;
    rta->rta_len = RTA_LENGTH(4);
    *(uint32_t *)RTA_DATA(rta) = sock;
    req.nl.nlmsg_len += rta->rta_len;

    rta = (struct rtattr *)(((char *)rta) + rta->rta_len);
    rta->rta_type = UDP_SPLICE_ATTR_SOCK2;
    rta->rta_len = RTA_LENGTH(4);
    *(uint32_t *)RTA_DATA(rta) = sock2;
    req.nl.nlmsg_len += rta->rta_len;

    if (timeout) {
        rta = (struct rtattr *)(((char *)rta) + rta->rta_len);
        rta->rta_type = UDP_SPLICE_ATTR_TIMEOUT;
        rta->rta_len = RTA_LENGTH(4);
        *(uint32_t *)RTA_DATA(rta) = timeout;
        req.nl.nlmsg_len += rta->rta_len;
    }

    pthread_mutex_lock(&h->lock);
    if (sendto(h->sock, &req, req.nl.nlmsg_len, 0, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        pthread_mutex_unlock(&h->lock);
        perror("sendto failed");
        return -1;
    }

    len = recv(h->sock, &res, sizeof(res), 0);
    pthread_mutex_unlock(&h->lock);

    if (len < 0) {
        perror("recv failed");
        return -1;
    }
    if (len != sizeof(res) || res.nl.nlmsg_type != NLMSG_ERROR) {
        errno = EBADMSG;
        return -1;
    }
    if (res.err.error) {
        errno = -res.err.error;
        return -1;
    }

    return 0;
}




