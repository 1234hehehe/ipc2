/*
    auth.c -- Authorization Management

    This modules supports a user/role/ability based authorization scheme.

    In this scheme Users have passwords and can have multiple roles. A role is associated with the ability to do
    things like "admin" or "user" or "support". A role may have abilities (which are typically verbs) like "add",
    "shutdown".

    When the web server starts up, it loads a route and authentication configuration file that specifies the Users,
    Roles and Routes.  Routes specify the required abilities to access URLs by specifying the URL prefix. Once logged
    in, the user's abilities are tested against the route abilities. When the web server receivess a request, the set of
    Routes is consulted to select the best route. If the routes requires abilities, the user must be logged in and
    authenticated.

    Three authentication backend protocols are supported:
        HTTP basic authentication which uses browser dialogs and clear text passwords (insecure unless over TLS)
        HTTP digest authentication which uses browser dialogs
        Web form authentication which uses a web page form to login (insecure unless over TLS)

    Copyright (c) All Rights Reserved. See details at the end of the file.
*/

/********************************* Includes ***********************************/


#include "goahead.h"
#include "AntsWebCommon.h"
#include "web_inter_api.h"
#if ME_GOAHEAD_AUTH

/*********************************** Locals ***********************************/

static WebsHash users = -1;
//static WebsHash roles = -1;
static char *masterSecret;

static WebsVerify verifyPassword = websVerifyPasswordFromFile;

pthread_rwlock_t rw_user = PTHREAD_RWLOCK_INITIALIZER;

/********************************** Forwards **********************************/


WebsUser *createUser(char *username, char *password,char *encrypted_password);

static void freeUser(WebsUser *up);



#if ME_GOAHEAD_DIGEST
//static char *calcDigest(Webs *wp, char *username, char *password);
static char *createDigestNonce(Webs *wp);
static char *parseDigestNonce(char *nonce, char **secret, char **realm, WebsTime *when);
#endif



/*
根据已有的用户 生成验证的用户Context
信息都存储在hash表中 ，非线程安全
*/
static void InitUserContext()
{
	return ;
}

/************************************ Code ************************************/

PUBLIC bool websVerifyPasswordFromFile(Webs *wp)
{
    char passbuf[ME_GOAHEAD_LIMIT_PASSWORD * 3 + 3];
    bool success;
    //ANTS_DVR_USER_EX struUsers;

    //try find user in session.
    pthread_rwlock_rdlock(&rw_user);
    if (!wp->username && (wp->user = websLookupUser(wp->username)) == 0)
    {
        DEBUGV3("[%s is not existed! ]\n", wp->username);
        pthread_rwlock_unlock(&rw_user);
        return 0;
    }
    LOGD("\nif we got response in header[%d  %s]!\n",wp->encoded,wp->authType);
    /*
        Verify the password. If using Digest auth, we compare the digest of the password.
        Otherwise we encode the plain-text password and compare that
     */
    if (wp->digest != NULL)
    {
        LOGD("wp->digest:%s\n", wp->digest);
    }
    if (wp->password != NULL)
    {
        LOGD("wp->password:%s\n", wp->password);
    }
    if (!wp->encoded)
    {
        fmt(passbuf, sizeof(passbuf), "%s:%s:%s", wp->username, ME_GOAHEAD_REALM, wp->password);
        wfree(wp->password);
        wp->password = websMD5(passbuf);
        wp->encoded = 1;
    }

    if (wp->digest)
    {
        success = smatch(wp->password, wp->digest);
    }
    else
    {
        success = smatch(wp->password, wp->user->password);
    }

#if LINUX
   //登录3次验证失败后 需要锁定账户和触发异常
   if(!success && strstr(wp->path, "frmUserLogin"))
   {
//      long ret = web_dev_login(wp->user->name, wp->password, NULL, 0);
   }
#endif
    pthread_rwlock_unlock(&rw_user);

    return success;
}

PUBLIC bool websAuthenticate(Webs *wp, char *ssid)
{
    WebsRoute *route = NULL;
    WebsValue *username = NULL;
    WebsValue *timeout = NULL;
    WebsValue *entry_num = NULL;
    int cached;
    int current_entry_num = 0;

    route = wp->route;

    LOGD("===Route Auth_type:[%s]\n",route->authType);
    if (!route || !route->authType /*|| autoLogin*/)
    {
        /* Authentication not required */
        return 1;
    }
    cached = 0;

    //if we have parsed session in the header sucessfully!
    pthread_rwlock_rdlock(&rw_session);
    if ((wp->session = websGetSessionBySSID(wp, ssid)) != NULL)
    {
        int b1, b2, b3;
        time_t current_sec =  time(NULL);
        b1 = b2 = b3 = 0;
        //checked if timeout

        timeout = websGetSessionVar(wp, WEBS_DIGESTAUTH_TIMEOUT);
        if(timeout)
        {
        	LOGD("======[%ld %ld]========\n",timeout->value.integer , current_sec);
        	//cached = 1;
        	if (timeout->value.integer < current_sec)
        	{
                if (slen(wp->authDetails) == 0)
                {
                    LOGD("[Digest Auth timeout! need re auth =%d %d\n",timeout->value.integer, current_sec);
                    //websSetSessionVar(wp, WEBS_DIGESTAUTH_TIMEOUT,valueInteger(0));//清零
                    if (route->askLogin)
                    {
                        (route->askLogin)(wp);
                    }
                    websRedirectByStatus(wp, HTTP_CODE_UNAUTHORIZED);
                    pthread_rwlock_unlock(&rw_session);
                    return 0;
                }
        	}
        	else
        	{
        		b1 = 1;
        	}
        }
        //checked if  max visit num
        entry_num = websGetSessionVar(wp, WEBS_DIGESTAUTH_ENTRY);
        if (entry_num)
        {
            current_entry_num = entry_num->value.integer;
            LOGD("======Digest Auth entry num:%d\n", current_entry_num);
            if(entry_num->value.integer <= 0)
            {
                if(slen(wp->authDetails) == 0)
                {
                    LOGD("[Digest Auth entry num Zero!! need re auth.]\n");
                    if (route->askLogin)
                    {
                        (route->askLogin)(wp);
                    }
                    websRedirectByStatus(wp, HTTP_CODE_UNAUTHORIZED);
                    pthread_rwlock_unlock(&rw_session);
                    return 0;
                }
            }
            else
            {
                b2 = 1;
            }
        }
        // Retrieve authentication state from the session storage. Faster than re-authenticating.
        if ((username = websGetSessionVar(wp, WEBS_SESSION_USERNAME)) != NULL)
        {
            b3 = 1;
            wfree(wp->username);
            wp->username = sclone(username->value.string);
            LOGD("[ Session Cached! username:%s]\n", wp->username);
        }
        cached = b1&&b2&&b3;
        if (strstr(wp->path, "frmUserLogout"))
        {
            cached = 1;
        }
    }
    pthread_rwlock_unlock(&rw_session);

    if (0 == cached)
    {
        if (wp->authType && !smatch(wp->authType, route->authType))
        {
            websError(wp, HTTP_CODE_UNAUTHORIZED, "Access denied. Wrong authentication protocol type.");
            return 0;
        }
        if (wp->authDetails && route->parseAuth)
        {
            if (!(route->parseAuth)(wp))
            {
                wp->username = NULL;
            }
        }
        if (!wp->username || !*wp->username)
        {
            LOGD("\nCan't parse any username in Auth header!,you should auth request,send HTTP_CODE_UNAUTHORIZED back!\n");
            if (route->askLogin)
            {
                (route->askLogin)(wp);
            }
            websRedirectByStatus(wp, HTTP_CODE_UNAUTHORIZED);
            return 0;
        }
        LOGD(" Verify the request!\n");
        if (!(route->verify)(wp))
        {
            if (route->askLogin)
            {
                (route->askLogin)(wp);
            }
            websRedirectByStatus(wp, HTTP_CODE_UNAUTHORIZED);
            return 0;
        }
        //Store authentication state and user in session storage
        pthread_rwlock_wrlock(&rw_session);
        if((wp->session = websGetSessionBySSID(wp, ssid)) != NULL)
        {
	        //re auth successfully
	        //set timeout and max visit nums
	        LOGD("\nRefresh Session var:[WEBS_DIGESTAUTH_TIMEOUT:%ld  WEBS_DIGESTAUTH_ENTRY:%d]\n", time(NULL) + WEBS_DIGESTAUTH_MAX_TIMEOUT,WEBS_DIGESTAUTH_MAX_ENTRY_NUM);
	        websSetSessionVar(wp, WEBS_DIGESTAUTH_TIMEOUT, valueInteger(time(NULL)+ WEBS_DIGESTAUTH_MAX_TIMEOUT));
	        websSetSessionVar(wp, WEBS_DIGESTAUTH_ENTRY, valueInteger(WEBS_DIGESTAUTH_MAX_ENTRY_NUM));
        }
        pthread_rwlock_unlock(&rw_session);
    }

    return 1;
}


PUBLIC int websOpenAuth(int minimal)
{
    char sbuf[64];

    if ((users = hashCreate(-1)) < 0)
	{
        return -1;
    }
    InitUserContext();
    fmt(sbuf, sizeof(sbuf), "%x:%x", rand(), time(0));
    masterSecret = websMD5(sbuf);

    return 0;
}


PUBLIC void websCloseAuth()
{
    WebsKey     *key, *next;

    wfree(masterSecret);

    pthread_rwlock_wrlock(&rw_user);
    if (users >= 0) {
        for (key = hashFirst(users); key; key = next) {
            next = hashNext(users, key);
            freeUser(key->content.value.symbol);
        }
        hashFree(users);
        users = -1;
    }
    pthread_rwlock_rdlock(&rw_user);
}




 WebsUser *createUser(char *username, char *password,char *encrypted_password)
{
	WebsUser    *user;

	if ((user = walloc(sizeof(WebsUser))) == 0) {
		return 0;
	}
	user->name = sclone(username);
	user->roles = sclone(NULL);
	user->password = sclone(encrypted_password);
    user->pwd = sclone(password);
	user->abilities = -1;
	return user;
}


WebsUser *websAddUser(char *username, char *password,char* encrypted_pwd )
{
	WebsUser    *user;

	if (!username) {
		DEBUGV3("User is missing name");
		return 0;
	}

	if (websLookupUser(username)) {
		DEBUGV3("User %s already exists", username);
		/* Already exists */

		return 0;
	}
	if ((user = createUser(username, password,encrypted_pwd)) == 0) {

		return 0;
	}
	if (hashEnter(users, username, valueSymbol(user), 0) == 0) {

		return 0;
	}

	return user;
}


PUBLIC int websRemoveUser(char *username)
{
	WebsKey     *key;
    int ret;

	if ((key = hashLookup(users, username)) != 0) {
		freeUser(key->content.value.symbol);
	}
	 ret = hashDelete(users, username);
     return ret;
}


static void freeUser(WebsUser *up)
{
	hashFree(up->abilities);
	wfree(up->name);
	wfree(up->password);
    wfree(up->pwd);
	wfree(up->roles);
	wfree(up);
}


PUBLIC int websSetUserPassword(char *username, char *password,char* encrypted_pwd)
{
	WebsUser    *user;

	if ((user = websLookupUser(username)) == 0) {

		return -1;
	}
	wfree(user->pwd);
	user->pwd = sclone(password);
    wfree(user->password);
	user->password = sclone(encrypted_pwd);
	return 0;
}




WebsUser *websLookupUser(char *username)
{
	WebsKey     *key;

	if ((key = hashLookup(users, username)) == 0)
	{
		return 0;
	}
	return (WebsUser*) key->content.value.symbol;
}




PUBLIC WebsHash websGetUsers()
{
	return users;
}



static void basicLogin(Webs *wp)
{
	wfree(wp->authResponse);
	wp->authResponse = sfmt("X-Basic realm=\"%s\"", ME_GOAHEAD_REALM);
}
void websSetPasswordStoreVerify(WebsVerify verify)
{
	verifyPassword = verify;
}


WebsVerify websGetPasswordStoreVerify()
{
	return verifyPassword;
}


static bool parseBasicDetails(Webs *wp)
{
    char    *cp, *userAuth;

    /*
        Split userAuth into userid and password
     */
    cp = 0;
    if ((userAuth = websDecode64(wp->authDetails)) != 0) {
        if ((cp = strchr(userAuth, ':')) != NULL) {
            *cp++ = '\0';
        }
    }
    wfree(wp->username);
    wfree(wp->password);
    if (cp) {
        wp->username = sclone(userAuth);
        wp->password = sclone(cp);
        wp->encoded = 0;
    } else {
        wp->username = sclone(NULL);
        wp->password = sclone(NULL);
    }

    DEBUGV3("[%s  %s]\n",wp->username,wp->password);

    wfree(userAuth);
    return 1;
}


#if ME_GOAHEAD_DIGEST
static void digestLogin(Webs *wp)
{
    WEB_NONCE_NODE_T *node_cur = NULL;
    char  *nonce, *opaque;
    char* custom_digest;

    nonce = createDigestNonce(wp);
    /* Opaque is unused. Set to anything */
    opaque = "5ccc069c403ebaf9f0171e9517f40e41";
    wfree(wp->authResponse);

    //to prevent auth dialogs pop up on browser end,we need use custom header X-Digest!
    //but for curl libs.it can't take custom headers.

    if (strstr(wp->userAgent,"curl/"))
    {
        //for curl I8H_SDK default digest auth
        custom_digest = "Digest";
    }
    else
    {
        //for browser
        custom_digest = "X-Digest";
    }

    if(strstr(wp->path,"/onvif/"))
    {
        custom_digest = "Digest";
    }

    node_cur = Common_Malloc(sizeof(WEB_NONCE_NODE_T), 0, __FUNCTION__, __LINE__);
    node_cur->nonce = Common_StrDup(nonce, __FUNCTION__, __LINE__);
    node_cur->ncount = 1;
    node_cur->time_gen = time(NULL);
    Common_DList_InsertTail(g_ovfs_web->pNonceList, (void *)node_cur, sizeof(WEB_NONCE_NODE_T));

    wp->authResponse = sfmt(
        "%s realm=\"%s\", domain=\"%s\", qop=\"%s\", nonce=\"%s\", opaque=\"%s\", algorithm=\"%s\", stale=\"%s\"",
        custom_digest, ME_GOAHEAD_REALM, websGetServerUrl(), "auth", nonce, opaque, "MD5", "FALSE");

    wfree(nonce);
}


static bool parseDigestDetails(Webs *wp)
{
    WebsTime when;
    char *decoded = NULL;
    char *value = NULL;
    char *tok = NULL;
    char *key = NULL;
    char *keyBuf = NULL;
    char *dp = NULL;
    char *sp = NULL;
    char *secret = NULL;
    char *realm = NULL;
    int seenComma;

    key = keyBuf = sclone(wp->authDetails);
    while (*key) {
        while (*key && isspace((uchar) *key))
        {
            key ++;
        }
        tok = key;
        while (*tok && !isspace((uchar) *tok) && *tok != ',' && *tok != '=')
        {
            tok++;
        }
        if (*tok)
        {
            *tok++ = '\0';
        }

        while (isspace((uchar) *tok))
        {
            tok++;
        }
        seenComma = 0;
        if (*tok == '\"')
        {
            value = ++tok;
            while (*tok != '\"' && *tok != '\0')
            {
                tok++;
            }
        }
        else
        {
            value = tok;
            while (*tok != ',' && *tok != '\0')
            {
                tok++;
            }
            seenComma++;
        }
        if (*tok) {
            *tok++ = '\0';
        }

        /*
            Handle back-quoting
         */
        if (strchr(value, '\\'))
		{
            for (dp = sp = value; *sp; sp++)
			{
                if (*sp == '\\')
				{
                    sp++;
                }
                *dp++ = *sp++;
            }
            *dp = '\0';
        }

        /*
            user, response, oqaque, uri, realm, nonce, nc, cnonce, qop
         */
        switch (tolower((uchar) *key))
		{
        case 'a':
            if (scaselesscmp(key, "algorithm") == 0)
			{
                break;
            }
			else if (scaselesscmp(key, "auth-param") == 0)
			{
                break;
            }
            break;

        case 'c':
            if (scaselesscmp(key, "cnonce") == 0)
			{
                wfree(wp->cnonce);
                wp->cnonce = sclone(value);
            }
            break;

        case 'd':
            if (scaselesscmp(key, "domain") == 0)
			{
                break;
            }
            break;

        case 'n':
            if (scaselesscmp(key, "nc") == 0)
			{
                wfree(wp->nc);
                wp->nc = sclone(value);
            }
			else if (scaselesscmp(key, "nonce") == 0)
			{
                wfree(wp->nonce);
                wp->nonce = sclone(value);
            }
            break;

        case 'o':
            if (scaselesscmp(key, "opaque") == 0)
			{
                wfree(wp->opaque);
                wp->opaque = sclone(value);
            }
            break;

        case 'q':
            if (scaselesscmp(key, "qop") == 0) {
                wfree(wp->qop);
                wp->qop = sclone(value);
            }
            break;

        case 'r':
            if (scaselesscmp(key, "realm") == 0)
            {
                wfree(wp->realm);
                wp->realm = sclone(value);
            }
            else if (scaselesscmp(key, "response") == 0)
            {
                wfree(wp->password);
                wp->password = sclone(value);
                wp->encoded = 1;
            }
            break;

        case 's':
            if (scaselesscmp(key, "stale") == 0)
	     {
                break;
            }

        case 'u':
            if (scaselesscmp(key, "uri") == 0) {
                wfree(wp->digestUri);
                wp->digestUri = sclone(value);
            }
			else if (scaselesscmp(key, "username") == 0 || scaselesscmp(key, "user") == 0)
			{
                wfree(wp->username);
				wp->username = sclone(value);
            }
            break;

        default:
            /*  Just ignore keywords we don't understand */
            ;
        }
        key = tok;
        if (!seenComma) {
            while (*key && *key != ',') {
                key++;
            }
            if (*key) {
                key++;
            }
        }
    }
    wfree(keyBuf);

    if (wp->username == 0 || wp->realm == 0 || wp->nonce == 0 || wp->route == 0 || wp->password == 0)
	{
        return 0;
    }
    if (wp->qop && (wp->cnonce == 0 || wp->nc == 0))
	{
        return 0;
    }
    if (wp->qop == 0)
	{
        wp->qop = sclone("");
    }
    /*
        Validate the nonce value - prevents replay attacks
     */
    when = 0;
    secret = NULL;
    realm = NULL;
    decoded = parseDigestNonce(wp->nonce, &secret, &realm, &when);
    if (!smatch(masterSecret, secret))
	{
        DEBUGV3("Access denied: Nonce mismatch\n");
        wfree(decoded);
        return 0;
    }
    else if (!smatch(realm, ME_GOAHEAD_REALM))
    {
        DEBUGV3( "Access denied: Realm mismatch\n");
        wfree(decoded);
        return 0;
    }
    else if (!smatch(wp->qop, "auth"))
    {
       DEBUGV3( "Access denied: Bad qop\n");
        wfree(decoded);
        return 0;
    }
    else if ((when + (5 * 60)) < time(0))
    {
        DEBUGV3( "Access denied: Nonce is stale[%ld]\n",when);
        wfree(decoded);
        return 0;
    }
#if 0
    if (!wp->user)
    {
        if ((wp->user = websLookupUser(wp->username)) == 0)
        {
            DEBUGV3("Access denied: user[%s] is unknown\n",wp->username);
            wfree(decoded);
            return 0;
        }
    }
#endif
    wfree(decoded);
#if 0
    wfree(wp->digest);
    wp->digest = calcDigest(wp, 0, wp->user->password);
#endif

    return 1;
}


/*
    Create a nonce value for digest authentication (RFC 2617)
 */
static char *createDigestNonce(Webs *wp)
{
    static int64 next = 0;
    char nonce[256];

    fmt(nonce, sizeof(nonce), "%s:%s:%x:%x", masterSecret, ME_GOAHEAD_REALM, time(0), next++);

    return websEncode64(nonce);
}


static char *parseDigestNonce(char *nonce, char **secret, char **realm, WebsTime *when)
{
    char *tok = NULL;
    char *decoded = NULL;
    char *whenStr = NULL;

    if ((decoded = websDecode64(nonce)) == 0)
    {
        return 0;
    }

    *secret = stok(decoded, ":", &tok);
    *realm = stok(NULL, ":", &tok);
    whenStr = stok(NULL, ":", &tok);
    *when = hextoi(whenStr);

    return decoded;
}


#if 0
/*
   Get a Digest value using the MD5 algorithm -- See RFC 2617 to understand this code.
 */
static char *calcDigest(Webs *wp, char *username, char *password)
{
    char  a1Buf[256], a2Buf[256], digestBuf[256];
    char  *ha1, *ha2, *method, *result;

    /*
        Compute HA1. If username == 0, then the password is already expected to be in the HA1 format
        (MD5(username:realm:password).
     */
    LOGD("password:%s\n", password);
    if (username == 0)
    {
        ha1 = sclone(password);
    }
    else
    {
        fmt(a1Buf, sizeof(a1Buf), "%s:%s:%s", username, wp->realm, password);
        ha1 = websMD5(a1Buf);
    }

    /*
        HA2
     */
    method = wp->method;
    fmt(a2Buf, sizeof(a2Buf), "%s:%s", method, wp->digestUri);
    ha2 = websMD5(a2Buf);

    /*
        H(HA1:nonce:HA2)
     */
    LOGD("wp->qop:%s\n", wp->qop);
    if (scmp(wp->qop, "auth") == 0)
    {
        fmt(digestBuf, sizeof(digestBuf), "%s:%s:%s:%s:%s:%s", ha1, wp->nonce, wp->nc, wp->cnonce, wp->qop, ha2);
    }
    else if (scmp(wp->qop, "auth-int") == 0)
    {
        fmt(digestBuf, sizeof(digestBuf), "%s:%s:%s:%s:%s:%s", ha1, wp->nonce, wp->nc, wp->cnonce, wp->qop, ha2);
    }
    else
    {
        fmt(digestBuf, sizeof(digestBuf), "%s:%s:%s", ha1, wp->nonce, ha2);
    }
    LOGD("digestBuf:%s\n", digestBuf);
    result = websMD5(digestBuf);
    wfree(ha1);
    wfree(ha2);
    return result;
}
#endif
#endif /* ME_GOAHEAD_DIGEST */


PUBLIC int websSetRouteAuth(WebsRoute *route, char *auth)
{
    WebsParseAuth parseAuth;
    WebsAskLogin  askLogin;

    askLogin = 0;
    parseAuth = 0;
    if (smatch(auth, "basic"))
    {
        askLogin = basicLogin;
        parseAuth = parseBasicDetails;
    }
	else if (smatch(auth, "digest"))
	{
        askLogin = digestLogin;
        parseAuth = parseDigestDetails;
    }
	else
	{
        auth = 0;
    }
    route->authType = sclone(auth);
    route->askLogin = askLogin;
    route->parseAuth = parseAuth;

    return 0;
}
#endif /* ME_GOAHEAD_AUTH */
