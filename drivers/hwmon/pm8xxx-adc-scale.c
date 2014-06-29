/*
 * Copyright (c) 2011, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

  

#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/module.h>
#include <linux/mfd/pm8xxx/pm8xxx-adc.h>
#include <hsad/config_interface.h>
#define KELVINMIL_DEGMIL	273160
#define BATT_NTC_TYPE_LEN   20

/* Units for temperature below (on x axis) is in 0.1DegC as
   required by the battery driver. Note the resolution used
   here to compute the table was done for DegC to milli-volts.
   In consideration to limit the size of the table for the given
   temperature range below, the result is linearly interpolated
   and provided to the battery driver in the units desired for
   their framework which is 0.1DegC. True resolution of 0.1DegC
   will result in the below table size to increase by 10 times */

static const struct pm8xxx_adc_map_pt adcmap_btm_threshold_universal[] = {
	{-400,        1669.835617},
	{-390,        1663.227791},
	{-380,        1656.387482},
	{-370,        1649.311539},
	{-360,        1641.996988},
	{-350,        1634.44104},
	{-340,        1626.641103},
	{-330,        1618.594787},
	{-320,        1610.299918},
	{-310,        1601.754545},
	{-300,        1592.956947},
	{-290,        1583.962255},
	{-280,        1574.701368},
	{-270,        1565.172694},
	{-260,        1555.374977},
	{-250,        1545.307309},
	{-240,        1534.96915},
	{-230,        1524.360338},
	{-220,        1513.481107},
	{-210,        1502.332094},
	{-200,        1490.914357},
	{-190,        1479.324416},
	{-180,        1467.448178},
	{-170,        1455.287678},
	{-160,        1442.84556},
	{-150,        1430.125096},
	{-140,        1417.130192},
	{-130,        1403.865399},
	{-120,        1390.335917},
	{-110,        1376.5476},
	{-100,        1362.50695},
	{-90,        1348.317526},
	{-80,        1333.866924},
	{-70,        1319.164112},
	{-60,        1304.218863},
	{-50,        1289.041734},
	{-40,        1273.644042},
	{-30,        1258.037838},
	{-20,        1242.235865},
	{-10,        1226.251524},
	{0,          1210.098827},
	{10,         1193.715343},
	{20,         1177.214938},
	{30,         1160.611637},
	{40,         1143.919692},
	{50,         1127.153533},
	{60,         1110.327713},
	{70,         1093.456862},
	{80,         1076.55563},
	{90,         1059.638638},
	{100,        1042.720431},
	{110,        1025.818537},
	{120,        1008.942987},
	{130,        992.1079423},
	{140,        975.3272995},
	{150,        958.6146512},
	{160,        941.9832436},
	{170,        925.4459389},
	{180,        909.0151812},
	{190,        892.7029646},
	{200,        876.5208051},
	{210,        860.4797154},
	{220,        844.5901836},
	{230,        828.862155},
	{240,        813.3050168},
	{250,        797.9275875},
	{260,        782.7381076},
	{270,        767.7442354},
	{280,        752.9530441},
	{290,        738.371023},
	{300,        724.004081},
	{310,        709.8598514},
	{320,        695.940906},
	{330,        682.2511993},
	{340,        668.7941573},
	{350,        655.5726923},
	{360,        642.5892195},
	{370,        629.8456739},
	{380,        617.3435292},
	{390,        605.0838167},
	{400,        593.0671454},
	{410,        581.301536},
	{420,        569.77786},
	{430,        558.4953172},
	{440,        547.452793},
	{450,        536.648879},
	{460,        526.0818929},
	{470,        515.749899},
	{480,        505.6507268},
	{490,        495.7819909},
	{500,        486.1411085},
	{510,        476.7287788},
	{520,        467.5379467},
	{530,        458.5654933},
	{540,        449.8081766},
	{550,        441.2626461},
	{560,        432.9254566},
	{570,        424.7930815},
	{580,        416.8619253},
	{590,        409.1283354},
	{600,        401.5886131},
	{610,        394.235202},
	{620,        387.0689872},
	{630,        380.0862087},
	{640,        373.2830928},
	{650,        366.6558609},
	{660,        360.2007364},
	{670,        353.9139518},
	{680,        347.7917552},
	{690,        341.8304161},
	{700,        336.0262307},
	{710,        330.3716914},
	{720,        324.8678714},
	{730,        319.5111709},
	{740,        314.2980308},
	{750,        309.224936},
	{760,        304.2884186},
	{770,        299.4850606},
	{780,        294.8114964}
};

static const struct pm8xxx_adc_map_pt adcmap_btm_threshold_old[] = {
	{-400,        1728},
	{-390,        1723},
	{-380,        1718},
	{-370,        1713},
	{-360,        1708},
	{-350,        1702},
	{-340,        1696},
	{-330,        1690},
	{-320,        1683},
	{-310,        1676},
	{-300,        1669},
	{-290,        1661},
	{-280,        1653},
	{-270,        1645},
	{-260,        1636},
	{-250,        1627},
	{-240,        1618},
	{-230,        1608},
	{-220,        1598},
	{-210,        1587},
	{-200,        1576},
	{-190,        1565},
	{-180,        1553},
	{-170,        1541},
	{-160,        1528},
	{-150,        1515},
	{-140,        1502},
	{-130,        1488},
	{-120,        1474},
	{-110,        1459},
	{-100,        1444},
	{-90,         1429},
	{-80,         1413},
	{-70,         1397},
	{-60,         1380},
	{-50,         1364},
	{-40,         1347},
	{-30,         1329},
	{-20,         1311},
	{-10,         1293},
	{0,           1275},
	{10,          1257},
	{20,          1238},
	{30,          1219},
	{40,          1200},
	{50,          1181},
	{60,          1162},
	{70,          1142},
	{80,          1123},
	{90,          1103},
	{100,         1083},
	{110,         1064},
	{120,         1044},
	{130,         1024},
	{140,         1005},
	{150,          985},
	{160,          966},
	{170,          947},
	{180,          927},
	{190,          908},
	{200,          889},
	{210,          871},
	{220,          852},
	{230,          834},
	{240,          816},
	{250,          798},
	{260,          780},
	{270,          763},
	{280,          746},
	{290,          729},
	{300,          713},
	{310,          696},
	{320,          681},
	{330,          665},
	{340,          650},
	{350,          635},
	{360,          620},
	{370,          606},
	{380,          592},
	{390,          578},
	{400,          565},
	{410,          552},
	{420,          539},
	{430,          527},
	{440,          514},
	{450,          503},
	{460,          491},
	{470,          480},
	{480,          469},
	{490,          458},
	{500,          448},
	{510,          438},
	{520,          428},
	{530,          419},
	{540,          410},
	{550,          401},
	{560,          392},
	{570,          384},
	{580,          376},
	{590,          368},
	{600,          360},
	{610,          353},
	{620,          345},
	{630,          338},
	{640,          332},
	{650,          325},
	{660,          319},
	{670,          312},
	{680,          307},
	{690,          301},
	{700,          295},
	{710,          290},
	{720,          284},
	{730,          279},
	{740,          274},
	{750,          269},
	{760,          265},
	{770,          260},
	{780,          256}
};
char old_ntc_type[] = "OLD_NTC";

static const struct pm8xxx_adc_map_pt adcmap_pa_therm[] = {
	{1763.850005,      -40},
	{1761.224775,      -39},
	{1758.432361,      -38},
	{1755.463965,      -37},
	{1752.310483,      -36},
	{1748.962498,      -35},
	{1745.403401,      -34},
	{1741.629465,      -33},
	{1737.630377,      -32},
	{1733.39555,       -31},
	{1728.914145,      -30},
	{1724.166597,      -29},
	{1719.149433,      -28},
	{1713.851144,      -27},
	{1708.260073,      -26},
	{1702.364419,      -25},
	{1696.142126,      -24},
	{1689.590733,      -23},
	{1682.698284,      -22},
	{1675.452844,      -21},
	{1667.842581,      -20},
	{1659.843997,      -19},
	{1651.456788,      -18},
	{1642.669757,      -17},
	{1633.472055,      -16},
	{1623.853245,      -15},
	{1613.790111,      -14},
	{1603.286041,      -13},
	{1592.332341,      -12},
	{1580.921082,      -11},
	{1569.045097,      -10},
	{1556.683872,      -9},
	{1543.846214,      -8},
	{1530.52803,       -7},
	{1516.726364,      -6},
	{1502.439566,      -5},
	{1487.652559,      -4},
	{1472.381069,      -3},
	{1456.627646,      -2},
	{1440.396379,      -1},
	{1423.692908,      0},
	{1406.510187,      1},
	{1388.871764,      2},
	{1370.78816,       3},
	{1352.271699,      4},
	{1333.336214,      5},
	{1313.984467,      6},
	{1294.247155,      7},
	{1274.143028,      8},
	{1253.692332,      9},
	{1232.916667,      10},
	{1211.828649,      11},
	{1190.463875,      12},
	{1168.848021,      13},
	{1147.007007,      14},
	{1124.968274,      15},
	{1102.752578,      16},
	{1080.396837,      17},
	{1057.930252,      18},
	{1035.381557,      19},
	{1012.78048,       20},
	{990.1512797,      21},
	{967.5299689,      22},
	{944.945647,       23},
	{922.4266787,      24},
	{900,      	   25},
	{877.693085,       26},
	{855.531594,       27},
	{833.540442,       28},
	{811.7441398,      29},
	{790.164577,       30},
	{768.8312045,      31},
	{747.7585503,      32},
	{726.9665623,      33},
	{706.4717432,      34},
	{686.2902328,      35},
	{666.4454416,      36},
	{646.9432465,      37},
	{627.7953174,      38},
	{609.012336,       39},
	{590.6037978,      40},
	{572.5866781,      41},
	{554.9588376,      42},
	{537.7249596,      43},
	{520.8883667,      44},
	{504.4525088,      45},
	{488.4315779,      46},
	{472.8136694,      47},
	{457.5993302,      48},
	{442.7857275,      49},
	{428.3721724,      50},
	{414.3841297,      51},
	{400.7897764,      52},
	{387.5846599,      53},
	{374.7631731,      54},
	{362.3217545,      55},
	{350.2522829,      56},
	{338.5490173,      57},
	{327.2065855,      58},
	{316.2161093,      59},
	{305.5725539,      60},
	{295.2734868,      61},
	{285.3049334,      62},
	{275.6588118,      63},
	{266.3282364,      64},
	{257.3050605,      65},
	{248.5861448,      66},
	{240.1575094,      67},
	{232.0129527,      68},
	{224.1426023,      69},
	{216.5402342,      70},
	{209.2007117,      71},
	{202.1125256,      72},
	{195.2690153,      73},
	{188.6609474,      74},
	{182.2822762,      75},
	{176.1283652,      76},
	{170.188207,       77},
	{164.4561181,      78},
	{158.923989,       79},
	{153.586255,       80},
	{148.4389996,      81},
	{143.4718988,      82},
	{138.6792993,      83},
	{134.0562706,      84},
	{129.5955456,      85},
	{125.2947481,      86},
	{121.1460976,      87},
	{117.1425829,      88},
	{113.2811617,      89},
	{109.5554569,      90},
	{105.9631352,      91},
	{102.4974663,      92},
	{99.15367147,      93},
	{95.92789882,      94},
	{92.81509101,      95},
	{89.81330462,      96},
	{86.91725049,      97},
	{84.12260516,      98},
	{81.42603449,      99},
	{78.82302093,      100},
	{76.31335346,      101},
	{73.89126282,      102},
	{71.55419926,      103},
	{69.29733472,      104},
	{67.11798616,      105},
	{65.01787674,      106},
	{62.98978059,      107},
	{61.03198091,      108},
	{59.14047963,      109},
	{57.31459925,      110},
	{55.55476589,      111},
	{53.8557452,       112},
	{52.21337756,      113},
	{50.62685871,      114},
	{49.09423051,      115},
	{47.61919597,      116},
	{46.19293956,      117},
	{44.81456466,      118},
	{43.48201716,      119},
	{42.19436643,      120},
	{40.95639852,      121},
	{39.7591469,       122},
	{38.60048328,      123},
	{37.48056333,      124},
	{36.39723389,      125}
};

static const struct pm8xxx_adc_map_pt adcmap_ntcg_104ef_104fb[] = {
	{696483,	-40960},
	{649148,	-39936},
	{605368,	-38912},
	{564809,	-37888},
	{527215,	-36864},
	{492322,	-35840},
	{460007,	-34816},
	{429982,	-33792},
	{402099,	-32768},
	{376192,	-31744},
	{352075,	-30720},
	{329714,	-29696},
	{308876,	-28672},
	{289480,	-27648},
	{271417,	-26624},
	{254574,	-25600},
	{238903,	-24576},
	{224276,	-23552},
	{210631,	-22528},
	{197896,	-21504},
	{186007,	-20480},
	{174899,	-19456},
	{164521,	-18432},
	{154818,	-17408},
	{145744,	-16384},
	{137265,	-15360},
	{129307,	-14336},
	{121866,	-13312},
	{114896,	-12288},
	{108365,	-11264},
	{102252,	-10240},
	{96499,		-9216},
	{91111,		-8192},
	{86055,		-7168},
	{81308,		-6144},
	{76857,		-5120},
	{72660,		-4096},
	{68722,		-3072},
	{65020,		-2048},
	{61538,		-1024},
	{58261,		0},
	{55177,		1024},
	{52274,		2048},
	{49538,		3072},
	{46962,		4096},
	{44531,		5120},
	{42243,		6144},
	{40083,		7168},
	{38045,		8192},
	{36122,		9216},
	{34308,		10240},
	{32592,		11264},
	{30972,		12288},
	{29442,		13312},
	{27995,		14336},
	{26624,		15360},
	{25333,		16384},
	{24109,		17408},
	{22951,		18432},
	{21854,		19456},
	{20807,		20480},
	{19831,		21504},
	{18899,		22528},
	{18016,		23552},
	{17178,		24576},
	{16384,		25600},
	{15631,		26624},
	{14916,		27648},
	{14237,		28672},
	{13593,		29696},
	{12976,		30720},
	{12400,		31744},
	{11848,		32768},
	{11324,		33792},
	{10825,		34816},
	{10354,		35840},
	{9900,		36864},
	{9471,		37888},
	{9062,		38912},
	{8674,		39936},
	{8306,		40960},
	{7951,		41984},
	{7616,		43008},
	{7296,		44032},
	{6991,		45056},
	{6701,		46080},
	{6424,		47104},
	{6160,		48128},
	{5908,		49152},
	{5667,		50176},
	{5439,		51200},
	{5219,		52224},
	{5010,		53248},
	{4810,		54272},
	{4619,		55296},
	{4440,		56320},
	{4263,		57344},
	{4097,		58368},
	{3938,		59392},
	{3785,		60416},
	{3637,		61440},
	{3501,		62464},
	{3368,		63488},
	{3240,		64512},
	{3118,		65536},
	{2998,		66560},
	{2889,		67584},
	{2782,		68608},
	{2680,		69632},
	{2581,		70656},
	{2490,		71680},
	{2397,		72704},
	{2310,		73728},
	{2227,		74752},
	{2147,		75776},
	{2064,		76800},
	{1998,		77824},
	{1927,		78848},
	{1860,		79872},
	{1795,		80896},
	{1736,		81920},
	{1673,		82944},
	{1615,		83968},
	{1560,		84992},
	{1507,		86016},
	{1456,		87040},
	{1407,		88064},
	{1360,		89088},
	{1314,		90112},
	{1271,		91136},
	{1228,		92160},
	{1189,		93184},
	{1150,		94208},
	{1112,		95232},
	{1076,		96256},
	{1042,		97280},
	{1008,		98304},
	{976,		99328},
	{945,		100352},
	{915,		101376},
	{886,		102400},
	{859,		103424},
	{832,		104448},
	{807,		105472},
	{782,		106496},
	{756,		107520},
	{735,		108544},
	{712,		109568},
	{691,		110592},
	{670,		111616},
	{650,		112640},
	{631,		113664},
	{612,		114688},
	{594,		115712},
	{577,		116736},
	{560,		117760},
	{544,		118784},
	{528,		119808},
	{513,		120832},
	{498,		121856},
	{483,		122880},
	{470,		123904},
	{457,		124928},
	{444,		125952},
	{431,		126976},
	{419,		128000}
};

static int32_t pm8xxx_adc_map_linear(const struct pm8xxx_adc_map_pt *pts,
		uint32_t tablesize, int32_t input, int64_t *output)
{
	bool descending = 1;
	uint32_t i = 0;

	if ((pts == NULL) || (output == NULL))
		return -EINVAL;

	/* Check if table is descending or ascending */
	if (tablesize > 1) {
		if (pts[0].x < pts[1].x)
			descending = 0;
	}

	while (i < tablesize) {
		if ((descending == 1) && (pts[i].x < input)) {
			/* table entry is less than measured
				value and table is descending, stop */
			break;
		} else if ((descending == 0) &&
				(pts[i].x > input)) {
			/* table entry is greater than measured
				value and table is ascending, stop */
			break;
		} else {
			i++;
		}
	}

	if (i == 0)
		*output = pts[0].y;
	else if (i == tablesize)
		*output = pts[tablesize-1].y;
	else {
		/* result is between search_index and search_index-1 */
		/* interpolate linearly */
		*output = (((int32_t) ((pts[i].y - pts[i-1].y)*
			(input - pts[i-1].x))/
			(pts[i].x - pts[i-1].x))+
			pts[i-1].y);
	}

	return 0;
}

static int32_t pm8xxx_adc_map_batt_therm(const struct pm8xxx_adc_map_pt *pts,
		uint32_t tablesize, int32_t input, int64_t *output)
{
	bool descending = 1;
	uint32_t i = 0;

	if ((pts == NULL) || (output == NULL))
		return -EINVAL;

	/* Check if table is descending or ascending */
	if (tablesize > 1) {
		if (pts[0].y < pts[1].y)
			descending = 0;
	}

	while (i < tablesize) {
		if ((descending == 1) && (pts[i].y < input)) {
			/* table entry is less than measured
				value and table is descending, stop */
			break;
		} else if ((descending == 0) && (pts[i].y > input)) {
			/* table entry is greater than measured
				value and table is ascending, stop */
			break;
		} else {
			i++;
		}
	}

	if (i == 0) {
		*output = pts[0].x;
	} else if (i == tablesize) {
		*output = pts[tablesize-1].x;
	} else {
		/* result is between search_index and search_index-1 */
		/* interpolate linearly */
		*output = (((int32_t) ((pts[i].x - pts[i-1].x)*
			(input - pts[i-1].y))/
			(pts[i].y - pts[i-1].y))+
			pts[i-1].x);
	}

	return 0;
}

int32_t pm8xxx_adc_scale_default(int32_t adc_code,
		const struct pm8xxx_adc_properties *adc_properties,
		const struct pm8xxx_adc_chan_properties *chan_properties,
		struct pm8xxx_adc_chan_result *adc_chan_result)
{
	bool negative_rawfromoffset = 0, negative_offset = 0;
	int64_t scale_voltage = 0;

	if (!chan_properties || !chan_properties->offset_gain_numerator ||
		!chan_properties->offset_gain_denominator || !adc_properties
		|| !adc_chan_result)
		return -EINVAL;

	scale_voltage = (adc_code -
		chan_properties->adc_graph[ADC_CALIB_ABSOLUTE].adc_gnd)
		* chan_properties->adc_graph[ADC_CALIB_ABSOLUTE].dx;
	if (scale_voltage < 0) {
		negative_offset = 1;
		scale_voltage = -scale_voltage;
	}
	do_div(scale_voltage,
		chan_properties->adc_graph[ADC_CALIB_ABSOLUTE].dy);
	if (negative_offset)
		scale_voltage = -scale_voltage;
	scale_voltage += chan_properties->adc_graph[ADC_CALIB_ABSOLUTE].dx;

	if (scale_voltage < 0) {
		if (adc_properties->bipolar) {
			scale_voltage = -scale_voltage;
			negative_rawfromoffset = 1;
		} else {
			scale_voltage = 0;
		}
	}

	adc_chan_result->measurement = scale_voltage *
				chan_properties->offset_gain_denominator;

	/* do_div only perform positive integer division! */
	do_div(adc_chan_result->measurement,
				chan_properties->offset_gain_numerator);

	if (negative_rawfromoffset)
		adc_chan_result->measurement = -adc_chan_result->measurement;

	/* Note: adc_chan_result->measurement is in the unit of
	 * adc_properties.adc_reference. For generic channel processing,
	 * channel measurement is a scale/ratio relative to the adc
	 * reference input */
	adc_chan_result->physical = adc_chan_result->measurement;

	return 0;
}
EXPORT_SYMBOL_GPL(pm8xxx_adc_scale_default);

static int64_t pm8xxx_adc_scale_ratiometric_calib(int32_t adc_code,
		const struct pm8xxx_adc_properties *adc_properties,
		const struct pm8xxx_adc_chan_properties *chan_properties)
{
	int64_t adc_voltage = 0;
	bool negative_offset = 0;

	if (!chan_properties || !chan_properties->offset_gain_numerator ||
		!chan_properties->offset_gain_denominator || !adc_properties)
		return -EINVAL;

	adc_voltage = (adc_code -
		chan_properties->adc_graph[ADC_CALIB_RATIOMETRIC].adc_gnd)
		* adc_properties->adc_vdd_reference;
	if (adc_voltage < 0) {
		negative_offset = 1;
		adc_voltage = -adc_voltage;
	}
	do_div(adc_voltage,
		chan_properties->adc_graph[ADC_CALIB_RATIOMETRIC].dy);
	if (negative_offset)
		adc_voltage = -adc_voltage;

	return adc_voltage;
}

int32_t pm8xxx_adc_scale_batt_therm(int32_t adc_code,
		const struct pm8xxx_adc_properties *adc_properties,
		const struct pm8xxx_adc_chan_properties *chan_properties,
		struct pm8xxx_adc_chan_result *adc_chan_result)
{
	int64_t bat_voltage = 0;
    char batt_type[BATT_NTC_TYPE_LEN+1];
    int is_old_ntc_type;

    memset(batt_type, 0, BATT_NTC_TYPE_LEN+1);

	bat_voltage = pm8xxx_adc_scale_ratiometric_calib(adc_code,
			adc_properties, chan_properties);

	get_hw_config("pm/batt_ntc_type", batt_type, BATT_NTC_TYPE_LEN, NULL);
	is_old_ntc_type = (strncasecmp(batt_type, old_ntc_type, BATT_NTC_TYPE_LEN)==0)? 1:0;
	if(is_old_ntc_type)
	{
	    return pm8xxx_adc_map_batt_therm(
			adcmap_btm_threshold_old,
			ARRAY_SIZE(adcmap_btm_threshold_old),
			bat_voltage,
			&adc_chan_result->physical);
	}
	else
    {
	    return pm8xxx_adc_map_batt_therm(
                adcmap_btm_threshold_universal,
                ARRAY_SIZE(adcmap_btm_threshold_universal),
                bat_voltage,
                &adc_chan_result->physical);
	}
}
EXPORT_SYMBOL_GPL(pm8xxx_adc_scale_batt_therm);

int32_t pm8xxx_adc_scale_pa_therm(int32_t adc_code,
		const struct pm8xxx_adc_properties *adc_properties,
		const struct pm8xxx_adc_chan_properties *chan_properties,
		struct pm8xxx_adc_chan_result *adc_chan_result)
{
	int64_t pa_voltage = 0;

	pa_voltage = pm8xxx_adc_scale_ratiometric_calib(adc_code,
			adc_properties, chan_properties);

	return pm8xxx_adc_map_linear(
			adcmap_pa_therm,
			ARRAY_SIZE(adcmap_pa_therm),
			pa_voltage,
			&adc_chan_result->physical);
}
EXPORT_SYMBOL_GPL(pm8xxx_adc_scale_pa_therm);

int32_t pm8xxx_adc_scale_batt_id(int32_t adc_code,
		const struct pm8xxx_adc_properties *adc_properties,
		const struct pm8xxx_adc_chan_properties *chan_properties,
		struct pm8xxx_adc_chan_result *adc_chan_result)
{
	int64_t batt_id_voltage = 0;

	batt_id_voltage = pm8xxx_adc_scale_ratiometric_calib(adc_code,
			adc_properties, chan_properties);
	adc_chan_result->physical = batt_id_voltage;
	adc_chan_result->physical = adc_chan_result->measurement;

	return 0;
}
EXPORT_SYMBOL_GPL(pm8xxx_adc_scale_batt_id);

int32_t pm8xxx_adc_scale_pmic_therm(int32_t adc_code,
		const struct pm8xxx_adc_properties *adc_properties,
		const struct pm8xxx_adc_chan_properties *chan_properties,
		struct pm8xxx_adc_chan_result *adc_chan_result)
{
	int64_t pmic_voltage = 0;
	bool negative_offset = 0;

	if (!chan_properties || !chan_properties->offset_gain_numerator ||
		!chan_properties->offset_gain_denominator || !adc_properties
		|| !adc_chan_result)
		return -EINVAL;

	pmic_voltage = (adc_code -
		chan_properties->adc_graph[ADC_CALIB_ABSOLUTE].adc_gnd)
		* chan_properties->adc_graph[ADC_CALIB_ABSOLUTE].dx;
	if (pmic_voltage < 0) {
		negative_offset = 1;
		pmic_voltage = -pmic_voltage;
	}
	do_div(pmic_voltage,
		chan_properties->adc_graph[ADC_CALIB_ABSOLUTE].dy);
	if (negative_offset)
		pmic_voltage = -pmic_voltage;
	pmic_voltage += chan_properties->adc_graph[ADC_CALIB_ABSOLUTE].dx;

	if (pmic_voltage > 0) {
		/* 2mV/K */
		adc_chan_result->measurement = pmic_voltage*
			chan_properties->offset_gain_denominator;

		do_div(adc_chan_result->measurement,
			chan_properties->offset_gain_numerator * 2);
	} else {
		adc_chan_result->measurement = 0;
	}
	/* Change to .001 deg C */
	adc_chan_result->measurement -= KELVINMIL_DEGMIL;
	adc_chan_result->physical = (int32_t)adc_chan_result->measurement;

	return 0;
}
EXPORT_SYMBOL_GPL(pm8xxx_adc_scale_pmic_therm);

/* Scales the ADC code to 0.001 degrees C using the map
 * table for the XO thermistor.
 */
int32_t pm8xxx_adc_tdkntcg_therm(int32_t adc_code,
		const struct pm8xxx_adc_properties *adc_properties,
		const struct pm8xxx_adc_chan_properties *chan_properties,
		struct pm8xxx_adc_chan_result *adc_chan_result)
{
	int64_t xo_thm = 0;

	if (!chan_properties || !chan_properties->offset_gain_numerator ||
		!chan_properties->offset_gain_denominator || !adc_properties
		|| !adc_chan_result)
		return -EINVAL;

	xo_thm = pm8xxx_adc_scale_ratiometric_calib(adc_code,
			adc_properties, chan_properties);
	xo_thm <<= 4;
	pm8xxx_adc_map_linear(adcmap_ntcg_104ef_104fb,
		ARRAY_SIZE(adcmap_ntcg_104ef_104fb),
		xo_thm, &adc_chan_result->physical);

	return 0;
}
EXPORT_SYMBOL_GPL(pm8xxx_adc_tdkntcg_therm);

int32_t pm8xxx_adc_batt_scaler(struct pm8xxx_adc_arb_btm_param *btm_param,
		const struct pm8xxx_adc_properties *adc_properties,
		const struct pm8xxx_adc_chan_properties *chan_properties)
{
	int rc;
	char batt_type[BATT_NTC_TYPE_LEN+1];
    int is_old_ntc_type;

    get_hw_config("pm/batt_ntc_type", batt_type, BATT_NTC_TYPE_LEN, NULL);
	is_old_ntc_type = (strncasecmp(batt_type, old_ntc_type, BATT_NTC_TYPE_LEN)==0)? 1:0;

	if(is_old_ntc_type)
	{
	    rc = pm8xxx_adc_map_linear(
		    adcmap_btm_threshold_old,
		    ARRAY_SIZE(adcmap_btm_threshold_old),
		    (btm_param->low_thr_temp),
		    &btm_param->low_thr_voltage);
	}
	else
	{
        rc = pm8xxx_adc_map_linear(
            adcmap_btm_threshold_universal,
            ARRAY_SIZE(adcmap_btm_threshold_universal),
            (btm_param->low_thr_temp),
            &btm_param->low_thr_voltage);
	}
	if (rc)
		return rc;

	btm_param->low_thr_voltage *=
		chan_properties->adc_graph[ADC_CALIB_RATIOMETRIC].dy;
	do_div(btm_param->low_thr_voltage, adc_properties->adc_vdd_reference);
	btm_param->low_thr_voltage +=
		chan_properties->adc_graph[ADC_CALIB_RATIOMETRIC].adc_gnd;

	if(is_old_ntc_type)
	{
        rc = pm8xxx_adc_map_linear(
            adcmap_btm_threshold_old,
            ARRAY_SIZE(adcmap_btm_threshold_old),
            (btm_param->high_thr_temp),
            &btm_param->high_thr_voltage);
	}
	else
	{
        rc = pm8xxx_adc_map_linear(
            adcmap_btm_threshold_universal,
            ARRAY_SIZE(adcmap_btm_threshold_universal),
            (btm_param->high_thr_temp),
            &btm_param->high_thr_voltage);
	}
	if (rc)
		return rc;

	btm_param->high_thr_voltage *=
		chan_properties->adc_graph[ADC_CALIB_RATIOMETRIC].dy;
	do_div(btm_param->high_thr_voltage, adc_properties->adc_vdd_reference);
	btm_param->high_thr_voltage +=
		chan_properties->adc_graph[ADC_CALIB_RATIOMETRIC].adc_gnd;


	return rc;
}
EXPORT_SYMBOL_GPL(pm8xxx_adc_batt_scaler);
