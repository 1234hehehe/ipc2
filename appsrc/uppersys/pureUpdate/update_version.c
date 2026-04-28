#include <stdio.h>

#include "libcommon_api.h"
#include "update_version.h"
#include "update_common.h"
#include "libupdate_api.h"

#define FACTORY_CFG_PATH       "/usr/etc/factoryinfo.json"
#define CORE_CFG_PATH          "/usr/etc/cfgfiles/Core.json"

#define MEDIASERVER_CFG_PATH   "/usr/etc/cfgfiles/MediaServer.json"
#define WEBSERVER_CFG_PATH     "/usr/etc/cfgfiles/Webserver.json"
#define ONVIF_CFG_PATH         "/usr/etc/cfgfiles/Onvif.json"
#define ALIIOT_CFG_PATH         "/usr/etc/cfgfiles/AliIoT4ovfs.json"
#define DEFAULT_CFG_PATH_AUTO  "/tmp/auto/res/default/Core.json"
#define WEBSERVER_DEF_CFG_PATH_CUSTOM     "/usr/etc/default/Webserver.json"
#define WEBSERVER_DEF_CFG_PATH "/update/res/default/Webserver.json"
#define DEFAULT_CFG_PATH       "/update/res/default/Core.json"

#define SID_CFG_PATH       "/usr/etc/Sid.json"

static int g_nBuildSvn = 1645;

typedef struct _tagSerial_Hardware_Map
{
    S8 szSerial[5];
    S8 szHardware[64];
} Serial_Hardware_Map;

static Serial_Hardware_Map g_stSerialHardwareMap[] =
{
#if 0
    {"016a", "AEVISION_HI3516A_016A"},
    {"016b", "AEVISION_HI3516D_016B"},
    {"016d", "AEVISION_HI3516D_016D"},
    {"016e", "AEVISION_HI3516A_016E"},
    {"016f", "AEVISION_HI3516D_HDMI"},
    {"0187", "AEVISION_HI3516CV300_0187"},
    {"018c", "AEVISION_HI3516A_018C"},
    {"018a", "AEVISION_MSTAR_313E"},
    {"018d", "AEVISION_MSTAR_316DM"},
    {"01d0", "AEVISION_HI3519"},
    {"01d1", "AEVISION_HI3519"},
    {"026a", "FSAN_HI3516A"},
    {"026b", "FSAN_HI3516D"},
    {"026b", "FLOORLINE_HI3516D"},
    {"0281", "FSAN_HI3516D"},
    {"0281", "FLOORLINE_HI3516D"},
    {"0281", "TYCO_HI3516D"},
    {"0281", "FSAN_HI3516D_0281"},
    {"0281", "FSAN_HI3516D_RUIWEI_FACE"},
    {"0284", "FSAN_HI3516A_0284"},
    {"0285", "FSAN_HI3516D_0285"},
    {"0285", "FLOORLINE_HI3516D"},
    {"0287", "FSAN_HI3516CV300_0287"},
    {"0288", "FSAN_MSTAR_XIAOHONGBIAO"},
    {"0288", "FSAN_MSTAR_316DC"},
    {"0289", "FSAN_MSTAR"},
    {"028a", "FSAN_MSTAR_313E"},
    {"028b", "FSAN_HI3516D_DOME_028B"},
    {"02d0", "FSAN_HI3519"},
    {"02d0", "FSAN_HI3519_SMART"},
    {"02d0", "HORIZON_HI3519"}, // old
    {"02d0", "HI3519_AI_BOARD"},
    {"04d0", "HI3519_AI_BOARD"},
    {"04d0", "HORIZON_HI3519_RGMII"}, // new
    {"02d2", "FSAN_HI3519_FISHEYE"},
    {"02d3", "FSAN_HI3516AV200"},
    {"0388", "FSAN_MSTAR_316DC"},
    {"038a", "FSAN_MSTAR_313E"},
    {"09d0", "PV_HI3519"},
    {"09d1", "PV_HI3516D"},
    {"09d0", "PV_HI3519_DOUBLE"},
    {"0588", "DS_MSTAR_316DC"},
    {"058a", "DS_MSTAR_313E"},
    {"0a88", "GAOTONG_MSTAR_316DC"},
    {"0a8a", "GAOTONG_MSTAR_313E"},

    {"0b6b", "H16D_D81_Q38"},
    {"0b6b", "C116D_D1801_0B6B"},
    {"0b6b", "C116D_D1801_03xx"},
    {"0b6b", "HORIZON_HI3516D_X1"},
    {"0b6b", "HI3516D_AI_BOARD"},
    {"0b81", "H16D_D71_R56"},
    {"0b87", "H16CV3_D71_R56"},
    {"0b88", "ZKTECO_MSTAR_316DC"},
    {"0b8c", "H16E_D81_R56"},
    {"0bd0", "H19V1_D81_W386"},
    {"0bd3", "HI3516AV200_AI_BOARD"},
    {"0bd3", "H16AV2_D81_W386"},
    {"0bd3", "H16AV2_D91_R56"},
    {"0b88", "DS_MSTAR_316DC"},
    {"0b8a", "DS_MSTAR_313E"},
    {"0b8d", "DS_MSTAR_316DM"},

    {"0bf0", "JZT30X_D81_W386"},
    {"0bf1", "JZT30N_D81_W386"},
    {"0bf1", "JZT30L_D81_W386"},
#endif

    {"0bd5", "H16CV5_D81_Q38"},
    {"0bd7", "H16CV5_D81_Q38"},

    {"0bd7", "H16CV5_D93_Q38_FC"},
    {"0bd5", "H16CV5_D93_Q38_FR"},

    {"0bd6", "H16CV5_D91_R56_FR"},
    {"0bd8", "H16CV5_D91_R56_FC"},
    {"0bdx", "H16DV3_D92_Q60"},
    {"0bd9", "H16EV2_D91_R56"},
    {"0bd4", "H16EV2_D91_R56_PERSON"},
    {"0bda", "H16EV2_D92_Q38_MINI"},
    {"0bdb", "H16EV2_D92_Q38"},
    {"0bdc", "H16EV2_D92_Q38_PERSON"},
    {"0b6d", "H16EV2_D92_Q38_PERSON"},/*变焦*/
    {"0bdd", "H16EV3_D93_Q38"},
    {"0bde", "H16EV3_D93_Q38_NF"},
    {"0bdf", "H16EV2_D92_Q38_EXT"},

    {"0b6c", "H16EV2_D92_Q38_MINI"},
    {"06bb", "H16EV2_D92_Q38"},
    {"06bc", "H16EV2_D92_Q38_PERSON"},
    {"06bd", "H16EV2_D92_Q38"},

    {"03ba", "H16EV2_D96_Q38_MINI"},
    {"03bf", "H16EV2_D96_Q38_MINIA"},
    {"03b8", "H16EV2_D92_Q38_FACEC"}, //lens
    {"03bb", "H16EV2_D92_Q38_FACEC"},
    {"03b9", "H16EV2_D92_Q38_PERSONC"}, //lens
    {"03bc", "H16EV2_D92_Q38_PERSONC"},
    {"03bd", "H16EV2_D96_Q38_PERSONA"},
    {"03be", "H16EV2_D96_Q38_STD"},
    {"03b7", "H16EV2_D92_Q38_P2P"},
	{"033d", "H16EV2_D01_Q38_PERSONAN"},
	{"033e", "H16EV2_D96_Q38_P2P"},

	{"0370", "JZT40XP_D11_Q44"},
	{"0370", "JZT40XP_D11_Q38"},
	{"0350", "JZT40N_D13_Q38_8MP"},
	{"0351", "JZT40N_D13_Q38_5MP"},
	{"0343", "JZT40N_D13_Q38_5MP_HSDOME"},
	{"0346", "JZT40N_D13_Q38_5MP_HSDOME"},
    {"0380", "JZT40XP_D21_Q38"},
    {"0390", "JZT41L_D21_Q38"},

	{"0373", "JZT31X_D95_Q38_PERSON"},
	{"0374", "JZT31X_D95_Q38_P2P"},
	{"0375", "JZT31X_D01_Q38_P2P_HSDOME"},
	{"0345", "JZT31X_D01_Q38_P2P_HSDOME"},
	{"0376", "JZT31X_D95_Q38_H5"},
	{"0377", "JZT31N_D96_Q38_P2P"},
    {"0347", "JZT31N_D96_Q38_P2P"},
    {"8377", "JZT31N_D96_Q38_P2P"},
    {"8347", "JZT31N_D96_Q38_P2P"},

	{"0379", "JZT31N_D96_Q38_PERSON"},
	{"037a", "JZT31N_D96_Q38_MINI"},
	{"037b", "JZT31L_D20_Q38_MINI"},
	{"037c", "JZT31N_D21_Q38_MINI"},
	{"037d", "JZT31N_D02_Q38_P2P"},
	{"037e", "JZT31N_D01_Q38_P2P"},
	{"037f", "JZT31N_D02_Q38_P2P_HSDOME"},
	{"034f", "JZT31N_D02_Q38_P2P_HSDOME"},
    {"0372", "JZT31X_D01_Q38_P2P_HSWIFIDOME"},
    {"035f", "JZT31N_D02_Q38_P2P_HSWIFIDOME"},
	{"0371", "JZT31X_D11_Q38_WIFIP2P"},
	{"035c", "JZT31X_D13_Q38_WIFIP2P"},
	{"035d", "JZT31X_D13_Q38_P2P_HSWIFIDOME"},
    {"035e", "JZT31N_D01_Q38_WIFIP2P"},

	{"034b", "JZT31N_D41_Q38"},

	{"0353", "JZT31X_D11_Q38_PERSON"},
	{"8286", "JZT31X_D11_Q38_PERSON"},
	{"0354", "JZT31X_D11_Q38_P2P"},
	{"034c", "JZT31X_D13_Q38_P2P_HSDOME"},
	{"034d", "JZT31X_D13_Q38_P2P_HSDOME"},
	{"0355", "JZT31X_D12_Q38_PERSON"},
	{"0356", "JZT31X_D12_Q38_P2P"},
	{"035a", "JZT31N_D11_Q38_MINI4MP"},
	{"035b", "JZT31N_D11_Q38_MINI4MP_P2P"},
    {"735a", "JZT31N_D11_Q38_MINI4MP"},

	{"0344", "JZT31X_D95_Q38_P2P"}, 	//t31x_d01_q38_p2p
	{"0342", "JZT31X_D11_Q38_P2P"}, 	//t31x_d11_q38_p2p
    {"0340", "JZT40N_D13_Q38"},
	{"8340", "JZT40N_D13_Q38"},
	{"0385", "JZT40N_D13_Q38_HSDOME"},
	{"0386", "JZT40N_D13_Q38_HSDOME"},
    {"0341", "JZT40N_D13_Q38_4KP2P"},
	{"0381", "JZT40N_D13_Q38_4KP2P_HSDOME"},
	{"0382", "JZT40N_D13_Q38_4KP2P_HSDOME"},

    {"07da", "H16EV2_D94_Q38_MEIAN_MINI"}, //sd
    {"07db", "H16EV2_D94_Q38_MEIANFACE"}, //sd face
    {"07dc", "H16EV2_D94_Q38_MEIAN"}, //wifi sd person
    {"07dd", "H16EV2_D94_Q38_MEIANFACE"}, //wifi sd face

    {"0bd0", "H16EV3_D95_Q38_FACE"}, // face  zkto
    {"0bd1", "H16EV3_D95_Q38_PERSON"}, //person zkto
    {"0bb0", "H16EV3_D95_Q38_FACE4MP"}, // face  zkto
    {"0bb1", "H16EV3_D95_Q38_PERSON4MP"}, //person zkto

	{"0d87", "H16EV2_D96_Q38_P2P_XD"}, //ev200 p2p xiaoding
	{"0ddb", "H16CV5_D01_Q38S_PERSON_XD"}, //cv500 person xiaoding

	{"037a", "JZT31N_D96_Q38_MINI"},
    {"038a", "JZT31N_D96_Q38_MINI"},
	{"0377", "JZT31N_D96_Q38_P2P"},
	{"0379", "JZT31N_D96_Q38_PERSON"},
	{"0373", "JZT31X_D95_Q38_PERSON"},
	{"0376", "JZT31X_D95_Q38_H5"},

    {"0bbe", "H16EV2_D92_Q38_DOME"}, //person zkto
    {"0bbf", "H16EV3_D95_Q38_DOME"}, //person zkto

	//hdt
	{"03b0", "H16EV3_D95_Q38_FACE"}, // face
    {"03b1", "H16EV3_D95_Q38_PERSON"}, //person
    {"03b2", "H16EV3_D95_Q38_FACE"}, // face lens
    {"03b3", "H16EV3_D95_Q38_PERSON"}, //person lens
	{"0333", "H16EV3_D95_Q38_P2P"},

    {"0306", "H16CV5_D01_Q38_PERSON"},
    {"03db", "H16CV5_D01_Q38_FACEC"},
    {"03de", "H16CV5_D01_Q38_FACER"},

    {"0305", "H16CV5_D01_Q38S_PERSON"},
    {"0307", "H16CV5_D01_Q38S_MOTOR"},
    {"03dc", "H16CV5_D01_Q38S_FACEC"},
    {"03dd", "H16CV5_D01_Q38S_FACER"},

	{"03d7", "H16DV3_D01_Q38S_FACER"},
	{"03d8", "H16DV3_D01_Q38S_FACEC"},

	{"0309", "H16AV3_D01_Q38S"},

	//ds
	{"05B0", "H16EV3_D95_Q38_FACE"}, // face
	{"05b1", "H16EV3_D95_Q38_PERSON"}, //person
	{"05b2", "H16EV3_D95_Q38_FACE"}, // face lens
	{"05b3", "H16EV3_D95_Q38_PERSON"}, //person lens
	{"05b4", "H16EV3_D95_Q38_PERSON"}, //mojing
	{"05b5", "H16EV2_D92_Q38_PERSON"}, //mojing

	//fsan
	{"8279", "JZT31N_D11_W386"}, //t31n_d11_w386
	{"8273", "JZT31X_D11_W386"}, //t31x_d11_w386

	{"0220", "H16EV2_D97_W386"},
	{"0225", "H16EV3_D98_W386"},
	{"0bd1", "H16EV3_D98_W386"},

	{"0b60", "H16CV5_D91_R56_IVS30"},

	{"06be", "H16EV2_D92_Q38_TUSHI"}, //person lens
	{"06bf", "H16EV3_D95_Q38_TUSHI"}, //person lens

	{"0bb2", "H16AV3_D04_Q38_FR"},

	{"0506", "H16CV5_D01_Q38_PERSON"},
	{"05db", "H16CV5_D01_Q38_FACEC"},
	{"05de", "H16CV5_D01_Q38_FACER"},

	{"0505", "H16CV5_D01_Q38S_PERSON"},
	{"0507", "H16CV5_D01_Q38S_MOTOR"},
	{"05dc", "H16CV5_D01_Q38S_FACEC"},
	{"05dd", "H16CV5_D01_Q38S_FACER"},

	{"05d0", "H16DV3_D01_Q38S_FACEC"},
	{"05d1", "H16DV3_D01_Q38S_FACER"},

	{"06d8", "H16EV3_D11_W3711_XZC"},
	{"0378", "JZT31N_D11_X40_WIFIDOME"},
	{"0359", "JZT31N_D11_X40_WIFIDOME_16M"},
	{"0349", "JZT31N_D11_X40_WIFIDOME_16M_V2"},
	{"0389", "JZT31N_D11_X40_WIFIDOME_16M_V3"},
	{"8389", "JZT31N_D11_X40_WIFIDOME_16M_V3"},
	{"0358", "JZT31N_D11_X40"},
	{"0357", "JZT31N_D11_X40_PERSON"},
	{"034a", "JZT31N_D11_X40_PERSON_V2"},
	{"0352", "JZT31X_D11_X40_WIFIDOME_PERSON"},
	{"0348", "JZT31X_D11_X40_WIFIDOME_PERSON_V2"},
	{"0388", "JZT31X_D11_X40_WIFIDOME_PERSON_V3"},
	{"8388", "JZT31X_D11_X40_WIFIDOME_PERSON_V3"},

	{"0383", "JZT31X_D11_X40_4GDOME_PERSON_V3"},
	{"8383", "JZT31X_D11_X40_4GDOME_PERSON_V3"},
	{"0384", "JZT31N_D11_X40_4GDOME_16M_V3"},
	{"8384", "JZT31N_D11_X40_4GDOME_16M_V3"},
	{"038b", "JZT31X_D41_X40_DOME"},
	{"838b", "JZT31X_D41_X40_DOME"},
	{"038e", "JZT31X_D41_X40_DOME"},
	{"838e", "JZT31X_D41_X40_DOME"},
	{"038d", "JZT31N_D41_X40_DOME"},
	{"838d", "JZT31N_D41_X40_DOME"},
	{"038f", "JZT31N_D41_X40_DOME"},
	{"838f", "JZT31N_D41_X40_DOME"},

    {"038c", "JZT31N_D31_Q38"},


	{"0607", "JZT40XP_D12_W386_MOTOR"},

	{"0311", "JZT41L_D31_Q38_ALARM"},
    {"0312", "JZT41N_D31_Q38_ALARM"},
    {"0318", "JZT41L_D31_Q38"},
	{"031a", "JZT41N_D32_Q38_DOME"},
	{"031b", "JZT41N_D32_Q38_DOME"},
	{"031c", "JZT41N_D32_Q38_DOME"},
	{"032a", "JZT41N_D41_X40_AISPDOME"},
	{"032b", "JZT41N_D41_X40_AISPDOME"},
	{"032c", "JZT41N_D41_X40_AISPDOME"},
	{"032d", "JZT41N_D41_X40_AISPDOME"},
    {"0387", "JZT40N_D31_X65_STITCH"},
    {"0315", "JZT41N_D33_Q40_FULL"},
    {"8315", "JZT41N_D33_Q40_FULL"},
	{"0314", "JZT41N_D33_Q40_FULL"},
	{"8314", "JZT41N_D33_Q40_FULL"},
	{"0313", "JZT41N_D33_Q40_FULL"},
	{"8313", "JZT41N_D33_Q40_FULL"},
	{"0316", "JZT41N_D33_Q40_FULL_DOME"},
	{"031d", "JZT41N_D33_Q40_FULL_DOME"},
	{"031e", "JZT41N_D33_Q40_FULL_DOME"},
	{"0320", "JZT41N_D42_Q38_AIISP"},
    {"0321", "JZT41N_D42_Q38_AIISP4MP"},

    {"0325", "JZT31X_D41_Q38_PERSON"},

    {"032e", "JZT31N_D44_Q38"},
    {"034e", "JZT31N_D45_Q38"},

    {"032f", "JZT31N_D44_Q38_P2P"},

    {"030a", "JZT31N_D43_Q38"},
    {"030c", "JZT31N_D42_Q38"},

    {"0322", "JZT32L_D51_Q38"},
    {"0323", "JZT32L_D51_X52"},
    {"0324", "JZT32L_D51_X52"},

	{"0328", "JZT32L_D41_Q38"},
    {"0329", "JZT32N_D41_Q38"},
    {"0327", "JZT32N_D41_Q38"},
    {"0326", "JZT32N_D41_Q38"},

    {"03a0", "JZT32L_D53_Q38"},
    {"03a1", "JZT33L_D51_Q38"},
    {"03a2", "JZT33N_D51_Q38"},
    {"03a8", "JZT33N_D51_Q38"},

    {"03a3", "JZT41N_D51_Q38_FULL"},
    {"03a5", "JZT41N_D51_Q38_FULL"},
    {"03a4", "JZT33L_D61_Q38"},
    {"03a6", "JZT41N_D51_Q38_FULL_DOME"},
    {"03ae", "JZT41N_D51_Q38_FULL_DOME"},
    {"03a7", "JZT33L_D51_X40"},
    {"03a9", "JZT33L_D51_X40"},
    {"03aa", "JZT33L_D61_Q38_P2P"},
    {"03ab", "JZT33L_D61_Q38_P2P"},

    {"03d0", "JZT32N_D52_X40_2EYE"},
    {"03d1", "JZT32N_D52_X40_2EYE"},

    {"03d2", "JZT32N_D51_Q52"},
    {"03d3", "JZT32N_D51_X52"},
    {"03d4", "JZT32N_D51_X52"},
    {"03d5", "JZT32N_D51_Q38_DOME"},
    {"03d6", "JZT32N_D51_Q38_DOME"},

	{"03d9", "JZT32N_D41_Q38_FULL"},
    {"03da", "JZT32N_D41_Q38_FULL"},

    {"7311", "JZT41L_D31_Q38_ALARM"},
    {"7322", "JZT32L_D51_Q38"},
    {"7388", "JZT31X_D11_X40_WIFIDOME_PERSON_V3"},


};

static DeviceVersion_S g_stVersion;

DeviceVersion_S* update_version_GetHandle()
{
    return &g_stVersion;
}

static S32 updateLoadCfg(const S8 *pCfgFilePath, cJSON_Struct **pTempJson)
{
	S32 nStrLen = 0;
	char *pConfigString = NULL;

    FILE *fp = NULL;

	if ((NULL == pCfgFilePath) || (NULL == pTempJson) || (*pTempJson != NULL))
	{
	    LOGE("Enter parameters error.\n");
		return -1;
	}

	fp = fopen(pCfgFilePath, "rb");
    if (NULL == fp)
    {
        LOGI("fopen %s error.\n", pCfgFilePath);
        return -1;
    }

	fseek(fp, 0, SEEK_END);
	nStrLen = ftell(fp);
	fseek(fp, 0, SEEK_SET);

    if (nStrLen > 0)
	{
		pConfigString = (char *)Common_Malloc(nStrLen, 0, __FUNCTION__, __LINE__);
		if (pConfigString != NULL)
		{
			if(nStrLen == fread(pConfigString, 1, (U32)nStrLen, fp))
			{
				*pTempJson = Common_Json_Parse(pConfigString, NULL, NULL);
			}
		}
	}
    fclose(fp);

    if (pConfigString != NULL)
	{
		Common_Free(pConfigString, __FUNCTION__, __LINE__);
		pConfigString = NULL;
	}

    if (NULL == *pTempJson)
	{
		return -1;
	}

	return 0;
}

static S32 updateSaveCfg(const S8 *pcPath, cJSON_Struct *pConfig)
{
    S32 nRet    = -1;
	S32 nStrLen =  0;

    FILE *fp    = NULL;
	S8 *pConfigString = NULL;

    fp = fopen(pcPath, "wb+");
	if (fp != NULL)
	{
		pConfigString = Common_Json_Print(pConfig, &nStrLen);
		if (pConfigString && nStrLen > 0)
		{
			if(nStrLen + 1 == fwrite(pConfigString,1,(U32)nStrLen + 1,fp))
			{
				nRet = 0;
			}
		}

        fclose(fp);
	}

    if (pConfigString != NULL)
	{
		Common_Free(pConfigString,__FUNCTION__,__LINE__);
		pConfigString = NULL;
	}

    return nRet;
}

static S32 getMediaServerConfig(cJSON_Struct **ppstConfig)
{
    S32 ret = -1;
    cJSON_Struct *pMediaServerConfig = NULL;
    cJSON_Struct *pRtspConfig        = NULL;

    ret = updateLoadCfg(MEDIASERVER_CFG_PATH, &pMediaServerConfig);
    if ((0 == ret) && (NULL != pMediaServerConfig))
    {
        pRtspConfig = Common_Json_DetachItem(pMediaServerConfig, -1, "MediaServer");
        Common_Json_Delete(pMediaServerConfig);

        *ppstConfig = Common_Json_DetachItem(pRtspConfig, -1, "Rtsp");
        Common_Json_Delete(pRtspConfig);

        return 0;
    }

    return -1;
}

static S32 getWebServerConfig(cJSON_Struct **ppstConfig)
{
    S32 ret = -1;
    ret = updateLoadCfg(WEBSERVER_CFG_PATH, ppstConfig);
    if (0 != ret)
    {
        LOGI("Load %s failed.\n", WEBSERVER_CFG_PATH);
        ret = updateLoadCfg(WEBSERVER_DEF_CFG_PATH_CUSTOM, ppstConfig);
    }
    if (0 != ret)
    {
        LOGI("Load %s failed.\n", WEBSERVER_CFG_PATH);
        ret = updateLoadCfg(WEBSERVER_DEF_CFG_PATH, ppstConfig);
    }

    return ret;
}

static S32 getOnvifConfig(cJSON_Struct **ppstConfig)
{
    return updateLoadCfg(ONVIF_CFG_PATH, ppstConfig);
}

static S32 getFactoryConfig(cJSON_Struct **ppstConfig)
{
    return updateLoadCfg(FACTORY_CFG_PATH, ppstConfig);
}

static S32 getCoreConfig(cJSON_Struct **ppstConfig)
{
    S32 ret = -1;
    cJSON_Struct *pCoreConfig = NULL;

    ret = updateLoadCfg(CORE_CFG_PATH, &pCoreConfig);
    if ((0 == ret) && (NULL != pCoreConfig))
    {
        *ppstConfig = Common_Json_DetachItem(pCoreConfig, -1, "Version");
        Common_Json_Delete(pCoreConfig);

        return 0;
    }

    return -1;
}

static S32 getAliIotConfig(cJSON_Struct **ppstConfig)
{
    S32 ret = -1;
	S8 *szStringVal = NULL;
	S32 nIntValue = 0,nCode = 0;
	cJSON_Struct *pInParam = NULL,*pOutParam = NULL;
    cJSON_Struct *pData = NULL;

    //ret = updateLoadCfg(ALIIOT_CFG_PATH, ppstConfig);

    //if(ret != 0)
    {
        pInParam = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
        Common_Json_SetAttrValue(pInParam,-1,"Header",Common_Json_Type_Object,NULL,0,0);
        Common_Json_SetAttrValue(pInParam,-1,"Header/Uri",Common_Json_Type_String,"/webserver/customaction",0,0);
        Common_Json_SetAttrValue(pInParam,-1,"Header/Method",Common_Json_Type_String,"put",0,0);
        pData = Common_Json_SetAttrValueObj(pInParam, "Data");
        Common_Json_SetAttrValueObj(pData, "Header");
        Common_Json_SetAttrValueStr(pData, "Header/Uri", "frmAliIoTCfg");
        Common_Json_SetAttrValueObj(pData, "Header/Auth");
        Common_Json_SetAttrValueInt(pData, "Header/Auth/Method", 1);
        Common_Json_SetAttrValueStr(pData, "Header/Auth/Username", "(null)");
        Common_Json_SetAttrValueStr(pData, "Header/Auth/Password", "ovfsZSJQZLHL");
        Common_Json_SetAttrValueObj(pData, "Data");
        Common_Json_SetAttrValueInt(pData, "Data/Dev", 1);
        Common_Json_SetAttrValueInt(pData, "Data/Type", 0);
        Common_Json_SetAttrValueInt(pData, "Data/Ch", 0);
        Common_Json_SetAttrValueObj(pData, "Data/Data");
        Update_Tcp_Require("127.0.0.1",10009,pInParam,&pOutParam,3000);
        if (pOutParam != NULL)
        {
            /*char *str = Common_Json_Print(pOutParam, NULL);
            LOGD("str:[%s]\n",str);
            Common_Free(str,__FUNCTION__,__LINE__);*/
            nCode = -1;
            Common_Json_GetAttrValue(pOutParam,-1,"/Header/Code",NULL,NULL,&nCode,NULL);
            if (nCode == 0 || nCode == 200)
            {
                ret = 0;
                cJSON_Struct *pDataJson;
    			pDataJson = Common_Json_GetItem(pOutParam,-1,"/Data");
    			if (NULL != pDataJson)
    			{
                    *ppstConfig = Common_Json_Duplicate(pDataJson, 1);
                }
            }

            Common_Json_Delete(pOutParam);
        }

        if(pInParam)
        {
            Common_Json_Delete(pInParam);
            pInParam = NULL;
        }
    }

    return ret;
}

static S32 getPtzConfig(cJSON_Struct **ppstConfig)
{
    S32 ret = -1;
	S8 *szStringVal = NULL;
	S32 nIntValue = 0,nCode = 0;
	cJSON_Struct *pInParam = NULL,*pOutParam = NULL;

    //ret = updateLoadCfg(ALIIOT_CFG_PATH, ppstConfig);

    //if(ret != 0)
    {
        pInParam = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
        Common_Json_SetAttrValue(pInParam,-1,"Header",Common_Json_Type_Object,NULL,0,0);
        Common_Json_SetAttrValue(pInParam,-1,"Header/Uri",Common_Json_Type_String,"/ptz/cmd",0,0);
        Common_Json_SetAttrValue(pInParam,-1,"Header/Method",Common_Json_Type_String,"get",0,0);

        Common_Json_SetAttrValue(pInParam,-1,"Data",Common_Json_Type_Object,NULL,0,0);
        Common_Json_SetAttrValue(pInParam,-1,"Data/Type",Common_Json_Type_Number,NULL,47,0);
        Update_Tcp_Require("127.0.0.1",10009,pInParam,&pOutParam,3000);
        if (pOutParam != NULL)
        {
            nCode = -1;
            Common_Json_GetAttrValue(pOutParam,-1,"/Header/Code",NULL,NULL,&nCode,NULL);
            if (nCode == 0 || nCode == 200)
            {
                ret = 0;
                cJSON_Struct *pDataJson;
    			pDataJson = Common_Json_GetItem(pOutParam,-1,"/Data");
    			if (NULL != pDataJson)
    			{
                    *ppstConfig = Common_Json_Duplicate(pDataJson, 1);
                }
            }

            Common_Json_Delete(pOutParam);
        }

        if(pInParam)
        {
            Common_Json_Delete(pInParam);
            pInParam = NULL;
        }
    }

    return ret;
}

static S32 getDefaultConfig(cJSON_Struct **ppstConfig)
{
    S32 ret = -1;
    cJSON_Struct *pDefConfig = NULL;

    ret = updateLoadCfg(DEFAULT_CFG_PATH_AUTO, &pDefConfig);
    if (0 != ret)
    {
        ret = updateLoadCfg(DEFAULT_CFG_PATH, &pDefConfig);
    }

    if ((0 == ret) && (NULL != pDefConfig))
    {
        *ppstConfig = Common_Json_DetachItem(pDefConfig, -1, "Version");
        Common_Json_Delete(pDefConfig);

        return 0;
    }

    return -1;
}

static S32 getSidConfig(cJSON_Struct **ppstConfig)
{
    return updateLoadCfg(SID_CFG_PATH, ppstConfig);
}

S32 setSidConfig(cJSON_Struct *pConfig)
{
    return updateSaveCfg(SID_CFG_PATH, pConfig);
}



static S32 checkRight(S8 *pSerial, S8 *szHardware)
{
    S32 bActivated = 0;
    S32 nNum = 0, i;
    if (pSerial == NULL || szHardware == NULL)
    {
        return bActivated;
    }

    nNum = sizeof(g_stSerialHardwareMap) / sizeof(g_stSerialHardwareMap[0]);
    for (i = 0; i < nNum; i++)
    {
        if((Common_StrniCmp(pSerial, g_stSerialHardwareMap[i].szSerial, 4) == 0)
            && (0 == Common_StriCmp(szHardware, g_stSerialHardwareMap[i].szHardware)))
        {
            bActivated = 1;
            break;
        }
    }

    return bActivated;
}

//  TODO:  如果custom分区损坏，需要考虑从其他分区获取版本号信息.
S32 updateLoadVer(Update_Version_S *pstVersion)
{
    S32 ret = -1;
    S32 nIntValue  = -1;
    S8 *pSerial    = NULL;
    S8 *pStringVal = NULL;
    cJSON_Struct *pFactoryInfoJson = NULL;
    cJSON_Struct *pCoreConfig      = NULL;
    cJSON_Struct *pDefaultCoreConfig   = NULL;

    cJSON_Struct *pMediaServerJson = NULL;
    cJSON_Struct *pWebserverJson   = NULL;
    cJSON_Struct *pOnvifJson       = NULL;
    cJSON_Struct *pAliIotJson       = NULL;
    cJSON_Struct *pPtzJson       = NULL;
    cJSON_Struct *pSidJson       = NULL;

    ret = getMediaServerConfig(&pMediaServerJson);
    if (0 != ret)
    {
        LOGI("Get mediaserver config error.\n");
        //return -1;
    }

    ret = getWebServerConfig(&pWebserverJson);
    if (0 != ret)
    {
        LOGI("Get webserver config error.\n");
        //return -1;
    }

    ret = getOnvifConfig(&pOnvifJson);
    if (0 != ret)
    {
        LOGI("Get onvif config error.\n");
        //return -1;
    }

    ret = getFactoryConfig(&pFactoryInfoJson);
    if (0 != ret)
    {
        LOGI("Get factory config error.\n");
        //return -1;
    }

    ret = getCoreConfig(&pCoreConfig);
    if (0 != ret)
    {
        LOGI("Get core config error.\n");
        //Common_Json_Delete(pFactoryInfoJson);
        //return -1;
    }

    ret = getAliIotConfig(&pAliIotJson);
    if (0 != ret)
    {
        LOGI("Get iot config error.\n");
        //Common_Json_Delete(pFactoryInfoJson);
        //return -1;
    }

    ret = getPtzConfig(&pPtzJson);

    getSidConfig(&pSidJson);

    nIntValue = 0;
    (void)Common_Json_GetAttrValue(pMediaServerJson, -1, "Enable", NULL, NULL, &nIntValue, NULL);
    pstVersion->lRtspEnable = nIntValue;

    nIntValue = 0;
    (void)Common_Json_GetAttrValue(pMediaServerJson, -1, "RtspPort", NULL, NULL, &nIntValue, NULL);
    pstVersion->lRtspPort = nIntValue;

    // RtspHttpPort
    nIntValue = 0;
    (void)Common_Json_GetAttrValue(pMediaServerJson, -1, "HttpPort", NULL, NULL, &nIntValue, NULL);
    pstVersion->lRtspHttpPort= nIntValue;

    nIntValue = 0;
    (void)Common_Json_GetAttrValue(pWebserverJson, -1, "HttpPort", NULL, NULL, &nIntValue, NULL);
    pstVersion->lHttpPort = nIntValue;

    nIntValue = 0;
    (void)Common_Json_GetAttrValue(pWebserverJson, -1, "HttpsPort", NULL, NULL, &nIntValue, NULL);
    pstVersion->lHttpsPort= nIntValue;

    //nIntValue = 0;
    //(void)Common_Json_GetAttrValue(pOnvifJson, -1, "OnvifPort", NULL, NULL, &nIntValue, NULL);
    pstVersion->lOnvifPort = nIntValue;

    nIntValue = -1;
    if(Common_File_IsExist("/root/bin/ovfs_onvif"))
    {//ovfs_onvif
        (void)Common_Json_GetAttrValue(pOnvifJson, -1, "AdaptiveIp", NULL, NULL, &nIntValue, NULL);
    }
    else
    {//libonvif
        Common_Json_GetAttrValueInt(pWebserverJson, "OnvifCfg/AdaptiveIp", &nIntValue);
    }

    pstVersion->lOnvifAdaptiveIp = nIntValue;

    pStringVal = NULL;
    if(NULL != pstVersion->szIotQrCode)
    {
        Common_Free(pstVersion->szIotQrCode, __FUNCTION__, __LINE__);
        pstVersion->szIotQrCode = NULL;
    }
    if(Common_Json_GetAttrValueStr(pAliIotJson, "QRCode", &pStringVal))
    {
        pstVersion->szIotQrCode = Common_StrDup(pStringVal, __FUNCTION__, __LINE__);
    }

    pStringVal = NULL;
    if(NULL != pstVersion->szMovementVersion)
    {
        Common_Free(pstVersion->szMovementVersion, __FUNCTION__, __LINE__);
        pstVersion->szMovementVersion = NULL;
    }
    if(Common_Json_GetAttrValueStr(pPtzJson, "version", &pStringVal))
    {
        pstVersion->szMovementVersion = Common_StrDup(pStringVal, __FUNCTION__, __LINE__);
    }

    (void)Common_Json_GetAttrValue(pCoreConfig, -1, "DeviceName", NULL, &pStringVal, NULL, NULL);
	if(NULL != pstVersion->szDeviceName)
	{
		Common_Free(pstVersion->szDeviceName, __FUNCTION__, __LINE__);
		pstVersion->szDeviceName = NULL;
	}
    if (NULL != pStringVal)
    {
        pstVersion->szDeviceName = Common_StrDup(pStringVal, __FUNCTION__, __LINE__);
    }
    else
    {
        ret = getDefaultConfig(&pDefaultCoreConfig);
        if (0 != ret)
        {
            LOGI("Get default config error.\n");
            //Common_Json_Delete(pCoreConfig);
            //Common_Json_Delete(pFactoryInfoJson);
            //return -1;
        }

        pStringVal = NULL;
        (void)Common_Json_GetAttrValue(pDefaultCoreConfig, -1, "DeviceName", NULL, &pStringVal, NULL, NULL);
        if (NULL != pStringVal)
        {
            pstVersion->szDeviceName = Common_StrDup(pStringVal, __FUNCTION__, __LINE__);
        }
        else
        {

            pStringVal = NULL;
            (void)Common_Json_GetAttrValue(pFactoryInfoJson, -1, "/FactoryInfo/DeviceName", NULL, &pStringVal, NULL, NULL);
            if (NULL != pStringVal)
            {
                pstVersion->szDeviceName = Common_StrDup(pStringVal, __FUNCTION__, __LINE__);
            }
            else
            {
                pstVersion->szDeviceName = Common_StrDup((char *)"", __FUNCTION__, __LINE__);
            }
        }
    }

    pStringVal = NULL;
    (void)Common_Json_GetAttrValue(pFactoryInfoJson, -1, "/FactoryInfo/ProductName", NULL, &pStringVal, NULL, NULL);
	if(NULL != pstVersion->szProductName)
	{
		Common_Free(pstVersion->szProductName, __FUNCTION__, __LINE__);
		pstVersion->szProductName = NULL;
	}
    if (NULL != pStringVal)
    {
        pstVersion->szProductName = Common_StrDup(pStringVal, __FUNCTION__, __LINE__);
    }
    else
    {
        pstVersion->szProductName = Common_StrDup((char *)"", __FUNCTION__, __LINE__);
    }

    pStringVal = NULL;
    (void)Common_Json_GetAttrValue(pFactoryInfoJson, -1, "/FactoryInfo/ManufacturerName", NULL, &pStringVal, NULL, NULL);
	if(NULL != pstVersion->szManufacturer)
	{
		Common_Free(pstVersion->szManufacturer, __FUNCTION__, __LINE__);
		pstVersion->szManufacturer = NULL;
	}
    if (NULL != pStringVal)
    {
        pstVersion->szManufacturer = Common_StrDup(pStringVal, __FUNCTION__, __LINE__);
    }
    else
    {
        pstVersion->szManufacturer = Common_StrDup((char *)"", __FUNCTION__, __LINE__);
    }

    pStringVal = NULL;
    (void)Common_Json_GetAttrValue(pFactoryInfoJson, -1, "/FactoryInfo/Brand", NULL, &pStringVal, NULL, NULL);
	if(NULL != pstVersion->szBrand)
	{
		Common_Free(pstVersion->szBrand, __FUNCTION__, __LINE__);
		pstVersion->szBrand = NULL;
	}
    if (pStringVal != NULL)
    {
        pstVersion->szBrand = Common_StrDup(pStringVal, __FUNCTION__, __LINE__);
    }
    else
    {
        pstVersion->szBrand = Common_StrDup((char *)"", __FUNCTION__, __LINE__);
    }

    pStringVal = NULL;
    (void)Common_Json_GetAttrValue(pFactoryInfoJson, -1, "/FactoryInfo/Customer", NULL, &pStringVal, NULL, NULL);
	if(NULL != pstVersion->szCustomer)
	{
		Common_Free(pstVersion->szCustomer, __FUNCTION__, __LINE__);
		pstVersion->szCustomer = NULL;
	}
    if (pStringVal != NULL)
    {
        pstVersion->szCustomer = Common_StrDup(pStringVal, __FUNCTION__, __LINE__);
    }
    else
    {
        pstVersion->szCustomer = Common_StrDup((char *)"", __FUNCTION__, __LINE__);
    }

    pStringVal = NULL;
    (void)Common_Json_GetAttrValue(pFactoryInfoJson, -1, "/FactoryInfo/Country", NULL, &pStringVal, NULL, NULL);
	if(NULL != pstVersion->szCountry)
	{
		Common_Free(pstVersion->szCountry, __FUNCTION__, __LINE__);
		pstVersion->szCountry = NULL;
	}
    if (pStringVal != NULL)
    {
        pstVersion->szCountry = Common_StrDup(pStringVal, __FUNCTION__, __LINE__);
    }
    else
    {
        pstVersion->szCountry = Common_StrDup((char *)"", __FUNCTION__, __LINE__);
    }

    pStringVal = NULL;
    (void)Common_Json_GetAttrValue(pFactoryInfoJson, -1, "/FactoryInfo/City", NULL, &pStringVal, NULL, NULL);
	if(NULL != pstVersion->szCity)
	{
		Common_Free(pstVersion->szCity, __FUNCTION__, __LINE__);
		pstVersion->szCity = NULL;
	}
    if (pStringVal != NULL)
    {
        pstVersion->szCity = Common_StrDup(pStringVal, __FUNCTION__, __LINE__);
    }
    else
    {
        pstVersion->szCity = Common_StrDup((char *)"", __FUNCTION__, __LINE__);
    }

    pStringVal = NULL;
    (void)Common_Json_GetAttrValue(pFactoryInfoJson, -1, "/FactoryInfo/Web", NULL, &pStringVal, NULL, NULL);
	if(NULL != pstVersion->szWeb)
	{
		Common_Free(pstVersion->szWeb, __FUNCTION__, __LINE__);
		pstVersion->szWeb = NULL;
	}
    if (pStringVal != NULL)
    {
        pstVersion->szWeb = Common_StrDup(pStringVal, __FUNCTION__, __LINE__);
    }
    else
    {
        pstVersion->szWeb = Common_StrDup((char *)"", __FUNCTION__, __LINE__);
    }

    pStringVal = NULL;
    (void)Common_Json_GetAttrValue(pFactoryInfoJson, -1, "/FactoryInfo/Tel", NULL, &pStringVal, NULL, NULL);
	if(NULL != pstVersion->szTel)
	{
		Common_Free(pstVersion->szTel, __FUNCTION__, __LINE__);
		pstVersion->szTel = NULL;
	}
    if (pStringVal != NULL)
    {
        pstVersion->szTel = Common_StrDup(pStringVal, __FUNCTION__, __LINE__);
    }
    else
    {
        pstVersion->szTel = Common_StrDup((char *)"", __FUNCTION__, __LINE__);
    }

    pStringVal = NULL;
    (void)Common_Json_GetAttrValue(pFactoryInfoJson, -1, "/FactoryInfo/Copyright", NULL, &pStringVal, NULL, NULL);
	if(NULL != pstVersion->szCopyRight)
	{
		Common_Free(pstVersion->szCopyRight, __FUNCTION__, __LINE__);
		pstVersion->szCopyRight = NULL;
	}
    if (pStringVal != NULL)
    {
        pstVersion->szCopyRight = Common_StrDup(pStringVal, __FUNCTION__, __LINE__);
    }
    else
    {
        pstVersion->szCopyRight = Common_StrDup((char *)"", __FUNCTION__, __LINE__);
    }

    pStringVal = NULL;
    (void)Common_Json_GetAttrValue(pFactoryInfoJson, -1, "/FactoryInfo/DeviceType", NULL, &pStringVal, NULL, NULL);
	if(NULL != pstVersion->szDeviceType)
	{
		Common_Free(pstVersion->szDeviceType, __FUNCTION__, __LINE__);
		pstVersion->szDeviceType = NULL;
	}
    if (pStringVal != NULL)
    {
        pstVersion->szDeviceType = Common_StrDup(pStringVal, __FUNCTION__, __LINE__);
    }
    else
    {
        pstVersion->szDeviceType = Common_StrDup((char *)"IPC", __FUNCTION__, __LINE__);
    }

    pStringVal = NULL;
    (void)Common_Json_GetAttrValue(pCoreConfig, -1, "DeviceTypeString", NULL, &pStringVal, NULL, NULL);
	if(NULL != pstVersion->szDeviceTypeString)
	{
		Common_Free(pstVersion->szDeviceTypeString, __FUNCTION__, __LINE__);
		pstVersion->szDeviceTypeString = NULL;
	}
    if (pStringVal != NULL)
    {
        pstVersion->szDeviceTypeString = Common_StrDup(pStringVal, __FUNCTION__, __LINE__);
    }
    else
    {
        pstVersion->szDeviceTypeString = Common_StrDup((char *)"ipc_normal", __FUNCTION__, __LINE__);
    }

    pStringVal = NULL;
    (void)Common_Json_GetAttrValue(pFactoryInfoJson, -1, "/FactoryInfo/DeviceModel", NULL, &pStringVal, NULL, NULL);
	if(NULL != pstVersion->szDeviceModel)
	{
		Common_Free(pstVersion->szDeviceModel, __FUNCTION__, __LINE__);
		pstVersion->szDeviceModel = NULL;
	}
    if (pStringVal != NULL)
    {
        pstVersion->szDeviceModel = Common_StrDup(pStringVal, __FUNCTION__, __LINE__);
    }
    else
    {
        pstVersion->szDeviceModel = Common_StrDup((char *)"", __FUNCTION__, __LINE__);
    }

    pStringVal = NULL;
    (void)Common_Json_GetAttrValue(pFactoryInfoJson, -1, "/FactoryInfo/SwVersion", NULL, &pStringVal, NULL, NULL);
	if(NULL != pstVersion->szVersion)
	{
		Common_Free(pstVersion->szVersion, __FUNCTION__, __LINE__);
		pstVersion->szVersion = NULL;
	}
    if (pStringVal != NULL)
    {
        pstVersion->szVersion = Common_StrDup(pStringVal, __FUNCTION__, __LINE__);
    }
    else
    {
        //避免乱码
        pstVersion->szVersion = Common_StrDup((char *)"1.0.0", __FUNCTION__, __LINE__);
    }

    (void)Common_Json_GetAttrValue(pFactoryInfoJson, -1, "/FactoryInfo/SvnNumber", NULL, NULL, &nIntValue, NULL);
    if (nIntValue != 0)
    {
        pstVersion->nSvnNumber = nIntValue;
    }
    else
    {
        pstVersion->nSvnNumber = g_nBuildSvn;
    }

    S8 * hardver = update_common_GetHardwareVersion();
	if(NULL != pstVersion->szHardVersion)
	{
		Common_Free(pstVersion->szHardVersion, __FUNCTION__, __LINE__);
		pstVersion->szHardVersion = NULL;
	}
    if (NULL == hardver)
    {
        pStringVal = NULL;
        (void)Common_Json_GetAttrValue(pFactoryInfoJson, -1, "/FactoryInfo/HwVersion", NULL, &pStringVal, NULL, NULL);
        if (pStringVal != NULL)
        {
            pstVersion->szHardVersion = Common_StrDup(pStringVal, __FUNCTION__, __LINE__);
        }
        else
        {
            pstVersion->szHardVersion = Common_StrDup((char *)"1.0.0", __FUNCTION__, __LINE__);
        }
    }
    else
    {
        pstVersion->szHardVersion = Common_StrDup((char *)hardver, __FUNCTION__, __LINE__);
    }

    pStringVal = NULL;
    (void)Common_Json_GetAttrValue(pFactoryInfoJson, -1, "/FactoryInfo/ProductDate", NULL, &pStringVal, NULL, NULL);
	if(NULL != pstVersion->szProductDate)
	{
		Common_Free(pstVersion->szProductDate, __FUNCTION__, __LINE__);
		pstVersion->szProductDate = NULL;
	}
    if (pStringVal != NULL)
    {
        pstVersion->szProductDate = Common_StrDup(pStringVal, __FUNCTION__, __LINE__);
    }
    else
    {
        //避免版本号随机
        pstVersion->szProductDate = Common_StrDup((char *)"", __FUNCTION__, __LINE__);
    }

    pStringVal = NULL;
    (void)Common_Json_GetAttrValue(pFactoryInfoJson, -1, "/FactoryInfo/BuildDate", NULL, &pStringVal, NULL, NULL);
	if(NULL != pstVersion->szBuildDate)
	{
		Common_Free(pstVersion->szBuildDate, __FUNCTION__, __LINE__);
		pstVersion->szBuildDate = NULL;
	}
    if (pStringVal != NULL)
    {
        pstVersion->szBuildDate = Common_StrDup(pStringVal, __FUNCTION__, __LINE__);
    }
    else
    {
        //避免版本号随机
        pstVersion->szBuildDate = Common_StrDup((char *)"", __FUNCTION__, __LINE__);
    }

    pStringVal = NULL;
    (void)Common_Json_GetAttrValue(pFactoryInfoJson, -1, "/FactoryInfo/SensorModel", NULL, &pStringVal, NULL, NULL);
	if(NULL != pstVersion->szSensorModel)
	{
		Common_Free(pstVersion->szSensorModel, __FUNCTION__, __LINE__);
		pstVersion->szSensorModel = NULL;
	}
    if (pStringVal != NULL)
    {
        pstVersion->szSensorModel = Common_StrDup(pStringVal, __FUNCTION__, __LINE__);
    }
    else
    {
        pstVersion->szSensorModel = Common_StrDup((char *)"", __FUNCTION__, __LINE__);
    }

    pStringVal = NULL;
    (void)Common_Json_GetAttrValue(pFactoryInfoJson, -1, "/FactoryInfo/IsofDome", NULL, &pStringVal, NULL, NULL);
    if (pStringVal != NULL)
    {
        //'n','y','1','2','3'
        pstVersion->IsOfDome = (pStringVal[0] != 'n');
    }

    pStringVal = NULL;
    (void)Common_Json_GetAttrValue(pFactoryInfoJson, -1, "/FactoryInfo/IsofIr", NULL, &pStringVal, NULL, NULL);
    if (pStringVal != NULL)
    {
        pstVersion->IsOfIr = (pStringVal[0] == 'y');
    }

    nIntValue = 0;
    (void)Common_Json_GetAttrValue(pFactoryInfoJson, -1, "/FactoryInfo/AutoLens/LensSupport", NULL, NULL, &nIntValue, NULL);
    pstVersion->nLensSupport = nIntValue;

    pStringVal = NULL;
    (void)Common_Json_GetAttrValue(pFactoryInfoJson, -1, "/FactoryInfo/AutoLens/LensDrvType", NULL, &pStringVal, NULL, NULL);
	if(NULL != pstVersion->szLensDrvType)
	{
		Common_Free(pstVersion->szLensDrvType, __FUNCTION__, __LINE__);
		pstVersion->szLensDrvType = NULL;
	}
    if (pStringVal != NULL)
    {
        pstVersion->szLensDrvType = Common_StrDup(pStringVal, __FUNCTION__, __LINE__);
    }
    else
    {
        pstVersion->szLensDrvType = Common_StrDup((char *)"", __FUNCTION__, __LINE__);
    }

    pStringVal = NULL;
    (void)Common_Json_GetAttrValue(pFactoryInfoJson, -1, "/FactoryInfo/AutoLens/LensType", NULL, &pStringVal, NULL, NULL);
	if(NULL != pstVersion->szLensType)
	{
		Common_Free(pstVersion->szLensType, __FUNCTION__, __LINE__);
		pstVersion->szLensType = NULL;
	}
    if (pStringVal != NULL)
    {
        pstVersion->szLensType = Common_StrDup(pStringVal, __FUNCTION__, __LINE__);
    }
    else
    {
        pstVersion->szLensType = Common_StrDup((char *)"", __FUNCTION__, __LINE__);
    }

    nIntValue = 0;
    (void)Common_Json_GetAttrValue(pFactoryInfoJson, -1, "/FactoryInfo/AutoIris/IrisSupport", NULL, NULL, &nIntValue, NULL);
    pstVersion->nIrisSupport = nIntValue;

    pStringVal = NULL;
    (void)Common_Json_GetAttrValue(pFactoryInfoJson, -1, "/FactoryInfo/AutoIris/IrisType", NULL, &pStringVal, NULL, NULL);
	if(NULL != pstVersion->szIrisType)
	{
		Common_Free(pstVersion->szIrisType, __FUNCTION__, __LINE__);
		pstVersion->szIrisType = NULL;
	}
    if (pStringVal != NULL)
    {
        pstVersion->szIrisType = Common_StrDup(pStringVal, __FUNCTION__, __LINE__);
    }
    else
    {
        pstVersion->szIrisType = Common_StrDup((char *)"", __FUNCTION__, __LINE__);
    }

    pStringVal = NULL;
    (void)Common_Json_GetAttrValue(pFactoryInfoJson, -1, "/FactoryInfo/Hardware", NULL, &pStringVal, NULL, NULL);
	if(NULL != pstVersion->szHardware)
	{
		Common_Free(pstVersion->szHardware, __FUNCTION__, __LINE__);
		pstVersion->szHardware = NULL;
	}
    if (pStringVal != NULL)
    {
        pstVersion->szHardware = Common_StrDup(pStringVal, __FUNCTION__, __LINE__);
    }
/*
    S8 * uuid = update_common_GetUUID();
	if(NULL != pstVersion->szUUID)
	{
		Common_Free(pstVersion->szUUID, __FUNCTION__, __LINE__);
		pstVersion->szUUID = NULL;
	}
    pstVersion->szUUID = Common_StrDup(uuid, __FUNCTION__, __LINE__);
*/
	LOGI("szUUID:[%s]\n",pstVersion->szUUID);
/*
#ifdef PLATFORM_MS316
        S8 *szMethod = "UUID";
#else
        S8 *szMethod = "LEVEL0";
#endif
	S8 *szMethod  = pstVersion->szAuthMethod;

	if(NULL != pstVersion->szAuthMethod)
	{
		Common_Free(pstVersion->szAuthMethod, __FUNCTION__, __LINE__);
		pstVersion->szAuthMethod = NULL;
	}
    pstVersion->szAuthMethod = Common_StrDup(szMethod, __FUNCTION__, __LINE__);
*/
    pSerial = update_common_GetSerialNumber();
	if(NULL != pstVersion->szSerialNumber)
	{
		Common_Free(pstVersion->szSerialNumber, __FUNCTION__, __LINE__);
		pstVersion->szSerialNumber = NULL;
	}
    if (NULL == pSerial)
    {
        pStringVal = NULL;
        (void)Common_Json_GetAttrValue(pCoreConfig, -1, "SerialNumber", NULL, &pStringVal, NULL, NULL);
        if (NULL != pStringVal)
        {
            pstVersion->szSerialNumber = Common_StrDup(pStringVal, __FUNCTION__, __LINE__);
        }
        else
        {
            S8 sztmp[32] = {0};
            S8 *pszMac = NULL;
            S32 mac[6] = {0};

            Common_GetLocalNetInfo((char *)"eth0", 0, NULL, NULL, NULL, &pszMac);
            if (6 == sscanf(pszMac, "%02X:%02X:%02X:%02X:%02X:%02X", &mac[0], &mac[1],
                            &mac[2], &mac[3], &mac[4], &mac[5]) ||
                    6 == sscanf(pszMac, "%02x:%02x:%02x:%02x:%02x:%02x", &mac[0], &mac[1], &mac[2],
                                &mac[3], &mac[4], &mac[5]))
            {
                sprintf(sztmp, "00000000%02x%02x%02x%02x%02x%02x", mac[0], mac[1], mac[2],
                        mac[3], mac[4], mac[5]);
            }
            else
            {
                S32 nSec = 0, nMSec = 0;
                Common_GetCurrentTime(&nSec, &nMSec);
                sprintf(sztmp, "000000000000%08X", nSec);
            }
			LOGI("pszMac:[%s]\n",pszMac);
            if (NULL != pszMac)
            {
                Common_Free(pszMac, __FUNCTION__, __LINE__);
                pszMac = NULL;
            }

            pstVersion->szSerialNumber = Common_StrDup(sztmp, __FUNCTION__, __LINE__);
        }
    }
    else
    {
        pstVersion->szSerialNumber = Common_StrDup(pSerial, __FUNCTION__, __LINE__);
    }

	if(NULL != pstVersion->szStatus)
	{
		Common_Free(pstVersion->szStatus, __FUNCTION__, __LINE__);
		pstVersion->szStatus = NULL;
	}

    if (checkRight(pSerial, pstVersion->szHardware))
    {
        pstVersion->szStatus = Common_StrDup((char *)"Activated", __FUNCTION__, __LINE__);
        pstVersion->bActived = 1;
    }
    else
    {
        pstVersion->szStatus = Common_StrDup((char *)"Unactivated", __FUNCTION__, __LINE__);
        pstVersion->bActived = 0;
        LOGW("It's Unactivated.\n");
    }

    pStringVal = NULL;
    Common_Json_GetAttrValueStr(pSidJson, "Sid", &pStringVal);
	if(NULL != pstVersion->szSid)
	{
		Common_Free(pstVersion->szSid, __FUNCTION__, __LINE__);
		pstVersion->szSid = NULL;
	}
    if (pStringVal != NULL)
    {
        pstVersion->szSid = Common_StrDup(pStringVal, __FUNCTION__, __LINE__);
    }
    else
    {
        pstVersion->szSid = Common_StrDup((char *)"", __FUNCTION__, __LINE__);
    }

    Common_Json_Delete(pMediaServerJson);
    Common_Json_Delete(pWebserverJson);
    Common_Json_Delete(pOnvifJson);
    Common_Json_Delete(pAliIotJson);

    Common_Json_Delete(pCoreConfig);
    Common_Json_Delete(pDefaultCoreConfig);
    Common_Json_Delete(pFactoryInfoJson);
    return 0;
}

S32 updateLoadVersionFromCore(Update_Version_S *pstVersion)
{
	S8 *szStringVal = NULL;
	S32 nIntValue = 0,nCode = 0;
	cJSON_Struct *pInParam = NULL,*pOutParam = NULL;

#if 0
    static S32 nLastTimeCount = 0,nTimeCount = 0;
	Common_GetSystemCount(&nTimeCount,NULL);
	if (nTimeCount < nLastTimeCount + 10)
	{
		return 0;
	}
	nLastTimeCount = nTimeCount;
#endif

	pInParam = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
	Common_Json_SetAttrValue(pInParam,-1,"Header",Common_Json_Type_Object,NULL,0,0);
	Common_Json_SetAttrValue(pInParam,-1,"Header/Uri",Common_Json_Type_String,"/Core/Version",0,0);
	Common_Json_SetAttrValue(pInParam,-1,"Header/Method",Common_Json_Type_String,"get",0,0);
	Update_Tcp_Require("127.0.0.1",10009,pInParam,&pOutParam,3000);
	if (pOutParam != NULL)
	{
		nCode = -1;
		Common_Json_GetAttrValue(pOutParam,-1,"/Header/Code",NULL,NULL,&nCode,NULL);
		if (nCode == 0 || nCode == 200)
		{
			cJSON_Struct *pDataJson;
			pDataJson = Common_Json_GetItem(pOutParam,-1,"/Data");
			if (NULL != pDataJson)
			{
				szStringVal = NULL;
				Common_Json_GetAttrValue(pDataJson,-1,"DeviceName",NULL,&szStringVal,NULL,NULL);
				if (szStringVal != NULL)
				{
					if (pstVersion->szDeviceName != NULL)
					{
						if(0 != Common_StrCmp(pstVersion->szDeviceName, szStringVal))
						{
							Common_Free(pstVersion->szDeviceName, __FUNCTION__, __LINE__);
							pstVersion->szDeviceName = NULL;
						}
					}

                    if(NULL == pstVersion->szDeviceName)
					{
						pstVersion->szDeviceName = Common_StrDup(szStringVal,__FUNCTION__,__LINE__);
					}
				}

                szStringVal = NULL;
				Common_Json_GetAttrValue(pDataJson,-1,"SerialNumber",NULL,&szStringVal,NULL,NULL);
				if (szStringVal != NULL)
				{
					if (pstVersion->szSerialNumber != NULL)
					{
						if(0 != Common_StrCmp(pstVersion->szSerialNumber,szStringVal))
						{
							Common_Free(pstVersion->szSerialNumber,__FUNCTION__,__LINE__);
							pstVersion->szSerialNumber = NULL;
						}
					}

                    if(NULL == pstVersion->szSerialNumber)
					{
						pstVersion->szSerialNumber = Common_StrDup(szStringVal,__FUNCTION__,__LINE__);
					}
				}

                szStringVal = NULL;
				Common_Json_GetAttrValue(pDataJson,-1,"Sid",NULL,&szStringVal,NULL,NULL);
				if (szStringVal != NULL)
				{
					if (pstVersion->szSid != NULL)
					{
						if(0 != Common_StrCmp(pstVersion->szSid,szStringVal))
						{
							Common_Free(pstVersion->szSid,__FUNCTION__,__LINE__);
							pstVersion->szSid = NULL;
						}
					}

                    if(NULL == pstVersion->szSid)
					{
						pstVersion->szSid = Common_StrDup(szStringVal,__FUNCTION__,__LINE__);
					}
				}

				szStringVal = NULL;
				Common_Json_GetAttrValue(pDataJson,-1,"Hardware",NULL,&szStringVal,NULL,NULL);
				if (szStringVal != NULL && pstVersion->szHardware == NULL)
				{
					pstVersion->szHardware = Common_StrDup(szStringVal,__FUNCTION__,__LINE__);
				}

                szStringVal = NULL;
				Common_Json_GetAttrValue(pDataJson,-1,"UUID",NULL,&szStringVal,NULL,NULL);
				if (szStringVal != NULL)
				{
				    if (pstVersion->szUUID != NULL)
			        {
			            LOGD("The old UUID: %s.\n", pstVersion->szUUID);
			            Common_Free(pstVersion->szUUID, __FUNCTION__, __LINE__);
                        pstVersion->szUUID = NULL;
			        }

					pstVersion->szUUID = Common_StrDup(szStringVal, __FUNCTION__, __LINE__);
				}
                LOGD("Get UUID: %s\n", szStringVal, pstVersion->szUUID);

                szStringVal = NULL;
				Common_Json_GetAttrValue(pDataJson,-1,"AuthMethod",NULL,&szStringVal,NULL,NULL);
				if (szStringVal != NULL && pstVersion->szAuthMethod == NULL)
				{
					pstVersion->szAuthMethod = Common_StrDup(szStringVal,__FUNCTION__,__LINE__);
				}

				szStringVal = NULL;
				Common_Json_GetAttrValue(pDataJson,-1,"ProductName",NULL,&szStringVal,NULL,NULL);
				if (szStringVal != NULL && pstVersion->szProductName == NULL)
				{
					pstVersion->szProductName = Common_StrDup(szStringVal,__FUNCTION__,__LINE__);
				}
				szStringVal = NULL;
				Common_Json_GetAttrValue(pDataJson,-1,"DeviceType",NULL,&szStringVal,NULL,NULL);
				if (szStringVal != NULL && pstVersion->szDeviceType == NULL)
				{
					pstVersion->szDeviceType = Common_StrDup(szStringVal,__FUNCTION__,__LINE__);
				}
				szStringVal = NULL;
				Common_Json_GetAttrValue(pDataJson,-1,"DeviceModel",NULL,&szStringVal,NULL,NULL);
				if (szStringVal != NULL && pstVersion->szDeviceModel == NULL)
				{
					pstVersion->szDeviceModel = Common_StrDup(szStringVal,__FUNCTION__,__LINE__);
				}

                szStringVal = NULL;
				Common_Json_GetAttrValue(pDataJson,-1,"Country",NULL,&szStringVal,NULL,NULL);
				if (szStringVal != NULL && pstVersion->szCountry == NULL)
				{
					pstVersion->szCountry = Common_StrDup(szStringVal,__FUNCTION__,__LINE__);
				}

                szStringVal = NULL;
				Common_Json_GetAttrValue(pDataJson,-1,"City",NULL,&szStringVal,NULL,NULL);
				if (szStringVal != NULL && NULL == pstVersion->szCity)
				{
					pstVersion->szCity = Common_StrDup(szStringVal,__FUNCTION__,__LINE__);
				}

                szStringVal = NULL;
				Common_Json_GetAttrValue(pDataJson,-1,"Web",NULL,&szStringVal,NULL,NULL);
				if (szStringVal != NULL && NULL == pstVersion->szWeb)
				{
					pstVersion->szWeb = Common_StrDup(szStringVal,__FUNCTION__,__LINE__);
				}

                szStringVal = NULL;
				Common_Json_GetAttrValue(pDataJson,-1,"Tel",NULL,&szStringVal,NULL,NULL);
				if (szStringVal != NULL && NULL == pstVersion->szTel)
				{
					pstVersion->szTel = Common_StrDup(szStringVal,__FUNCTION__,__LINE__);
				}

                szStringVal = NULL;
				Common_Json_GetAttrValue(pDataJson,-1,"Copyright",NULL,&szStringVal,NULL,NULL);
				if (szStringVal != NULL && NULL == pstVersion->szCopyRight)
				{
					pstVersion->szCopyRight = Common_StrDup(szStringVal,__FUNCTION__,__LINE__);
				}

                szStringVal = NULL;
				Common_Json_GetAttrValue(pDataJson,-1,"Manufacturer",NULL,&szStringVal,NULL,NULL);
				if (szStringVal != NULL && NULL == pstVersion->szManufacturer)
				{
					pstVersion->szManufacturer = Common_StrDup(szStringVal,__FUNCTION__,__LINE__);
				}

                szStringVal = NULL;
				Common_Json_GetAttrValue(pDataJson,-1,"Brand",NULL,&szStringVal,NULL,NULL);
				if (szStringVal != NULL && NULL == pstVersion->szBrand)
				{
					pstVersion->szBrand = Common_StrDup(szStringVal,__FUNCTION__,__LINE__);
				}

                szStringVal = NULL;
				Common_Json_GetAttrValue(pDataJson,-1,"Customer",NULL,&szStringVal,NULL,NULL);
				if (szStringVal != NULL && NULL == pstVersion->szCustomer)
				{
					pstVersion->szCustomer = Common_StrDup(szStringVal,__FUNCTION__,__LINE__);
				}

                szStringVal = NULL;
				Common_Json_GetAttrValue(pDataJson,-1,"SensorModel",NULL,&szStringVal,NULL,NULL);
				if (szStringVal != NULL && NULL == pstVersion->szSensorModel)
				{
					pstVersion->szSensorModel = Common_StrDup(szStringVal,__FUNCTION__,__LINE__);
				}

				nIntValue = 0;
				Common_Json_GetAttrValue(pDataJson,-1,"IsOfDome",NULL,NULL,&nIntValue,NULL);
				pstVersion->IsOfDome = nIntValue;

				nIntValue = 0;
				Common_Json_GetAttrValue(pDataJson,-1,"IsOfIr",NULL,NULL,&nIntValue,NULL);
				pstVersion->IsOfIr = nIntValue;


				nIntValue = 0;
				Common_Json_GetAttrValue(pDataJson,-1,"LensSupport",NULL,NULL,&nIntValue,NULL);

				pstVersion->nLensSupport = nIntValue;

				szStringVal = NULL;
				Common_Json_GetAttrValue(pDataJson,-1,"LensDrvType",NULL,&szStringVal,NULL,NULL);
				if (szStringVal != NULL && pstVersion->szLensDrvType == NULL)
				{
					pstVersion->szLensDrvType = Common_StrDup(szStringVal,__FUNCTION__,__LINE__);
				}

                szStringVal = NULL;
				Common_Json_GetAttrValue(pDataJson,-1,"LensType",NULL,&szStringVal,NULL,NULL);
				if (szStringVal != NULL && pstVersion->szLensType == NULL)
				{
					pstVersion->szLensType = Common_StrDup(szStringVal,__FUNCTION__,__LINE__);
				}

                nIntValue = 0;
				Common_Json_GetAttrValue(pDataJson,-1,"IrisSupport",NULL,NULL,&nIntValue,NULL);
				pstVersion->nIrisSupport = nIntValue;

                szStringVal = NULL;
				Common_Json_GetAttrValue(pDataJson,-1,"IrisType",NULL,&szStringVal,NULL,NULL);
				if (szStringVal != NULL && NULL == pstVersion->szIrisType)
				{
					pstVersion->szIrisType = Common_StrDup(szStringVal,__FUNCTION__,__LINE__);
				}

				szStringVal = NULL;
				Common_Json_GetAttrValue(pDataJson,-1,"Version",NULL,&szStringVal,NULL,NULL);
				if (szStringVal != NULL && NULL == pstVersion->szVersion)
				{
					pstVersion->szVersion = Common_StrDup(szStringVal,__FUNCTION__,__LINE__);
				}

				nIntValue = 0;
				Common_Json_GetAttrValue(pDataJson,-1,"SvnNumber",NULL,NULL,&nIntValue,NULL);
				pstVersion->nSvnNumber = nIntValue;
				if(NULL == pstVersion->szHardVersion)
				{
					szStringVal = NULL;
					Common_Json_GetAttrValue(pDataJson,-1,"HardVersion",NULL,&szStringVal,NULL,NULL);
					if (szStringVal != NULL)
					{
						pstVersion->szHardVersion = Common_StrDup(szStringVal,__FUNCTION__,__LINE__);
					}
				}

                szStringVal = NULL;
				if(NULL == pstVersion->szBuildDate)
				{
					Common_Json_GetAttrValue(pDataJson,-1,"BuildDate",NULL,&szStringVal,NULL,NULL);
					if (szStringVal != NULL)
					{
						pstVersion->szBuildDate = Common_StrDup(szStringVal,__FUNCTION__,__LINE__);
					}
				}

                szStringVal = NULL;
				if(pstVersion->szProductDate == NULL)
				{
					Common_Json_GetAttrValue(pDataJson,-1,"ProductDate",NULL,&szStringVal,NULL,NULL);
					if (szStringVal != NULL)
					{
						pstVersion->szProductDate = Common_StrDup(szStringVal,__FUNCTION__,__LINE__);
					}
				}

                szStringVal = NULL;
				Common_Json_GetAttrValue(pDataJson,-1,"Status",NULL,&szStringVal,NULL,NULL);
				if (szStringVal != NULL)
				{
					if(pstVersion->szStatus != NULL)
					{
						if(0 != Common_StrCmp(szStringVal, pstVersion->szStatus))
						{
							Common_Free(pstVersion->szStatus,__FUNCTION__,__LINE__);
							pstVersion->szStatus = NULL;
						}
					}

                    if(NULL == pstVersion->szStatus)
					{
						pstVersion->szStatus = Common_StrDup(szStringVal,__FUNCTION__,__LINE__);
					}
				}
			}
		}
	}

    Common_Json_Delete(pInParam);
	Common_Json_Delete(pOutParam);
	return 0;
}

S32 update_version_Init(DeviceVersion_S *pstDeviceVersion)
{
    S32 ret = -1;

    if (NULL == pstDeviceVersion)
    {
        return -1;
    }

    //支持接口重入
    if (1 == pstDeviceVersion->lVersionOk)
    {
        return 0;
    }

    memset((void *)pstDeviceVersion, 0x00, sizeof(DeviceVersion_S));
    ret = Common_Lock_Create(&pstDeviceVersion->phVersionLock, "Update_Version_Lock");
    if (0 != ret)
    {
        LOGE("Create update_version lock error.\n");
        return ret;
    }

    (void)Common_Lock(pstDeviceVersion->phVersionLock);
    ret = updateLoadVer(&pstDeviceVersion->stUpdateVersion);
    (void)Common_UnLock(pstDeviceVersion->phVersionLock);

    if (0 != ret)
    {
        LOGE("update load verions fail.\n");
        Common_Lock_Destroy(&pstDeviceVersion->phVersionLock);
        pstDeviceVersion->phVersionLock = NULL;
        return ret;
    }

    pstDeviceVersion->lVersionOk = 1;
    return 0;
}

S32 update_version_LoadVer(DeviceVersion_S *pstVersion, S32 lforce, S32 lSource)
{
    S32 ret = -1;

    if (NULL == pstVersion)
    {
        return -1;
    }

    if ((1 == pstVersion->lVersionOk) && (0 == lforce))
    {
        return 0;
    }

    if (NULL == pstVersion->phVersionLock)
    {
        (void)Common_Lock_Create(&pstVersion->phVersionLock, "Update_version_lock");
    }

    (void)Common_Lock(pstVersion->phVersionLock);

    if (LOADVER_FROM_CORE == lSource)
    {
        //LOGD("updateLoadVersionFromCore.\n");
        ret = updateLoadVersionFromCore(&pstVersion->stUpdateVersion);
    }
    else //(LOADVER_FROM_MTD == lSource)
    {
        //LOGD("updateLoadVer from mtd devices.\n");
        ret = updateLoadVer(&pstVersion->stUpdateVersion);
    }

    (void)Common_UnLock(pstVersion->phVersionLock);
    if (0 != ret)
    {
        LOGE("update load verions fail.\n");
        return ret;
    }

    pstVersion->lVersionOk = 1;
    return 0;
}

S8 *update_version_GetSNByDevVer(DeviceVersion_S *pstVersion)
{
    S8* szSerialNumber = NULL;

    if (NULL == pstVersion)
    {
        return NULL;
    }

    if (1 != pstVersion->lVersionOk)
    {
        //主动加载设备版本号.
        (void)update_version_LoadVer(pstVersion, 0, LOADVER_FROM_MTD);
    }

    if (1 != pstVersion->lVersionOk)
    {
        LOGE("The device version NOT READY.\n");
        return NULL;
    }

    (void)Common_Lock(pstVersion->phVersionLock);
	if ((pstVersion->stUpdateVersion.szSerialNumber != NULL) &&
		(pstVersion->stUpdateVersion.szStatus != NULL) &&
		(0 == Common_StrCmp("Activated", pstVersion->stUpdateVersion.szStatus)))
	{
		szSerialNumber = Common_StrDup(pstVersion->stUpdateVersion.szSerialNumber, __FUNCTION__, __LINE__);
	}
	(void)Common_UnLock(pstVersion->phVersionLock);

    return szSerialNumber;
}

S32 udpate_version_GetSN(S8 *pcSerialNum, S32 lLen)
{
    DeviceVersion_S* pstDevVerHandle = update_version_GetHandle();

    if (1 != pstDevVerHandle->lVersionOk)
    {
        LOGE("The device version NOT READY.\n");
        return -1;
    }

    if (NULL == pcSerialNum)
    {
        LOGE("pcSerialNum is NULL.\n");
        return -1;
    }

    if (NULL == pstDevVerHandle->stUpdateVersion.szSerialNumber)
    {
        LOGE("The IPC sn is NULL.\n");
        return -1;
    }

    if ((lLen <= strlen(pstDevVerHandle->stUpdateVersion.szSerialNumber)) || (lLen <= 0))
    {
        LOGE("Enter SN buffer len(%d) <= %d.\n", lLen, strlen(pstDevVerHandle->stUpdateVersion.szSerialNumber));
        return -1;
    }

    Common_Lock(pstDevVerHandle->phVersionLock);
    snprintf(pcSerialNum, lLen - 1, "%s", pstDevVerHandle->stUpdateVersion.szSerialNumber);
    Common_UnLock(pstDevVerHandle->phVersionLock);
    return 0;
}

S32 update_version_GetUUID(S8 *pcUUID, S32 lLen)
{
    DeviceVersion_S* pstDevVerHandle = update_version_GetHandle();

    if (1 != pstDevVerHandle->lVersionOk)
    {
        LOGE("The device version NOT READY.\n");
        return -1;
    }

    if (NULL == pcUUID)
    {
        LOGE("pcUUID is NULL.\n");
        return -1;
    }

    if (NULL == pstDevVerHandle->stUpdateVersion.szUUID)
    {
        LOGE("The UUID is NULL.\n");
        return -1;
    }

    if ((lLen <= strlen(pstDevVerHandle->stUpdateVersion.szUUID)) || (lLen <= 0))
    {
        LOGE("Enter uuid buffer len(%d) <= %d.\n", lLen, strlen(pstDevVerHandle->stUpdateVersion.szUUID));
        return -1;
    }

    Common_Lock(pstDevVerHandle->phVersionLock);
    snprintf(pcUUID, lLen - 1, "%s", pstDevVerHandle->stUpdateVersion.szUUID);
    Common_UnLock(pstDevVerHandle->phVersionLock);
    return 0;
}

S32 update_version_GetMac(S8 *pcMac, S32 lLen)
{
    int j = 0;
    DeviceVersion_S* pstDevVerHandle = update_version_GetHandle();

    if (1 != pstDevVerHandle->lVersionOk)
    {
        LOGE("The device version NOT READY.\n");
        return -1;
    }

    if (NULL == pcMac)
    {
        LOGE("pcMac is NULL.\n");
        return -1;
    }

    if (NULL == pstDevVerHandle->stUpdateVersion.szMac)
    {
        LOGE("The IPC mac is NULL.\n");
        return -1;
    }

    if ((lLen <= strlen(pstDevVerHandle->stUpdateVersion.szMac)) || (lLen <= 0))
    {
        LOGE("Enter MAC buffer len(%d) <= %d.\n", lLen, strlen(pstDevVerHandle->stUpdateVersion.szMac));
        return -1;
    }

    Common_Lock(pstDevVerHandle->phVersionLock);
    int i = 0;
    for (i = 0; i < strlen(pstDevVerHandle->stUpdateVersion.szMac); i++)
	{
		if (pstDevVerHandle->stUpdateVersion.szMac[i] != ':')
		{
			pcMac[j] = pstDevVerHandle->stUpdateVersion.szMac[i];
			j++;
		}
	}
    //snprintf(pcMac, lLen - 1, "%s", pstDevVerHandle->stUpdateVersion.szMac);
    Common_UnLock(pstDevVerHandle->phVersionLock);
    return 0;
}



