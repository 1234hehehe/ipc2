#include <stdlib.h>
#include <string.h>
#include "core_version.h"
#include "cjson.h"
#include "lib_encry.h"
#include "build_date.h"
#include "build_svn.h"
#include "common_json_str_ops.h"

#ifdef WIN32
#define CORE_FACTORY_PATH "C:\\tmp\\factoryinfo.json"
#else
#define CORE_FACTORY_PATH "/tmp/factoryinfo.json"
#endif
/*
Uri:/Core/Version
method:Get
Data:{
    DeviceName:"",
    DeviceType:"IPC",  // "IPC","NVR","DVR"...
    DeviceModel:"",
    Version:xxx,
    HardVersion:xxx
    BuildDate:xxx,
    SerialNumber:
}
Uri:/Core/DeviceName
method:Get/Set
Data:{
    DeviceName:"IPC",
}
*/

static S32 g_bInit = 0;
Core_Version_T g_tVersion;
cJSON_Struct *g_pFactoryInfoJson = NULL;
static Common_Lock_T g_hVersionLock = NULL;

struct _tagSerial_Hardware_Map
{
    S8 szSerial[5];
    S8 szHardware[32];
};

struct _tagSerial_Hardware_Map g_SerialHardwareMap[] =
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
#endif
    {"0b6b", "H16D_D81_Q38"},
    {"0b6b", "C116D_D1801_0B6B"},
    {"0b6b", "C116D_D1801_03xx"},
    {"0b6b", "HORIZON_HI3516D_X1"},
    {"0b6b", "HI3516D_AI_BOARD"},
    {"0b81", "H16D_D71_R56"},
    {"0b87", "H16CV3_D71_R56"},
    {"0b88", "ZKTECO_MSTAR_316DC"},
    {"0b8c", "H16E_D81_R56"},
//    {"0bd0", "H19V1_D81_W386"},
    {"0bd3", "HI3516AV200_AI_BOARD"},
    {"0bd3", "H16AV2_D81_W386"},
    {"0bd3", "H16AV2_D91_R56"},
    {"0b88", "DS_MSTAR_316DC"},
    {"0b8a", "DS_MSTAR_313E"},
    {"0b8d", "DS_MSTAR_316DM"},

    {"0bf0", "JZT30X_D81_W386"},
    {"0bf1", "JZT30N_D81_W386"},
    {"0bf1", "JZT30L_D81_W386"},

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
    {"0b6c", "H16EV2_D92_Q38_MINI"}, /*zk支持zkdc*/
    {"0bdb", "H16EV2_D92_Q38"},
    {"06bb", "H16EV2_D92_Q38"}, /*定焦，支持P2P*/
    {"06bd", "H16EV2_D92_Q38"}, /*变焦，支持P2P*/
    {"0bdc", "H16EV2_D92_Q38_PERSON"},
    {"06bc", "H16EV2_D92_Q38_PERSON"},/*定焦，支持P2P*/
    {"0b6d", "H16EV2_D92_Q38_PERSON"},/*变焦*/
    {"0bdd", "H16EV3_D93_Q38"},
    {"0bde", "H16EV3_D93_Q38_NF"},  /* 基于H16EV3_D93_Q38上替换为128M NandFlash */
    {"0bdf", "H16EV2_D92_Q38_EXT"}, /* 基于H16EV2_D92_Q38配合扩展板实现Wifi/SD等功能(裁剪部分智能功能). */
    /* 03** 恩智的版本 */
    {"03ba", "H16EV2_D96_Q38_MINI"},
    {"03bf", "H16EV2_D96_Q38_MINIA"},
    {"03b8", "H16EV2_D92_Q38_FACEC"}, //lens
    {"03bb", "H16EV2_D92_Q38_FACEC"},
    {"03b9", "H16EV2_D92_Q38_PERSONC"}, //lens
    {"03bc", "H16EV2_D92_Q38_PERSONC"},
    {"03bd", "H16EV2_D96_Q38_PERSONA"},
    {"03be", "H16EV2_D96_Q38_STD"},
    {"03b7", "H16EV2_D92_Q38_P2P"},
	{"033d", "H16EV2_D96_Q38_PERSONAN"},
	{"033e", "H16EV2_D96_Q38_P2P"},

	{"0370", "JZT40XP_D11_Q44"},
	{"0370", "JZT40XP_D11_Q38"},
	{"0350", "JZT40N_D13_Q38_8MP"},
	{"0351", "JZT40N_D13_Q38_5MP"},
	{"0343", "JZT40N_D13_Q38_5MP_HSDOME"},
	{"0346", "JZT40N_D13_Q38_5MP_HSDOME"},
    {"0380", "JZT40XP_D21_Q38"},
    {"0390", "JZT41L_D21_Q38"},

	{"037a", "JZT31N_D96_Q38_MINI"},	//t31n_d02_mini
	{"038a", "JZT31N_D96_Q38_MINI"},
	{"035a", "JZT31N_D11_Q38_MINI4MP"},	//t31n_d11_mini_4mp
	{"035b", "JZT31N_D11_Q38_MINI4MP_P2P"},	//t31n_d11_mini_4mp_p2p
	{"037b", "JZT31L_D20_Q38_MINI"},	//t31l_d01_mini
	{"037c", "JZT31N_D21_Q38_MINI"},	//t31n_d01_mini_3MP
	{"037d", "JZT31N_D02_Q38_P2P"},		//t31n_d02_p2p
	{"037e", "JZT31N_D01_Q38_P2P"},	//t31n_d01_p2p
	{"0377", "JZT31N_D96_Q38_P2P"},		//t31n_d01_p2p
	{"0347", "JZT31N_D96_Q38_P2P"},
	{"8377", "JZT31N_D96_Q38_P2P"},
    {"8347", "JZT31N_D96_Q38_P2P"},
    {"735a", "JZT31N_D11_Q38_MINI4MP"},

	{"034b", "JZT31N_D41_Q38"},

	{"0378", "JZT31N_D11_X40_WIFIDOME"},//wifi dome
	{"0359", "JZT31N_D11_X40_WIFIDOME_16M"},//wifi dome person
	{"0358", "JZT31N_D11_X40"},//t31n_gb28181_dome
	{"0357", "JZT31N_D11_X40_PERSON"},//t31n_person_dome
	{"034a", "JZT31N_D11_X40_PERSON_V2"},//t31n_person_dome
	{"0379", "JZT31N_D96_Q38_PERSON"},	//t31n_d01_person
	{"0373", "JZT31X_D95_Q38_PERSON"},	//t31x_d01_q38_person
	{"0374", "JZT31X_D95_Q38_P2P"},		//t31x_d01_q38_p2p
	{"0376", "JZT31X_D95_Q38_H5"},		//t31x_d01_q38_h5
	{"0375", "JZT31X_D01_Q38_P2P_HSDOME"},		//t31x_d01_q38_p2p_hs
	{"0345", "JZT31X_D01_Q38_P2P_HSDOME"},		//t31x_d01_q38_p2p_hs
	{"037f", "JZT31N_D02_Q38_P2P_HSDOME"},
	{"034f", "JZT31N_D02_Q38_P2P_HSDOME"},
    {"0372", "JZT31X_D01_Q38_P2P_HSWIFIDOME"},
    {"035f", "JZT31N_D02_Q38_P2P_HSWIFIDOME"},

	{"0353", "JZT31X_D11_Q38_PERSON"},	//t31x_d11_q38_person
	{"8286", "JZT31X_D11_Q38_PERSON"},
    {"0354", "JZT31X_D11_Q38_P2P"},     //t31x_d11_q38_p2p
	{"0355", "JZT31X_D12_Q38_PERSON"},	//t31x_d12_q38_person
	{"0356", "JZT31X_D12_Q38_P2P"}, 	//t31x_d12_q38_p2p
    {"034c", "JZT31X_D13_Q38_P2P_HSDOME"},
	{"034d", "JZT31X_D13_Q38_P2P_HSDOME"},

	{"0344", "JZT31X_D95_Q38_P2P"},		//t31x_d01_q38_p2p
	{"0342", "JZT31X_D11_Q38_P2P"}, 	//t31x_d11_q38_p2p
    {"0340", "JZT40N_D13_Q38"},
	{"8340", "JZT40N_D13_Q38"},
	{"0385", "JZT40N_D13_Q38_HSDOME"},
	{"0386", "JZT40N_D13_Q38_HSDOME"},
    {"0341", "JZT40N_D13_Q38_4KP2P"},
	{"0381", "JZT40N_D13_Q38_4KP2P_HSDOME"},
	{"0382", "JZT40N_D13_Q38_4KP2P_HSDOME"},

	{"038c", "JZT31N_D31_Q38"},

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

    {"030a", "JZT31N_D43_Q38"},
    {"030c", "JZT31N_D42_Q38"},

    {"07da", "H16EV2_D94_Q38_MEIAN_MINI"}, //sd
    {"07db", "H16EV2_D94_Q38_MEIANFACE"}, //sd face
    {"07dc", "H16EV2_D94_Q38_MEIAN"}, //wifi sd person
    {"07dd", "H16EV2_D94_Q38_MEIANFACE"}, //wifi sd face

	//fsan
	{"8279", "JZT31N_D11_W386"},	//t31n_d11_w386
	{"8273", "JZT31X_D11_W386"},	//t31x_d11_w386

    {"0bd0", "H16EV3_D95_Q38_FACE"}, // face  zkto
    {"0bd1", "H16EV3_D95_Q38_PERSON"}, //person zkto
    {"0bb0", "H16EV3_D95_Q38_FACE4MP"}, // face  zkto
    {"0bb1", "H16EV3_D95_Q38_PERSON4MP"}, //person zkto

	//xd
	{"0d87", "H16EV2_D92_Q38_P2P_XD"}, //ev200 autop2p xiaoding
	{"0ddb", "H16CV5_D01_Q38S_PERSON_XD"}, //cv500 person xiaoding

	//hdt
	{"03b0", "H16EV3_D95_Q38_FACE"}, // face
    {"03b1", "H16EV3_D95_Q38_PERSON"}, //person
    {"03b2", "H16EV3_D95_Q38_FACE"}, // face lens
    {"03b3", "H16EV3_D95_Q38_PERSON"}, //person lens

	{"0333", "H16EV3_D95_Q38_P2P"}, //person lens

	//ds
	{"05B0", "H16EV3_D95_Q38_FACE"}, // face
	{"05b1", "H16EV3_D95_Q38_PERSON"}, //person
	{"05b2", "H16EV3_D95_Q38_FACE"}, // face lens
	{"05b3", "H16EV3_D95_Q38_PERSON"}, //person lens
	{"05b4", "H16EV3_D95_Q38_PERSON"}, //mojing

//    {"05de", "H16CV5_D01_Q38"}, //mojing
	{"05b5", "H16EV2_D92_Q38_PERSON"}, //mojing

	{"0506", "H16CV5_D01_Q38_PERSON"},
	{"05db", "H16CV5_D01_Q38_FACEC"},
	{"05de", "H16CV5_D01_Q38_FACER"},

	{"0505", "H16CV5_D01_Q38S_PERSON"},
	{"0507", "H16CV5_D01_Q38S_MOTOR"},
	{"05dc", "H16CV5_D01_Q38S_FACEC"},
	{"05dd", "H16CV5_D01_Q38S_FACER"},

	//hdt
    {"0306", "H16CV5_D01_Q38_PERSON"},
    {"03db", "H16CV5_D01_Q38_FACEC"},
    {"03de", "H16CV5_D01_Q38_FACER"},

    {"0305", "H16CV5_D01_Q38S_PERSON"},
    {"0307", "H16CV5_D01_Q38S_MOTOR"},
    {"03dc", "H16CV5_D01_Q38S_FACEC"},
    {"03dd", "H16CV5_D01_Q38S_FACER"},

	{"03d8", "H16DV3_D01_Q38_FACEC"},
	{"03d7", "H16DV3_D01_Q38_FACER"},

	{"0309", "H16AV3_D01_Q38"},

    {"0220", "H16EV2_D97_W386"},
    {"0225", "H16EV3_D98_W386"},
    {"0bd1", "H16EV3_D98_W386"},

	{"0b60", "H16CV5_D91_R56_IVS30"},

	{"06be", "H16EV2_D92_Q38_TUSHI"}, //person lens

	{"0bb2", "H16AV3_D04_Q38_FR"},

    {"0509", "H16DV3_D01_Q38_PERSON"},
    {"05d0", "H16DV3_D01_Q38_FACEC"},
    {"05d1", "H16DV3_D01_Q38_FACER"},

	{"06d8", "H16EV3_D11_W3711_XZC"},

	{"0607", "JZT40XP_D12_W386_MOTOR"},

    {"7311", "JZT41L_D31_Q38_ALARM"},
    {"7322", "JZT32L_D51_Q38"},

};

/*
static void Version_FactoryInfoSave()
{
    FILE *pf = NULL;
    if (g_pFactoryInfoJson == NULL)
    {
        return;
    }
    pf = fopen(CORE_FACTORY_PATH,"wb+");
    if (pf != NULL)
    {
        S8 *pString = NULL;
        S32 nStrLen = 0;
        pString = Common_Json_Print(g_pFactoryInfoJson,&nStrLen);
        if (pString != NULL)
        {

            if (nStrLen > 0)
            {
                fwrite(pString,1,nStrLen,pf);
            }

            Common_Free(pString,__FUNCTION__,__LINE__);
            pString = NULL;
        }
        fclose(pf);
        pf = NULL;
    }
}

*/

S32 Version_IsActived()
{
    return g_tVersion.bActived;
}


S32 Version_checkRight(S8 *pSerial, S8 *szHardware)
{
    S32 bActivated = 0;
    S32 nNum = 0, i;
    if (pSerial == NULL || szHardware == NULL)
    {
        return bActivated;
    }
    nNum = sizeof(g_SerialHardwareMap) / sizeof(g_SerialHardwareMap[0]);
    for (i = 0; i < nNum; i++)
    {
        if(Common_StrniCmp(pSerial, g_SerialHardwareMap[i].szSerial, 4) == 0 &&
                0 == Common_StriCmp(szHardware, g_SerialHardwareMap[i].szHardware))
        {
            bActivated = 1;
            break;
        }
    }

    return bActivated;
}
static int static_checkEnvValue(FILE *fp, char *szBuffer, char *szCheckItem,
                                char *szCheckValue)
{
    char *szItem = NULL, *szItemValue = NULL, cSet;
    S32 bSet = 0, nCheckItemLen = 0;
    char szCheckItemQ[64 + 4];
    bSet = 1;
    nCheckItemLen = strlen(szCheckItem);
    if (nCheckItemLen > 63)
    {
        return 0;
    }
    strcpy(szCheckItemQ, szCheckItem);
    szCheckItemQ[nCheckItemLen] = '=';
    szCheckItemQ[nCheckItemLen + 1] = 0;
    szItem = strstr(szBuffer, szCheckItemQ);
    if (szItem != NULL)
    {
        szItemValue = szItem + nCheckItemLen + 1;
        if (szCheckValue == NULL || szCheckValue[0] == 0)
        {
            cSet = szItemValue[0];
            if (cSet == 0 || cSet == '\r' || cSet == '\n')
            {
                bSet = 0;
            }
        }
        else if(0 == Common_StrnCmp(szItemValue, szCheckValue, strlen(szCheckValue)))
        {
            cSet = szItemValue[strlen(szCheckValue)];
            if (cSet == 0 || cSet == '\r' || cSet == '\n')
            {
                bSet = 0;
            }
        }
    }
    else if (szCheckValue == NULL || szCheckValue[0] == 0)
    {
        bSet = 0;
    }
    if (bSet)
    {
        fprintf(fp, "%s %s\n", szCheckItem, szCheckValue);
    }
    return bSet;

}
void Version_WriteInfoToUbootEnv()
{
    char *szBuffer = NULL;
    char *szItem = NULL;
    //char *szItemValue = NULL,*szCheckItem = NULL,*szCheckValue = NULL,cSet;
    FILE *pFile = NULL;
    S32 bSet = 0;
    szBuffer = (char *)Common_Malloc(4 * 1024, 0, __FUNCTION__, __LINE__);
    if (szBuffer == NULL)
    {
        return;
    }
    pFile = fopen("/tmp/uboot_env.config", "wb+");
    if (pFile == NULL)
    {
        Common_Free(szBuffer, __FUNCTION__, __LINE__);
        szBuffer = NULL;
        return;
    }
    memset(szBuffer, 0, 4 * 1024);

    if (0 == Common_Exe_Cmd("/root/bin/fw_printenv", 5 * 1000, szBuffer,
                            4 * 1024 - 1))
    {
        szItem = strstr(szBuffer, "envstatus=saved");
        if (szItem != NULL)
        {
            S8 *pszMac = NULL, *pIpAddress = NULL, *pNetmask = NULL, *pGateway = NULL;
            //S32 mac[6];
            //S8 sztmp[32];
            //S8 *pSerial = NULL;


            Common_GetLocalNetInfo((char *)"eth0", 0, &pIpAddress, &pNetmask, &pGateway,
                                   &pszMac);
            bSet |= static_checkEnvValue(pFile, szBuffer, (char *)"ipaddr", pIpAddress);
            bSet |= static_checkEnvValue(pFile, szBuffer, (char *)"netmask", pNetmask);
            bSet |= static_checkEnvValue(pFile, szBuffer, (char *)"gatewayip", pGateway);
            bSet |= static_checkEnvValue(pFile, szBuffer, (char *)"ethaddr", pszMac);
            bSet |= static_checkEnvValue(pFile, szBuffer, (char *)"devicename",
                                         g_tVersion.szDeviceName);
            bSet |= static_checkEnvValue(pFile, szBuffer, (char *)"productname",
                                         g_tVersion.szProductName);
            bSet |= static_checkEnvValue(pFile, szBuffer, (char *)"devicetype",
                                         g_tVersion.szDeviceType);
            bSet |= static_checkEnvValue(pFile, szBuffer, (char *)"devicemodel",
                                         g_tVersion.szDeviceModel);
            bSet |= static_checkEnvValue(pFile, szBuffer, (char *)"serialnumber",
                                         g_tVersion.szSerialNumber);
            bSet |= static_checkEnvValue(pFile, szBuffer, (char *)"deviceuuid",
                                         g_tVersion.szUUID);
            bSet |= static_checkEnvValue(pFile, szBuffer, (char *)"devicestatus",
                                         g_tVersion.szStatus);
            bSet |= static_checkEnvValue(pFile, szBuffer, (char *)"contry",
                                         g_tVersion.szCountry);
            bSet |= static_checkEnvValue(pFile, szBuffer, (char *)"city",
                                         g_tVersion.szCity);
            bSet |= static_checkEnvValue(pFile, szBuffer, (char *)"web", g_tVersion.szWeb);
            bSet |= static_checkEnvValue(pFile, szBuffer, (char *)"tel", g_tVersion.szTel);
            bSet |= static_checkEnvValue(pFile, szBuffer, (char *)"copyright",
                                         g_tVersion.szCopyRight);
            bSet |= static_checkEnvValue(pFile, szBuffer, (char *)"manufacturer",
                                         g_tVersion.szManufacturer);
            bSet |= static_checkEnvValue(pFile, szBuffer, (char *)"brand",
                                         g_tVersion.szBrand);
            bSet |= static_checkEnvValue(pFile, szBuffer, (char *)"sensormodel",
                                         g_tVersion.szSensorModel);
            bSet |= static_checkEnvValue(pFile, szBuffer, (char *)"isofdome",
                                         (char *)(g_tVersion.IsOfDome ? "1" : "0"));
            bSet |= static_checkEnvValue(pFile, szBuffer, (char *)"version",
                                         g_tVersion.szVersion);
            bSet |= static_checkEnvValue(pFile, szBuffer, (char *)"hardversion",
                                         g_tVersion.szHardVersion);
            bSet |= static_checkEnvValue(pFile, szBuffer, (char *)"builddate",
                                         g_tVersion.szBuildDate);
            bSet |= static_checkEnvValue(pFile, szBuffer, (char *)"productdate",
                                         g_tVersion.szProductDate);
            bSet |= static_checkEnvValue(pFile, szBuffer, (char *)"hardware",
                                         g_tVersion.szHardware);

        }
    }
    Common_Free(szBuffer, __FUNCTION__, __LINE__);
    szBuffer = NULL;
    fclose(pFile);
    pFile = NULL;
    if (bSet)
    {
        Common_System("/root/bin/fw_setenv -s /tmp/uboot_env.config");
    }



}
static void Version_Sid_Load()
{
    FILE *fp = NULL;
    cJSON_Struct *pJs = NULL;
    S32 nStrLen = 0;
    S8 *pSidString = NULL,*szSid = NULL;
    fp = fopen("/usr/etc/Sid.json","rb");
    if(fp != NULL)
    {
        fseek(fp,0,SEEK_END);
        nStrLen = ftell(fp);
        fseek(fp,0,SEEK_SET);
        if(nStrLen > 0)
        {
            pSidString = (S8 *)Common_Malloc(nStrLen,0, __FUNCTION__,__LINE__);
            if(pSidString != NULL)
            {
                memset(pSidString,0,nStrLen);
                fread(pSidString,1,nStrLen,fp);
                pJs = Common_Json_Parse(pSidString, NULL,NULL);
                if(pJs)
                {
                    Common_Json_GetAttrValue(pJs, -1, "Sid", NULL, &szSid, 0, 0);
                    if(szSid != NULL)
                    {
                        g_tVersion.szSid = Common_StrDup(szSid, __FUNCTION__,__LINE__);
                    }
                    Common_Json_Delete_ex(pJs, __FUNCTION__,__LINE__);
                    pJs = NULL;
                }
                Common_Free(pSidString, __FUNCTION__,__LINE__);
                pSidString = NULL;
            }
        }

        fclose(fp);
        fp = NULL;
    }
}

static void Version_Sid_Save()
{
    FILE *fp = NULL;
    cJSON_Struct *pJs = NULL;
    S32 nStrLen = 0;
    S8 *pSidString = NULL;

    pJs = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
    if(pJs != NULL)
    {

        Common_Json_SetAttrValue(pJs, -1, "Sid", Common_Json_Type_String, g_tVersion.szSid, 0, 0);
        nStrLen = 0;
        pSidString = Common_Json_Print(pJs, &nStrLen);
        if(pSidString != NULL)
        {
            fp = fopen("/usr/etc/Sid.json","wb+");
            if(fp != NULL)
            {
                fwrite(pSidString,nStrLen + 1,1,fp);
                fclose(fp);
                fp = NULL;
            }

            Common_Free(pSidString, __FUNCTION__,__LINE__);
            pSidString = NULL;
        }
        Common_Json_Delete(pJs);
        pJs = NULL;
    }

}

static void Version_Load(ModuleHandle_T hModuleHandle)
{
    cJSON_Struct *pConfig = NULL, *pDefault = NULL, *thisCfg = NULL;
    S8 *szStringVal = NULL;
    S32 nIntValue = 0;
    S8 *szHardware = NULL;
    S8 *pSerial = NULL;
    // factoryInfo
    g_pFactoryInfoJson = NULL;
    if (g_hVersionLock == NULL)
    {
        Common_Lock_Create(&g_hVersionLock, NULL);
    }

    thisCfg = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0);

    Module_LoadConfigByType(hModuleHandle, Module_ConfigType_FactoryInfo,
                            &g_pFactoryInfoJson);
    Core_Version_load(hModuleHandle, Module_ConfigType_Normal, &pConfig);
    Core_Version_load(hModuleHandle, Module_ConfigType_Default, &pDefault);

    if(pDefault)
    {
        JsonOper_MergeObj((Common_cJSON_T *)thisCfg, (Common_cJSON_T *)pDefault, 0);
    }

    if(pConfig)
    {
        JsonOper_MergeObj((Common_cJSON_T *)thisCfg, (Common_cJSON_T *)pConfig, 0);
    }

    Common_Lock(g_hVersionLock);

    Common_Json_GetAttrValue(thisCfg, -1, "DeviceName", NULL, &szStringVal, NULL, NULL);
    if (szStringVal == NULL)
    {
        Common_Json_GetAttrValue(g_pFactoryInfoJson, -1, "/FactoryInfo/DeviceName",
                                 NULL, &szStringVal, NULL, NULL);
        if(szStringVal == NULL)szStringVal = "";
    }
    g_tVersion.szDeviceName = Common_StrDup(szStringVal, __FUNCTION__, __LINE__);

    szStringVal = NULL;
    Common_Json_GetAttrValue(thisCfg, -1, "ProductName", NULL, &szStringVal, NULL, NULL);
    if(szStringVal == NULL)
    {
        Common_Json_GetAttrValue(g_pFactoryInfoJson, -1, "/FactoryInfo/ProductName",
                                     NULL, &szStringVal, NULL, NULL);
        if(szStringVal == NULL)szStringVal = "";
    }
    g_tVersion.szProductName = Common_StrDup(szStringVal, __FUNCTION__, __LINE__);


    szStringVal = NULL;
    Common_Json_GetAttrValue(thisCfg, -1, "Manufacturer", NULL, &szStringVal, NULL, NULL);
    if(szStringVal == NULL)
    {
        Common_Json_GetAttrValue(g_pFactoryInfoJson, -1,
                             "/FactoryInfo/ManufacturerName", NULL, &szStringVal, NULL, NULL);
        if(szStringVal == NULL)szStringVal = "";
    }
    g_tVersion.szManufacturer = Common_StrDup(szStringVal, __FUNCTION__, __LINE__);

    szStringVal = NULL;
    Common_Json_GetAttrValue(pDefault, -1, "Vendor", NULL, &szStringVal, NULL, NULL);
    if(szStringVal == NULL)
    {
        g_tVersion.szVendor= Common_StrDup("HDT-NEUTRAL", __FUNCTION__, __LINE__);
    }
    else
    {
        g_tVersion.szVendor= Common_StrDup(szStringVal, __FUNCTION__, __LINE__);
    }

    szStringVal = NULL;
    Common_Json_GetAttrValue(thisCfg, -1, "Brand", NULL, &szStringVal, NULL, NULL);
    if(szStringVal == NULL)
    {
        Common_Json_GetAttrValue(g_pFactoryInfoJson, -1, "/FactoryInfo/Brand", NULL,
                             &szStringVal, NULL, NULL);
        if(szStringVal == NULL)szStringVal = "";
    }
    g_tVersion.szBrand = Common_StrDup(szStringVal, __FUNCTION__, __LINE__);

    szStringVal = NULL;
    Common_Json_GetAttrValue(thisCfg, -1, "Customer", NULL, &szStringVal, NULL, NULL);
    if (szStringVal == NULL)
    {
       Common_Json_GetAttrValue(g_pFactoryInfoJson, -1, "/FactoryInfo/Customer", NULL,
                             &szStringVal, NULL, NULL);
       if(szStringVal == NULL)szStringVal = "";
    }
    g_tVersion.szCustomer = Common_StrDup(szStringVal, __FUNCTION__, __LINE__);

    szStringVal = NULL;
    Common_Json_GetAttrValue(thisCfg, -1, "Country", NULL, &szStringVal, NULL, NULL);
    if (szStringVal == NULL)
    {
       Common_Json_GetAttrValue(g_pFactoryInfoJson, -1, "/FactoryInfo/Country", NULL,
                             &szStringVal, NULL, NULL);
       if(szStringVal == NULL)szStringVal = "";
    }
    g_tVersion.szCountry = Common_StrDup(szStringVal, __FUNCTION__, __LINE__);


    szStringVal = NULL;
    Common_Json_GetAttrValue(thisCfg, -1, "City", NULL, &szStringVal, NULL, NULL);
    if (szStringVal == NULL)
    {
        Common_Json_GetAttrValue(g_pFactoryInfoJson, -1, "/FactoryInfo/City", NULL,
                                 &szStringVal, NULL, NULL);
        if(szStringVal == NULL)szStringVal = "";
    }
    g_tVersion.szCity = Common_StrDup(szStringVal, __FUNCTION__, __LINE__);

    szStringVal = NULL;
    Common_Json_GetAttrValue(thisCfg, -1, "Web", NULL, &szStringVal, NULL, NULL);
    if (szStringVal == NULL)
    {
        Common_Json_GetAttrValue(g_pFactoryInfoJson, -1, "/FactoryInfo/Web", NULL,
                             &szStringVal, NULL, NULL);
        if(szStringVal == NULL)szStringVal = "";
    }
    g_tVersion.szWeb = Common_StrDup(szStringVal, __FUNCTION__, __LINE__);

    szStringVal = NULL;
    Common_Json_GetAttrValue(thisCfg, -1, "Tel", NULL, &szStringVal, NULL, NULL);
    if (szStringVal == NULL)
    {
        Common_Json_GetAttrValue(g_pFactoryInfoJson, -1, "/FactoryInfo/Tel", NULL,
                             &szStringVal, NULL, NULL);
        if(szStringVal == NULL)szStringVal = "";
    }
    g_tVersion.szTel = Common_StrDup(szStringVal, __FUNCTION__, __LINE__);

    szStringVal = NULL;
    Common_Json_GetAttrValue(thisCfg, -1, "Copyright", NULL, &szStringVal, NULL, NULL);
    if (szStringVal == NULL)
    {
        Common_Json_GetAttrValue(g_pFactoryInfoJson, -1, "/FactoryInfo/Copyright", NULL,
                             &szStringVal, NULL, NULL);
        if(szStringVal == NULL)szStringVal = "";
    }
    g_tVersion.szCopyRight = Common_StrDup(szStringVal, __FUNCTION__, __LINE__);

    szStringVal = NULL;
    Common_Json_GetAttrValue(g_pFactoryInfoJson, -1, "/FactoryInfo/DeviceType",
                             NULL, &szStringVal, NULL, NULL);
    if (szStringVal != NULL)
    {
        g_tVersion.szDeviceType = Common_StrDup(szStringVal, __FUNCTION__, __LINE__);
    }
    else
    {
        g_tVersion.szDeviceType = Common_StrDup((char *)"IPC", __FUNCTION__, __LINE__);
    }

    szStringVal = NULL;
    Common_Json_GetAttrValue(thisCfg, -1, "DeviceModel", NULL, &szStringVal, NULL, NULL);
    if (szStringVal == NULL)
    {
        Common_Json_GetAttrValue(g_pFactoryInfoJson, -1, "/FactoryInfo/DeviceModel",
                             NULL, &szStringVal, NULL, NULL);
        if(szStringVal == NULL)szStringVal = "";
    }
    g_tVersion.szDeviceModel = Common_StrDup(szStringVal, __FUNCTION__, __LINE__);

    szStringVal = NULL;
    Common_Json_GetAttrValue(g_pFactoryInfoJson, -1, "/FactoryInfo/SwVersion", NULL,
                             &szStringVal, NULL, NULL);
    if (szStringVal != NULL)
    {
        g_tVersion.szVersion = Common_StrDup(szStringVal, __FUNCTION__, __LINE__);
    }
    else
    {
        g_tVersion.szVersion = Common_StrDup((char *)"1.0.0", __FUNCTION__, __LINE__);

    }



    nIntValue = -1;
    if(NULL != Common_Json_GetAttrValue(g_pFactoryInfoJson, -1,
                                        "/FactoryInfo/SvnNumber", NULL, NULL, &nIntValue, NULL) && nIntValue > 0)
    {
        g_tVersion.nSvnNumber = nIntValue;
    }
    else
    {
        g_tVersion.nSvnNumber = g_nBuildSvn;
    }

    szStringVal = NULL;
    Common_Json_GetAttrValue(g_pFactoryInfoJson, -1, "/FactoryInfo/HwVersion", NULL,
                             &szStringVal, NULL, NULL);
    if (szStringVal != NULL)
    {
        g_tVersion.szHardVersion = Common_StrDup(szStringVal, __FUNCTION__, __LINE__);
    }
    else
    {
        g_tVersion.szHardVersion = Common_StrDup((char *)"1.0.0", __FUNCTION__,
                                   __LINE__);
    }
    szStringVal = NULL;
    Common_Json_GetAttrValue(g_pFactoryInfoJson, -1, "/FactoryInfo/ProductDate",
                             NULL, &szStringVal, NULL, NULL);
    if (szStringVal != NULL)
    {
        g_tVersion.szProductDate = Common_StrDup(szStringVal, __FUNCTION__, __LINE__);
    }
    else
    {
        g_tVersion.szProductDate = Common_StrDup((char *)"", __FUNCTION__, __LINE__);
    }

    szStringVal = NULL;
    Common_Json_GetAttrValue(g_pFactoryInfoJson, -1, "/FactoryInfo/BuildDate", NULL,
                             &szStringVal, NULL, NULL);
    if (szStringVal != NULL && szStringVal[0] != 0)
    {
        g_tVersion.szBuildDate = Common_StrDup(szStringVal, __FUNCTION__, __LINE__);
    }
    else
    {
        if (g_szBuildDate != NULL)
        {
            g_tVersion.szBuildDate = Common_StrDup(g_szBuildDate, __FUNCTION__, __LINE__);
        }
        else
        {
            g_tVersion.szBuildDate = Common_StrDup((char *)"", __FUNCTION__, __LINE__);
        }

    }




    szStringVal = NULL;
    Common_Json_GetAttrValue(g_pFactoryInfoJson, -1, "/FactoryInfo/SensorModel",
                             NULL, &szStringVal, NULL, NULL);
    if (szStringVal != NULL)
    {
        g_tVersion.szSensorModel = Common_StrDup(szStringVal, __FUNCTION__, __LINE__);
    }
    else
    {
        g_tVersion.szSensorModel = Common_StrDup((char *)"", __FUNCTION__, __LINE__);
    }

    szStringVal = NULL;
    Common_Json_GetAttrValue(g_pFactoryInfoJson, -1, "/FactoryInfo/IsofDome", NULL,
                             &szStringVal, NULL, NULL);
    if (szStringVal != NULL)
    {
    	if(szStringVal[0] == 'y')
    	{
        	g_tVersion.IsOfDome = 1;
    	}
		else if(szStringVal[0] == 'n')
    	{
        	g_tVersion.IsOfDome = 0;
    	}
		else
		{
			g_tVersion.IsOfDome = atoi(szStringVal);
		}
    }
    szStringVal = NULL;
    Common_Json_GetAttrValue(g_pFactoryInfoJson, -1, "/FactoryInfo/IsofIr", NULL,
                             &szStringVal, NULL, NULL);
    if (szStringVal != NULL)
    {
        g_tVersion.IsOfIr = (szStringVal[0] == 'y');
    }

    nIntValue = 0;
    Common_Json_GetAttrValue(g_pFactoryInfoJson, -1,
                             "/FactoryInfo/AutoLens/LensSupport", NULL, NULL, &nIntValue, NULL);
    g_tVersion.nLensSupport = nIntValue;

    szStringVal = NULL;
    Common_Json_GetAttrValue(g_pFactoryInfoJson, -1,
                             "/FactoryInfo/AutoLens/LensDrvType", NULL, &szStringVal, NULL, NULL);
    if (szStringVal != NULL)
    {
        g_tVersion.szLensDrvType = Common_StrDup(szStringVal, __FUNCTION__, __LINE__);
    }
    else
    {
        g_tVersion.szLensDrvType = Common_StrDup((char *)"", __FUNCTION__, __LINE__);
    }
    szStringVal = NULL;
    Common_Json_GetAttrValue(g_pFactoryInfoJson, -1,
                             "/FactoryInfo/AutoLens/LensType", NULL, &szStringVal, NULL, NULL);
    if (szStringVal != NULL)
    {
        g_tVersion.szLensType = Common_StrDup(szStringVal, __FUNCTION__, __LINE__);
    }
    else
    {
        g_tVersion.szLensType = Common_StrDup((char *)"", __FUNCTION__, __LINE__);
    }

    nIntValue = 0;
    Common_Json_GetAttrValue(g_pFactoryInfoJson, -1,
                             "/FactoryInfo/AutoIris/IrisSupport", NULL, NULL, &nIntValue, NULL);
    g_tVersion.nIrisSupport = nIntValue;

    szStringVal = NULL;
    Common_Json_GetAttrValue(g_pFactoryInfoJson, -1,
                             "/FactoryInfo/AutoIris/IrisType", NULL, &szStringVal, NULL, NULL);
    if (szStringVal != NULL)
    {
        g_tVersion.szIrisType = Common_StrDup(szStringVal, __FUNCTION__, __LINE__);
    }
    else
    {
        g_tVersion.szIrisType = Common_StrDup((char *)"", __FUNCTION__, __LINE__);
    }

    Common_Json_GetAttrValue(g_pFactoryInfoJson, -1, "/FactoryInfo/Hardware", NULL,
                             &szHardware, NULL, NULL);
    if(szHardware != NULL)
    {
        g_tVersion.szHardware = Common_StrDup(szHardware, __FUNCTION__, __LINE__);
    }
    g_tVersion.szUUID = Common_StrDup(Module_GetSerialNumber(1), __FUNCTION__,
                                      __LINE__);
    g_tVersion.szAuthMethod = Common_StrDup(Module_VersionAuth_GetMethod(),
                                            __FUNCTION__, __LINE__);

    pSerial =  Module_GetSerialNumber(0);

    if(NULL == pSerial)
    {
        szStringVal = NULL;
        Common_Json_GetAttrValue(pConfig, -1, "SerialNumber", NULL, &szStringVal, NULL,
                                 NULL);
        if (szStringVal != NULL)
        {
            g_tVersion.szSerialNumber = Common_StrDup(szStringVal, __FUNCTION__, __LINE__);
        }
        else
        {
            S8 sztmp[32];
            S8 *pszMac = NULL;
            S32 mac[6];

            Common_GetLocalNetInfo((char *)"eth0", 0, NULL, NULL, NULL, &pszMac);
            if (pszMac != NULL &&
                    (6 == sscanf(pszMac, "%02X:%02X:%02X:%02X:%02X:%02X", &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]) ||
                    6 == sscanf(pszMac, "%02x:%02x:%02x:%02x:%02x:%02x", &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5])))
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

            if (NULL != pszMac)
            {
                Common_Free(pszMac, __FUNCTION__, __LINE__);
                pszMac = NULL;
            }

            g_tVersion.szSerialNumber = Common_StrDup(sztmp, __FUNCTION__, __LINE__);
        }


    }
    else
    {
        //S32 bActivated = 1;
        g_tVersion.szSerialNumber = Common_StrDup(pSerial, __FUNCTION__, __LINE__);
    }

	if( strncmp(g_tVersion.szSerialNumber, "031a", 4) == 0 ||
		strncmp(g_tVersion.szSerialNumber, "0313", 4) == 0 ||
		strncmp(g_tVersion.szSerialNumber, "8313", 4) == 0 ||
		strncmp(g_tVersion.szSerialNumber, "03a3", 4) == 0 ||
		strncmp(g_tVersion.szSerialNumber, "032a", 4) == 0 ||
		strncmp(g_tVersion.szSerialNumber, "032c", 4) == 0 ||
		strncmp(g_tVersion.szSerialNumber, "0327", 4) == 0 ||
		strncmp(g_tVersion.szSerialNumber, "03a2", 4) == 0 ||
		strncmp(g_tVersion.szSerialNumber, "03d0", 4) == 0 ||
		strncmp(g_tVersion.szSerialNumber, "03da", 4) == 0)
	{
		g_tVersion.szDeviceTypeString = Common_StrDup((char *)"ipc_guard_wifi", __FUNCTION__, __LINE__);
	}
	else if(strncmp(g_tVersion.szSerialNumber, "031b", 4) == 0 ||
			strncmp(g_tVersion.szSerialNumber, "0314", 4) == 0 ||
			strncmp(g_tVersion.szSerialNumber, "8314", 4) == 0 ||
			strncmp(g_tVersion.szSerialNumber, "03a5", 4) == 0 ||
			strncmp(g_tVersion.szSerialNumber, "032b", 4) == 0 ||
			strncmp(g_tVersion.szSerialNumber, "032d", 4) == 0 ||
		    strncmp(g_tVersion.szSerialNumber, "0329", 4) == 0 ||
		    strncmp(g_tVersion.szSerialNumber, "03a8", 4) == 0 ||
		    strncmp(g_tVersion.szSerialNumber, "03d1", 4) == 0 ||
		    strncmp(g_tVersion.szSerialNumber, "03d9", 4) == 0)
	{
		g_tVersion.szDeviceTypeString = Common_StrDup((char *)"ipc_guard_4g", __FUNCTION__, __LINE__);
	}
	else if(strncmp(g_tVersion.szSerialNumber, "031e", 4) == 0 ||
            strncmp(g_tVersion.szSerialNumber, "0323", 4) == 0 ||
            strncmp(g_tVersion.szSerialNumber, "03a7", 4) == 0 ||
            strncmp(g_tVersion.szSerialNumber, "03ae", 4) == 0 ||
            strncmp(g_tVersion.szSerialNumber, "03d3", 4) == 0)
	{
		g_tVersion.szDeviceTypeString = Common_StrDup((char *)"ipc_guard_dome_wifi", __FUNCTION__, __LINE__);
	}
	else if(strncmp(g_tVersion.szSerialNumber, "031d", 4) == 0 ||
            strncmp(g_tVersion.szSerialNumber, "0324", 4) == 0 ||
            strncmp(g_tVersion.szSerialNumber, "03a6", 4) == 0 ||
            strncmp(g_tVersion.szSerialNumber, "03a9", 4) == 0 ||
            strncmp(g_tVersion.szSerialNumber, "03d4", 4) == 0)
	{
		g_tVersion.szDeviceTypeString = Common_StrDup((char *)"ipc_guard_dome_4g", __FUNCTION__, __LINE__);
	}
	else
	{
		szStringVal = NULL;
	    Common_Json_GetAttrValue(pDefault, -1, "DeviceTypeString", NULL, &szStringVal, NULL, NULL);
	    if (szStringVal != NULL)
	    {
	        g_tVersion.szDeviceTypeString = Common_StrDup(szStringVal, __FUNCTION__, __LINE__);
	    }
	    else
	    {
	        g_tVersion.szDeviceTypeString = Common_StrDup((char *)"ipc_normal", __FUNCTION__, __LINE__);
	    }
	}

    if (Version_checkRight(pSerial, szHardware))
    {
        g_tVersion.szStatus = Common_StrDup((char *)"Activated", __FUNCTION__,
                                            __LINE__);
        g_tVersion.bActived = 1;
    }
    else
    {
        g_tVersion.szStatus = Common_StrDup((char *)"Unactivated", __FUNCTION__,
                                            __LINE__);
        g_tVersion.bActived = 0;
    }
    Common_UnLock(g_hVersionLock);
    Common_Json_Delete(pConfig);
    Common_Json_Delete(pDefault);

    if(thisCfg)
    {
        Common_Json_Delete(thisCfg);
        thisCfg = NULL;
    }
    // Version_FactoryInfoSave();
}

static void Version_ReloadSerial()
{
    S8 *pSerial =  Module_GetSerialNumber(0);
    S8 *pUUID = Module_GetSerialNumber(1);
    Common_Lock(g_hVersionLock);
    if(NULL != pSerial)
    {
        Common_Free(g_tVersion.szSerialNumber, __FUNCTION__, __LINE__);
        g_tVersion.szSerialNumber = Common_StrDup(pSerial, __FUNCTION__, __LINE__);

    }
    if (pUUID != NULL)
    {
        Common_Free(g_tVersion.szUUID, __FUNCTION__, __LINE__);
        g_tVersion.szUUID = NULL;
    }
    g_tVersion.szUUID = Common_StrDup(pUUID, __FUNCTION__, __LINE__);

    if (Version_checkRight(pSerial, g_tVersion.szHardware))
    {
        Common_Free(g_tVersion.szStatus, __FUNCTION__, __LINE__);
        g_tVersion.szStatus = Common_StrDup((char *)"Activated", __FUNCTION__,
                                            __LINE__);
        g_tVersion.bActived = 1;
    }
    else
    {
        Common_Free(g_tVersion.szStatus, __FUNCTION__, __LINE__);
        g_tVersion.szStatus = Common_StrDup((char *)"Unactivated", __FUNCTION__,
                                            __LINE__);
        g_tVersion.bActived = 0;
    }
    Common_UnLock(g_hVersionLock);
}


static void Version_Save(ModuleHandle_T hModuleHandle)
{
    cJSON_Struct *pConfig = NULL;
    pConfig = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0);
    if (pConfig != NULL)
    {
        Common_Json_SetAttrValue(pConfig, -1, "DeviceName", Common_Json_Type_String,
                                 g_tVersion.szDeviceName, 0, 0);
        Common_Json_SetAttrValue(pConfig, -1, "ProductName", Common_Json_Type_String,
                                 g_tVersion.szProductName, 0, 0);
        Common_Json_SetAttrValue(pConfig, -1, "DeviceType", Common_Json_Type_String,
                                 g_tVersion.szDeviceType, 0, 0);
        Common_Json_SetAttrValue(pConfig, -1, "DeviceTypeString", Common_Json_Type_String,
                                 g_tVersion.szDeviceTypeString, 0, 0);
        Common_Json_SetAttrValue(pConfig, -1, "DeviceModel", Common_Json_Type_String,
                                 g_tVersion.szDeviceModel, 0, 0);

        Common_Json_SetAttrValueStr(pConfig, "Vendor", g_tVersion.szVendor);

        Common_Json_SetAttrValue(pConfig, -1, "Country", Common_Json_Type_String,
                                 g_tVersion.szCountry, 0, 0);
        Common_Json_SetAttrValue(pConfig, -1, "City", Common_Json_Type_String,
                                 g_tVersion.szCity, 0, 0);

        Common_Json_SetAttrValue(pConfig, -1, "Web", Common_Json_Type_String,
                                 g_tVersion.szWeb, 0, 0);
        Common_Json_SetAttrValue(pConfig, -1, "Tel", Common_Json_Type_String,
                                 g_tVersion.szTel, 0, 0);
        Common_Json_SetAttrValue(pConfig, -1, "Copyright", Common_Json_Type_String,
                                 g_tVersion.szCopyRight, 0, 0);
        Common_Json_SetAttrValue(pConfig, -1, "Manufacturer", Common_Json_Type_String,
                                 g_tVersion.szManufacturer, 0, 0);
        Common_Json_SetAttrValue(pConfig, -1, "Brand", Common_Json_Type_String,
                                 g_tVersion.szBrand, 0, 0);
        Common_Json_SetAttrValue(pConfig, -1, "Customer", Common_Json_Type_String,
                                 g_tVersion.szCustomer, 0, 0);

        Common_Json_SetAttrValue(pConfig, -1, "SensorModel", Common_Json_Type_String,
                                 g_tVersion.szSensorModel, 0, 0);
        Common_Json_SetAttrValue(pConfig, -1, "IsOfDome", Common_Json_Type_Number, NULL,
                                 g_tVersion.IsOfDome, 0);
        Common_Json_SetAttrValue(pConfig, -1, "IsOfIr", Common_Json_Type_Number, NULL,
                                 g_tVersion.IsOfIr, 0);
        Common_Json_SetAttrValue(pConfig, -1, "LensSupport", Common_Json_Type_Number,
                                 NULL, g_tVersion.nLensSupport, 0);
        Common_Json_SetAttrValue(pConfig, -1, "LensDrvType", Common_Json_Type_String,
                                 g_tVersion.szLensDrvType, 0, 0);
        Common_Json_SetAttrValue(pConfig, -1, "LensType", Common_Json_Type_String,
                                 g_tVersion.szLensType, 0, 0);
        Common_Json_SetAttrValue(pConfig, -1, "IrisSupport", Common_Json_Type_Number,
                                 NULL, g_tVersion.nIrisSupport, 0);
        Common_Json_SetAttrValue(pConfig, -1, "IrisType", Common_Json_Type_String,
                                 g_tVersion.szIrisType, 0, 0);

        Common_Json_SetAttrValue(pConfig, -1, "Version", Common_Json_Type_String,
                                 g_tVersion.szVersion, 0, 0);
        Common_Json_SetAttrValue(pConfig, -1, "SvnNumber", Common_Json_Type_Number,
                                 NULL, g_tVersion.nSvnNumber, 0);
        Common_Json_SetAttrValue(pConfig, -1, "HardVersion", Common_Json_Type_String,
                                 g_tVersion.szHardVersion, 0, 0);
        Common_Json_SetAttrValue(pConfig, -1, "BuildDate", Common_Json_Type_String,
                                 g_tVersion.szBuildDate, 0, 0);
        Common_Json_SetAttrValue(pConfig, -1, "ProductDate", Common_Json_Type_String,
                                 g_tVersion.szProductDate, 0, 0);
        Common_Json_SetAttrValue(pConfig, -1, "Hardware", Common_Json_Type_String,
                                 g_tVersion.szHardware, 0, 0);

        Common_Json_SetAttrValue(pConfig, -1, "SerialNumber", Common_Json_Type_String,
                                 g_tVersion.szSerialNumber, 0, 0);
        Common_Json_SetAttrValue(pConfig, -1, "UUID", Common_Json_Type_String,
                                 g_tVersion.szUUID, 0, 0);
        Common_Json_SetAttrValue(pConfig, -1, "Status", Common_Json_Type_String,
                                 g_tVersion.szStatus, 0, 0);
    }
    Core_Version_Save(hModuleHandle, pConfig);
}
S32 Core_Version_Init(ModuleHandle_T hModuleHandle)
{
    if (g_bInit)
    {
        return 0;
    }
    memset(&g_tVersion, 0, sizeof(g_tVersion));
    g_bInit = 1;
    Version_Sid_Load();
    Version_Load(hModuleHandle);
    Version_Save(hModuleHandle);
    return 0;
}



S32 Core_Version_CallFunctions(ModuleHandle_T hModuleHandle,
                               cJSON_Struct *pInParams, cJSON_Struct **pOutParams)
{
#if 0
    char *out = NULL;

    LOGI("recv call input:%s \n",
         out = Common_cJSON_PrintUnformatted((Common_cJSON_T *)pInParams, NULL));
    if(out)
    {
        Common_Free(out, __FUNCTION__, __LINE__);
    }
#endif

    S32 nRet = -1;
    S8 *szUri = NULL;
    S8 *szMethod = NULL;
    cJSON_Struct *pOutJson = NULL;
    //S32 nCurrType = 0;
    Common_Json_GetAttrValue(pInParams, -1, "Header/Uri", NULL, &szUri, NULL, NULL);
    if(0 != Common_StriCmp((char *)"/Core/Version", szUri) &&
       0 != Common_StriCmp((char *)"/Core/DeviceName", szUri) &&
	   0 != Common_StriCmp("/Core/VersionAuth",szUri) &&
	   0 != Common_StriCmp("/Core/Sid",szUri))
    {
        return -1;
    }
    Common_Json_GetAttrValue(pInParams, -1, "Header/Method", NULL, &szMethod, NULL,
                             NULL);
    Core_Version_Init(hModuleHandle);
    do
    {
        if(0 == Common_StriCmp((char *)"/Core/Version", szUri))
        {
            if(0 == Common_StriCmp((char *)"Get", szMethod))
            {
                pOutJson = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0);
                if (pOutJson != NULL)
                {
                    //S8 szVersion[32];
                    Common_Lock(g_hVersionLock);
                    Common_Json_SetAttrValue(pOutJson, -1, "Header", Common_Json_Type_Object, NULL,
                                             0, 0);
                    Common_Json_SetAttrValue(pOutJson, -1, "Header/Code", Common_Json_Type_Number,
                                             NULL, 0, 0);
                    Common_Json_SetAttrValue(pOutJson, -1, "Data", Common_Json_Type_Object, NULL, 0,
                                             0);
                    Common_Json_SetAttrValue(pOutJson, -1, "Data/DeviceName",
                                             Common_Json_Type_String, g_tVersion.szDeviceName, 0, 0);
                    Common_Json_SetAttrValue(pOutJson, -1, "Data/ProductName",
                                             Common_Json_Type_String, g_tVersion.szProductName, 0, 0);
                    Common_Json_SetAttrValue(pOutJson, -1, "Data/DeviceType",
                                             Common_Json_Type_String, g_tVersion.szDeviceType, 0, 0);
                    Common_Json_SetAttrValue(pOutJson, -1, "Data/DeviceTypeString",
                                             Common_Json_Type_String, g_tVersion.szDeviceTypeString, 0, 0);
                    Common_Json_SetAttrValue(pOutJson, -1, "Data/DeviceModel",
                                             Common_Json_Type_String, g_tVersion.szDeviceModel, 0, 0);

                    Common_Json_SetAttrValue(pOutJson, -1, "Data/Country", Common_Json_Type_String,
                                             g_tVersion.szCountry, 0, 0);
                    Common_Json_SetAttrValue(pOutJson, -1, "Data/City", Common_Json_Type_String,
                                             g_tVersion.szCity, 0, 0);

                    Common_Json_SetAttrValue(pOutJson, -1, "Data/Web", Common_Json_Type_String,
                                             g_tVersion.szWeb, 0, 0);
                    Common_Json_SetAttrValue(pOutJson, -1, "Data/Tel", Common_Json_Type_String,
                                             g_tVersion.szTel, 0, 0);
                    Common_Json_SetAttrValue(pOutJson, -1, "Data/Copyright",
                                             Common_Json_Type_String, g_tVersion.szCopyRight, 0, 0);
                    Common_Json_SetAttrValue(pOutJson, -1, "Data/Manufacturer",
                                             Common_Json_Type_String, g_tVersion.szManufacturer, 0, 0);
                    Common_Json_SetAttrValue(pOutJson, -1, "Data/Vendor",
                                             Common_Json_Type_String, g_tVersion.szVendor, 0, 0);

                    Common_Json_SetAttrValue(pOutJson, -1, "Data/Brand", Common_Json_Type_String,
                                             g_tVersion.szBrand, 0, 0);
                    Common_Json_SetAttrValue(pOutJson, -1, "Data/Customer", Common_Json_Type_String,
                                             g_tVersion.szCustomer, 0, 0);

                    Common_Json_SetAttrValue(pOutJson, -1, "Data/SensorModel",
                                             Common_Json_Type_String, g_tVersion.szSensorModel, 0, 0);
                    Common_Json_SetAttrValue(pOutJson, -1, "Data/IsOfDome", Common_Json_Type_Number,
                                             NULL, g_tVersion.IsOfDome, 0);
                    Common_Json_SetAttrValue(pOutJson, -1, "Data/IsOfIr", Common_Json_Type_Number,
                                             NULL, g_tVersion.IsOfIr, 0);
                    Common_Json_SetAttrValue(pOutJson, -1, "Data/LensSupport",
                                             Common_Json_Type_Number, NULL, g_tVersion.nLensSupport, 0);
                    Common_Json_SetAttrValue(pOutJson, -1, "Data/LensDrvType",
                                             Common_Json_Type_String, g_tVersion.szLensDrvType, 0, 0);
                    Common_Json_SetAttrValue(pOutJson, -1, "Data/LensType", Common_Json_Type_String,
                                             g_tVersion.szLensType, 0, 0);
                    Common_Json_SetAttrValue(pOutJson, -1, "Data/IrisSupport",
                                             Common_Json_Type_Number, NULL, g_tVersion.nIrisSupport, 0);
                    Common_Json_SetAttrValue(pOutJson, -1, "Data/IrisType", Common_Json_Type_String,
                                             g_tVersion.szIrisType, 0, 0);
                    Common_Json_SetAttrValue(pOutJson, -1, "Data/Version", Common_Json_Type_String,
                                             g_tVersion.szVersion, 0, 0);
                    Common_Json_SetAttrValue(pOutJson, -1, "Data/SvnNumber",
                                             Common_Json_Type_Number, NULL, g_tVersion.nSvnNumber, 0);
                    Common_Json_SetAttrValue(pOutJson, -1, "Data/HardVersion",
                                             Common_Json_Type_String, g_tVersion.szHardVersion, 0, 0);
                    Common_Json_SetAttrValue(pOutJson, -1, "Data/BuildDate",
                                             Common_Json_Type_String, g_tVersion.szBuildDate, 0, 0);
                    Common_Json_SetAttrValue(pOutJson, -1, "Data/ProductDate",
                                             Common_Json_Type_String, g_tVersion.szProductDate, 0, 0);


                    Common_Json_SetAttrValue(pOutJson, -1, "Data/Hardware", Common_Json_Type_String,
                                             g_tVersion.szHardware, 0, 0);

    				if( g_tVersion.szUUID == NULL)
    				{
    					Version_ReloadSerial();
    					if(g_tVersion.szUUID)
    						Version_Save(hModuleHandle);
    				}

                    Common_Json_SetAttrValue(pOutJson, -1, "Data/UUID", Common_Json_Type_String,
                                             g_tVersion.szUUID, 0, 0);
                    Common_Json_SetAttrValue(pOutJson, -1, "Data/AuthMethod",
                                             Common_Json_Type_String, g_tVersion.szAuthMethod, 0, 0);

                    Common_Json_SetAttrValue(pOutJson, -1, "Data/SerialNumber",
                                             Common_Json_Type_String, g_tVersion.szSerialNumber, 0, 0);

                    if(g_tVersion.szSid)
                    {
                        Common_Json_SetAttrValueStr(pOutJson, "Data/Sid", g_tVersion.szSid);
                    }

                    Common_Json_SetAttrValue(pOutJson, -1, "/Data/Status", Common_Json_Type_String,
                                             g_tVersion.szStatus, 0, 0);
                    Common_UnLock(g_hVersionLock);
                    Common_Time_T LTime;
                    S8 szDataTime[64];
                    Common_GetLocalTime(&LTime);
                    snprintf(szDataTime, 63, "%04d%02d%02d%02d%02d%02d", LTime.year, LTime.month,
                             LTime.day, LTime.hour, LTime.min, LTime.sec);
                    szDataTime[63] = 0;

                    Common_Json_SetAttrValue(pOutJson, -1, "/Data/DateTime",
                                             Common_Json_Type_String, szDataTime, 0, 0);
                }
            }
            else if(0 == Common_StriCmp((char *)"Put", szMethod))
            {
                S8 *strTmp = NULL;
                cJSON_Struct *pLoadConfig = NULL;
                Module_LoadConfigByType(hModuleHandle, Module_ConfigType_Default, &pLoadConfig);

                if(Common_Json_GetAttrValueObj(pLoadConfig, "Version") == NULL)
                {
                    Common_Json_SetAttrValueObj(pLoadConfig, "Version");
                }
                Common_Lock(g_hVersionLock);
                if(Common_Json_GetAttrValueStr(pInParams, "Data/DeviceName", &strTmp))
                {
                    Common_Free(g_tVersion.szDeviceName, __FUNCTION__, __LINE__);
                    g_tVersion.szDeviceName = Common_StrDup(strTmp, __FUNCTION__, __LINE__);
                    Common_Json_SetAttrValueStr(pLoadConfig, "Version/DeviceName", strTmp);
                }
                if(Common_Json_GetAttrValueStr(pInParams, "Data/DeviceModel", &strTmp))
                {
                    Common_Free(g_tVersion.szDeviceModel, __FUNCTION__, __LINE__);
                    g_tVersion.szDeviceModel = Common_StrDup(strTmp, __FUNCTION__, __LINE__);
                    Common_Json_SetAttrValueStr(pLoadConfig, "Version/DeviceModel", strTmp);
                }
                if(Common_Json_GetAttrValueStr(pInParams, "Data/Manufacturer", &strTmp))
                {
                    Common_Free(g_tVersion.szManufacturer, __FUNCTION__, __LINE__);
                    g_tVersion.szManufacturer = Common_StrDup(strTmp, __FUNCTION__, __LINE__);
                    Common_Json_SetAttrValueStr(pLoadConfig, "Version/Manufacturer", strTmp);
                }
                if(Common_Json_GetAttrValueStr(pInParams, "Data/Country", &strTmp))
                {
                    Common_Free(g_tVersion.szCountry, __FUNCTION__, __LINE__);
                    g_tVersion.szCountry = Common_StrDup(strTmp, __FUNCTION__, __LINE__);
                    Common_Json_SetAttrValueStr(pLoadConfig, "Version/Country", strTmp);
                }
                if(Common_Json_GetAttrValueStr(pInParams, "Data/City", &strTmp))
                {
                    Common_Free(g_tVersion.szCity, __FUNCTION__, __LINE__);
                    g_tVersion.szCity = Common_StrDup(strTmp, __FUNCTION__, __LINE__);
                    Common_Json_SetAttrValueStr(pLoadConfig, "Version/City", strTmp);
                }

                Common_UnLock(g_hVersionLock);
                Version_Save(hModuleHandle);

                Module_SaveConfigByType(hModuleHandle, Module_ConfigType_Default, pLoadConfig);

                Common_Json_Delete(pLoadConfig);
                pLoadConfig = NULL;

            }
            else
            {
                nRet = MODULE_ERROR_TYPE_METHOD_UNSUPPORT;
                break;
            }
            nRet = 0;
        }
        else if(0 == Common_StriCmp((char *)"/Core/VersionAuth", szUri))
        {
            if(0 == Common_StriCmp((char *)"Put", szMethod))
            {
                char *szLicence = NULL;
                S32 nBurnRet = 0;
                Common_Json_GetAttrValue(pInParams, -1, "Data/Licence", NULL, &szLicence, NULL,
                                         NULL);
                if(szLicence == NULL)
                {
                    nRet = MODULE_ERROR_TYPE_INVALIDPARAM;
                    break;
                }
                nBurnRet = Module_VersionAuth_burn(szLicence);
                if(0 != nBurnRet)
                {
                    nRet = MODULE_ERROR_TYPE_INVALIDPARAM;
                    break;
                }
                nRet = 0;
                Version_ReloadSerial();
                Version_Save(hModuleHandle);
            }
            else
            {
                nRet = MODULE_ERROR_TYPE_METHOD_UNSUPPORT;
                break;
            }
        }
        else if(0 == Common_StriCmp((char *)"/Core/VersionEraseAuth", szUri))
        {
            if(0 == Common_StriCmp((char *)"Put", szMethod))
            {
                //char *szLicence = NULL;
                S32 nBurnRet = 0;

                nBurnRet = Module_VersionAuth_Erase();
                if(0 != nBurnRet)
                {
                    nRet = MODULE_ERROR_TYPE_INVALIDPARAM;
                    break;
                }
                nRet = 0;
                Version_ReloadSerial();
                Version_Save(hModuleHandle);
            }
            else
            {
                nRet = MODULE_ERROR_TYPE_METHOD_UNSUPPORT;
                break;
            }
        }
        else if(0 == Common_StriCmp((char *)"/Core/DeviceName", szUri))
        {
            if(0 == Common_StriCmp((char *)"Get", szMethod))
            {
                pOutJson = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0);
                if (pOutJson != NULL)
                {
                    Common_Json_SetAttrValue(pOutJson, -1, "Header", Common_Json_Type_Object, NULL,
                                             0, 0);
                    Common_Json_SetAttrValue(pOutJson, -1, "Header/Code", Common_Json_Type_Number,
                                             NULL, 0, 0);
                    Common_Json_SetAttrValue(pOutJson, -1, "Data", Common_Json_Type_Object, NULL, 0,
                                             0);
                    Common_Lock(g_hVersionLock);
                    Common_Json_SetAttrValue(pOutJson, -1, "Data/DeviceName",
                                             Common_Json_Type_String, g_tVersion.szDeviceName, 0, 0);
                    Common_UnLock(g_hVersionLock);
                }
                nRet = 0;
            }
            else if(0 == Common_StriCmp((char *)"Put", szMethod))
            {
                S8 *szDeviceName = NULL, *szOld = NULL, *szNew = NULL;
                Common_Json_GetAttrValue(pInParams, -1, "Data/DeviceName", NULL, &szDeviceName,
                                         NULL, NULL);
                szOld = g_tVersion.szDeviceName;
                if (szDeviceName != NULL)
                {
                    szNew = Common_StrDup(szDeviceName, __FUNCTION__, __LINE__);
                }
                Common_Lock(g_hVersionLock);
                g_tVersion.szDeviceName = szNew;
                Common_UnLock(g_hVersionLock);
                Common_Free(szOld, __FUNCTION__, __LINE__);
                Version_Save(hModuleHandle);

				nRet = 0;
			}
			else
			{
				nRet = MODULE_ERROR_TYPE_METHOD_UNSUPPORT;
				break;
			}
		}
		else if(0 == Common_StriCmp("/Core/Sid",szUri))
		{
			if(0 == Common_StriCmp("Get",szMethod))
			{
				pOutJson = Common_Json_New_ex(NULL,Common_Json_Type_Object,NULL,0,0, __FUNCTION__,__LINE__);
				if (pOutJson != NULL)
				{
					char *szNewSN = NULL;
					Common_Json_SetAttrValue(pOutJson,-1,"Header",Common_Json_Type_Object,NULL,0,0);
					Common_Json_SetAttrValue(pOutJson,-1,"Header/Code",Common_Json_Type_Number,NULL,0,0);
					Common_Json_SetAttrValue(pOutJson,-1,"Data",Common_Json_Type_Object,NULL,0,0);

					Common_Lock(g_hVersionLock);
					Common_Json_SetAttrValue(pOutJson,-1,"Data/Sid",Common_Json_Type_String,g_tVersion.szSid,0,0);
					Common_UnLock(g_hVersionLock);
				}
				nRet = 0;
			}
			else if(0 == Common_StriCmp("Put",szMethod))
			{
				S8 *szSid = NULL;
				Common_Json_GetAttrValue(pInParams,-1,"Data/Sid",NULL,&szSid,NULL,NULL);
                if(szSid == NULL)
                {
                    nRet = MODULE_ERROR_TYPE_INVALIDPARAM;
                }
                else
                {
    				Common_Lock(g_hVersionLock);
    				if(g_tVersion.szSid == NULL || 0 != Common_StrCmp(g_tVersion.szSid, szSid))
    				{
                        if (g_tVersion.szSid != NULL)
        				{
                            Common_Free(g_tVersion.szSid,__FUNCTION__,__LINE__);
        				}

                        g_tVersion.szSid = Common_StrDup(szSid,__FUNCTION__,__LINE__);
                        Version_Sid_Save();
                    }

                    Common_UnLock(g_hVersionLock);
				    nRet = 0;
                }


			}
			else
			{
				nRet = MODULE_ERROR_TYPE_METHOD_UNSUPPORT;
				break;
			}
		}

	} while (0);
	if (pOutJson != NULL)
	{
		if (pOutParams != NULL)
		{
			*pOutParams = pOutJson;
			pOutJson = NULL;
		}
	}
	else if(pOutParams != NULL)
	{
		pOutJson = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
		if (pOutJson != NULL)
		{
			Common_Json_SetAttrValue(pOutJson,-1,"Header",Common_Json_Type_Object,NULL,0,0);
			Common_Json_SetAttrValue(pOutJson,-1,"Header/Code",Common_Json_Type_Number,NULL,nRet,0);
			*pOutParams = pOutJson;
			pOutJson = NULL;
		}

    }
    Common_Json_Delete(pOutJson);
    pOutJson = NULL;
    return 0;
}
