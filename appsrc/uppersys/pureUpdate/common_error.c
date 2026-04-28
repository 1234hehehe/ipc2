#include "libcommon_api.h"
#include "libcommon_struct.h"

typedef struct
{
	int nCode;
	char pDesc[40];

}COMMON_STATUS_CODE_T;
static COMMON_STATUS_CODE_T g_tStatusCode[]=
{
	{100,"Continue"},
	{101,"Switching Protocols"},
	{102,"Processing"},
	{200,"OK"},
	{201,"Created"},
	{202,"Accepted"},
	{203,"Non-Authoritative Information"},
	{204,"No Content"},
	{205,"Reset Content"},
	{206,"Partial Content"},
	{207,"Multi-Status"},
	{208,"Already Reported"},
	{226,"IM Used"},
	{250,"Low on Storage Space"},
	{300,"Multiple Choices"},
	{301,"Moved Permanently"},
	{302,"Moved Temporarily"},
	{303,"See Other"},
	{304,"Not Modified"},
	{305,"Use Proxy"},
	{307,"Temporary Redirect"},
	{308,"Permanent Redirect"},
	{400,"Bad Request"},
	{401,"Unauthorized"},
	{402,"Payment Required"},
	{403,"Forbidden"},
	{404,"Not Found"},
	{405,"Method Not Allowed"},
	{406,"Not Acceptable"},
	{407,"Proxy Authentication Required"},
	{408,"Request Time-out"},
	{409,"Conflict"},
	{410,"Gone"},
	{411,"Length Required"},
	{412,"Precondition Failed"},
	{413,"Request Entity Too Large"},
	{414,"Request-URI Too Large"},
	{415,"Unsupported Media Type"},
	{416,"Range Not Satisfiable"},
	{417,"Expectation Failed"},
	{421,"Misdirected Request"},
	{422,"Unprocessable Entity"},
	{423,"Locked"},
	{424,"Failed Dependency"},
	{425,"Unassigned"},
	{426,"Upgrade Required"},
	{428,"Precondition Required"},
	{429,"Too Many Requests"},
	{431,"Request Header Fields Too Large"},
	{451,"Parameter Not Understood"},
	{452,"Conference Not Found"},
	{453,"Not Enough Bandwidth"},
	{454,"Session Not Found"},
	{455,"Method Not Valid in This State"},
	{456,"Header Field Not Valid for Resource"},
	{457,"Invalid Range"},
	{458,"Parameter Is Read-Only"},
	{459,"Aggregate operation not allowed"},
	{460,"Only aggregate operation allowed"},
	{461,"Unsupported transport"},
	{462,"Destination unreachable"},
	{500,"Internal Server Error"},
	{501,"Not Implemented"},
	{502,"Bad Gateway"},
	{503,"Service Unavailable"},
	{504,"Gateway Time-out"},
	{505,"Version not supported"},
	{506,"Variant Also Negotiates"},
	{507,"Insufficient Storage"},
	{508,"Loop Detected"},
	{510,"Not Extended"},
	{511,"Network Authentication Required"},
	{551,"Option not supported"}
};
static S8 *g_pUnassigned = (S8 *)"Unassigned";
S8* Common_FindErrString(S32 errCode)
{
	size_t i;
	for (i = 0; i < sizeof(g_tStatusCode)/sizeof(g_tStatusCode[0]);i++)
	{
		if (g_tStatusCode[i].nCode == errCode)
		{
			return g_tStatusCode[i].pDesc;
		}
		
	}

	return g_pUnassigned;
}                                                  
