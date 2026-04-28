/*
    goahead.c -- Main program for GoAhead

    Usage: goahead [options] [documents] [IP][:port] ...
        Options:
        --auth authFile        # User and role configuration
        --background           # Run as a Linux daemon
        --home directory       # Change to directory to run
        --log logFile:level    # Log to file file at verbosity level
        --route routeFile      # Route configuration file
        --verbose              # Same as --log stdout:2
        --version              # Output version information

    Copyright (c) All Rights Reserved. See details at the end of the file.
 */

/********************************* Includes ***********************************/

#include    "goahead.h"

/********************************* Defines ************************************/

#include  "AntsWebCommon.h"
#include "ovfs_web_rest.h"
#include <sys/vfs.h>

#if LINUX
/*
    start nginx server
*/
int StartNginxServer(){
    char start_cmd[128] = {0};
    sprintf(start_cmd,"%s -p %s",NGX_BIN_FILE,NGX_ROOT_PATH);
    DEBUGV3("[start nginx cmd :%s]\n",start_cmd);
    Common_System(start_cmd);
	return 1;
}


/*
    stop nginx server
*/
int StopNginxServer(){
    char stop_cmd[128] = {"killall nginx"};
    DEBUGV3("[stop nginx cmd :%s]\n",stop_cmd);
    Common_System(stop_cmd);
	return 1;
}
/*
    restart nginx server
*/
int RestartNginxServer()
{
    DEBUGV3("[Restart nginx..........]\n");
    StopNginxServer();
    StartNginxServer();
	return 1;
}
/*
when user access the multicard goform to chang one of  https  http port or both.we change the conf file content
*/

int MVConfFile_now_to_old()
{
    char mv_cmd[128] = {0};
    sprintf(mv_cmd,"mv %s %s",NGX_CONF_FILE,NGX_CONF_OLDFILE);
    DEBUGV3("[%s]\n",mv_cmd);
    Common_System(mv_cmd);
   return 1;
}

int MVConfFile_tmp_to_now()
{
    char mv_cmd[128] = {0};
    sprintf(mv_cmd,"mv %s %s",NGX_CONF_TMPFILE,NGX_CONF_FILE);
    DEBUGV3("[%s]\n",mv_cmd);
    Common_System(mv_cmd);


    //��Ϊ����update�е������ļ��Ḳ��root�µ������ļ�
    memset(mv_cmd,0,128);
    sprintf(mv_cmd,"cp %s %s",NGX_CONF_FILE,NGX_CONF_FILE2);
    DEBUGV3("[%s]\n",mv_cmd);
    Common_System(mv_cmd);

	return 1;
}

#define WRITE_STR(s) fwrite(s,strlen(s),1,fp);
extern OVFS_WEB_CONTEXT_T *g_ovfs_web;
int generate_default_ngx_conf()
{


    struct statfs diskInfo;
    statfs("/dev",&diskInfo);
    unsigned long long totalBlocks = diskInfo.f_bsize;
    unsigned long long freeDisk = diskInfo.f_bfree*totalBlocks;
    printf("freespace = %lld %d %d\n",freeDisk,diskInfo.f_bsize,diskInfo.f_bfree);
    unsigned long freeDisk_m = freeDisk/1048576;

	FILE* fp = fopen(NGX_CONF_FILE,"w+");
	if(fp == NULL){
		LOGD("FATAL Error!Open nginx conf file failed!\n");
		return -1;

	}

	WRITE_STR("user root;\n")
    WRITE_STR("daemon off;\n")
    WRITE_STR("master_process off;\n")
	WRITE_STR("worker_processes 1;\n")
	WRITE_STR("\n")
	WRITE_STR("error_log /dev/null crit;\n")
    //		"\tuse epoll;\n"
	WRITE_STR("events{\n"
		"\tworker_connections 32;\n"
		"\tmulti_accept on;\n"
		"}\n");
	WRITE_STR("http{\n"
		"include mime.types;\n"
		"default_type application/octet-stream;\n"
		"access_log off;\n"
		"sendfile on;\n"
		"tcp_nopush on;\n"
		"tcp_nodelay on;\n"
		"etag on;\n"
		"keepalive_timeout 20s;\n"
		"client_body_temp_path /dev/nginx_upload_tmp 3 3;\n"
		"open_file_cache_min_uses 1;\n"
		"open_file_cache_valid 1m;\n");

	WRITE_STR("server{\n"
		"\tindex index.html login.asp;\n"
		"\taccess_log off;\n"
		"\tserver_tokens off;\n"
		"\tserver_name localhost;\n");

	//write http port
	if(g_ovfs_web->enable_http == TRUE){
		fprintf(fp,"\tlisten %d;\n",g_ovfs_web->httpport);
        if(g_ovfs_web->support_ipv6)
        {
            fprintf(fp,"\tlisten [::]:%d;\n",g_ovfs_web->httpport);
        }
	}
	//write https
	if(g_ovfs_web->enable_https == TRUE){
		fprintf(fp,"\tlisten %d default ssl;\n",g_ovfs_web->httpsport);

        if(g_ovfs_web->support_ipv6)
        {
            fprintf(fp,"\tlisten [::]:%d default ssl;\n",g_ovfs_web->httpsport);
        }

		if(g_ovfs_web->enbale_http_redirect_to_https == TRUE){
			WRITE_STR("\tssl on;\n");
		}
		WRITE_STR("\tssl_certificate ../ssl/self.crt;\n"
			"\tssl_certificate_key ../ssl/self.key;\n"
			"\tssl_session_cache shared:SSL:1m;\n"
			"\tssl_session_timeout 5m;\n");

		if(g_ovfs_web->custom_support_protocols && slen(g_ovfs_web->custom_support_protocols)){
			//WRITE_STR("ssl_protocols TLSv1.1 TLSv1.2;\n");
			fprintf(fp,"\tssl_protocols  %s;\n",g_ovfs_web->custom_support_protocols);
		}else{

			WRITE_STR("\tssl_protocols TLSv1.1 TLSv1.2;\n");
		}

		if(g_ovfs_web->custom_ciphers && slen(g_ovfs_web->custom_ciphers)){

			//WRITE_STR("ssl_ciphers EECDH+AESGCM:EDH+AESGCM:AES256+EECDH:AES256+EDH:CHACHA20:AES128:AES256:GCM:!DH:!RC4:!aNULL:!eNULL:!LOW:!3DES:!MD5:!EXP:!PSK:!SRP:!DSS;\n");
			fprintf(fp,"\tssl_ciphers  %s;\n",g_ovfs_web->custom_ciphers);
		}else{

			WRITE_STR("\tssl_ciphers EECDH+AESGCM:EDH+AESGCM:AES256+EECDH:AES256+EDH:CHACHA20:AES128:AES256:GCM:!DH:!RC4:!aNULL:!eNULL:!LOW:!3DES:!MD5:!EXP:!PSK:!SRP:!DSS;\n");
		}




		WRITE_STR("add_header Strict-Transport-Security max-age=31536000;\n"
        	"ssl_prefer_server_ciphers on;\n")
		if(g_ovfs_web->enbale_http_redirect_to_https == TRUE){
			fprintf(fp,"error_page 497 https://$host:%d$uri?$args;\n",g_ovfs_web->httpsport);
		}
	}

	WRITE_STR("add_header X-Frame-Options SAMEORIGIN;\n"
        "add_header X-Content-Type-Options nosniff;\n"
        "add_header X-XSS-Protection \"1; mode=block\";\n"
        "add_header Cache-Control no-cache;\n"
        "if_modified_since exact;\n"
          "location ~.*\\.(jpg|jpeg|png|gif)$\n"
         " { \n"
          "  expires 7d; \n"
         " }\n"

          "location ~.*\\.(css|asp)?$\n"
          "{\n"
         "   expires 7d;\n"
          "}\n"
          "location ~.*\\.wasm$\n"
          "{\n"
         "   expires 7d;\n"
          "}\n"
          "#js\n"
          "location ~.*\\.js$\n"
          "{\n"
            "set $time 7d;\n"
            "if ($uri ~ \"/language/*\") {\n"
            "    set $time -1;\n"
            "}\n"
            "expires $time;\n"
          "}\n"
        "# pass the onvif request to Onvif server\n"
        "location  /onvif/ {\n"
            "\tproxy_read_timeout 60;\n"
            "\tproxy_connect_timeout 60;\n"
            "\tproxy_buffering off;\n"
            "\tproxy_set_header HostAddr $host;\n"
            "\tproxy_set_header RemoteAddr $remote_addr;\n"
            "\tproxy_set_header Scheme $scheme;\n"
            "\t#proxy_pass   http://127.0.0.1:29/onvif/;\n"
            "\tproxy_pass   http://unix:/var/run/onvif/unix;\n"
        "}\n"
        "location  ^~ /page/ {\n"
            "\tproxy_read_timeout 60;\n"
            "\tproxy_connect_timeout 60;\n"
            "\tproxy_buffering off;\n"
            "\tproxy_set_header HostAddr $host;\n"
            "\tproxy_set_header RemoteAddr $remote_addr;\n"
            "\tproxy_pass   http://127.0.0.1:7100/page/;\n"
        "}\n"
        "location ~* /(?:digest|t8s)/upload {\n"
           "\tlimit_except POST          { deny all; }\n"
           "\t #client_body_temp_path /dev/upload_tmp;\n"
           "\t #client_body_in_file_only on;\n"
           "\t client_body_buffer_size 128k;\n");
    fprintf(fp,"\t client_max_body_size %ldM;\n",freeDisk_m);
           WRITE_STR(
           "\t #fastcgi_pass 127.0.0.1:28;\n"
           "\t fastcgi_pass unix:/var/run/webserver/unix;\n"
           "\t fastcgi_param  SCRIPT_FILENAME  /scripts$fastcgi_script_name;\n"
           "\t include        fastcgi_params;\n"
            "\t#file path\n"
            "\t#fastcgi_param  REQUEST_BODY_FILE  $request_body_file;\n"
            "\t#wo don't need send all huge post data to fcgi,it may kill your app.\n"
            "\tfastcgi_request_buffering off;\n"
            "\tfastcgi_pass_request_body on;\n"
            "\tfastcgi_connect_timeout 300;\n"
            "\tfastcgi_send_timeout 300;\n"
            "\tfastcgi_read_timeout 300;\n"
        "} \n"
        "location /language/lang {\n"
        "    #default language js\n"
        "    set $scriptname \"en_gb.js\";\n"
        "    if ($http_cookie ~ \"s_Language=([^;]+)(?:;|$)\") {\n"
        "        set $scriptname $1;\n"
        "    }\n"
        "    rewrite \"^/language/lang$\" /language/$scriptname;\n"
        "}\n"
        "# pass the PHP scripts to FastCGI server listening on 127.0.0.1:9000\n"
        "location ~* /(?:digest|basic|goform|t8s)/[^\\s]*$ {\n"
        "    #fastcgi_pass   127.0.0.1:28;\n"
        "    fastcgi_pass unix:/var/run/webserver/unix;\n"
        "    fastcgi_param   SCRIPT_FILENAME  /scripts$fastcgi_script_name;\n"
        "    include         fastcgi_params;\n"
        "    fastcgi_connect_timeout 300;\n"
        "    fastcgi_send_timeout 300;\n"
        "    fastcgi_read_timeout 300;\n"
        "}\n"
        "location ~* /(?:BoardSys|MediaServer|Core|Network|Alarm|Record)/[^\\s]*$ {\n"
       "	   fastcgi_pass 127.0.0.1:30;\n"
        "	   fastcgi_param  SCRIPT_FILENAME /scripts$fastcgi_script_name;\n"
       " 	   include        fastcgi_params;\n"
        "	   fastcgi_connect_timeout 300;\n"
       " 	   fastcgi_send_timeout 300;\n"
       " 	   fastcgi_read_timeout 300;   \n"
       " } \n"
       "location ~* /(?:iAPI)/[^\\s]*$ {\n"
       "     #fastcgi_pass 127.0.0.1:31;\n"
	"		fastcgi_pass unix:/var/run/iapi3/unix;\n"
    "        fastcgi_param  SCRIPT_FILENAME /scripts$fastcgi_script_name;\n"
    "    include        fastcgi_params;\n"
     "       fastcgi_connect_timeout 300;\n"
     "       fastcgi_send_timeout 300;\n"
     "       fastcgi_read_timeout 300;   \n"
     "   }\n"
     "location ~* /(?:ws)/[^\\s]*$ {\n"
	 "		#proxy_pass http://127.0.0.1:7681;\n"
	 "		proxy_pass http://unix:/var/run/websocket/unix;\n"
     "      proxy_http_version 1.1;\n"
     "		proxy_set_header Upgrade $http_upgrade;\n"
     "      proxy_set_header Connection \"upgrade\";\n"
     "   }\n"
     "location ~* /(?:httpstream)/[^\\s]*$ {\n"
	 "		proxy_pass http://127.0.0.1:32;\n"
     "      proxy_http_version 1.1;\n"
     "   }\n"
     "}\n"
     "}\n")
	fclose(fp);
	return 0;
}
#endif
