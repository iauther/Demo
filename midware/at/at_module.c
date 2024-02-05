#include "at_module.h"

static at_module_t *g_at_module = NULL;

int at_module_register(at_module_t *module)
{
    if (!g_at_module) {
        g_at_module = module;
        return 0;
    }

    return -1;
}

int at_module_register_default()
{
    g_at_module = NULL;

    return 0;
}

int at_module_init(void)
{
    if (g_at_module && g_at_module->init) {
        return g_at_module->init();
    }
    return -1;
}

int at_module_parse_domain(const char *host_name, char *host_ip, size_t host_ip_len)
{
    if (g_at_module && g_at_module->parse_domain) {
        return g_at_module->parse_domain(host_name, host_ip, host_ip_len);
    }
    return -1;
}

int at_module_connect(const char *ip, const char *port, at_proto_t proto)
{
    if (g_at_module && g_at_module->connect) {
        return g_at_module->connect(ip, port, proto);
    }
    return -1;
}

int at_module_connect_with_size(const char *ip, const char *port, at_proto_t proto, size_t socket_buffer_size)
{
    if (g_at_module && g_at_module->connect_with_size) {
        return g_at_module->connect_with_size(ip, port, proto, socket_buffer_size);
    }
    return -1;
}

int at_module_send(int sock, const void *buf, size_t len)
{
    if (g_at_module && g_at_module->send) {
        return g_at_module->send(sock, buf, len);
    }
    return -1;
}

int at_module_recv(int sock, void *buf, size_t len)
{
    if (g_at_module && g_at_module->recv) {
        return g_at_module->recv(sock, buf, len);
    }
    return -1;
}

int at_module_recv_timeout(int sock, void *buf, size_t len, uint32_t timeout)
{
    if (g_at_module && g_at_module->recv_timeout) {
        return g_at_module->recv_timeout(sock, buf, len, timeout);
    }
    return -1;
}

int at_module_sendto(int sock, char *ip, char *port, const void *buf, size_t len)
{
    if (g_at_module && g_at_module->sendto) {
        return g_at_module->sendto(sock, ip, port, buf, len);
    }
    return -1;
}

int at_module_recvfrom(int sock, void *buf, size_t len)
{
    if (g_at_module && g_at_module->recvfrom) {
        return g_at_module->recvfrom(sock, buf, len);
    }
    return -1;
}

int at_module_recvfrom_timeout(int sock, void *buf, size_t len, uint32_t timeout)
{
    if (g_at_module && g_at_module->recvfrom_timeout) {
        return g_at_module->recvfrom_timeout(sock, buf, len, timeout);
    }
    return -1;
}

int at_module_close(int sock)
{
    if (g_at_module && g_at_module->close) {
        return g_at_module->close(sock);
    }
    return -1;
}

