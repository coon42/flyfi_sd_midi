/*********************************************************************
 *
 * Code testing the basic functionality of STM32 on VL discovery kit
 * The code displays message via UART1 using printf mechanism
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of either the GNU General Public License version 2
 * or the GNU Lesser General Public License version 2.1, both as
 * published by the Free Software Foundation.
 *
 *********************************************************************
 * FileName:    main.c
 * Depends:
 * Processor:   STM32F100RBT6B
 *
 * Author               Date       Comment
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * Kubik                13.11.2010 Initial code
 * Kubik                14.11.2010 Added debug code
 * Kubik                15.11.2010 Debug now goes to UART2
 * Kubik                ??.11.2010 Added test code for quadrature encoder
 * Kubik                 4.12.2010 Implemented SD/MMC card support
 ********************************************************************/

// important:
//
// - commented out int fputc(int ch, FILE * f) in printf.c and redefined in main.h to get printf sending directly to USART1
// - commented out struct _reent *_impure_ptr = &r; in printf.c to prevent a linker error


//-------------------------------------------------------------------
// Includes
#include <stdio.h>
#include <inttypes.h>
#include "stm32f10x.h"
#include "uart.h"
#include "midifile.h"




void playMidiFile(const char *pFilename)
{
	_MIDI_FILE pMF;
	BOOL open_success;
	char str[128];
	int ev;

	midiFileOpen(&pMF, pFilename, &open_success);

	if (open_success)
	{
		MIDI_MSG msg;
		int i, iNum;
		unsigned int j;
		int any_track_had_data;

		midiReadInitMessage(&msg);
		iNum = midiReadGetNumTracks(&pMF);


		any_track_had_data = 1;

		while(any_track_had_data)
		{
			any_track_had_data = 0;

			for(i=0;i < iNum;i++)
			{
				if(midiReadGetNextMessage(&pMF, i, &msg)) // maybe this function will be the hardest part
				{
					any_track_had_data = 1;

					printf("[Track: %d]", i);

					if (msg.bImpliedMsg)
					{ ev = msg.iImpliedMsg; }
					else
					{ ev = msg.iType; }


					printf(" %d ", msg.dwAbsPos);


					if (muGetMIDIMsgName(str, ev))
						printf("%s", str);

					switch(ev)
					{
					case	msgNoteOff:
						muGetNameFromNote(str, msg.MsgData.NoteOff.iNote);
						printf("(%d) %s", msg.MsgData.NoteOff.iChannel, str);
						break;
					case	msgNoteOn:
						muGetNameFromNote(str, msg.MsgData.NoteOn.iNote);
						printf("(%d) %s %d", msg.MsgData.NoteOn.iChannel, str, msg.MsgData.NoteOn.iVolume);
						break;
					case	msgSetPitchWheel:
						printf("(%d) %d", msg.MsgData.PitchWheel.iChannel,
							msg.MsgData.PitchWheel.iPitch);
						break;

					case	msgMetaEvent:
						printf("---- ");
						switch(msg.MsgData.MetaEvent.iType)
						{
						case	metaLyric:
							printf("Lyric = '%s'", msg.MsgData.MetaEvent.Data.Text.pData);
							break;
						case	metaSetTempo:
							printf("Tempo = %d", msg.MsgData.MetaEvent.Data.Tempo.iBPM);
							break;
						}
						break;
					}

					printf("\r\n");
				}
			}
		}

		//midiReadFreeMessage(&msg);
	}
	else
	{
		printf("Open Failed!\nInvalid MIDI-File Header!\n");

	}
}



//-------------------------------------------------------------------
// Defines

//---------------------------------------------------------------------------
// Static variables

//---------------------------------------------------------------------------
// Local functions



//---------------------------------------------------------------------------
// Local functions

int main(void) {
	GPIO_InitTypeDef GPIO_InitStructure;
    uint32_t i, j;

	// Output SYSCLK clock on MCO pin
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	RCC_MCOConfig(RCC_MCO_SYSCLK);


	//
	// Configure debug trigger as output (GPIOA pin 0)
	//

    GPIO_WriteBit(GPIOA, GPIO_Pin_0, Bit_SET);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOA, &GPIO_InitStructure);


	//
	// Configure peripherals used - basically enable their clocks to enable them
	//

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO | RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOC, ENABLE);

    //
    // Configure LEDs - we use them as an indicator the platform is alive somehow
    //

    InitializeUart(115200);


	if (SysTick_Config(SystemCoreClock / 1000)) {
        printf("Systick failed, system halted\n");
		while (1);
	}

	mount_sd_card();


	playMidiFile("overworld_deep3.mid");
	//playMidiFile("jammind3.mid");


    // loop forever
    while(1);
}






















