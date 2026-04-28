/*
    route.c -- Route Management

    This module implements the loading of a route configuration file
    and the routing of requests.

    The route configuration is loaded form a text file that uses the schema (see route.txt)
        uri: type: uri: method: ability [ability...]: redirect
        user: name: password: role [role...]
        role: name: ability [ability...]

    Copyright (c) All Rights Reserved. See details at the end of the file.
*/

/********************************* Includes ***********************************/

#include    "goahead.h"
#include    "cjson.h"
#include    "AntsWebCommon.h"
#include "ovfs_web_rest.h"
/*********************************** Locals ***********************************/

static WebsRoute **routes = 0;
static WebsHash handlers = -1;
WebsHash actionTable = -1;            /* Symbol table for actions */
static int routeCount = 0;
static int routeMax = 0;

#define WEBS_MAX_ROUTE 16               /* Maximum passes over route set */

/********************************** Forwards **********************************/

static bool continueHandler(Webs *wp);
static void freeRoute(WebsRoute *route);
static void growRoutes();
static int lookupRoute(char *uri);
static bool redirectHandler(Webs *wp);
static bool actionHandler(Webs *wp);
static void closeAction();

extern int web_action_prepare(webs_t wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct **header, cJSON_Struct **indata, cJSON_Struct **outdata);
extern int web_action_clean(webs_t wp, int retCode, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);
extern const char *Ovfs_Web_StrError(int error);
extern int name_in_white_list(char* name);

/************************************ Code ************************************/
void webs_route_request(Webs *wp)
{
    WebsRoute   *route = NULL;
    WebsHandler *handler = NULL;
    ssize plen;
    ssize len;
    int safeMethod;
    int i;

    if(smatch(wp->method, "OPTIONS"))
    {
        websResponse(wp, 200, NULL);
        return;
    }

    safeMethod = smatch(wp->method, "POST") || smatch(wp->method, "GET") || smatch(wp->method, "HEAD");
    plen = slen(wp->path);
    wp->route = 0;

    for (i = 0; i < routeCount; i++)
    {
        route = routes[i];

        if (plen < route->prefixLen)
		    continue;
        len = min(route->prefixLen, plen);
        if (strncmp(wp->path, route->prefix, len) != 0)
        {
            continue;
        }

        if ((route->protocol != NULL) && (!smatch(route->protocol, wp->protocol)))
        {
            DEBUGV3( "Route %s does not match protocol %s", route->prefix, wp->protocol);
            continue;
        }

        if (route->methods >= 0)
        {
            if (!hashLookup(route->methods, wp->method))
            {
                DEBUGV3( "Route %s does not match method %s", route->prefix, wp->method);
                continue;
            }
        }
        else if (!safeMethod)
        {
            continue;
        }

        if (route->extensions >= 0 && (wp->ext == NULL || !hashLookup(route->extensions, &wp->ext[1])))
        {
            DEBUGV3("Route %s doesn match extension %s", route->prefix, wp->ext ? wp->ext : "");
            continue;
        }

        wp->route = route;

        handler = route->handler;
        if (NULL == handler)
        {
            continue;
        }

	/*	if(!name_in_white_list(wp->pathlast))
		{
			if(g_ovfs_web->enable_session && !smatch(wp->pathlast,"frmUserLogin")){
				MUTEX_LOCK(g_ovfs_web->hReqSessionLock);
				LOGD("%d %s\n",g_ovfs_web->enable_session ,wp->pathlast);
				WEB_LOGINSUCCESS_NODE_T* tmp = Common_DList_Search(g_ovfs_web->userLoginList, (void *)wp, web_loginsuccess_nodecompareV3);
		        if ((tmp == NULL))
		        {
		        	LOGE("Need Login !!!!!!\n");
					websError(wp, HTTP_CODE_FORBIDDEN, "Session Enabled!You need login server before request any API resources!");
					MUTEX_UNLOCK(g_ovfs_web->hReqSessionLock);
		            return;
		        }
				MUTEX_UNLOCK(g_ovfs_web->hReqSessionLock);
			}
		}*/


        // (*handler->service)(wp)调用成功返回1
        if ((NULL == handler->service) || (*handler->service)(wp))
        {
            /* Handler matches */
            return;
        }

        wp->route = NULL;
        if (wp->flags & WEBS_REROUTE)
        {
            wp->flags &= ~WEBS_REROUTE;
            if (++wp->routeCount >= WEBS_MAX_ROUTE)
            {
                break;
            }
            i = 0;
        }
    }
    if (wp->routeCount >= WEBS_MAX_ROUTE)
    {
        LOGI("Route loop for %s\n", wp->url);
    }
    websError(wp, HTTP_CODE_NOT_FOUND, "Cannot find suitable route for request.");

    return ;
}
/*
    Route the request. If wp->route is already set, test routes after that route
 */

PUBLIC void websRouteRequest(Webs *wp,char* ssid)
{
    WebsRoute   *route;
    WebsHandler *handler;
    ssize       plen, len;
    bool        safeMethod,isWhiteNames;
    int         i;

    safeMethod = smatch(wp->method, "POST") || smatch(wp->method, "GET") || smatch(wp->method, "HEAD");
    plen = slen(wp->path);
    wp->route = 0;
    //firstly we check if it is a whiteName request
    if (strstr(wp->path,"upload") || strstr(wp->path,"Capture")
        	|| strstr(wp->path,"frmSetConfigFile")
        	|| strstr(wp->path,"frmGetConfigFile"))
    {
       isWhiteNames = TRUE;
    }
    else
    {
       isWhiteNames = FALSE;
    }

    if (ssid)
    {
        LOGW("SSID:%s\n", ssid);
    }

    pthread_rwlock_rdlock(&rw_session);
    //no session and not white names
    if((wp->session = websGetSessionBySSID(wp, ssid)) == NULL && !isWhiteNames)
    {
        if(NULL == strstr(wp->path,"frmUserLogin"))
        {
            websError(wp, HTTP_CODE_FORBIDDEN, "Can't get proper session,You need Login first!");
            pthread_rwlock_unlock(&rw_session);
            return;
        }
    }
    pthread_rwlock_unlock(&rw_session);

    for (i = 0; i < routeCount; i++)
    {
        route = routes[i];

        if (plen < route->prefixLen)
		    continue;
        len = min(route->prefixLen,  plen);

        /*
            Match route
         */
        if (strncmp(wp->path, route->prefix, len) != 0)
        {
            continue;
        }

        if (route->protocol && !smatch(route->protocol, wp->protocol))
        {
            DEBUGV3( "Route %s does not match protocol %s", route->prefix, wp->protocol);
            continue;
        }

        if (route->methods >= 0)
        {
            if (!hashLookup(route->methods, wp->method))
            {
                DEBUGV3( "Route %s does not match method %s", route->prefix, wp->method);
                continue;
            }
        }
        else if (!safeMethod)
        {
            continue;
        }

        if (route->extensions >= 0 && (wp->ext == NULL || !hashLookup(route->extensions, &wp->ext[1])))
        {
            DEBUGV3("Route %s doesn match extension %s", route->prefix, wp->ext ? wp->ext : "");
            continue;
        }
        wp->route = route;
        //非白名单的api才进入判断逻辑
        if(!isWhiteNames)
        {
            BOOL needAuth = FALSE;
            //验证类型匹配
            if(route->authType && wp->authType)
            {
                needAuth = smatch(route->authType, wp->authType);
            }
            if (needAuth && !websAuthenticate(wp, ssid))
            {
                return;
            }
        }
        if ((handler = route->handler) == 0)
        {
            continue;
        }

        pthread_rwlock_wrlock(&rw_session);
        if ((wp->session = websGetSessionBySSID(wp, ssid)) != NULL)
        {
            //we increse the timestamp,refresh it
            {
                WebsSession* session = wp->session;
                if (session)
                {
            		session->expires = time(0) + ME_GOAHEAD_LIMIT_SESSION_LIFE;
                }
            }
            //decrese the entry num
            {
                WebsValue* val = websGetSessionVar(wp, WEBS_DIGESTAUTH_ENTRY);
                if (val)
                {
                    websSetSessionVar(wp, WEBS_DIGESTAUTH_ENTRY , valueInteger(val->value.integer - 1));
                    LOGD("[SSID:%s  usrname:%s entry_num: %d]\n", wp->session->id, wp->username, val->value.integer);
                }
            }
        }
        pthread_rwlock_unlock(&rw_session);

        if (!handler->service || (*handler->service)(wp))
        {
            /* Handler matches */
            return;
        }
        wp->route = 0;
        if (wp->flags & WEBS_REROUTE)
        {
            wp->flags &= ~WEBS_REROUTE;
            if (++wp->routeCount >= WEBS_MAX_ROUTE)
            {
                break;
            }
            i = 0;
        }
    }
    if (wp->routeCount >= WEBS_MAX_ROUTE)
    {
        LOGI("Route loop for %s\n", wp->url);
    }
    websError(wp, HTTP_CODE_NOT_FOUND, "Cannot find suitable route for request.");
}

/*
    If pos is < 0, then add to the end. Otherwise insert at specified position
 */
WebsRoute *websAddRoute(char *uri, char *handler, int pos)
{
    WebsRoute *route = NULL;
    WebsKey *key = NULL;

    if (uri == NULL || *uri == '\0')
    {
        LOGE("Route has bad URI");
        return NULL;
    }
    if ((route = walloc(sizeof(WebsRoute))) == 0)
    {
        return NULL;
    }

    memset(route, 0, sizeof(WebsRoute));
    route->prefix = sclone(uri);
    route->prefixLen = slen(uri);
    route->abilities = -1;
    route->extensions = -1;
    route->methods = -1;
    route->redirects = -1;

    if (!handler)
    {
        handler = "file";
    }

    if ((key = hashLookup(handlers, handler)) == NULL)
    {
        LOGE("Cannot find route handler %s\n", handler);
        wfree(route->prefix);
        wfree(route);
        return NULL;
    }
    route->handler = key->content.value.symbol;
    route->verify = websGetPasswordStoreVerify();

    growRoutes();

    if (pos < 0)
    {
        pos = routeCount;
    }
    if (pos < routeCount)
    {
        memmove(&routes[pos+1], &routes[pos], sizeof(WebsRoute*)*routeCount - pos);
    }
    routes[pos] = route;
    routeCount ++;

    return route;
}


PUBLIC int websSetRouteMatch(WebsRoute *route, char *dir, char *protocol, WebsHash methods, WebsHash extensions,
        WebsHash abilities, WebsHash redirects)
{

    if (dir) {
        route->dir = sclone(dir);
    }
    route->protocol = protocol ? sclone(protocol) : 0;
    route->abilities = abilities;
    route->extensions = extensions;
    route->methods = methods;
    route->redirects = redirects;
    return 0;
}


static void growRoutes()
{
    if (routeCount >= routeMax) {
        routeMax += 16;
        if ((routes = wrealloc(routes, sizeof(WebsRoute*) * routeMax)) == 0) {
            DEBUGV3("Cannot grow routes");
        }
    }
}


static int lookupRoute(char *uri)
{
    WebsRoute   *route;
    int         i;

    for (i = 0; i < routeCount; i++) {
        route = routes[i];
        if (smatch(route->prefix, uri)) {
            return i;
        }
    }
    return -1;
}


static void freeRoute(WebsRoute *route)
{

    if (route->abilities >= 0) {
        hashFree(route->abilities);
    }
    if (route->extensions >= 0) {
        hashFree(route->extensions);
    }
    if (route->methods >= 0) {
        hashFree(route->methods);
    }
    if (route->redirects >= 0) {
        hashFree(route->redirects);
    }
    wfree(route->prefix);
    wfree(route->dir);
    wfree(route->protocol);
    wfree(route->authType);
    wfree(route);
}


PUBLIC int websRemoveRoute(char *uri)
{
    int         i;

    if ((i = lookupRoute(uri)) < 0) {
        return -1;
    }
    freeRoute(routes[i]);
    for (; i < routeCount; i++) {
        routes[i] = routes[i+1];
    }
    routeCount--;
    return 0;
}


int websOpenRoute()
{
    if ((handlers = hashCreate(-1)) < 0)
    {
        return -1;
    }
    websDefineHandler("continue", continueHandler, NULL, NULL, 0);
    websDefineHandler("redirect", redirectHandler, NULL, NULL, 0);
    websDefineHandler("action", NULL, actionHandler, closeAction, 0);

    return 0;
}


void websCloseRoute()
{
    WebsHandler *handler;
    WebsKey     *key;
    int         i;

    if (handlers >= 0)
	{
        for (key = hashFirst(handlers); key; key = hashNext(handlers, key))
		{
            handler = key->content.value.symbol;
            if (handler->close)
			{
                (*handler->close)();
            }
            wfree(handler->name);
            wfree(handler);
        }
        hashFree(handlers);
        handlers = -1;
    }
    if (routes)
	{
        for (i = 0; i < routeCount; i++)
		{
            freeRoute(routes[i]);
        }
        wfree(routes);
        routes = 0;
    }
    routeCount = routeMax = 0;
}


int websDefineHandler(char *name, WebsHandlerProc match, WebsHandlerProc service, WebsHandlerClose close, int flags)
{
    WebsHandler     *handler;

    if ((handler = walloc(sizeof(WebsHandler))) == 0)
	{
        return -1;
    }

    memset(handler, 0, sizeof(WebsHandler));

    handler->name = sclone(name);
    handler->match = match;
    handler->service = service;
    handler->close = close;
    handler->flags = flags;

    hashEnter(handlers, name, valueSymbol(handler), 0);

    return 0;
}

/*
    Handler to just continue matching other routes
 */
static bool continueHandler(Webs *wp)
{
    return 0;
}


/*
    Handler to redirect to the default (code zero) URI
 */
static bool redirectHandler(Webs *wp)
{
    return websRedirectByStatus(wp, 0) == 0;
}

/*
    Process an action request. Returns 1 always to indicate it handled the URL
    Return true to indicate the request was handled, even for errors.
 */
static bool actionHandler(Webs *wp)
{
    WebsKey *sp = NULL;
    char *actionName = wp->pathlast;
    WebsProc  web_action_proc;

    if (actionName == NULL || strlen(actionName) < 1)
    {
        websError(wp, HTTP_CODE_NOT_FOUND, "Missing action name");
        return 1;
    }
    /*
        Lookup the C action function first and then try tcl (no javascript support yet).
     */
    sp = hashLookup(actionTable, actionName);
    if (sp == NULL)
    {
        websError(wp, HTTP_CODE_NOT_FOUND, "Action %s is not defined", actionName);
    }
    else
    {
        web_action_proc = (WebsProc)sp->content.value.symbol;
        if (web_action_proc)
        {
            int ret = 0;
            OVFS_WEB_OPTION_S opt = {0};
            cJSON_Struct *header = NULL;
            cJSON_Struct *indata = NULL;
            cJSON_Struct *outdata = NULL;

            ret = web_action_prepare(wp, &opt, &header, &indata, &outdata);
            //LOGW("prepare error=%lx(%s) request=(%s %s) query=(%s)\n", ret, Ovfs_Web_StrError(ret), wp->method, wp->path, wp->query);
            if (g_ovfs_web->debugPrint)
            {
                PRINT_DBG("prepare error=%x(%s) request=(%s %s) query=(%s) indata=\n", ret, Ovfs_Web_StrError(ret), wp->method, wp->path, wp->query);
            }
            else if (wp->authDetails && ret != 0)
            {
                LOGE("prepare error=%x(%s) request=(%s %s) query=(%s)\n", ret, Ovfs_Web_StrError(ret), wp->method, wp->path, wp->query);
            }

            if (0 == ret)
            {

                ret = (*web_action_proc)(wp, &opt, header, indata, outdata);
                if (g_ovfs_web->debugPrint)
                {
                    LOGW("action error=%x(%s) request=(%s %s) query=(%s) outdata=\n", ret, Ovfs_Web_StrError(ret), wp->method, wp->path, wp->query);
                    //Common_Json_StandardPrint(outdata, NULL, NULL, NULL);
                }

                if (ret == 0)
                {
                    ovfs_web_write_log(header, actionName, &opt, "");
                }
            }

            web_action_clean(wp, ret, header, indata, outdata);
        }
    }

    return 1;
}

/*
    Define a function in the "action" map space
 */
int websDefineAction(cchar *name, void *fn)
{
    if (fn == NULL) {
        return -1;
    }
    hashEnter(actionTable, (char *)name, valueSymbol(fn), 0);

    return 0;
}


static void closeAction()
{
    if (actionTable != -1) {
        hashFree(actionTable);
        actionTable = -1;
    }
}


void websActionOpen()
{
    actionTable = hashCreate(WEBS_HASH_INIT);
}
