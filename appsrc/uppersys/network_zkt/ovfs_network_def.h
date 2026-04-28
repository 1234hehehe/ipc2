#ifndef _OVFS_NETWORK_DEF_H_
#define _OVFS_NETWORK_DEF_H_

#include "ovfs_comm_def.h"
#include "ovfs_comm_errno.h"

#define OVFS_MAX_ABILITY_DDNS_NUM 32
namespace ovfs_soft
{
typedef enum
{
    OVFS_ETH_EANBLE = 0x1,	//启用
    OVFS_ETN_CAN_SEARCH_DEV = 0x2,//搜索IPC
    OVFS_ETH_CAN_BE_ROUTER = 0x4,//默认路由
    OVFS_ETH_CAN_JOIN_BOND = 0x8,//加入bond
    OVFS_ETH_CAN_BE_SEARCHED = 0x10, //是否允许被搜索
}ovfs_net_ability_type;

typedef struct{
    S8 description[16];
    S8 if_name[16];
    OVFS_BOOL is_exist;
    U32 ability_mask;//bit0-是否启用，bit1-是否可以搜索IPC， bit2-是否可以作为默认路由， bit3-是否可以加入bond组
	U32 board_id;
}ovfs_eth_status;

typedef struct{
    S8 description[16];
    S8 bound_name[16];
    OVFS_BOOL is_support; //是否支持
    U32 board_id;
}ovfs_bound_status;

typedef struct{
    U32 total_eth_num ; 						//网卡个数
    ovfs_eth_status eth_status[OVFS_MAX_ETH_CARD_NUM]; 	//各个网卡的转态
    U32 bond_num ;//支持的bonding网卡个数
    ovfs_bound_status bond_status[OVFS_MAX_ETH_CARD_NUM]; //bonding网卡状态
    OVFS_BOOL has_poe;
}ovfs_network_capability;

typedef struct
{
    U16    year;   //年，是多少就多少，不需要1900做处理
    U16    month;  //月，是多少就多少，不需要加1减1做处理
    U16    day;    //日，是多少就多少

    U16    hour;
    U16    min;
    U16    sec;
}ovfs_time_struct;

//本地连接配置
typedef struct
{
    OVFS_BOOL is_ip_v4;
    OVFS_BOOL auto_get_ip;

    //IP地址，静态有效
    U8 ip_v4[4];	//IP v4 为4个字节
    U16 ip_v6[8];	//IP v6 为8个short

    U8 sub_mask[4];	// 子网掩码	4个字节
    U8 gate_way[4];	// 网关 4个字节

    S8 dns1[OVFS_MAX_DNS_NAME_LEN];		// 主DNS
    S8 dns2[OVFS_MAX_DNS_NAME_LEN];		// 次DNS
}ovfs_local_connection;

typedef struct
{
    OVFS_BOOL is_ipv4;
    S8 ipv4[OVFS_IPV4_STRING_LEN + 1];
    S8 ipv6[OVFS_IPV6_STRING_LEN + 1];
}ovfs_ipaddr_struct;

typedef struct
{
    S8 	if_name[OVFS_MAX_ETH_IFNAME_SIZE];	//网卡名称
    OVFS_BOOL  	dhcp;								//是否启用dhcp
    OVFS_BOOL ipv6_support;
    S32 dhcpv6;                             //0-静态ip 1-有状态地址(dhcp) 2-无状态地址(SLAAC)
    S8	mac[OVFS_MAC_STRING_LEN];			//mac地址
    ovfs_ipaddr_struct	ip;							//ip地址
    ovfs_ipaddr_struct	netmask;					//子网掩码
    ovfs_ipaddr_struct	gate_way;					//网关地址
    S32 ipv6_prefixlen;
}ovfs_netcard_config;

typedef struct
{
    int nDnsMode;		//0-有线dns，1-4g dns，2-wifi dns..
    OVFS_BOOL auto_dns;	//是否自动获取域名服务器
    ovfs_ipaddr_struct 	dns1;	/* 域名服务器1的IP地址 */
    ovfs_ipaddr_struct 	dns2;	/* 域名服务器2的IP地址 */
}ovfs_local_net_dns;


typedef struct
{
    U16 				http_port;							//http 端口
    U16				onvif_port;             			// ! ONVIF端口
    U16                rtsp_port;             				// ! RTSP端口
    U16				private_port;						//私有协议端口
}ovfs_local_net_port_cfg;


typedef struct
{
    S8 if_name[OVFS_MAX_ETH_IFNAME_SIZE];						//虚拟网卡编号
    OVFS_BOOL is_enable;					//是否启用虚拟网卡
    OVFS_BOOL dhcp;							//是否启用dhcp
    S8	mac[OVFS_MAC_STRING_LEN];	//mac地址
    ovfs_ipaddr_struct	ip;					//ip地址
    ovfs_ipaddr_struct	netmask;			//子网掩码
    ovfs_ipaddr_struct	gate_way;			//网关地址
    U32 if_num;						//绑定在虚拟网卡上的网卡个数
    S8  if_array[OVFS_MAX_ETH_CARD_NUM][OVFS_MAX_ETH_IFNAME_SIZE]; //绑定在虚拟网卡上的网卡名称
}ovfs_bounding_eth_config;


//pppoe配置
typedef struct
{
    OVFS_BOOL enable;			// 是否启动PPPOE拔号
    S8 user_name[32];	// 用户名
    S8 password[32];		// 密码
}ovfs_pppoe_config;

//pppoe拨号状态
typedef struct
{
    OVFS_BOOL pppoe_ok;	//拨号是否成功
    ovfs_ipaddr_struct ip;	//外网IP地址
    //U8 gate_way[4];	// 网关 4个字节
}ovfs_pppoe_status;


//ntp配置
typedef struct
{
    OVFS_BOOL enable;			// enable or disable internet time sync.(0,1)
    S8 server[64];		// timer server(域名或IP地址均可)
    U32 auto_set_time_interval; //ntp 自动校时间隔 按h 计算
}ovfs_ntp_config;


//时区_编号，后面实现的时候再去添加
typedef enum
{
    //后面添加的时区在最后面添加 已经废弃的宏 不要删除
    //枚举值不要随意修改 IE会按枚举值索引语言
    OVFS_TIME_ZONE_WEST_LINE = 0,				   //国际日期变更线西
    OVFS_TIME_ZONE_SAMOA = 1,						//中途岛,萨摩亚群岛
    OVFS_TIME_ZONE_HAWAII = 2,						//夏威夷
    OVFS_TIME_ZONE_ALASKA = 3,						//阿拉斯加
    OVFS_TIME_ZONE_PACIFIC_OCEAN = 4,				//太平洋时间(美国和加拿大)
    OVFS_TIME_ZONE_MOUNTAIN = 5,					//山地时间(美国和加拿大)
    OVFS_TIME_ZONE_CENTRAL_CANADA = 6,          	//中部时间(美国和加拿大)
    OVFS_TIME_ZONE_EASTERN_TIME_CANADA = 7,     	  //东部时间(美国和加拿大)
    OVFS_TIME_ZONE_CARACAS = 8,                 	  //加拉加斯
    OVFS_TIME_ZONE_ATLANTIC_CANADA = 9,         	  //大西洋时间(加拿大)
    OVFS_TIME_ZONE_NEWFOUNDLAND = 10,            	  //纽芬兰
    OVFS_TIME_ZONE_GEORGETOWN = 11,              	  //乔治敦, 巴西利亚
    OVFS_TIME_ZONE_ATLANTIC_OCEAN = 12,          	  //中大西洋
    OVFS_TIME_ZONE_ANGLE_ISLANDS = 13,           	  //佛得角群岛,亚速尔群岛
    OVFS_TIME_ZONE_GREENWICH = 14,               	  //格林威治标准时间:都柏林, 爱丁堡, 伦敦, 里斯本
    OVFS_TIME_ZONE_AMSTERDAM = 15,               	  //阿姆斯特丹, 柏林, 罗马, 巴黎
    OVFS_TIME_ZONE_ATHENS = 16,                  	  //雅典, 耶路撒冷 ,伊斯坦布尔
    OVFS_TIME_ZONE_BAGHDAD = 17,                 	  //巴格达 ,科威特
    OVFS_TIME_ZONE_TEHERAN = 18,                 	  //德黑兰
    OVFS_TIME_ZONE_MOSCOW = 19,                  	  //莫斯科, 圣彼得堡, 伏尔加格勒
    OVFS_TIME_ZONE_KABUL = 20,                   	  //喀布尔
    OVFS_TIME_ZONE_ISB = 21,                     	  //伊斯兰堡, 卡拉奇 ,塔什干
    OVFS_TIME_ZONE_MADRAS = 22,                  	  //马德拉斯,钦奈, 加尔各答, 孟买, 新德里
    OVFS_TIME_ZONE_KATHMANDU = 23,               	  //加德满都
    OVFS_TIME_ZONE_NOVOSIBIRSK = 24,             	  //阿拉木图,达卡,新西伯利亚
    OVFS_TIME_ZONE_RANGOON = 25,                 	  //仰光
    OVFS_TIME_ZONE_BANGKOK = 26,                 	  //曼谷, 河内, 雅加达
    OVFS_TIME_ZONE_BEIJING = 27,                 	  //北京,香港特别行政区,乌鲁木齐,新加坡
    OVFS_TIME_ZONE_OSAKA = 28,                   	  //首尔,大阪, 札幌, 东京
    OVFS_TIME_ZONE_ADELAIDE = 29,                	//阿德莱德 ,达尔文
    OVFS_TIME_ZONE_CANBERRA = 30,                	  //堪培拉, 墨尔本, 悉尼
    OVFS_TIME_ZONE_SOLOMON_ISLANDS = 31,         	  //所罗门群岛, 新喀里多尼亚
    OVFS_TIME_ZONE_OSKLAND = 32,                 	  //奥克兰, 惠林顿 ,斐济 ,马加丹
    OVFS_TIME_ZONE_NUKUALOFA = 33,               	  //努库阿洛法

#if 0

	OVFS_TIME_ZONE_SANTIAGO = 23,                	  //圣地亚哥
	OVFS_TIME_ZONE_ASUNCION = 24,                	  //亚松森

	OVFS_TIME_ZONE_BRASILIA = 26,                	  //巴西利亚
	OVFS_TIME_ZONE_BUENOS_AIRES = 27,            	  //布宜诺斯艾利斯
	OVFS_TIME_ZONE_GREENLAND = 28,               	  //格陵兰
	OVFS_TIME_ZONE_CAYENNE = 29,                 	  //卡宴, 福塔雷萨
	OVFS_TIME_ZONE_MONTEVIDEO = 30,              	  //蒙得维的亚
	OVFS_TIME_ZONE_SALVADOR = 31,                	  //萨尔瓦多
	OVFS_TIME_ZONE_COORDINATED_2 = 32,           	  //协调世界时-02

	OVFS_TIME_ZONE_ANGLE_ISLANDS = 34,           	  //佛得角群岛
	OVFS_TIME_ZONE_AZORES_ISLANDS = 35,          	  //亚速尔群岛
	OVFS_TIME_ZONE_GREENWICH = 36,               	  //格林威治标准时间:都柏林, 爱丁堡, 伦敦, 里斯本
	OVFS_TIME_ZONE_CASABLANK = 37,               	  //卡莎布兰卡
	OVFS_TIME_ZONE_MONROVIA = 38,                	  //蒙罗维亚, 雷克雅未克
	OVFS_TIME_ZONE_CORRDINATED = 39,             	  //协调世界时
	OVFS_TIME_ZONE_AMSTERDAM = 40,               	  //阿姆斯特丹, 柏林, 伯尔尼, 罗马, 斯德哥尔摩, 维也纳
	OVFS_TIME_ZONE_BELGRADE = 41,                	  //贝尔格莱德, 布拉迪斯拉发, 布达佩斯, 卢布尔雅那
	OVFS_TIME_ZONE_BRUSSELS = 42,                	  //布鲁塞尔, 哥本哈根, 马德里, 巴黎
	OVFS_TIME_ZONE_SARAJEVO = 43,                	  //萨拉热窝, 斯科普里, 华沙, 萨格勒布
	OVFS_TIME_ZONE_WESTERN_CENTRAL_AFRICA = 44,  	  //中非西部
	OVFS_TIME_ZONE_WINDHOEK = 45,                	  //温得和克
	OVFS_TIME_ZONE_AMMAN = 46,                   	  //安曼
	OVFS_TIME_ZONE_BEIRUT = 47,                  	  //贝鲁特
	OVFS_TIME_ZONE_DAMASCUS = 48,                	  //大马士革
	OVFS_TIME_ZONE_HRE = 49,                     	  //哈拉雷, 比勒陀利亚
	OVFS_TIME_ZONE_HELSINKI = 50,                	  //赫尔辛基, 基辅, 里加, 索菲亚, 塔林, 维尔纽斯
	OVFS_TIME_ZONE_CAIRO = 51,                   	  //开罗
	OVFS_TIME_ZONE_NICOSIA = 52,                 	  //尼科西亚
	OVFS_TIME_ZONE_ATHENS = 53,                  	  //雅典, 布加勒斯特
	OVFS_TIME_ZONE_JERUSALEM = 54,               	  //耶路撒冷
	OVFS_TIME_ZONE_ISTANBUL = 55,                	  //伊斯坦布尔
	OVFS_TIME_ZONE_KALININGRAD = 56,             	  //加里宁格勒, 明斯克
	OVFS_TIME_ZONE_BAGHDAD = 57,                 	  //巴格达
	OVFS_TIME_ZONE_KUWAIT = 58,                  	  //科威特, 利雅得
	OVFS_TIME_ZONE_NAIROBI = 59,                 	  //内罗毕
	OVFS_TIME_ZONE_MOSCOW = 60,                  	  //莫斯科, 圣彼得堡, 伏尔加格勒
	OVFS_TIME_ZONE_TEHERAN = 61,                 	  //德黑兰
	OVFS_TIME_ZONE_ABU_DHABI = 62,               	  //阿布扎比, 马斯喀特
	OVFS_TIME_ZONE_YEREVAN = 63,                 	  //埃里温
	OVFS_TIME_ZONE_BAKU = 64,                    	  //巴库
	OVFS_TIME_ZONE_TBILISI = 65,                 	  //第比利斯
	OVFS_TIME_ZONE_CAUCASUS = 66,                	  //高加索标准时间
	OVFS_TIME_ZONE_PORT_LOUIS = 67,              	  //路易港
	OVFS_TIME_ZONE_KABUL = 68,                   	  //喀布尔
	OVFS_TIME_ZONE_TASHKENT = 69,                	  //阿什哈巴德, 塔什干
	OVFS_TIME_ZONE_ISB = 70,                     	  //伊斯兰堡, 卡拉奇
	OVFS_TIME_ZONE_EKATERINBURG = 71,            	  //叶卡捷琳堡
	OVFS_TIME_ZONE_MADRAS = 72,                  	  //钦奈, 加尔各答, 孟买, 新德里
	OVFS_TIME_ZONE_SRIWALDEN = 73,               	  //斯里加亚渥登普拉
	OVFS_TIME_ZONE_KATHMANDU = 74,               	  //加德满都
	OVFS_TIME_ZONE_ASTANA = 75,                  	  //阿斯塔纳
	OVFS_TIME_ZONE_DACCA = 76,                   	  //达卡
	OVFS_TIME_ZONE_NOVOSIBIRSK = 77,             	  //新西伯利亚
	OVFS_TIME_ZONE_RANGOON = 78,                 	  //仰光
	OVFS_TIME_ZONE_BANGKOK = 79,                 	  //曼谷, 河内, 雅加达
	OVFS_TIME_ZONE_KRASNOYARSK = 80,             	  //克拉斯诺亚尔斯克
	OVFS_TIME_ZONE_BEIJING = 81,                 	  //北京, 重庆, 香港特别行政区, 乌鲁木齐
	OVFS_TIME_ZONE_KUALA_LUMPUR = 82,            	  //吉隆坡, 新加坡
	OVFS_TIME_ZONE_PERTH = 83,                   	  //珀斯
	OVFS_TIME_ZONE_TAIPEI = 84,                  	  //台北
	OVFS_TIME_ZONE_ULAN_BATOR = 85,              	  //乌兰巴托
	OVFS_TIME_ZONE_IKT = 86,                     	  //伊尔库茨克
	OVFS_TIME_ZONE_OSAKA = 87,                   	  //大阪, 札幌, 东京
	OVFS_TIME_ZONE_SEOUL = 88,                   	  //首尔
	OVFS_TIME_ZONE_YAKUTSK = 89,                 	  //雅库茨克
	OVFS_TIME_ZONE_ADELAIDE = 90,                	  //阿德莱德
	OVFS_TIME_ZONE_DARWIN = 91,                  	  //达尔文
	OVFS_TIME_ZONE_BNE = 92,                     	  //布里斯班
	OVFS_TIME_ZONE_GUAM = 93,                    	  //关岛, 莫尔兹比港
	OVFS_TIME_ZONE_HOBART = 94,                  	  //霍巴特
	OVFS_TIME_ZONE_CANBERRA = 95,                	  //堪培拉, 墨尔本, 悉尼
	OVFS_TIME_ZONE_VVO = 96,                     	  //符拉迪沃斯托克
	OVFS_TIME_ZONE_SOLOMON_ISLANDS = 97,         	  //所罗门群岛, 新喀里多尼亚
	OVFS_TIME_ZONE_MAGADAN = 98,                 	  //马加丹
	OVFS_TIME_ZONE_OSKLAND = 99,                 	  //奥克兰, 惠林顿
	OVFS_TIME_ZONE_SKIN_DROGBA = 100,             	  //皮德罗巴甫洛夫斯基-勘察加 - 旧用
	OVFS_TIME_ZONE_FIJI = 101,                   	  //斐济
	OVFS_TIME_ZONE_COORDINATED_12 = 102,          	  //协调世界时+12
	OVFS_TIME_ZONE_NUKUALOFA = 103,               	  //努库阿洛法
//新添加的宏
	OVFS_TIME_ZONE_CHRISTMAS_ISLANDS = 104,				//圣诞岛
	OVFS_TIME_ZONE_TRIPOLI = 105,						  //的黎波里
	OVFS_TIME_ZONE_EASTEUROPE = 106,					  //东欧

	OVFS_TIME_ZONE_MINSK = 107,				//明斯克
	OVFS_TIME_ZONE_CHOKURDAKH = 108,		//乔库尔达赫
#endif
	OVFS_TIME_ZONE_UNKNOW = 0x13149527, 	//未定义值

}ovfs_time_zone;

typedef struct
{
    ovfs_time_zone zone;
    OVFS_BOOL      enable_bias;    //使能微调
    U32       zone_bias;     //时区的微调 0 - 59  分钟 向前偏移
}ovfs_time_zone_cfg;


typedef enum
{
    OVFS_DST_WEEK,
    OVFS_DST_DATE
}ovfs_dst_mode;

typedef enum
{
    OVFS_WEEK_SUN = 0,  //从星期日开始 0 - 6  //不要改变值
    OVFS_WEEK_MON = 1,
    OVFS_WEEK_TUE = 2,
    OVFS_WEEK_WED = 3,
    OVFS_WEEK_THUR = 4,
    OVFS_WEEK_FRI = 5,
    OVFS_WEEK_SATU = 6,
}ovfs_week_day;

typedef enum
{
    OVFS_MONTH_WEEK_1 = 0,	//第1周  //不要改变值
    OVFS_MONTH_WEEK_2 = 1,	//第2周
    OVFS_MONTH_WEEK_3 = 2,	//第3周
    OVFS_MONTH_WEEK_4 = 3,	//第4周

    OVFS_MONTH_WEEK_LAST = 4,	//最后一周
}ovfs_week_per_month;

typedef enum
{
    OVFS_MONTH_1  = 0 ,	//1月  0 - 11   1到12月  //不要改变值
    OVFS_MONTH_2  = 1,	//2月
    OVFS_MONTH_3  = 2,	//3月
    OVFS_MONTH_4  = 3,	//4月
    OVFS_MONTH_5  = 4,	//5月
    OVFS_MONTH_6  = 5,	//6月
    OVFS_MONTH_7  = 6,	//7月
    OVFS_MONTH_8  = 7,	//8月
    OVFS_MONTH_9  = 8,	//9月
    OVFS_MONTH_10 = 9,	//10月
    OVFS_MONTH_11 = 10,	//11月
    OVFS_MONTH_12 = 11,	//12月
}ovfs_month_enum;


typedef struct
{
    ovfs_month_enum month;			//月 0-11表示1-12个月
    ovfs_week_per_month week_idx;	//第几个 0－第1个 1－第2个 2－第3个 3－第4个 4－最后一个
    ovfs_week_day week_day;			//星期几 0－星期日 1－星期一 2－星期二 3－星期三 4－星期四 5－星期五 6－星期六

    U16 hour;					//小时	开始时间0－23 结束时间1－23
    U16 min;					//分	0－59
}ovfs_dst_time;

typedef struct{
    ovfs_time_zone zone_type;
    int zone_hour;
    int zone_min;
}ovfs_dst_cap_node;


typedef struct
{
    ovfs_dst_time start_time;	//dst起始时间
    ovfs_dst_time stop_time;	//dst结束时间
}ovfs_dst_time_week;


typedef struct
{
    ovfs_time_struct start_time;	//dst起始时间
    ovfs_time_struct stop_time;	//dst结束时间
}ovfs_dst_time_date;

typedef struct
{
    OVFS_BOOL      enable;
    ovfs_dst_mode  mode;
    U32	   dst_bias;  //夏令时偏移值，30min, 60min, 90min, 120min, 以分钟计，传递原始数值
    union
    {
        ovfs_dst_time_week week;
        ovfs_dst_time_date date;
    }cfg;

}ovfs_dst_cfg;

typedef struct
{
	S8 ip[OVFS_IPV6_STRING_LEN];//地址
	U32 prefixlen;//子网前缀长度
}ovfs_inet_ipv6_struct;

typedef struct tag_ovfs_inet_ipv6_ip_info_node_t
{
	ovfs_inet_ipv6_struct info;
	struct tag_ovfs_inet_ipv6_ip_info_node_t *next;
	struct tag_ovfs_inet_ipv6_ip_info_node_t *prev;
}ovfs_inet_ipv6_ip_info_node;

typedef struct
{
	U32 addr_num;
	ovfs_inet_ipv6_ip_info_node *head;
	ovfs_inet_ipv6_ip_info_node *tail;
}ovfs_inet_ipv6_ip_info_list;


typedef struct _tag_ovfs_inet_ipv6_route_info_node_t
{
	U32 metric;
	S8 addr[OVFS_IPV6_STRING_LEN];
	struct _tag_ovfs_inet_ipv6_route_info_node_t *next;
	struct _tag_ovfs_inet_ipv6_route_info_node_t *prev;
}ovfs_inet_ipv6_route_info_node;

typedef struct
{
	U32 addr_num;
	ovfs_inet_ipv6_route_info_node *head;
	ovfs_inet_ipv6_route_info_node *tail;
}ovfs_inet_ipv6_route_info_list;



typedef struct _tag_ovfs_inet_ipv6_dns_info_node_t
{
	S8 addr[OVFS_IPV6_STRING_LEN];
	struct _tag_ovfs_inet_ipv6_dns_info_node_t *next;
	struct _tag_ovfs_inet_ipv6_dns_info_node_t *prev;
}ovfs_inet_ipv6_dns_info_node;

typedef struct
{
	U32 addr_num;
	ovfs_inet_ipv6_dns_info_node *head;
	ovfs_inet_ipv6_dns_info_node *tail;
}ovfs_inet_ipv6_dns_info_list;


typedef struct
{
	OVFS_BOOL dhcp;
	ovfs_inet_ipv6_ip_info_list *ip_config;
	ovfs_inet_ipv6_route_info_list *route_config;//默认网关
	ovfs_inet_ipv6_dns_info_list *dns_config;	//DNS
}ovfs_inet_ipv6_ifconfig;



typedef struct
{
	S8 ip[OVFS_IPV4_STRING_LEN];//地址
	S8 netmask[OVFS_IPV4_STRING_LEN];// 子网掩码
}ovfs_inet_ipv4_struct;


typedef struct _tag_ovfs_inet_ipv4_ip_info_node_t
{
	S8 label[16];
	ovfs_inet_ipv4_struct info;
	struct _tag_ovfs_inet_ipv4_ip_info_node_t *next;
	struct _tag_ovfs_inet_ipv4_ip_info_node_t *prev;
}ovfs_inet_ipv4_ip_info_node;

typedef struct
{
	U32 addr_num;
	ovfs_inet_ipv4_ip_info_node *head;
	ovfs_inet_ipv4_ip_info_node *tail;
}ovfs_inet_ipv4_ip_info_list;


typedef struct _tag_ovfs_inet_ipv4_route_info_node_t
{
	U32 metric;
	S8 addr[OVFS_IPV4_STRING_LEN];
	struct _tag_ovfs_inet_ipv4_route_info_node_t *next;
	struct _tag_ovfs_inet_ipv4_route_info_node_t *prev;
}ovfs_inet_ipv4_route_info_node;

typedef struct
{
	U32 addr_num;
	ovfs_inet_ipv4_route_info_node *head;
	ovfs_inet_ipv4_route_info_node *tail;
}ovfs_inet_ipv4_route_info_list;


typedef struct _tag_ovfs_inet_ipv4_dns_info_node_t
{
	S8 addr[OVFS_IPV4_STRING_LEN];
	struct _tag_ovfs_inet_ipv4_dns_info_node_t *next;
	struct _tag_ovfs_inet_ipv4_dns_info_node_t *prev;
}ovfs_inet_ipv4_dns_info_node;

typedef struct
{
	U32 addr_num;
	ovfs_inet_ipv4_dns_info_node *head;
	ovfs_inet_ipv4_dns_info_node *tail;
}ovfs_inet_ipv4_dns_info_list;



typedef struct
{
	OVFS_BOOL dhcp;
	ovfs_inet_ipv4_ip_info_list *ip_config;
	ovfs_inet_ipv4_route_info_list *route_config;//默认网关
	ovfs_inet_ipv4_dns_info_list *dns_config;	//DNS
}ovfs_inet_ipv4_ifconfig;

typedef struct
{
	S8 ifname[OVFS_MAX_ETH_IFNAME_SIZE];//
	U8 mac[6];		//物理地址
	OVFS_BOOL is_up;	//设备是否开始工作
	OVFS_BOOL is_slave;	//是否为从设备
	ovfs_inet_ipv4_ifconfig ipv4_config;
	ovfs_inet_ipv6_ifconfig ipv6_config;
}ovfs_if_config_struct;


typedef struct
{
	U32 ip_num;	//IP个数
	ovfs_inet_ipv4_struct ip[OVFS_MAX_IP_ADDR_NUM];
}ovfs_network_ipv4_ip_config;

typedef struct
{
	U32 route_num;//默认网关个数
	S8 ipv4_route[OVFS_MAX_IP_ADDR_NUM][OVFS_IPV4_STRING_LEN];//默认网关地址
	U32 metric[OVFS_MAX_IP_ADDR_NUM];//跃点个数
}ovfs_network_ipv4_route_config;

typedef struct
{
	U32 dns_num;//dns个数
	S8 ipv4_dns[OVFS_MAX_IP_ADDR_NUM][OVFS_IPV4_STRING_LEN];//dns地址;
}ovfs_network_ipv4_dns_config;


typedef struct
{
	OVFS_BOOL dhcp_enable;	//dhcp是否开启
	ovfs_network_ipv4_ip_config ip_config;
	ovfs_network_ipv4_route_config route_config;
	OVFS_BOOL auto_dns;//自动获取dns
	ovfs_network_ipv4_dns_config dns_config;
}ovfs_network_ipv4_config_struct;



typedef struct
{
	U32 ip_num;	//IP个数
	ovfs_inet_ipv6_struct ip[OVFS_MAX_IP_ADDR_NUM];
}ovfs_network_ipv6_ip_config;

typedef struct
{
	U32 route_num;//默认网关个数
	S8 ipv6_route[OVFS_MAX_IP_ADDR_NUM][OVFS_IPV6_STRING_LEN];//默认网关地址
	U32 metric[OVFS_MAX_IP_ADDR_NUM];//跃点个数
}ovfs_network_ipv6_route_config;

typedef struct
{
	U32 dns_num;//dns个数
	S8 ipv6_dns[OVFS_MAX_IP_ADDR_NUM][OVFS_IPV6_STRING_LEN];//dns地址;
}ovfs_network_ipv6_dns_config;


typedef struct
{
	OVFS_BOOL dhcp_enable;	//dhcp是否开启
	ovfs_network_ipv6_ip_config ip_config;
	ovfs_network_ipv6_route_config route_config;
	OVFS_BOOL auto_dns;//自动获取dns
	ovfs_network_ipv6_dns_config dns_config;
}ovfs_network_ipv6_config_struct;

typedef struct
{
	OVFS_BOOL is_enable;	//是否开启
	U32 if_num;	//绑定在虚拟网卡上的网卡个数
    S8  if_array[OVFS_MAX_ETH_CARD_NUM][OVFS_MAX_ETH_IFNAME_SIZE]; //绑定在虚拟网卡上的网卡名称
}ovfs_network_bond_config;



//邮件服务器配置
typedef struct
{
    OVFS_BOOL enable;

    OVFS_BOOL ssl_enable;		// TLS/SSL
    S32  server_port;			// 邮件服务器端口
    S8 server_addr[OVFS_MAX_EMAIL_USR_NAME_LEN];		// 服务器地址

    S8 user_name[OVFS_MAX_EMAIL_USR_NAME_LEN];	// 发送邮箱用户名
    S8 password[OVFS_MAX_EMAIL_USR_PASSWD_LEN];	// 发送邮箱密码
}ovfs_email_sender_cfg;

typedef struct
{
    S8 addr[OVFS_MAX_EMAIL_USR_NAME_LEN];	//邮箱地址
    S8 name[OVFS_MAX_EMAIL_USR_NAME_LEN];	//邮箱用户名
}ovfs_email_receiver_cfg;

typedef struct
{
    S8 fname[OVFS_MAX_FILE_PATH_LEN];
}ovfs_email_attachment_fname;



typedef enum
{
	ADDR = 0,
	BRDADDR,
	NETMASK,
	HWADDR,

	UNKNOW = 100,
}network_adapter_info_type;

typedef void (*ovfs_network_callback_func)(const char *dev_name, const char *ipaddr, const char *netmask, const char *gateway, const char *dns1, const char *dns2);

typedef struct _tagovfs_ddns_cap_info_node_t
{
	OVFS_BOOL enable;
	S8 ddns_name[32];
	S8 ddns_host[64];
	U32 version;
	U32 ddns_port;
}ovfs_ddns_cap_info_node;

typedef struct _tagovfs_ddns_ability_t
{
	U32 node_num;
	ovfs_ddns_cap_info_node ddns_cap[OVFS_MAX_ABILITY_DDNS_NUM];
}ovfs_ddns_ability;

typedef struct
{
	S8 ddns_name[64];
	S8 server_host[256];
	U32 server_port;
	S8 usr_name[64];
	S8 passwd[64];
	S8 domain_host[64];
}ovfs_ddns_cfg;

typedef enum
{
    OVFS_PROTOCOL_TCP = 0x0,
    OVFS_PROTOCOL_UDP,
}ovfs_upnp_port_protocol;

#if 0
typedef enum
{
    OVFS_PORT_RTSP,
    OVFS_PORT_RTMP,
    OVFS_PORT_HTTP,
    OVFS_PORT_HTTPS,
    OVFS_PORT_ONVIF,

    OVFS_PORT_MAX,//注意这个枚举放在最后，用于统计数组个数
}ovfs_port_type;
#endif

//UPNP端口信息
typedef struct
{
    S8 eth_name[OVFS_MAX_ETH_IFNAME_SIZE];
    OVFS_BOOL enable_upnp;
    U16 internal_port;
    U16 external_port;
    //ovfs_port_type port_type;
    ovfs_upnp_port_protocol port_protocol;
    OVFS_BOOL mapping_success;
    S8 externalIP[sizeof("xxx.xxx.xxx.xxx") + 1];
    OVFS_BOOL need_re_lease;//是否需要重新续约
}ovfs_upnp_port_detail_info;

typedef enum
{
	OVFS_DDNS_LIB_COMMON,
	OVFS_DDNS_LIB_CUSTOM
}ovfs_ddns_lib_type;



typedef enum
{
	OVFS_ETH_SPEED_10,
	OVFS_ETH_SPEED_100,
	OVFS_ETH_SPEED_1000,
	OVFS_ETH_SPEED_2500,
	OVFS_ETH_SPEED_10000,
	OVFS_ETH_SPEED_UNKNOW,
}ovfs_eth_speed;

typedef enum
{
	OVFS_DUPLEX_FULL,
	OVFS_DUPLEX_HALF,
	OVFS_DUPLEX_UNKNOW,
}ovfs_eth_duplex;


//诊断信息输出函数
typedef OVFS_ERR (*ovfs_diag_dump)(const S8 *format, ...);

//诊断对象，继承该对象之后就可以加入诊断组织。
class ovfs_diagnose_client
{
public:
	virtual OVFS_VOID diagnose_dump(U32 para, ovfs_diag_dump dump_fuc) = 0;

public:
	virtual ~ovfs_diagnose_client() {};
};

}



#endif
