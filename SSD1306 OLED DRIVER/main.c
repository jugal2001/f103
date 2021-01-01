//DEMO PROGRAM

//	PINS
//	SDA	| SCL
//	B11 | B10

#include "stm32f1xx.h"

#include "fonts.h"
#include "ssd1306.h"
#include "mydelay.h"

void i2c_init(void);
void gpio_init(void);
int main(void)
{
	//Initialise I2C2 and GPIO pins
	i2c_init();
	gpio_init();

	SSD1306_Init();  // Initialise the display

	SSD1306_GotoXY(0, 0);
	SSD1306_Puts("HELLO", &Font_7x10, 1);
	SSD1306_GotoXY(0, 10);
	SSD1306_Puts("WORLD", &Font_7x10, 1);
	SSD1306_GotoXY(0, 20);
	SSD1306_Puts("OLED TEST", &Font_7x10, 1);

	SSD1306_UpdateScreen(); //display

	while (1)
	{

		SSD1306_ScrollRight(0x00, 0x0f);    // scroll entire screen right
		delay_ms(500);
		SSD1306_ScrollLeft(0x00, 0x0f);  // scroll entire screen left
		delay_ms(500);
		SSD1306_Scrolldiagright(0x00, 0x0f); // scroll entire screen diagonal right
		delay_ms(500);
		SSD1306_Scrolldiagleft(0x00, 0x0f); // scroll entire screen diagonal left
		delay_ms(500);
		SSD1306_Stopscroll();   // stop scrolling. If not done, screen will keep on scrolling
		delay_ms(500);
		SSD1306_ToggleInvert();
		SSD1306_UpdateScreen();
	}
}

void i2c_init(void)
{
	RCC->APB1ENR |= RCC_APB1ENR_I2C2EN; //Enable clock for I2C2

	//calculate according to datasheet (here Fpclk1 = 8 mhz and I2C freq = 100KHz)
	I2C2->CR2 |= 8; 			// 8mhz Fpclk1
	I2C2->CCR |= 40;			// ((I2CPERIOD/2)/Tpclk1)
	I2C2->TRISE |= 9;			// (1000nS/Tpclk1) + 1
	I2C2->CR1 |= I2C_CR1_ACK;	//Enable ACKs
	I2C2->CR1 |= I2C_CR1_PE; 	//Enable the peripheral
}

void gpio_init(void)
{
	RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;			//Enable clock for port B
	GPIOB->CRH |= GPIO_CRH_MODE10 | GPIO_CRH_CNF10 | GPIO_CRH_MODE11 | GPIO_CRH_CNF11;	//Alternate function open drain output B11-SDA , B10-SCL
}
