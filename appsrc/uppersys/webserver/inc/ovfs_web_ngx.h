#ifndef OVFS_WEB_NGX_H
#define OVFS_WEB_NGX_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define OVFS_NGX_ROOT "/root/nginx"
#define OVFS_NGX_ROOT2 "/update/nginx"

#define OVFS_NGX_HTML OVFS_NGX_ROOT "/html"
#define OVFS_NGX_SSL OVFS_NGX_ROOT "/ssl"
#define OVFS_NGX_CONF_FILE OVFS_NGX_ROOT "/conf/nginx.conf"
#define OVFS_NGX_CONF_FILE2 OVFS_NGX_ROOT2 "/conf/nginx.conf"
#define OVFS_NGX_CONF_TMPFILE OVFS_NGX_ROOT "/conf/nginx_tmp.conf"
#define OVFS_NGX_CONF_OLDFILE OVFS_NGX_ROOT "/conf/nginx_old.conf"
#define OVFS_NGX_BIN_FILE OVFS_NGX_ROOT "/sbin/nginx"

void web_stop_nginx();
void web_change_web_port(int http_port, int https_port);
void web_ngx_config_replace();

#endif
