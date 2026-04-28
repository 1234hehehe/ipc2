#include "ovfs_web_ngx.h"
#include "libcommon_api.h"

extern int  Common_System(const char *cmd);

void web_stop_nginx()
{
    char cmdBuffer[1024];
        snprintf(cmdBuffer, sizeof(cmdBuffer),
            "/root/nginx/sbin/nginx -p /root/nginx -s stop");
        LOGW("stop nginx:[%s]\n", cmdBuffer);
        Common_System(cmdBuffer);

}

void web_change_web_port(int http_port, int https_port)
{
#if OLD_VERSION
   	FILE* fp = NULL;
    FILE* fpTmp = NULL;
	char line[256] = {0};
	char tmp[256] = {0};
	char *comment_chr = NULL;
	char *listen_chr = NULL;

	if ((fp = fopen(OVFS_NGX_CONF_FILE, "rb+")) != NULL)
	{
        fpTmp = fopen(OVFS_NGX_CONF_TMPFILE,"wb+");
		while (fgets(line, 256, fp) && !ferror(fp) && !feof(fp))
		{
			comment_chr = strchr(line, '#');
			listen_chr = strstr(line, "listen ");

			if (listen_chr)
			{
				if (!comment_chr  || (comment_chr && listen_chr < comment_chr))
				{
					if (https_port > 0 && strstr(line, "ssl"))
					{//https
						sprintf(tmp,"        listen    %d default ssl;\n", https_port);
					}
					else
					{//http
						sprintf(tmp,"        listen    %d;\n", http_port);
					}
					fwrite(tmp, strlen(tmp), 1, fpTmp);
				}
				else
				{
					fwrite(line, strlen(line), 1, fpTmp);
				}

			}
			else
			{
				fwrite(line,strlen(line),1,fpTmp);
			}

			memset(line,0,sizeof(line));
		}
		if(fpTmp)
			fclose(fpTmp);
		fclose(fp);
	}

	web_ngx_config_replace();
#else
	//nginx must alive before this shell command execute
    // zqf说nginx可执行文件路径固定为/root/nginx/sbin/nginx.
     //"sbin/nginx -s reload -p /root/nginx -c conf/nginx.conf || (cp -af conf/nginx.conf.bak conf/nginx.conf && sbin/nginx -s reload -p /root/nginx -c conf/nginx.conf);"
    //  "cp -af conf/nginx.conf conf/nginx.conf.bak;"
    //  "a=0;while [ $a -lt 10 ];do    sbin/nginx -s reload -p /root/nginx -c conf/nginx.conf;if [ $? -eq 0 ];then echo \"nginx reload conf file success!\";netstat -anp;break;else  echo \"nginx reload conf file failed!retry!\";fi;sleep 1;a=`expr $a + 1`;done;"

//   "sed -i -e \"s/listen[[:space:]]*[0-9]*;/listen      %d;/\" -e \"s/listen[[:space:]]*[0-9]*[[:space:]]*default[[:space:]]*ssl;/listen      %d default ssl;/\" conf/nginx.conf;"  -c conf/nginx.conf
    char cmdBuffer[1024];
    snprintf(cmdBuffer, sizeof(cmdBuffer),//"cd /root/nginx;"
		//"cp -af conf/nginx.conf /update/nginx/conf/nginx.conf;"
		//"sbin/nginx -p /root/nginx -s stop;"
	    "/root/nginx/sbin/nginx -p /root/nginx/ &");
	    //"if [ $? == 0 ]; then echo \"ok\" > /dev/ngx_start_status;else echo \"failed\" > /dev/ngx_start_status;fi",
        //http_port, https_port);
    printf("cmdBuffer=[%s]\n", cmdBuffer);
    Common_System(cmdBuffer);

#endif
	return ;
}

#if OLD_VERSION
void web_ngx_config_replace()
{
    char mv_cmd[128] = {0};
    sprintf(mv_cmd, "mv %s %s", OVFS_NGX_CONF_FILE, OVFS_NGX_CONF_OLDFILE);
    Common_System(mv_cmd);

    sprintf(mv_cmd, "mv %s %s", OVFS_NGX_CONF_TMPFILE, OVFS_NGX_CONF_FILE);
    Common_System(mv_cmd);

    sprintf(mv_cmd, "%s -p %s -s reload", OVFS_NGX_BIN_FILE, OVFS_NGX_ROOT);
    Common_System(mv_cmd);

	return ;
}
#endif
