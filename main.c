/**
  ******************************************************************************
  * @file    main.c
  * @author  3S0 FreeRTOs
  * @version V1.0
  * @date    24/10/2017
  * @brief   FreeRTOS Example project.
  ******************************************************************************
*/

//Como criar estrutura e ler determinado item de uma Queue???

/*
 *
 * Messages Queues
 * 2017-2018
 *
 */

/* Standard includes. */
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

#include <lcd.h>

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "lcd.h"
#include "queue.h"
#include "semphr.h"

#include "math.h"



/* Task priorities. */
//#define mainFLASH_TASK_PRIORITY	( tskIDLE_PRIORITY + 1)
#define mainLCD_TASK_PRIORITY   ( tskIDLE_PRIORITY + 1)
#define mainUSART_TASK_PRIORITY	( tskIDLE_PRIORITY + 1)
#define mainMUSIC_TASK_PRIORITY	( tskIDLE_PRIORITY + 1)

///* The rate at which the flash task toggles the LED. */
//#define mainFLASH_DELAY			( ( TickType_t ) 1000 / portTICK_RATE_MS )
///* The rate at which the temperature is read. */
//#define mainTEMP_DELAY			( ( TickType_t ) 100 / portTICK_RATE_MS )

/* Configure RCC clocks */
static void prvSetupRCC( void );

/* Configure GPIO. */
static void prvSetupGPIO( void );


// ADDED
void Timer3_Interrupt_func( void ); // Music Metronome Timer
void Timer4_func( void );           // Buzzer Sound Timer
void Play_Note(uint8_t note_nr, uint8_t oct, uint8_t on_off); //TEEEESTE
uint16_t BGR_Convert(uint8_t b, uint8_t g, uint8_t r);

// Interrupções dos botões
void External_Interrupt_func();

// Funções SPI
void SPI2_Init( void );
void CONFIG_SREG( void );
void SPI_Write(uint8_t adress, uint8_t send_data);
uint16_t SPI_Read(uint8_t address);

void display_xyz( void );
void Ler_Acelerometro( void );

void mouse(x, y, color);
void atualizar_player( void );


/* Simple LED toggle task. */
static void prvTarefaMusica( void *pvParameters );

/* LCD activity task. */
static void prvLcdTask( void *pvParameters );



/********** Useful functions **********/
///* USART2 configuration. */
//static void prvSetupUSART2( void );
//
///* USART2 send message. */
//static void prvSendMessageUSART2(char *message);

/***************************************/

//static void prvSetupEXTI1( void ); //ex2

/* Task 1 handle variable. */
TaskHandle_t HandleMusica;
TaskHandle_t HandleLcd;



/* Mutexes e Semáforos*/
SemaphoreHandle_t xMusicMutex;



typedef struct XY_obj_vals
{
	int16_t x;
	int16_t y;
} XY_obj_vals;


QueueHandle_t xAcelQueue;
QueueHandle_t xPlayerDisplayPosQueue;
QueueHandle_t xPlayerMapPosQueue;



int main( void )
{
	/*Setup the hardware, RCC, GPIO, etc...*/
    prvSetupRCC();
    prvSetupGPIO();

    // Inicializar SPI e configurar IC
	SPI2_Init();
	CONFIG_SREG();

    External_Interrupt_func();

    Timer4_func(); //Timer da freq do Buzzer
    Timer3_Interrupt_func(); //Music Metronome

    srand(time(NULL));

    // Default
    XY_obj_vals Default_XY;
    XY_obj_vals Default_map_XY;
    Default_XY.x = 30;
    Default_XY.y = 30;
    Default_map_XY.x = 30;
    Default_map_XY.y = 30;


//    xMusicMutex = xSemaphoreCreateMutex();
    xMusicMutex = xSemaphoreCreateBinary();//xSemaphoreCreateMutex(); /* Criar o mutex. */
	//if( xMusicMutex == NULL ){ /* Error creating the semaphore, it cannot be used. */}

    // Queue Dos valores do aceleremetro
    xAcelQueue = xQueueCreate( 1, sizeof( XY_obj_vals ) ); //ex1
	if( xAcelQueue == 0 ) {}
	else {}

	// Queue Dos valores da posição do Player no Display
	xPlayerDisplayPosQueue = xQueueCreate( 1, sizeof( XY_obj_vals ) ); //ex1
	if( xPlayerDisplayPosQueue == 0 ) {}
	else {}
	// Fill with Default
	static BaseType_t pxHigherPriorityTaskWoken;
	xQueueSendToBack(xPlayerDisplayPosQueue, &Default_XY, &pxHigherPriorityTaskWoken);
	xQueueSendToBack(xPlayerMapPosQueue, &Default_map_XY, &pxHigherPriorityTaskWoken);

	// Queue Dos valores da posição do Player no Mapa
	xPlayerMapPosQueue = xQueueCreate( 1, sizeof( XY_obj_vals ) ); //ex1
	if( xPlayerMapPosQueue == 0 ) {}
	else {}

	/* Create the tasks */
 	xTaskCreate( prvTarefaMusica, "TarefaMusica", configMINIMAL_STACK_SIZE+600, NULL, mainMUSIC_TASK_PRIORITY, &HandleMusica );
 	xTaskCreate( prvLcdTask, "Lcd", configMINIMAL_STACK_SIZE, NULL, mainLCD_TASK_PRIORITY, &HandleLcd );

	/* Start the scheduler. */
	vTaskStartScheduler();

	/* Will only get here if there was not enough heap space to create the idle task. */
	return 0;
}
/*-----------------------------------------------------------*/


static void prvTarefaMusica( void *pvParameters )
{
	uint16_t tempo = 1; // Time counts from 1 to t_max
//	uint16_t t_max = C_nr*32 = Nr de compassos * 32 fusas por compasso

	//Música em formato BMP (Buzzer Music Protocol) (Musica convertida em python de um ficheiro MIDI de uma partitura para uma matriz)

	/* TOTO AFRICA */
	/*static const uint16_t t_max = 1808;
	static const uint16_t music[629][3] = {{4,4,2},
								  {6,0,0},
								  {9,4,2},
								  {10,0,0},
								  {13,4,2},
								  {14,0,0},
								  {15,4,2},
								  {16,0,0},
								  {20,4,2},
								  {24,3,12},
								  {28,4,5},
								  {30,5,7},
								  {32,5,5},
								  {34,5,2},
								  {36,5,5},
								  {38,5,7},
								  {40,5,5},
								  {42,5,2},
								  {44,5,5},
								  {46,5,7},
								  {48,5,5},
								  {50,5,2},
								  {52,4,12},
								  {54,5,2},
								  {56,4,12},
								  {58,5,2},
								  {60,5,7},
								  {64,5,5},
								  {68,4,2},
								  {70,0,0},
								  {73,4,2},
								  {74,0,0},
								  {77,4,2},
								  {78,0,0},
								  {79,4,2},
								  {80,0,0},
								  {84,4,2},
								  {88,3,12},
								  {92,4,5},
								  {94,5,7},
								  {96,5,5},
								  {98,5,2},
								  {100,5,5},
								  {102,5,7},
								  {104,5,5},
								  {106,5,2},
								  {108,5,5},
								  {110,5,7},
								  {112,5,5},
								  {114,5,2},
								  {116,4,12},
								  {118,5,2},
								  {120,4,12},
								  {122,5,2},
								  {124,5,7},
								  {128,5,5},
								  {132,4,2},
								  {134,0,0},
								  {137,4,2},
								  {138,0,0},
								  {141,4,2},
								  {142,0,0},
								  {143,4,2},
								  {144,0,0},
								  {148,4,2},
								  {152,3,12},
								  {156,4,5},
								  {158,5,7},
								  {160,5,5},
								  {162,5,2},
								  {164,5,5},
								  {166,5,7},
								  {168,5,5},
								  {170,5,2},
								  {172,5,5},
								  {174,5,7},
								  {176,5,5},
								  {178,5,2},
								  {180,4,12},
								  {182,5,2},
								  {184,4,12},
								  {186,5,2},
								  {188,5,7},
								  {192,5,5},
								  {196,4,2},
								  {198,0,0},
								  {201,4,2},
								  {202,0,0},
								  {205,4,2},
								  {206,0,0},
								  {207,4,2},
								  {208,0,0},
								  {212,4,2},
								  {216,3,12},
								  {220,4,5},
								  {222,5,7},
								  {224,5,5},
								  {226,5,2},
								  {228,5,5},
								  {230,5,7},
								  {232,5,5},
								  {234,5,2},
								  {236,5,5},
								  {238,5,7},
								  {240,5,5},
								  {242,5,2},
								  {244,4,12},
								  {246,5,2},
								  {248,4,12},
								  {250,5,2},
								  {252,5,7},
								  {256,5,5},
								  {258,4,4},
								  {259,4,7},
								  {260,0,0},
								  {261,4,7},
								  {262,0,0},
								  {263,4,7},
								  {264,0,0},
								  {270,4,7},
								  {272,0,0},
								  {273,4,7},
								  {274,0,0},
								  {280,4,7},
								  {284,4,9},
								  {285,4,11},
								  {286,0,0},
								  {304,4,12},
								  {305,4,4},
								  {306,0,0},
								  {312,4,4},
								  {316,4,5},
								  {318,4,7},
								  {320,0,0},
								  {322,4,7},
								  {326,4,5},
								  {328,0,0},
								  {332,4,5},
								  {334,4,4},
								  {342,4,2},
								  {344,0,0},
								  {348,4,2},
								  {352,3,12},
								  {362,4,4},
								  {364,4,2},
								  {368,3,12},
								  {372,4,2},
								  {374,0,0},
								  {377,4,2},
								  {378,0,0},
								  {381,4,2},
								  {382,0,0},
								  {383,4,2},
								  {384,0,0},
								  {388,4,2},
								  {392,3,12},
								  {400,4,5},
								  {404,4,4},
								  {405,4,7},
								  {406,0,0},
								  {407,4,7},
								  {408,0,0},
								  {409,4,7},
								  {410,0,0},
								  {417,4,7},
								  {418,0,0},
								  {424,4,7},
								  {428,4,9},
								  {430,4,11},
								  {448,4,12},
								  {449,4,4},
								  {450,0,0},
								  {456,4,4},
								  {460,4,5},
								  {463,4,7},
								  {464,0,0},
								  {468,4,7},
								  {469,4,5},
								  {470,0,0},
								  {474,4,5},
								  {478,4,4},
								  {486,4,2},
								  {488,0,0},
								  {492,4,2},
								  {496,3,12},
								  {506,4,4},
								  {508,4,2},
								  {512,3,12},
								  {516,4,2},
								  {518,0,0},
								  {521,4,2},
								  {522,0,0},
								  {525,4,2},
								  {526,0,0},
								  {527,4,2},
								  {528,0,0},
								  {532,4,2},
								  {536,3,12},
								  {544,4,5},
								  {548,4,4},
								  {549,4,7},
								  {550,0,0},
								  {551,4,7},
								  {552,0,0},
								  {558,4,7},
								  {560,0,0},
								  {561,4,7},
								  {562,0,0},
								  {568,4,7},
								  {572,4,9},
								  {574,4,11},
								  {592,4,12},
								  {593,4,4},
								  {594,0,0},
								  {595,4,4},
								  {596,0,0},
								  {598,4,4},
								  {602,4,5},
								  {605,4,7},
								  {606,0,0},
								  {612,4,7},
								  {615,4,5},
								  {616,0,0},
								  {620,4,5},
								  {622,4,4},
								  {630,4,2},
								  {632,0,0},
								  {634,4,2},
								  {636,3,12},
								  {652,4,4},
								  {654,4,2},
								  {656,3,12},
								  {660,4,2},
								  {662,0,0},
								  {665,4,2},
								  {666,0,0},
								  {669,4,2},
								  {670,0,0},
								  {671,4,2},
								  {672,0,0},
								  {676,4,2},
								  {680,3,12},
								  {688,4,5},
								  {692,4,4},
								  {693,4,7},
								  {694,0,0},
								  {695,4,7},
								  {696,0,0},
								  {702,4,7},
								  {704,0,0},
								  {705,4,7},
								  {706,0,0},
								  {712,4,7},
								  {716,4,9},
								  {718,4,11},
								  {736,4,12},
								  {737,5,4},
								  {738,0,0},
								  {744,5,4},
								  {748,5,5},
								  {751,5,7},
								  {752,0,0},
								  {756,5,7},
								  {757,5,5},
								  {758,0,0},
								  {762,5,5},
								  {764,5,4},
								  {816,5,5},
								  {820,5,2},
								  {823,5,10},
								  {824,0,0},
								  {827,5,10},
								  {828,0,0},
								  {829,5,10},
								  {830,0,0},
								  {835,5,10},
								  {836,0,0},
								  {839,5,10},
								  {840,0,0},
								  {843,5,10},
								  {844,0,0},
								  {847,5,10},
								  {848,0,0},
								  {856,5,10},
								  {860,0,0},
								  {861,5,9},
								  {862,0,0},
								  {870,5,9},
								  {876,4,10},
								  {880,4,9},
								  {884,4,10},
								  {887,5,10},
								  {888,0,0},
								  {889,5,10},
								  {890,0,0},
								  {891,5,10},
								  {892,0,0},
								  {893,5,10},
								  {894,0,0},
								  {895,5,10},
								  {896,0,0},
								  {897,5,10},
								  {898,0,0},
								  {901,5,10},
								  {902,0,0},
								  {903,5,10},
								  {904,0,0},
								  {905,5,10},
								  {906,0,0},
								  {909,5,10},
								  {910,0,0},
								  {911,5,10},
								  {912,0,0},
								  {916,5,10},
								  {918,0,0},
								  {919,5,10},
								  {920,0,0},
								  {922,5,10},
								  {925,5,9},
								  {926,0,0},
								  {927,5,9},
								  {928,0,0},
								  {934,5,9},
								  {940,4,10},
								  {944,4,9},
								  {948,4,10},
								  {951,5,10},
								  {952,0,0},
								  {955,5,10},
								  {956,0,0},
								  {957,5,10},
								  {958,0,0},
								  {959,5,10},
								  {960,0,0},
								  {966,5,10},
								  {968,0,0},
								  {971,5,10},
								  {972,0,0},
								  {975,5,10},
								  {976,0,0},
								  {982,5,10},
								  {983,4,5},
								  {984,0,0},
								  {988,4,5},
								  {989,5,9},
								  {990,0,0},
								  {1008,5,9},
								  {1012,0,0},
								  {1015,5,10},
								  {1016,0,0},
								  {1017,5,10},
								  {1018,0,0},
								  {1019,5,10},
								  {1020,0,0},
								  {1021,5,10},
								  {1022,0,0},
								  {1023,5,10},
								  {1024,0,0},
								  {1025,5,10},
								  {1026,0,0},
								  {1029,5,10},
								  {1030,0,0},
								  {1033,5,10},
								  {1034,0,0},
								  {1037,5,10},
								  {1038,0,0},
								  {1044,5,10},
								  {1046,0,0},
								  {1047,5,10},
								  {1048,0,0},
								  {1050,5,10},
								  {1056,5,9},
								  {1066,5,9},
								  {1068,5,7},
								  {1072,5,5},
								  {1082,5,9},
								  {1084,5,10},
								  {1088,5,9},
								  {1104,5,7},
								  {1108,4,2},
								  {1110,0,0},
								  {1113,4,2},
								  {1114,0,0},
								  {1117,4,2},
								  {1118,0,0},
								  {1119,4,2},
								  {1120,0,0},
								  {1124,4,2},
								  {1128,3,12},
								  {1132,4,5},
								  {1134,5,7},
								  {1136,5,5},
								  {1138,5,2},
								  {1140,5,5},
								  {1142,5,7},
								  {1144,5,5},
								  {1146,5,2},
								  {1148,5,5},
								  {1150,5,7},
								  {1152,5,5},
								  {1154,5,2},
								  {1156,4,12},
								  {1158,5,2},
								  {1160,4,12},
								  {1162,5,2},
								  {1164,5,7},
								  {1168,5,5},
								  {1172,4,2},
								  {1174,0,0},
								  {1177,4,2},
								  {1178,0,0},
								  {1181,4,2},
								  {1182,0,0},
								  {1183,4,2},
								  {1184,0,0},
								  {1188,4,2},
								  {1192,3,12},
								  {1196,4,5},
								  {1198,5,7},
								  {1200,5,5},
								  {1202,5,2},
								  {1204,5,5},
								  {1206,5,7},
								  {1208,5,5},
								  {1210,5,2},
								  {1212,5,5},
								  {1214,5,7},
								  {1216,5,5},
								  {1218,5,2},
								  {1220,4,12},
								  {1222,5,2},
								  {1224,4,12},
								  {1226,5,2},
								  {1228,5,7},
								  {1232,5,5},
								  {1234,0,0},
								  {1236,6,4},
								  {1238,6,2},
								  {1240,5,12},
								  {1242,6,2},
								  {1244,5,12},
								  {1246,5,9},
								  {1248,5,12},
								  {1250,5,9},
								  {1252,5,7},
								  {1254,5,9},
								  {1256,5,7},
								  {1258,5,4},
								  {1260,5,7},
								  {1262,5,4},
								  {1264,5,2},
								  {1266,5,4},
								  {1268,5,2},
								  {1270,4,12},
								  {1272,5,2},
								  {1274,4,12},
								  {1276,4,9},
								  {1284,4,12},
								  {1286,5,2},
								  {1288,4,12},
								  {1290,4,9},
								  {1296,4,7},
								  {1302,0,0},
								  {1304,5,5},
								  {1306,5,4},
								  {1308,5,2},
								  {1310,4,12},
								  {1312,4,10},
								  {1314,4,9},
								  {1318,4,7},
								  {1322,4,12},
								  {1326,4,10},
								  {1328,4,9},
								  {1330,4,7},
								  {1334,4,5},
								  {1336,4,4},
								  {1338,4,12},
								  {1340,4,7},
								  {1342,4,5},
								  {1344,4,4},
								  {1348,4,2},
								  {1350,0,0},
								  {1353,4,2},
								  {1354,0,0},
								  {1357,4,2},
								  {1358,0,0},
								  {1359,4,2},
								  {1360,0,0},
								  {1364,4,2},
								  {1368,3,12},
								  {1376,4,5},
								  {1378,5,2},
								  {1380,4,12},
								  {1382,5,2},
								  {1384,5,4},
								  {1386,5,2},
								  {1388,5,4},
								  {1390,5,7},
								  {1392,5,4},
								  {1394,5,7},
								  {1396,5,9},
								  {1398,5,7},
								  {1399,6,3},
								  {1402,6,4},
								  {1406,6,2},
								  {1424,5,12},
								  {1425,5,4},
								  {1426,0,0},
								  {1432,5,4},
								  {1436,5,5},
								  {1440,5,7},
								  {1444,5,7},
								  {1445,5,5},
								  {1446,0,0},
								  {1450,5,5},
								  {1451,5,4},
								  {1452,0,0},
								  {1520,5,5},
								  {1524,5,2},
								  {1527,5,10},
								  {1528,0,0},
								  {1531,5,10},
								  {1532,0,0},
								  {1533,5,10},
								  {1534,0,0},
								  {1539,5,10},
								  {1540,0,0},
								  {1543,5,10},
								  {1544,0,0},
								  {1547,5,10},
								  {1548,0,0},
								  {1551,5,10},
								  {1552,0,0},
								  {1560,5,10},
								  {1564,0,0},
								  {1565,5,9},
								  {1566,0,0},
								  {1574,5,9},
								  {1580,4,10},
								  {1584,4,9},
								  {1588,4,10},
								  {1591,5,10},
								  {1592,0,0},
								  {1593,5,10},
								  {1594,0,0},
								  {1595,5,10},
								  {1596,0,0},
								  {1597,5,10},
								  {1598,0,0},
								  {1599,5,10},
								  {1600,0,0},
								  {1601,5,10},
								  {1602,0,0},
								  {1605,5,10},
								  {1606,0,0},
								  {1607,5,10},
								  {1608,0,0},
								  {1609,5,10},
								  {1610,0,0},
								  {1613,5,10},
								  {1614,0,0},
								  {1615,5,10},
								  {1616,0,0},
								  {1620,5,10},
								  {1622,0,0},
								  {1623,5,10},
								  {1624,0,0},
								  {1626,5,10},
								  {1629,5,9},
								  {1630,0,0},
								  {1631,5,9},
								  {1632,0,0},
								  {1638,5,9},
								  {1644,4,10},
								  {1648,4,9},
								  {1652,4,10},
								  {1655,5,10},
								  {1656,0,0},
								  {1659,5,10},
								  {1660,0,0},
								  {1661,5,10},
								  {1662,0,0},
								  {1663,5,10},
								  {1664,0,0},
								  {1670,5,10},
								  {1672,0,0},
								  {1675,5,10},
								  {1676,0,0},
								  {1679,5,10},
								  {1680,0,0},
								  {1686,5,10},
								  {1687,4,5},
								  {1688,0,0},
								  {1692,4,5},
								  {1693,5,9},
								  {1694,0,0},
								  {1712,5,9},
								  {1716,0,0},
								  {1719,5,10},
								  {1720,0,0},
								  {1721,5,10},
								  {1722,0,0},
								  {1723,5,10},
								  {1724,0,0},
								  {1725,5,10},
								  {1726,0,0},
								  {1727,5,10},
								  {1728,0,0},
								  {1729,5,10},
								  {1730,0,0},
								  {1733,5,10},
								  {1734,0,0},
								  {1737,5,10},
								  {1738,0,0},
								  {1741,5,10},
								  {1742,0,0},
								  {1748,5,10},
								  {1750,0,0},
								  {1751,5,10},
								  {1752,0,0},
								  {1754,5,10},
								  {1760,5,9},
								  {1770,5,9},
								  {1772,5,7},
								  {1776,5,5},
								  {1786,5,9},
								  {1788,5,10},
								  {1792,5,9},
								  {1808,5,7},
	};*/
	//Ievan Polkka
	uint16_t t_max = 256;
	static uint16_t music[105][3] = {{2,5,3},
							  {6,4,10},
							  {9,5,3},
							  {10,0,0},
							  {16,5,3},
							  {18,5,5},
							  {22,5,6},
							  {23,5,3},
							  {24,0,0},
							  {27,5,3},
							  {28,0,0},
							  {32,5,3},
							  {34,5,6},
							  {38,5,5},
							  {41,5,1},
							  {42,0,0},
							  {46,5,1},
							  {50,5,5},
							  {54,5,6},
							  {57,5,3},
							  {58,0,0},
							  {64,5,3},
							  {66,5,3},
							  {70,4,10},
							  {73,5,3},
							  {74,0,0},
							  {80,5,3},
							  {82,5,5},
							  {86,5,6},
							  {87,5,3},
							  {88,0,0},
							  {91,5,3},
							  {92,0,0},
							  {93,5,3},
							  {94,0,0},
							  {96,5,3},
							  {98,5,6},
							  {99,5,10},
							  {100,0,0},
							  {101,5,10},
							  {102,0,0},
							  {104,5,10},
							  {106,5,8},
							  {110,5,6},
							  {114,5,5},
							  {118,5,6},
							  {119,5,3},
							  {120,0,0},
							  {125,5,3},
							  {126,0,0},
							  {128,5,3},
							  {130,5,6},
							  {133,5,10},
							  {134,0,0},
							  {138,5,10},
							  {142,5,8},
							  {146,5,6},
							  {150,5,5},
							  {151,5,1},
							  {152,0,0},
							  {155,5,1},
							  {156,0,0},
							  {160,5,1},
							  {162,5,5},
							  {163,5,8},
							  {164,0,0},
							  {165,5,8},
							  {166,0,0},
							  {167,5,8},
							  {168,0,0},
							  {170,5,8},
							  {174,5,6},
							  {178,5,5},
							  {182,5,6},
							  {183,5,3},
							  {184,0,0},
							  {192,5,3},
							  {194,5,6},
							  {197,5,10},
							  {198,0,0},
							  {202,5,10},
							  {206,5,8},
							  {210,5,6},
							  {214,5,5},
							  {215,5,1},
							  {216,0,0},
							  {219,5,1},
							  {220,0,0},
							  {221,5,1},
							  {222,0,0},
							  {224,5,1},
							  {226,5,5},
							  {227,5,8},
							  {228,0,0},
							  {229,5,8},
							  {230,0,0},
							  {231,5,8},
							  {232,0,0},
							  {234,5,8},
							  {238,5,6},
							  {242,5,5},
							  {246,5,6},
							  {247,5,3},
							  {248,0,0},
							  {256,5,3},
	};
	/*
	uint16_t t_max = 512; // Super Mario World Ending Theme
	uint16_t music[106][3] = {{5,4,5},
							{8,4,8},
							{13,4,10},
							{24,4,8},
							{29,0,0},
							{32,4,10},
							{35,4,12},
							{37,0,0},
							{40,5,1},
							{43,4,12},
							{45,4,11},
							{56,4,10},
							{61,0,0},
							{64,4,4},
							{69,4,5},
							{72,4,8},
							{77,4,10},
							{88,4,8},
							{93,0,0},
							{96,4,9},
							{99,4,10},
							{101,0,0},
							{104,4,11},
							{107,4,10},
							{109,4,9},
							{120,4,8},
							{125,0,0},
							{128,4,9},
							{133,4,10},
							{136,4,12},
							{141,5,1},
							{152,4,10},
							{157,0,0},
							{160,4,9},
							{165,4,8},
							{168,4,9},
							{173,4,10},
							{184,4,5},
							{192,0,0},
							{195,5,5},
							{197,0,0},
							{200,5,1},
							{203,4,10},
							{205,0,0},
							{208,5,5},
							{211,5,1},
							{213,0,0},
							{216,4,10},
							{221,5,1},
							{224,5,8},
							{232,0,0},
							{237,5,3},
							{240,5,3},
							{245,5,5},
							{248,5,5},
							{256,5,6},
							{259,4,5},
							{261,0,0},
							{264,4,8},
							{269,4,10},
							{280,4,8},
							{285,0,0},
							{288,4,10},
							{291,4,12},
							{293,0,0},
							{296,5,1},
							{299,4,12},
							{301,4,11},
							{312,4,10},
							{317,0,0},
							{320,4,4},
							{323,4,5},
							{325,0,0},
							{328,4,8},
							{333,4,10},
							{344,4,8},
							{349,0,0},
							{352,4,9},
							{355,4,10},
							{357,0,0},
							{360,4,11},
							{363,4,10},
							{365,4,9},
							{376,4,8},
							{381,0,0},
							{384,4,9},
							{387,4,10},
							{389,0,0},
							{392,4,12},
							{397,5,1},
							{414,4,10},
							{416,4,9},
							{419,4,8},
							{421,0,0},
							{424,4,9},
							{429,4,10},
							{440,5,5},
							{448,0,0},
							{453,5,5},
							{456,5,4},
							{461,5,5},
							{464,5,1},
							{469,0,0},
							{472,5,1},
							{477,4,10},
							{488,5,1},
							{512,0,0},
	};*/
	/*uint16_t t_max = 130; //Megalovania Without pauses
	uint8_t music[40][3] = {{1,5,3},
							{3,5,3},
							{7,6,3},
							{13,5,10},
							{17,5,9},
							{22,5,8},
							{26,5,6},
							{28,5,3},
							{30,5,6},
							{32,5,8},
							{33,5,1},
							{35,5,1},
							{40,6,3},
							{46,5,10},
							{50,5,9},
							{54,5,8},
							{58,5,6},
							{61,5,3},
							{63,5,6},
							{65,5,8},
							{66,4,12},
							{68,4,12},
							{72,6,3},
							{78,5,10},
							{83,5,9},
							{87,5,8},
							{91,5,6},
							{93,5,3},
							{95,5,6},
							{97,5,8},
							{98,4,11},
							{101,4,11},
							{105,6,3},
							{111,5,10},
							{115,5,9},
							{120,5,8},
							{124,5,6},
							{126,5,3},
							{128,5,6},
							{130,5,8},
	};*/
	/*uint16_t t_max = 280; // Krusty Krab Theme
	uint16_t music[71][3] = {{2,6,3},
							{5,6,2},
							{8,6,1},
							{12,5,12},
							{16,5,8},
							{21,5,3},
							{25,5,8},
							{29,5,12},
							{33,5,8},
							{37,6,3},
							{42,5,12},
							{44,6,8},
							{47,6,7},
							{50,6,8},
							{53,6,10},
							{56,6,8},
							{58,6,7},
							{63,6,8},
							{67,6,3},
							{71,5,12},
							{75,5,8},
							{80,5,7},
							{84,5,5},
							{88,5,7},
							{92,5,8},
							{96,5,10},
							{101,5,12},
							{103,6,1},
							{106,5,12},
							{109,5,10},
							{113,6,3},
							{117,6,2},
							{122,6,3},
							{126,6,5},
							{130,6,7},
							{134,6,3},
							{138,6,1},
							{143,5,10},
							{147,5,12},
							{151,5,8},
							{155,5,3},
							{160,5,8},
							{164,5,12},
							{168,5,8},
							{172,6,3},
							{176,5,12},
							{179,6,8},
							{182,6,7},
							{185,6,8},
							{188,6,10},
							{190,6,8},
							{193,6,7},
							{197,6,8},
							{202,6,3},
							{206,5,12},
							{210,5,8},
							{213,5,12},
							{216,6,1},
							{218,6,3},
							{223,6,8},
							{227,6,3},
							{231,6,1},
							{235,5,10},
							{240,5,7},
							{244,5,8},
							{248,5,10},
							{252,5,8},
							{256,5,8},
							{261,5,7},
							{277,5,8},
							{280,0,0},
	};*/
	/*// Jazz Police (time/2)
    uint8_t music[27][3] = {{2,5,3},
    						{4,5,3},
    						{6,5,6},
    						{8,5,3},
    						{10,5,9},
    						{12,5,10},
    						{14,5,6},
    						{16,5,3},
    						{20,6,1},
    						{22,5,10},
    						{26,6,1},
    						{28,5,10},
    						{30,6,1},
    						{32,6,3},
    						{34,5,3},
    						{36,5,3},
    						{38,5,6},
    						{40,5,3},
    						{42,5,9},
    						{44,5,10},
    						{46,5,6},
    						{48,5,3},
    						{50,5,8},
    						{52,5,9},
    						{56,5,6},
    						{60,5,3},
    						{64,0,0},
    }; */ /*Formato:           {t, o, n}
    	 t = time stamp do fim da nota
    	 o = oitava (da 4 à 7)
    	 n = numero da nota (0=pause, 1=C, 2=C#, 3=D, ... , 12=B) */

    int16_t current_note_indx = 0;

    uint16_t dur_until = 0;
    uint16_t oct      = 0;
    uint16_t note_nr  = 0;

    for( ;; )
	{
    	xSemaphoreTake( xMusicMutex, ( TickType_t) portMAX_DELAY ); // Wait for Timer3 Metronome

    	/////////////////////
    	// Ler e tocar a Música
    	dur_until = music[current_note_indx][0];
		oct       = music[current_note_indx][1];
		note_nr   = music[current_note_indx][2];

		if (tempo > dur_until) //Mudar para a nota seguinte caso o tempo da anterior tenha acabado
		{
			current_note_indx++;
			oct      = music[current_note_indx][1];
			note_nr  = music[current_note_indx][2];
		}
		Play_Note(note_nr, oct+1, 0);//music_on_off);

		tempo++;
		if (tempo >= t_max)
		{
			tempo = 1;
			current_note_indx = 0;
//			GPIO_WriteBit(GPIOB, GPIO_Pin_0, 1-GPIO_ReadOutputDataBit(GPIOB, GPIO_Pin_0));
		}
    	/////////////////////

	}
}

uint8_t note_nr;
void Play_Note(uint8_t note_nr, uint8_t oct, uint8_t on_off)
{
	int16_t Octv[12] = {4368, 4123, 3892, 3674, 3468, 3273, 3089, 2916, 2752, 2598, 2452, 2314};
	uint16_t ARR;
	uint8_t mult = 0;

	if(note_nr == 0) TIM_Cmd(TIM4, 0);
	else
	{
		TIM_Cmd(TIM4, on_off);

		// Convert Octave to right ARR multiplication value
		if (oct < 4) mult = 8;
		else if (oct > 7) mult = 1;
		else mult = pow(2, (7-oct));


		ARR = Octv[note_nr-1] * mult;
		//Definir a frequência da nota
		TIM_SetAutoreload(TIM4, ARR-1);
	}
}

/*-----------------------------------------------------------*/
/* Example task to present characteres in ther display. */
static void prvLcdTask( void *pvParameters )
{
	lcd_init ( );

//	unsigned char carc[8];


	for( ;; )
	{

		Ler_Acelerometro();
		atualizar_player();



        vTaskDelay( ( TickType_t ) 50 / portTICK_RATE_MS);
	}
}
/*-----------------------------------------------------------*/



/*-----------------------------------------------------------*/
void Timer3_Interrupt_func( void ) // Music Metronome, bmps to Hz
{
	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
	NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);


	// Seminima a "X" bpm -> X/60 (bps = Hz)
	// Resolução = 8 (1 seminima = 8 fusas)
	// Freq = 8 * X/60 = res * bps
	// Bpms possíveis: 60 67 75 82 90 97 105 112 120 127 135 142 150 157 165 172 180 187 195 202
	// Timer freqs:     8  9 10 11 12 13  14  15  16  17  18  19  20  21  22  23  24  25  26  27
	uint8_t bpms = 120;//60;//150;
	uint8_t freq_fusas = (uint8_t) (8*bpms/60);

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;

	TIM_TimeBaseStructure.TIM_Period = 10000-1;
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1; //TIM_CKD_DIV1;
	TIM_TimeBaseStructure.TIM_Prescaler = (6400/freq_fusas)-1; //
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;

	TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);
	TIM_Cmd(TIM3, ENABLE);


	TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE );
}

void Timer4_func( void )// Freq for Buzzer
{
	// Buzzer pin
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	// Prioridade
	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
	NVIC_InitStructure.NVIC_IRQChannel = TIM4_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);



	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
	TIM_TimeBaseStructure.TIM_Period = 18512-1;// <- para 64MHz //(5846*4)-1; <- para 72MHz //auto-reload 0 at´e 65535
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseStructure.TIM_Prescaler = 7-1;
	// Com CK_INIT = 64MHz
	//64000000/((18511+1)*(6+1) = 493,888 Hz ~= 493.883 Hz
	// A = 439.899 ~= 440Hz (dif=0.0165, percent_err=0.0037%):
	// Opção 1: prescaler = 7-1;  Afinado em B4: 493.883Hz  [C4 - B7]
	// Opção 2: prescaler = 14-1; Afinado em B5: 246.94Hz   [C3 - B6]

	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM4, &TIM_TimeBaseStructure);

	// Saída para o PB9 -> Buzzer
	TIM_OCInitTypeDef TIM_OCInitStructure;
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_Toggle;
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_Pulse = 0;
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
	TIM_OC4Init(TIM4, &TIM_OCInitStructure); //PB1
	TIM_Cmd(TIM4, DISABLE); //Ligar o buzzer apenas quando desejado

}


void External_Interrupt_func()
{
	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
	NVIC_InitStructure.NVIC_IRQChannel = EXTI1_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource1);
	EXTI_InitTypeDef EXTI_InitStructure;
	EXTI_InitStructure.EXTI_Line = EXTI_Line1;
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStructure);
}

uint16_t BGR_Convert(uint8_t b, uint8_t g, uint8_t r)
{
    b = (b*0xF)/255;
    g = (g*0xF)/255;
    r = (r*0xFF)/255;
    return ((b << 12) | (g << 8) | (r));
}


//static void prvUSART2Interrupt ( void )
//{
//	NVIC_InitTypeDef NVIC_InitStructure;
//
//	USART_ITConfig(USART2, USART_IT_RXNE, ENABLE );
//	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
//
//	NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
//	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
//	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
//	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
//
//	NVIC_Init(&NVIC_InitStructure);
//}


//static void prvSetupRCC( void )
//{
//    /* RCC configuration - 72 MHz */
//    ErrorStatus HSEStartUpStatus;
//
//    RCC_DeInit();
//    /*Enable the HSE*/
//    RCC_HSEConfig(RCC_HSE_ON);
//    /* Wait untill HSE is ready or time out */
//    HSEStartUpStatus = RCC_WaitForHSEStartUp();
//    if(HSEStartUpStatus == SUCCESS)
//    {
//        /* Enable The Prefetch Buffer */
//        FLASH_PrefetchBufferCmd(FLASH_PrefetchBuffer_Enable);
//        /* 72 MHZ - 2 wait states */
//        FLASH_SetLatency(FLASH_Latency_2);
//
//        /* No division HCLK = SYSCLK */
//        RCC_HCLKConfig(RCC_SYSCLK_Div1);
//        /* PCLK1 = HCLK/2 (36MHz) */
//        RCC_PCLK1Config(RCC_HCLK_Div2);
//        /* PCLK2 = HCLK (72MHz)*/
//        RCC_PCLK2Config(RCC_HCLK_Div1);
//
//        /* Use PLL with HSE=12MHz */
//        RCC_PLLConfig(RCC_PLLSource_HSE_Div1, RCC_PLLMul_6);
//        /* Enable the PLL */
//        RCC_PLLCmd(ENABLE);
//        /* Wait for PLL ready */
//        while (RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET );
//
//        /* Select the PLL as system clock source */
//        RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);
//        /* Wait until PLL is used as system clock */
//        while( RCC_GetSYSCLKSource() != 0x08 );
//    }
//    else
//    {
//        while(1);
//    }
//}
///*-----------------------------------------------------------*/


static void prvSetupRCC( void ) // RCC COM HSI
{

    RCC_DeInit();
    /*Enable the HSI*/
    RCC_HSICmd(ENABLE);
    /* Wait untill HSI is ready or time out */
    while(RCC_GetFlagStatus(RCC_FLAG_HSIRDY) == RESET){};

    //SET HSI AS SYSCLK SRC. CONFIGURE HCLK, PCLK1 & PCLK2
    RCC_SYSCLKConfig(RCC_SYSCLKSource_HSI);
    RCC_HCLKConfig(RCC_SYSCLK_Div1);
    RCC_PCLK1Config(RCC_HCLK_Div2);
    RCC_PCLK2Config(RCC_HCLK_Div1);

    //000 Zero wait state, if 0  MHz < SYSCLK <= 24 MHz
    //001 One wait state, if  24 MHz < SYSCLK <= 48 MHz
    //010 Two wait states, if 48 MHz < SYSCLK <= 72 MHz */
    FLASH_SetLatency(FLASH_Latency_2);

    //DISABLE PLL
    RCC_PLLCmd(DISABLE);

    //CHANGE PLL SRC AND MULTIPLIER
    RCC_PLLConfig(RCC_PLLSource_HSI_Div2, RCC_PLLMul_16);

    //ENABLE PLL
    //WAIT FOR IT TO BE READY
    //SET SYSCLK SRC AS PLLCLK
    RCC_PLLCmd(ENABLE);
    while(RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET){};
    RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);

    FLASH_SetLatency(FLASH_Latency_2);

    //SET HCLK = SYSCLK = 64MHZ
    RCC_HCLKConfig(RCC_SYSCLK_Div1);

    //SET PCLK1 = HCLK/2 = 32MHZ
    RCC_PCLK1Config(RCC_HCLK_Div2);

    //SET PCLK2 = HCLK = 64MHZ
    RCC_PCLK2Config(RCC_HCLK_Div1);

}
/*-----------------------------------------------------------*/


static void prvSetupGPIO( void )
{
    /* GPIO configuration - GREEN LED*/
    GPIO_InitTypeDef GPIO_InitStructure;

    /* Enable GPIOB clock */
    RCC_APB2PeriphClockCmd( RCC_APB2Periph_GPIOB , ENABLE );

    GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_0;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

    GPIO_Init(GPIOB, &GPIO_InitStructure);


    RCC_APB2PeriphClockCmd( RCC_APB2Periph_GPIOA , ENABLE ); //Botão A1 (SW5)

	GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_1;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

	GPIO_Init(GPIOA, &GPIO_InitStructure);

}
/*-----------------------------------------------------------*/



void prvSetupUSART2( void )
{
USART_InitTypeDef USART_InitStructure;
GPIO_InitTypeDef GPIO_InitStructure;

    /* USART2 is configured as follow:
        - BaudRate = 115200 baud
        - Word Length = 8 Bits
        - 1 Stop Bit
        - No parity
        - Hardware flow control disabled (RTS and CTS signals)
        - Receive and transmit enabled */

    /* Enable GPIOA clock */
    RCC_APB2PeriphClockCmd( RCC_APB2Periph_GPIOA , ENABLE );

    /* USART Periph clock enable */
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);

    GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_2;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_3;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);


    USART_InitStructure.USART_BaudRate = 115200;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;

    /* Configure the USART2 */
    USART_Init(USART2, &USART_InitStructure);
    /* Enable the USART2 */
    USART_Cmd(USART2, ENABLE);
 }

/*-----------------------------------------------------------*/



static void prvSendMessageUSART2(char *message)
{
uint16_t cont_aux=0;

    while(cont_aux != strlen(message))
    {
        USART_SendData(USART2, (uint8_t) message[cont_aux]);
        while(USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET)
        {
        }
        cont_aux++;
    }
}
/*-----------------------------------------------------------*/



// SPI Functions

void SPI2_Init()
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    GPIO_InitTypeDef GPIO_InitStructure;

    //PB13 CLK e PB15 MOSI (output)
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13 | GPIO_Pin_15 ;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	//PB14 MISO (input)
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_14;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);


	//PD2 CS
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD, ENABLE);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOD, &GPIO_InitStructure);

	GPIO_WriteBit(GPIOD, GPIO_Pin_2, 1); //Cs a 1


    // Step 1: Initialize SPI2
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE);

    // Initialization struct
    SPI_InitTypeDef SPI_InitStruct;
    SPI_InitStruct.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_128;
    SPI_InitStruct.SPI_CPHA = SPI_CPHA_2Edge;
    SPI_InitStruct.SPI_CPOL = SPI_CPOL_High;
    SPI_InitStruct.SPI_DataSize = SPI_DataSize_8b;
    SPI_InitStruct.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
    SPI_InitStruct.SPI_FirstBit = SPI_FirstBit_MSB;
    SPI_InitStruct.SPI_Mode = SPI_Mode_Master;
    SPI_InitStruct.SPI_NSS = SPI_NSS_Soft;
    SPI_InitStruct.SPI_CRCPolynomial = 7;
    SPI_Init(SPI2, &SPI_InitStruct);
    SPI_Cmd(SPI2, ENABLE);
//    GPIO_WriteBit(GPIOD, GPIO_Pin_2, 1); //Cs a 1
}


// Escreve a data na adress
void SPI_Write(uint8_t adress, uint8_t send_data)
{
	while ( SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_TXE) == RESET);
	SPI_I2S_SendData( SPI2, adress );
	SPI_I2S_ClearFlag(SPI2, SPI_I2S_FLAG_TXE);
	while ( SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_RXNE) == RESET);
    SPI_I2S_ReceiveData( SPI2 );
    SPI_I2S_ClearFlag(SPI2, SPI_I2S_FLAG_RXNE);

	while ( SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_TXE) == RESET);
	SPI_I2S_SendData( SPI2, send_data );
	SPI_I2S_ClearFlag(SPI2, SPI_I2S_FLAG_TXE);
	while ( SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_RXNE) == RESET);
	SPI_I2S_ReceiveData( SPI2 );
    SPI_I2S_ClearFlag(SPI2, SPI_I2S_FLAG_RXNE);
}

// Retorna a data que está na address
uint16_t SPI_Read(uint8_t address)
{
	uint16_t receive_data = 0;
	address |= 0b10000000;// Para ler o oitavo bit tem de ser 1

	GPIO_WriteBit(GPIOD, GPIO_Pin_2, 0); // Cs a 0

	while ( SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_TXE) == RESET);
	SPI_I2S_SendData( SPI2, address );
	SPI_I2S_ClearFlag(SPI2, SPI_I2S_FLAG_TXE);
	while ( SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_RXNE) == RESET);
    SPI_I2S_ReceiveData( SPI2 );
    SPI_I2S_ClearFlag(SPI2, SPI_I2S_FLAG_RXNE);


	while ( SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_TXE) == RESET);
	SPI_I2S_SendData( SPI2, 0x00 );
	SPI_I2S_ClearFlag(SPI2, SPI_I2S_FLAG_TXE);
	while ( SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_RXNE) == RESET);
    receive_data = SPI_I2S_ReceiveData( SPI2 );
    SPI_I2S_ClearFlag(SPI2, SPI_I2S_FLAG_RXNE);

    GPIO_WriteBit(GPIOD, GPIO_Pin_2, 1); // Cs a 1

	return receive_data; // Devolver a data recebida
}

void CONFIG_SREG()
{
	// SREG1
		GPIO_WriteBit(GPIOD, GPIO_Pin_2, 0); // colocar pino em baixo
		SPI_Write(0x20, 0b11000111);	// Endereço do registo CTRL_REG1
		// Sensor ativo - 11; Dividir por 8 - 11 (2560Hz); ST desligado - 0; Zen,Yen,Xen (eixos) ativos - 111;
		GPIO_WriteBit(GPIOD, GPIO_Pin_2, 1); // colocar pino alto

	//SREG2
		GPIO_WriteBit(GPIOD, GPIO_Pin_2, 0); // colocar pino em baixo
		SPI_Write(0x21, 0b01000000);	// Endereço do registo CTRL_REG2
		// +/-2g; BDU=1; BLE=0; ROOT=0; IEN=0; DRDY=0; SIM=0 (4 wire); DAS=0; (12 bit right shift)
		GPIO_WriteBit(GPIOD, GPIO_Pin_2, 1); // colocar pino alto

 	//SREG3
		GPIO_WriteBit(GPIOD, GPIO_Pin_2, 0); // colocar pino em baixo
		SPI_Write(0x21, 0b10000000);	// Endereço do registo CTRL_REG3
		GPIO_WriteBit(GPIOD, GPIO_Pin_2, 1); // colocar pino alto
}


void Ler_Acelerometro( void )
{

	uint8_t x_h = 0, x_l=0, y_h=0, y_l=0, z_h=0, z_l=0;
	int16_t x, y, z;

	x_l=SPI_Read(0x28);
	x_h=SPI_Read(0x29);

	y_l=SPI_Read(0x2A);
	y_h=SPI_Read(0x2B);

	z_l=SPI_Read(0x2C);
	z_h=SPI_Read(0x2D);

	x = (x_h<<8) | x_l;
	y = (y_h<<8) | y_l;
	z = (z_h<<8) | z_l;


	XY_obj_vals Acel_vals;
	Acel_vals.x = x;
	Acel_vals.y = y;
	static BaseType_t pxHigherPriorityTaskWoken;
	xQueueReset(xAcelQueue);//Clear xAcelQueue
	xQueueSendToBack(xAcelQueue, &Acel_vals, &pxHigherPriorityTaskWoken);

}


// Função Coordenadas por SPI
void display_xyz( void )
{
	// Receive Acelerometer Values
	XY_obj_vals Acel_vals;
	if( xAcelQueue != 0 ) xQueuePeek( xAcelQueue, &Acel_vals, ( TickType_t ) portMAX_DELAY );

//	unsigned char carc[20];
//	memset(carc, 0, sizeof(carc));
//	sprintf(carc, "x:%d y:%d   ", Acel_vals.x, Acel_vals.y);
//	lcd_draw_string(10, 30, carc, 0xFFFF, 1);

}


void mouse(x, y, color)
{
	lcd_draw_pixel(x, y, color);     lcd_draw_pixel(x, y+1, color);   lcd_draw_pixel(x+1, y+1, color);
	lcd_draw_pixel(x, y+2, color);   lcd_draw_pixel(x+1, y+2, color); lcd_draw_pixel(x+2, y+2, color);
	lcd_draw_pixel(x, y+3, color);   lcd_draw_pixel(x+1, y+3, color); lcd_draw_pixel(x+2, y+3, color);
	lcd_draw_pixel(x+3, y+3, color); lcd_draw_pixel(x, y+4, color);   lcd_draw_pixel(x+1, y+4, color);
	lcd_draw_pixel(x+2, y+4, color); lcd_draw_pixel(x+3, y+4, color); lcd_draw_pixel(x+4, y+4, color);
	lcd_draw_pixel(x, y+5, color);   lcd_draw_pixel(x+1, y+5, color); lcd_draw_pixel(x+2, y+5, color);
	lcd_draw_pixel(x+3, y+5, color); lcd_draw_pixel(x+4, y+5, color); lcd_draw_pixel(x+5, y+5, color);
	lcd_draw_pixel(x, y+6, color);   lcd_draw_pixel(x+1, y+6, color); lcd_draw_pixel(x+2, y+6, color);
	lcd_draw_pixel(x+3, y+6, color); lcd_draw_pixel(x+4, y+6, color); lcd_draw_pixel(x+5, y+6, color);
	lcd_draw_pixel(x+6, y+6, color); lcd_draw_pixel(x, y+7, color);   lcd_draw_pixel(x+1, y+7, color);
	lcd_draw_pixel(x+2, y+7, color); lcd_draw_pixel(x+3, y+7, color); lcd_draw_pixel(x, y+8, color);
	lcd_draw_pixel(x+1, y+8, color); lcd_draw_pixel(x+3, y+8, color); lcd_draw_pixel(x+4, y+8, color);
	lcd_draw_pixel(x, y+9, color);   lcd_draw_pixel(x+3, y+9, color); lcd_draw_pixel(x+4, y+9, color);
																															      lcd_draw_pixel(x+4, y+10, color);lcd_draw_pixel(x+5, y+10, color);																														  lcd_draw_pixel(x+4, y+11, color);lcd_draw_pixel(x+5, y+11, color);
}


void print_paredes( void )
{


}




void atualizar_player( void )
{
	// Receive Acelerometer Values
	XY_obj_vals Acel_vals;

	if( xAcelQueue != 0 ) xQueuePeek( xAcelQueue, &Acel_vals, ( TickType_t ) portMAX_DELAY );
	int16_t acel_x = Acel_vals.x;
	int16_t acel_y = Acel_vals.y;


	// Receive Player Display Position Values
	XY_obj_vals Display_Pos_vals;
	if( xPlayerDisplayPosQueue != 0 ) xQueuePeek( xPlayerDisplayPosQueue, &Display_Pos_vals, ( TickType_t ) portMAX_DELAY );
	uint16_t x_pos = Display_Pos_vals.x;
	uint16_t y_pos = Display_Pos_vals.y;

	XY_obj_vals Display_Map_vals;
	if( xPlayerMapPosQueue != 0 ) xQueuePeek( xPlayerMapPosQueue, &Display_Map_vals, ( TickType_t ) portMAX_DELAY );
	uint16_t x_map_pos = Display_Map_vals.x;
	uint16_t y_map_pos = Display_Map_vals.y;



	mouse(x_pos, y_pos, 0x0000); //clear mouse

	uint8_t limit = 20;
	if (acel_x >  limit)    x_pos+=3;x_map_pos+=3;
	if (acel_x < -limit)    x_pos-=3;x_map_pos-=3;
	if (acel_x >  limit*4)  x_pos+=4;x_map_pos+=4;
	if (acel_x < -limit*4)  x_pos-=4;x_map_pos-=4;
	if (acel_x >  limit*8)  x_pos+=5;x_map_pos+=5;
	if (acel_x < -limit*8)  x_pos-=5;x_map_pos-=5;

	if (acel_y >  limit)    y_pos+=3;y_map_pos+=3;
	if (acel_y < -limit)    y_pos-=3;y_map_pos-=3;
	if (acel_y >  limit*4)  y_pos+=4;y_map_pos+=4;
	if (acel_y < -limit*4)  y_pos-=4;y_map_pos-=4;
	if (acel_y >  limit*8)  y_pos+=5;y_map_pos+=5;
	if (acel_y < -limit*8)  y_pos-=5;y_map_pos-=5;

	if (x_map_pos < 0) x_map_pos = 0;
	if (x_map_pos > 256-7) x_map_pos = 256-7;
	if (y_map_pos < 0) y_map_pos = 0;
	if (y_map_pos > 320-12) y_map_pos = 320-12;

	if (x_pos < 0) x_pos = 0;
	if (x_pos > 128-7) x_pos = 128-7;
	if (y_pos < 0) y_pos = 0;
	if (y_pos > 160-12) y_pos = 160-12;



	mouse(x_pos, y_pos, 0xFFFF); //print mouse


	Display_Pos_vals.x = x_pos;
	Display_Pos_vals.y = y_pos;

	Display_Pos_vals.x = x_map_pos;
	Display_Pos_vals.y = y_map_pos;

	static BaseType_t pxHigherPriorityTaskWoken;
	xQueueReset(xPlayerMapPosQueue);//Clear xAcelQueue
	xQueueSendToBack(xPlayerMapPosQueue, &Display_Map_vals, &pxHigherPriorityTaskWoken);

	xQueueReset(xPlayerDisplayPosQueue);//Clear xAcelQueue
	xQueueSendToBack(xPlayerDisplayPosQueue, &Display_Pos_vals, &pxHigherPriorityTaskWoken);
}

