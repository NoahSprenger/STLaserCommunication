/*
 * lcd.cpp
 *
 *  Created on: May 19, 2022
 *      Author: noahr
 */

#include"lcd.h"

lcd::lcd(GPIO_TypeDef* rsPort, uint16_t rsPin, GPIO_TypeDef* enablePort, uint16_t enablePin, GPIO_TypeDef* d4Port, uint16_t d4Pin, GPIO_TypeDef* d5Port, uint16_t d5Pin, GPIO_TypeDef* d6Port, uint16_t d6Pin, GPIO_TypeDef* d7Port, uint16_t d7Pin, TIM_HandleTypeDef* timer) {
	_timer = timer;
	_rs_pin = rsPin;
	_rs_port = rsPort;

	_enable_pin = enablePin;
	_enable_port = enablePort;
	_data_pins[0] = d4Pin;
	_data_pins[1] = d5Pin;
	_data_pins[2] = d6Pin;
	_data_pins[3] = d7Pin;
	_data_ports[0] = d4Port;
	_data_ports[1] = d5Port;
	_data_ports[2] = d6Port;
	_data_ports[3] = d7Port;

	_displayfunction = LCD_4BITMODE | LCD_1LINE | LCD_5x8DOTS;

	begin(16, 1);
}

void lcd::begin(uint8_t cols, uint8_t lines, uint8_t dotsize){
	if (lines > 1) {
		_displayfunction |= LCD_2LINE;
	}
	_numlines = lines;

	setRowOffsets(0x00, 0x40, 0x00 + cols, 0x40 + cols);

	if ((dotsize != LCD_5x8DOTS) && (lines == 1)) {
		_displayfunction |= LCD_5x10DOTS;
	}

//	 this should be handled my main
	// set rs pin output
	// set rw pin output
	// set enable pin output

	// set all data pins output

	delay_us(50000); // 50 ms delay for device init
	// pull rs and rw low to begin commands
	HAL_GPIO_WritePin(_rs_port, _rs_pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(_enable_port, _enable_pin, GPIO_PIN_RESET);
	// set to 4 bit mode
	write4bits(0x03);
	delay_us(4500); // 4.1ms

	// second try
	write4bits(0x03);
	delay_us(150);

	// third try
	write4bits(0x03);

	// set to 4 bit interface
	write4bits(0x02);

	command(LCD_FUNCTIONSET | _displayfunction);

	_displaycontrol = LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF;
	display();
	clear();
	_displaymode = LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT;
	command(LCD_ENTRYMODESET | _displaymode);
}

void lcd::setRowOffsets(int row1, int row2, int row3, int row4){
	_row_offsets[0] = row1;
	_row_offsets[1] = row2;
	_row_offsets[2] = row3;
	_row_offsets[3] = row4;
}

// user high level commands
void lcd::clear() {
	command(LCD_CLEARDISPLAY);
	delay_us(2000);
}

void lcd::home() {
	command(LCD_RETURNHOME);
	delay_us(2000);
}

void lcd::setCursor(uint8_t col, uint8_t row) {
	const size_t max_lines = sizeof(_row_offsets) / sizeof(*_row_offsets);
	if (row >= max_lines) {
		row = max_lines - 1;
	}
	if (row >= _numlines) {
		row = _numlines - 1;
	}
	command(LCD_SETDDRAMADDR | (col + _row_offsets[row]));
}

void lcd::noDisplay() {
	_displaycontrol &= ~LCD_DISPLAYON;
	command(LCD_DISPLAYCONTROL | _displaycontrol);
}

void lcd::display() {
  _displaycontrol |= LCD_DISPLAYON;
  command(LCD_DISPLAYCONTROL | _displaycontrol);
}

// Turns the underline cursor on/off
void lcd::noCursor() {
  _displaycontrol &= ~LCD_CURSORON;
  command(LCD_DISPLAYCONTROL | _displaycontrol);
}
void lcd::cursor() {
  _displaycontrol |= LCD_CURSORON;
  command(LCD_DISPLAYCONTROL | _displaycontrol);
}

// Turn on and off the blinking cursor
void lcd::noBlink() {
  _displaycontrol &= ~LCD_BLINKON;
  command(LCD_DISPLAYCONTROL | _displaycontrol);
}
void lcd::blink() {
  _displaycontrol |= LCD_BLINKON;
  command(LCD_DISPLAYCONTROL | _displaycontrol);
}

// These commands scroll the display without changing the RAM
void lcd::scrollDisplayLeft(void) {
  command(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVELEFT);
}
void lcd::scrollDisplayRight(void) {
  command(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVERIGHT);
}

// This is for text that flows Left to Right
void lcd::leftToRight(void) {
  _displaymode |= LCD_ENTRYLEFT;
  command(LCD_ENTRYMODESET | _displaymode);
}

// This is for text that flows Right to Left
void lcd::rightToLeft(void) {
  _displaymode &= ~LCD_ENTRYLEFT;
  command(LCD_ENTRYMODESET | _displaymode);
}

// This will 'right justify' text from the cursor
void lcd::autoscroll(void) {
  _displaymode |= LCD_ENTRYSHIFTINCREMENT;
  command(LCD_ENTRYMODESET | _displaymode);
}

// This will 'left justify' text from the cursor
void lcd::noAutoscroll(void) {
  _displaymode &= ~LCD_ENTRYSHIFTINCREMENT;
  command(LCD_ENTRYMODESET | _displaymode);
}


// Allows us to fill the first 8 CGRAM locations
// with custom characters
void lcd::createChar(uint8_t location, uint8_t charmap[]) {
  location &= 0x7; // we only have 8 locations 0-7
  command(LCD_SETCGRAMADDR | (location << 3));
  for (int i=0; i<8; i++) {
    write(charmap[i]);
  }
}

/*********** mid level commands, for sending data/cmds */

inline void lcd::command(uint8_t value) {
  send(value, GPIO_PIN_RESET);
}

inline size_t lcd::write(uint8_t value) {
  send(value, GPIO_PIN_SET);
  return 1; // assume success
}

/************ low level data pushing commands **********/

// write either command or data, with automatic 4/8-bit selection
void lcd::send(uint8_t value, GPIO_PinState mode) {
  HAL_GPIO_WritePin(_rs_port, _rs_pin, mode);
  // if there is a RW pin indicated, set it low to Write

  write4bits(value>>4);
  write4bits(value);
}

void lcd::pulseEnable(void) {
	HAL_GPIO_WritePin(_enable_port, _enable_pin, GPIO_PIN_RESET);
	delay_us(1);
	HAL_GPIO_WritePin(_enable_port, _enable_pin, GPIO_PIN_SET);
	delay_us(1);
	HAL_GPIO_WritePin(_enable_port, _enable_pin, GPIO_PIN_RESET);
	delay_us(100);
}

void lcd::write4bits(uint8_t value) {
	for (int i = 0; i < 4; i++) {
		if (((value >> i) & 0x01) == 1) {
			HAL_GPIO_WritePin(_data_ports[i], _data_pins[i], GPIO_PIN_SET);
		} else {
			HAL_GPIO_WritePin(_data_ports[i], _data_pins[i], GPIO_PIN_RESET);
		}

	}
	pulseEnable();
}

void lcd::delay_us(uint16_t us) {
	__HAL_TIM_SET_COUNTER(_timer, 0);
	while (__HAL_TIM_GET_COUNTER(_timer) < us);
}
