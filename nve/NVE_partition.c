/*
 *  NVE_partition.c
 *  
 *  Copyright:	(C) Copyright 2009 Marvell International Ltd.
 *  
 *  Author: Chen Yajia<>
 *  
 *  This program is free software; you can redistribute it and/or 
 *
 *  modify it under the terms of the GNU General Public License version 2 as
 *  
 *  publishhed by the Free Software Foundation.
 *
 *  use for CONFIG_BOARD_IS_HUAWEI_PXA920
 */
 
/******************************************************************************
  Copyright (c) 2011-2111 HUAWEI Incorporated. All Rights Reserved.
*******************************************************************************/


#include "NVE_partition.h"
#include <stdio.h>

static struct NVE_partittion nve_partition= {
	/*               nve_partition_name[32],      nve_version,          nve_age,    nve_block_id, nve_block_count,      valid_items,     nv_checksum,   reserved[72]*/
	.header = {"TD-nvm-extension-partition",                1,                1,               1,               1,             1023,               0,          ""},
	.NVs = {
		/* nv_number,   nv_name[8],  nv_property,   valid_size,     reserved,            nv_data[104]*/
		{          0,    "SWVERSI",            0,            5,            0, "T8300"},
		{          1,    "BOARDID",            1,           32,            0, 0},
		{          2,    "SN"     ,            1,           32,            0, 0},
		{          3,    "MACADDR",            1,           32,            0, "MAC"},
		{          4,    "WEBPWD" ,            1,           16,            0, 0},
		{          5,    "IMEI"   ,            1,            20,            0, "IMEI"},
		{          6,    "RSV6"   ,            0,          104,            0, 0},
		{          7,    "RSV7"   ,            0,          104,            0, 0},
		{          8,    "RSV8"   ,            0,          104,            0, 0},
		{          9,    "RSV9"   ,            0,          104,            0, 0},
		{         10,    "RSV10"  ,            0,          104,            0, 0},
		{         11,    "RSV11"  ,            0,          104,            0, 0},
		{         12,    "RSV12"  ,            0,          104,            0, 0},
		{         13,    "RSV13"  ,            0,          104,            0, 0},
		{         14,    "RSV14"  ,            0,          104,            0, 0},
		{         15,    "RSV15"  ,            0,          104,            0, 0},
		{         16,    "RSV16"  ,            0,          104,            0, 0},
		{         17,    "DHCPSET",            1,           40,            0, 0},
		{         18,    "CURPROF",            1,            1,            0, 0},
		{         19,    "FWEN"   ,            1,            1,            0, 0},
		{         20,    "WANPING",            1,            1,            0, 0},
		{         21,    "IPFLTEN",            1,            1,            0, 0},
		{         22,    "TLTEN"  ,            1,            1,            0, 0},
		{         23,    "IPFLT0" ,            1,           64,            0, 0},
		{         24,    "IPFLT1" ,            1,           64,            0, 0},
		{         25,    "IPFLT2" ,            1,           64,            0, 0},
		{         26,    "IPFLT3" ,            1,           64,            0, 0},
		{         27,    "IPFLT4" ,            1,           64,            0, 0},
		{         28,    "IPFLT5" ,            1,           64,            0, 0},
		{         29,    "IPFLT6" ,            1,           64,            0, 0},
		{         30,    "IPFLT7" ,            1,           64,            0, 0},
		{         31,    "IPFLT8" ,            1,           64,            0, 0},
		{         32,    "IPFLT9" ,            1,           64,            0, 0},
		{         33,    "IPFLT10",            1,           64,            0, 0},
		{         34,    "IPFLT11",            1,           64,            0, 0},
		{         35,    "IPFLT12",            1,           64,            0, 0},
		{         36,    "IPFLT13",            1,           64,            0, 0},
		{         37,    "IPFLT14",            1,           64,            0, 0},
		{         38,    "IPFLT15",            1,           64,            0, 0},
		{         39,    "PTFW0"  ,            1,           56,            0, 0},
		{         40,    "PTFW1"  ,            1,           56,            0, 0},
		{         41,    "PTFW2"  ,            1,           56,            0, 0},
		{         42,    "PTFW3"  ,            1,           56,            0, 0},
		{         43,    "PTFW4"  ,            1,           56,            0, 0},
		{         44,    "PTFW5"  ,            1,           56,            0, 0},
		{         45,    "PTFW6"  ,            1,           56,            0, 0},
		{         46,    "PTFW7"  ,            1,           56,            0, 0},
		{         47,    "PTFW8"  ,            1,           56,            0, 0},
		{         48,    "PTFW9"  ,            1,           56,            0, 0},
		{         49,    "PTFW10" ,            1,           56,            0, 0},
		{         50,    "PTFW11" ,            1,           56,            0, 0},
		{         51,    "PTFW12" ,            1,           56,            0, 0},
		{         52,    "PTFW13" ,            1,           56,            0, 0},
		{         53,    "PTFW14" ,            1,           56,            0, 0},
		{         54,    "PTFW15" ,            1,           56,            0, 0},
		{         55,    "DMZSET" ,            1,            8,            0, 0},
		{         56,    "PTTRG0" ,            1,           70,            0, 0},
		{         57,    "PTTRG1" ,            1,           70,            0, 0},
		{         58,    "PTTRG2" ,            1,           70,            0, 0},
		{         59,    "PTTRG3" ,            1,           70,            0, 0},
		{         60,    "PTTRG4" ,            1,           70,            0, 0},
		{         61,    "PTTRG5" ,            1,           70,            0, 0},
		{         62,    "PTTRG6" ,            1,           70,            0, 0},
		{         63,    "PTTRG7" ,            1,           70,            0, 0},
		{         64,    "PTTRG8" ,            1,           70,            0, 0},
		{         65,    "PTTRG9" ,            1,           70,            0, 0},
		{         66,    "PTTRG10",            1,           70,            0, 0},
		{         67,    "PTTRG11",            1,           70,            0, 0},
		{         68,    "PTTRG12",            1,           70,            0, 0},
		{         69,    "PTTRG13",            1,           70,            0, 0},
		{         70,    "PTTRG14",            1,           70,            0, 0},
		{         71,    "PTTRG15",            1,           70,            0, 0},
		{         72,    "UPNPEN" ,            1,            1,            0, 0},
		{         73,    "WIFIBP1",            1,          104,            0, 0},
		{         74,    "WIFIBP2",            1,           20,            0, 0},
		{         75,    "WIFISP1",            1,          104,            0, 0},
		{         76,    "WIFISP2",            1,          104,            0, 0},
		{         77,    "DIALMOD",            1,            1,            0, 0},
		{         78,    "WIFIPIN",            1,           12,            0, 0},
		{         79,    "MACFBW" ,            1,            4,            0, 0},
		{         80,    "MACFL0" ,            1,           33,            0, 0},
		{         81,    "MACFL1" ,            1,           33,            0, 0},
		{         82,    "MACFL2" ,            1,           33,            0, 0},
		{         83,    "MACFL3" ,            1,           33,            0, 0},
		{         84,    "MACFL4" ,            1,           33,            0, 0},
		{         85,    "MACFL5" ,            1,           33,            0, 0},
		{         86,    "MACFL6" ,            1,           33,            0, 0},
		{         87,    "MACFL7" ,            1,           33,            0, 0},
		{         88,    "MACFL8" ,            1,           33,            0, 0},
		{         89,    "MACFL9" ,            1,           33,            0, 0},
		{         90,    "MACFL10",            1,           33,            0, 0},
		{         91,    "MACFL11",            1,           33,            0, 0},
		{         92,    "MACFL12",            1,           33,            0, 0},
		{         93,    "MACFL13",            1,           33,            0, 0},
		{         94,    "MACFL14",            1,           33,            0, 0},
		{         95,    "MACFL15",            1,           33,            0, 0},
		{         96,    "WANMTU" ,            1,            4,            0, 0},
		{         97,    "ONCONN" ,            1,            8,            0, 0},
		{         98,    "IPOPLEN",            1,            1,            0, 0},
		{         99,    "SDEN"   ,            1,            4,            0, 0},
		{        100,    "DNSSET" ,            1,           36,            0, 0},
		{        101,    "USBPORT",            1,            1,            0, 0},
		{        102,    "WEBD0"  ,            1,           20,            0, 0},
		{        103,    "WEBD1"  ,            1,           20,            0, 0},
		{        104,    "WEBD2"  ,            1,           20,            0, 0},
		{        105,    "WEBD3"  ,            1,           20,            0, 0},
		{        106,    "WEBD4"  ,            1,           20,            0, 0},
		{        107,    "WEBD5"  ,            1,           20,            0, 0},
		{        108,    "WEBD6"  ,            1,           20,            0, 0},
		{        109,    "WEBD7"  ,            1,           20,            0, 0},
		{        110,    "WEBD8"  ,            1,           20,            0, 0},
		{        111,    "WEBD9"  ,            1,           20,            0, 0},
		{        112,    "WEBD10" ,            1,           20,            0, 0},
		{        113,    "WEBD11" ,            1,           20,            0, 0},
		{        114,    "WEBD12" ,            1,           20,            0, 0},
		{        115,    "WEBD13" ,            1,           20,            0, 0},
		{        116,    "WEBD14" ,            1,           20,            0, 0},
		{        117,    "WEBD15" ,            1,           20,            0, 0},
		{        118,    "CURLANG",            1,            4,            0, 0},
		{        119,    "MDATE"  ,            1,           20,            0, "2010-09-31 10:10:10"},
		{        120,    "VER1"   ,            1,          104,            0, 0},
		{        121,    "VER2"   ,            1,          104,            0, 0},
		{        122,    "VER3"   ,            1,           12,            0, 0},
		{        123,    "MEID"   ,            1,           20,            0, 0},
		{        124,    "ESN"    ,            1,           20,            0, 0},
		{        125,    "SVN"    ,            1,           20,            0, 0},
		{        126,    "UMID"   ,            1,           20,            0, 0},
		{        127,    "APPR011",            1,          104,            0, 0},
		{        128,    "APPR012",            1,          104,            0, 0},
		{        129,    "APPR013",            1,           64,            0, 0},
		{        130,    "APPR021",            1,          104,            0, 0},
		{        131,    "APPR022",            1,          104,            0, 0},
		{        132,    "APPR023",            1,           64,            0, 0},
		{        133,    "APPR031",            1,          104,            0, 0},
		{        134,    "APPR032",            1,          104,            0, 0},
		{        135,    "APPR033",            1,           64,            0, 0},
		{        136,    "APPR041",            1,          104,            0, 0},
		{        137,    "APPR042",            1,          104,            0, 0},
		{        138,    "APPR043",            1,           64,            0, 0},
		{        139,    "APPR051",            1,          104,            0, 0},
		{        140,    "APPR052",            1,          104,            0, 0},
		{        141,    "APPR053",            1,           64,            0, 0},
		{        142,    "APPR061",            1,          104,            0, 0},
		{        143,    "APPR062",            1,          104,            0, 0},
		{        144,    "APPR063",            1,           64,            0, 0},
		{        145,    "APPR071",            1,          104,            0, 0},
		{        146,    "APPR072",            1,          104,            0, 0},
		{        147,    "APPR073",            1,           64,            0, 0},
		{        148,    "APPR081",            1,          104,            0, 0},
		{        149,    "APPR082",            1,          104,            0, 0},
		{        150,    "APPR083",            1,           64,            0, 0},
		{        151,    "APPR091",            1,          104,            0, 0},
		{        152,    "APPR092",            1,          104,            0, 0},
		{        153,    "APPR093",            1,           64,            0, 0},
		{        154,    "APPR101",            1,          104,            0, 0},
		{        155,    "APPR102",            1,          104,            0, 0},
		{        156,    "APPR103",            1,           64,            0, 0},
		{        157,    "APPR111",            1,          104,            0, 0},
		{        158,    "APPR112",            1,          104,            0, 0},
		{        159,    "APPR113",            1,           64,            0, 0},
		{        160,    "APPR121",            1,          104,            0, 0},
		{        161,    "APPR122",            1,          104,            0, 0},
		{        162,    "APPR123",            1,           64,            0, 0},
		{        163,    "APPR131",            1,          104,            0, 0},
		{        164,    "APPR132",            1,          104,            0, 0},
		{        165,    "APPR133",            1,           64,            0, 0},
		{        166,    "APPR141",            1,          104,            0, 0},
		{        167,    "APPR142",            1,          104,            0, 0},
		{        168,    "APPR143",            1,           64,            0, 0},
		{        169,    "APPR151",            1,          104,            0, 0},
		{        170,    "APPR152",            1,          104,            0, 0},
		{        171,    "APPR153",            1,           64,            0, 0},
		{        172,    "APPR161",            1,          104,            0, 0},
		{        173,    "APPR162",            1,          104,            0, 0},
		{        174,    "APPR163",            1,           64,            0, 0},
		{        175,    "FAC11"  ,            1,          104,            0, 0},
		{        176,    "FAC12"  ,            1,           24,            0, 0},
		{        177,    "FAC21"  ,            1,          104,            0, 0},
		{        178,    "FAC22"  ,            1,           24,            0, 0},
#ifdef FACTORY_MODE
		{        179,	 "U2DIAG" ,            0,            4,            0, 27},
#else
		{        179,	 "U2DIAG" ,            0,            4,            0, 28},
#endif
		{        180,    "BTIME"  ,            1,           32,            0, 0},
		{        181,    "ESWVR"  ,            0,           32,            0, "V100R001C01B001SP06"},
		{        182,    "ISWVR"  ,            0,           32,            0, "V100R001C01B001SP06"},
		{        183,    "EDBVR"  ,            0,           32,            0, 0},
		{        184,    "IDBVR"  ,            0,           32,            0, 0},
		{        185,    "EHWVR"  ,            0,           32,            0, "HS1T830M"},
		{        186,    "IHWVR"  ,            0,           32,            0, "HS1T830M"},
		{        187,    "EDUTVR" ,            0,           32,            0, 0},
		{        188,    "IDUTVR" ,            0,           32,            0, 0},
		{        189,    "CFGVR"  ,            0,           32,            0, 0},
		{        190,    "PRLVR"  ,            0,           32,            0, 0},
		{        191,    "SWUPFLG",            0,            1,            0, 0},
		{        192,	 "TMMI"   ,            1,            4,            0, 0},
		{        193,    "MACWLAN",            1,           20,            0, 0},
		{        194,    "MACBT"  ,            1,           20,            0, 0},
		{        195,    "SSID"   ,            1,           20,            0, 0},
		{        196,    "WIFIKEY",            1,           64,            0, 0},
		{        197,    "BDINFO" ,            1,          104,            0, 0},
		{        198,    "MFLAG" ,             1,          104,            0, 0},
		{        199,    "BATPARA",            1,           10,            0, 0},
		{        200,    "BOOTINF",            1,           16,            0, 0},
		{        201,    "PSEN"   ,            1,          104,            0, 0},

		/*Key B*/
		{        202,    "KEYB1"   ,            0,          64,            0, 		{ 0xaf, 0x51, 0x05, 0xfa, 0x34, 0x3e, 0x9d, 0x8e, 0x72, 0x29, 0x4f, 0xb8, 0xe7, 
                                                                                                            0x52, 0xa7, 0x03,0xf5, 0x4f, 0x9b, 0x40, 0x38, 0x26, 0xf8, 0xdd, 0x06, 0xcf, 
                                                                                                            0x26, 0x28, 0xec, 0xe4, 0x96, 0x80,0x6e, 0x18, 0x2a, 0xb0, 0xe8, 0x85, 0x91, 
                                                                                                            0xf6, 0xc0, 0xee, 0x78, 0x73, 0xcb, 0x69, 0x40, 0x9e,0x73, 0x5c, 0x62, 0x10, 
                                                                                                            0x5d, 0xd2, 0xe2, 0x8b, 0xd4, 0x54, 0x28, 0x80, 0x68, 0x36, 0xcd, 0xb8}/*keyB_Modulus1*/},
		{        203,    "KEYB2"   ,            0,          64,            0, 		{ 0xd9, 0x4b, 0x20, 0x4a, 0xce, 0x06, 0xd3, 0x42, 0xd2, 0x4e, 0xd8, 0x24, 0xc6, 
                                                                                                            0x98, 0x8b, 0x7d,0xb3, 0xbd, 0x84, 0x0b, 0x50, 0x07, 0x1d, 0x29, 0x1a, 0xa4, 
                                                                                                            0xa8, 0xcd, 0xa9, 0x18, 0x7a, 0x3f,0x69, 0x86, 0x16, 0xfb, 0x8a, 0xe3, 0x98, 
                                                                                                            0xf0, 0x01, 0x1a, 0x3e, 0x38, 0xef, 0x31, 0x31, 0x2f,0x07, 0xab, 0xa3, 0x16, 
                                                                                                            0xb3, 0x58, 0x58, 0xd8, 0xe5, 0xfe, 0x7e, 0x7e, 0xf8, 0xc0, 0x12, 0x09}/*keyB_Modulus2*/},
		{        204,    "KEYB3"   ,            0,          64,            0, 		{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                                                                                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                                                                                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                                                                                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                                                                                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                                                                                            0x00, 0x00, 0x00, 0x00}/*keyB_public_exponent1*/},
		{        205,    "KEYB4"   ,            0,          64,            0, 		{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00, 
                                                                                                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00, 
                                                                                                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00, 
                                                                                                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00, 
                                                                                                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00, 
                                                                                                            0x00, 0x01, 0x00, 0x01}/*keyB_public_exponent2*/},
#ifdef FACTORY_MODE
		{        206,    "NOGUI"  ,            0,            1,            0, 1},
#else
		{        206,    "NOGUI"  ,            0,            1,            0, 0},
#endif
              {        207,    "FELICA"  ,            0,            20,            0, 0},
              {        208,    "CAMID"  ,       0,            32,            0, 0},
              {        209,    "LIFETC"  ,       0,            64,            0, 0},
	      {        210,    "USBMODE",        0,            20,            0, "imgstate"},
              {        211,    "LOGCTL"  ,       0,            1,            0, 1},
              {        212,    "WARRANT"  ,       1,            8,            0, 0},
              {        213,    "REFURBI"  ,       1,            16,            0, 0},
              {        214,    "VOLTAGE"  ,       0,            8,            0, "4200"},
              {        215,    "LIFEDS"  ,       1,            64,            0, 0},
	},
};

int main(int argc, char *argv[])
{

	FILE 	*file1;
	size_t 	wcount = 0;
	unsigned int	i = 0; 
	if(argc != 2){
		printf("USAGE:\"exec\" filename\n");
		return -1;
	}
	
	printf("Parameter check passed!\n");

	for(i=0; i<nve_partition.header.valid_items; i++){
		
		if(strnlen(nve_partition.NVs[i].nv_name,sizeof(nve_partition.NVs[i].nv_name)) == sizeof(nve_partition.NVs[i].nv_name)){

			printf("ERROR:The string nvname in NVs[%d] is too long\n",i);
		
			return -1;
		}
		
		/*
		 * nv_number in struct NVs should start from 0 to 1022(MAXIMUM), or valid_items in header woule be adjusted
		 */
		if(nve_partition.NVs[i].nv_number != i){
			nve_partition.header.valid_items = i;
			break;
		}	
	}
	
	printf("Input check passed and %d valid items left\n", nve_partition.header.valid_items);
	
	file1 = fopen(argv[1], "w");
	if(file1 == NULL){
		printf("fopen file %s failed!\n",argv[1]);
		return -1;
	}
	
	printf("fopen file %s succeed!\n",argv[1]);

	wcount = fwrite(&nve_partition, sizeof(nve_partition), 1, file1);
	if(wcount != 1){
		printf("fwrite file %s failed!\n",argv[1]);
		fclose(file1);
		return -1;
	}
	
	printf("fwrite file %s succeed and %d write succeed!\n\n\n",argv[1], (int)sizeof(nve_partition));
	fclose(file1);
			
	return 0;
}
