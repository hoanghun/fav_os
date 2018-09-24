#include "keyboard.h"
#include <Windows.h>

#include <iostream>

HANDLE hConsoleInput = GetStdHandle(STD_INPUT_HANDLE);
bool Std_In_Redirected = GetFileType(hConsoleInput) != FILE_TYPE_CHAR;

bool Init_Keyboard() {
	//pokusime se vypnout echo na konzoli
	//mj. na Win prestanou detekovat a "krast" napr. Ctrl+C

	if (Std_In_Redirected) return true;	//neni co prepinat s presmerovanym vstupem
		
	DWORD mode;
	bool echo_off = GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), &mode);
	if (echo_off) echo_off = SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), mode & (~(ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT)));
	return echo_off;
}

bool Peek_Char() {
	if (Std_In_Redirected) {		
		return true;	//pokud je uz vstup uzavren, pak bude vzdy dostupny znak signalizujici konec vstupu/souboru
	}

	INPUT_RECORD record;
	DWORD read;
	
	//zkontrolujeme prvni udalost ve frone, protoze jich tam muze byt vic a ruzneho typu
	if (PeekConsoleInputA(hConsoleInput, &record, 1, &read) && (read > 0)) {
		if (record.EventType == KEY_EVENT) 
			return true;		
	}

	return false;
}

uint16_t Read_Char() {
	char ch;
	DWORD read;
	
	if (ReadFile(hConsoleInput, &ch, 1, &read, NULL))	//ReadConsoleA by neprecetlo presmerovany vstup ze souboru
		return read > 0 ? ch : kiv_hal::NControl_Codes::NUL;
		else return kiv_hal::NControl_Codes::EOT;	//chyba, patrne je zavrene vstupni handle
	
}


void __stdcall Keyboard_Handler(kiv_hal::TRegisters &context) {
	switch (static_cast<kiv_hal::NKeyboard>(context.rax.h)) {
		case kiv_hal::NKeyboard::Peek_Char:	context.flags.non_zero = Peek_Char() ? 1 : 0;
											break;

		case kiv_hal::NKeyboard::Read_Char: context.rax.x = Read_Char();
											break;

		default: context.flags.carry = 1;			
	}
}