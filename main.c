#include "config.h"
#include "stm32g4_sys.h"

#include "stm32g4_systick.h"
#include "stm32g4_gpio.h"
#include "stm32g4_uart.h"
#include "stm32g4_utils.h"

#include "tft_ili9341/stm32g4_ili9341.h"
#include "NFC03A1/stm32g4_nfc03a1.h"

#include <stdio.h>
#include <stdbool.h>
#include <string.h>


#define wait		100	//ms

typedef enum{
	ili9341_CS=0,nfc03a1_CS=1
}SlaveSelect ;


void slave_non_ecoute(void);
void slave_ecoute(SlaveSelect Slave_CS);





uint8_t UID_read_HEXA [ISO14443A_MAX_UID_SIZE];


char UID_read_STR [30];
char UID_aurized_1 [9]="a4ae1ab8";
char UID_aurized_2 [9]="c3a02ca8";




#define LED_rouge_GPIO_config()  BSP_GPIO_pin_config(GPIOA, GPIO_PIN_0 , GPIO_MODE_OUTPUT_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_HIGH , GPIO_NO_AF);
#define LED_verte_GPIO_config()   BSP_GPIO_pin_config(GPIOA, GPIO_PIN_1 , GPIO_MODE_OUTPUT_PP, GPIO_NOPULL,  GPIO_SPEED_FREQ_HIGH ,GPIO_NO_AF);
#define Buzzer_GPIO_config()      BSP_GPIO_pin_config(GPIOA, GPIO_PIN_2, GPIO_MODE_OUTPUT_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_LOW, GPIO_NO_AF);


void son_valid_bip(int n){
	for (int i=1 ;i<=n;i++){
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2 , GPIO_PIN_SET);//relais hight
		HAL_Delay(1000);
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2 , GPIO_PIN_RESET);//relais hight
		HAL_Delay(150);
	}
}

void son_invalid_bip(int n){
	for (int i=1 ;i<=n;i++){
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2 , GPIO_PIN_SET);//relais hight
		HAL_Delay(150);
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2 , GPIO_PIN_RESET);//relais hight
		HAL_Delay(150);
	}
}

void initialisation(void)
{

	ILI9341_Init(); //CS à l'état haut en fin d'initialisation du TFT

	BSP_NFC03A1_Init(PCD);/*
	appel ConfigManager_HWInit();
	qui appel à son tour  ConfigManager_Init();
	qui appel à son tour  drv95HF_InitilizeSerialInterface
	qui appel aussi drv95HF_InitializeSPI(void)
	et gère la configuration avec l'état RFTRANS_95HF_NSS_HIGH();
	sachant que :
	 #define RFTRANS_95HF_SPI_NSS_GPIO_PORT	GPIOB
     #define RFTRANS_95HF_SPI_NSS_PIN		GPIO_PIN_6
     #define RFTRANS_95HF_NSS_LOW()			HAL_GPIO_WritePin(RFTRANS_95HF_SPI_NSS_GPIO_PORT, RFTRANS_95HF_SPI_NSS_PIN, 0)
     #define RFTRANS_95HF_NSS_HIGH()			HAL_GPIO_WritePin(RFTRANS_95HF_SPI_NSS_GPIO_PORT, RFTRANS_95HF_SPI_NSS_PIN, 1)
     donc j'en déduis que le CS du NFC est à l'état haut en fin d'initialisation
	 */


	/*au cas où slave_non_ecoute();
ILI9341_Init();
slave_non_ecoute();
BSP_NFC03A1_Init(PCD);
slave_non_ecoute();*/
}

void etat_initial_des_actionneurs(){
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_RESET); //led rouge éteinte
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1 , GPIO_PIN_RESET); // led verte éteinte
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2 , GPIO_PIN_RESET); // buzzer éteint

}




void SPI_REINIT_FOR_NFC03A1(void){
	  BSP_SPI_Init(NFC_SPI, FULL_DUPLEX, MASTER, SPI_BAUDRATEPRESCALER_256);
}

void SPI_REINIT_FOR_ILI9341(void){
	BSP_SPI_Init(ILI9341_SPI, FULL_DUPLEX, MASTER, SPI_BAUDRATEPRESCALER_16);
}


void slave_ecoute(SlaveSelect Slave_CS){
	slave_non_ecoute();
	switch(Slave_CS){
	case ili9341_CS :
	SPI_REINIT_FOR_ILI9341();
	RFTRANS_95HF_NSS_HIGH() ;
	ILI9341_CS_RESET();
	break ;

	case nfc03a1_CS :
	SPI_REINIT_FOR_NFC03A1();
	ILI9341_CS_SET() ;
	RFTRANS_95HF_NSS_LOW() ;
	break ;

	default :
	break ;
	}
}

void slave_non_ecoute(void){
	RFTRANS_95HF_NSS_HIGH();  ILI9341_CS_SET() ;
}


bool uid_valide(char *uid_tagged){
	 return memcmp(uid_tagged,UID_aurized_1,8)==0 || memcmp(uid_tagged,UID_aurized_2,8)==0;
	}



void write_LED(bool b)
{
	HAL_GPIO_WritePin(LED_GREEN_GPIO, LED_GREEN_PIN, b);
}
/* On "utilise" le caractère pour vider le buffer de réception */

bool char_received(uart_id_t uart_id)
{
	if( BSP_UART_data_ready(uart_id) )	/* Si un caractère est reçu sur l'UART 2*/
	{
		BSP_UART_get_next_byte(uart_id);
		return true;
	}
	else
		return false;
}

void heartbeat(void)
{
	while(! char_received(UART2_ID) )
	{
		write_LED(false);
		HAL_Delay(50);
		write_LED(true);
		HAL_Delay(1500);
	}
}


/**
 * @brief  Point d'entrée de votre application
 */
int main(void)
{
	/* Cette ligne doit rester la première de votre main !
	 * Elle permet d'initialiser toutes les couches basses des drivers (Hardware Abstraction Layer),
	 * condition préalable indispensable à l'exécution des lignes suivantes.
	 */
	HAL_Init();

	/* Initialisation des périphériques utilisés dans votre programme */
	BSP_GPIO_enable();
	BSP_UART_init(UART2_ID,115200);
	BSP_SYS_set_std_usart(UART2_ID, UART2_ID, UART2_ID);



	/* Indique que les printf sont dirigés vers l'UART2 */
	//BSP_SYS_set_std_usart(UART2_ID, UART2_ID, UART2_ID);

	/* Initialisation du port de la led Verte (carte Nucleo) */
	BSP_GPIO_pin_config(LED_GREEN_GPIO, LED_GREEN_PIN, GPIO_MODE_OUTPUT_PP,GPIO_NOPULL,GPIO_SPEED_FREQ_HIGH,GPIO_NO_AF);

	/* Hello student */
	//printf("Hi <Student>, can you read me?\n");

	//heartbeat();

	LED_rouge_GPIO_config();
	LED_verte_GPIO_config();
	Buzzer_GPIO_config()

	initialisation();

	etat_initial_des_actionneurs();








	/* Tâche de fond, boucle infinie, Infinite loop,... quelque soit son nom vous n'en sortirez jamais */
	while (1) {

		memset(UID_read_STR, 0, sizeof(UID_read_STR));
		memset(UID_read_HEXA, 0, ISO14443A_MAX_UID_SIZE);

		slave_ecoute(nfc03a1_CS);
		uint8_t tag = ConfigManager_TagHunting(TRACK_ALL);

		if (tag == TRACK_NFCTYPE2) {

			ISO14443A_CARD infos;
			BSP_NFC03A1_get_ISO14443A_infos(&infos);
			uint8_t i;
			//printf("uid = ");
			for (i = 0; i < infos.UIDsize; i++) {
				//printf("%02x ",infos.UID[i]);
				//printf("\n");
				UID_read_HEXA[i] = infos.UID[i];
			}
			/*pour des raisons de simplitude on va lire que les 4 premiers données de infos.UIDsize
			 peut importe la longeur des données*/
			if (infos.UIDsize >= 4) {
				sprintf(UID_read_STR, "%02x%02x%02x%02x", UID_read_HEXA[0],UID_read_HEXA[1], UID_read_HEXA[2], UID_read_HEXA[3]);
				slave_ecoute(ili9341_CS);
				//ILI9341_Puts(25,200, UID_read_STR , &Font_7x10, ILI9341_COLOR_BROWN, ILI9341_COLOR_WHITE);


				ILI9341_PutBigs(25, 70, "             ", &Font_7x10,ILI9341_COLOR_BLUE, ILI9341_COLOR_BLACK, 3, 3);
				ILI9341_PutBigs(25, 100,"             ", &Font_7x10,ILI9341_COLOR_BLUE, ILI9341_COLOR_BLACK, 3, 3);
				ILI9341_PutBigs(25, 130,"          ", &Font_7x10,ILI9341_COLOR_BLUE, ILI9341_COLOR_BLACK, 3, 3);

				ILI9341_PutBigs(0, 0, "check your UID CARD : ", &Font_7x10,ILI9341_COLOR_BLUE, ILI9341_COLOR_BLACK, 3, 3);
				ILI9341_PutBigs(25, 100, UID_read_STR, &Font_7x10,ILI9341_COLOR_BLUE, ILI9341_COLOR_BLACK, 3, 3);
				HAL_Delay(1000);
				ILI9341_PutBigs(0, 0,"                     ", &Font_7x10,ILI9341_COLOR_BLUE, ILI9341_COLOR_BLACK, 3, 3);
				if(uid_valide(UID_read_STR)){
					ILI9341_PutBigs(25, 150, "UID autorized", &Font_7x10,ILI9341_COLOR_BLUE, ILI9341_COLOR_GREEN , 2, 3);
					HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1 , GPIO_PIN_SET); // led verte allumé
					son_valid_bip(1);
					HAL_Delay(1000);
					ILI9341_PutBigs(0, 150, "                      ", &Font_7x10,ILI9341_COLOR_BLUE, ILI9341_COLOR_BLACK , 3, 3);

				}
				else{
					HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0 , GPIO_PIN_SET); // led rouge allumé
					ILI9341_PutBigs(25, 150, "UID denided", &Font_7x10,ILI9341_COLOR_BLUE, ILI9341_COLOR_RED , 2, 3);
					son_invalid_bip(3);
					HAL_Delay(1000);
					ILI9341_PutBigs(0, 150, "                      ", &Font_7x10,ILI9341_COLOR_BLUE, ILI9341_COLOR_BLACK , 3, 3);

				}

			}

			else {
				strcpy(UID_read_STR, "tag non identifie");
			}
		}

		else {

			slave_ecoute(ili9341_CS);
			//ILI9341_Puts(25,200, UID_read_STR , &Font_7x10, ILI9341_COLOR_BROWN, ILI9341_COLOR_WHITE);
			ILI9341_PutBigs(25, 70, " waiting   ", &Font_7x10,ILI9341_COLOR_BLUE, ILI9341_COLOR_BLACK, 3, 3);
			ILI9341_PutBigs(25, 100,"  for     ", &Font_7x10,ILI9341_COLOR_BLUE, ILI9341_COLOR_BLACK, 3, 3);
			ILI9341_PutBigs(25, 130,"  tags    ", &Font_7x10,ILI9341_COLOR_BLUE, ILI9341_COLOR_BLACK, 3, 3);


		}

		etat_initial_des_actionneurs();

		HAL_Delay(wait);


	}
}


