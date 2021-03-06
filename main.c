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
#include "MoppyEx.h"


//***********************************************************************************************
// System-Einstellungen
//***********************************************************************************************


void SetupSystem (void)
{
	SystemCoreClockUpdate();
	InitializeUart(115200);
	//InitializeUart(921600);


	GPIO_InitTypeDef GPIO_InitStructure;
	uint32_t i, j;

	// Output SYSCLK clock on MCO pin
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	RCC_MCOConfig(RCC_MCO_SYSCLK);

	GPIO_WriteBit(GPIOA, GPIO_Pin_0, Bit_SET);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO | RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOC, ENABLE);


	if (SysTick_Config(SystemCoreClock / 1000)) {
		printf("Systick failed, system halted\n");
		while (1);
	}

	mount_sd_card();
	SetupMoppyEx();
}


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

					printf(" %06d ", msg.dwAbsPos);


					if (muGetMIDIMsgName(str, ev))
						printf("%s  ", str);

					switch(ev)
					{
					case	msgNoteOff:
						muGetNameFromNote(str, msg.MsgData.NoteOff.iNote);
						printf("(%d) %s", msg.MsgData.NoteOff.iChannel, str);
						break;
					case	msgNoteOn:
						muGetNameFromNote(str, msg.MsgData.NoteOn.iNote);
						printf("  (%d) %s %d", msg.MsgData.NoteOn.iChannel, str, msg.MsgData.NoteOn.iVolume);
						break;
					case	msgNoteKeyPressure:
						muGetNameFromNote(str, msg.MsgData.NoteKeyPressure.iNote);
						printf("(%d) %s %d", msg.MsgData.NoteKeyPressure.iChannel,
							str,
							msg.MsgData.NoteKeyPressure.iPressure);
						break;
					case	msgSetParameter:
						muGetControlName(str, msg.MsgData.NoteParameter.iControl);
						printf("(%d) %s -> %d", msg.MsgData.NoteParameter.iChannel,
							str, msg.MsgData.NoteParameter.iParam);
						break;
					case	msgSetProgram:
						muGetInstrumentName(str, msg.MsgData.ChangeProgram.iProgram);
						printf("(%d) %s", msg.MsgData.ChangeProgram.iChannel, str);
						break;
					case	msgChangePressure:
						muGetControlName(str, msg.MsgData.ChangePressure.iPressure);
						printf("(%d) %s", msg.MsgData.ChangePressure.iChannel, str);
						break;
					case	msgSetPitchWheel:
						printf("(%d) %d", msg.MsgData.PitchWheel.iChannel,
							msg.MsgData.PitchWheel.iPitch);
						break;

					case	msgMetaEvent:
						printf("---- ");
						switch(msg.MsgData.MetaEvent.iType)
						{
						case	metaMIDIPort:
							printf("MIDI Port = %d", msg.MsgData.MetaEvent.Data.iMIDIPort);
							break;

						case	metaSequenceNumber:
							printf("Sequence Number = %d",msg.MsgData.MetaEvent.Data.iSequenceNumber);
							break;

						case	metaTextEvent:
							printf("Text = '%s'",msg.MsgData.MetaEvent.Data.Text.pData);
							break;
						case	metaCopyright:
							printf("Copyright = '%s'",msg.MsgData.MetaEvent.Data.Text.pData);
							break;
						case	metaTrackName:
							printf("Track name = '%s'",msg.MsgData.MetaEvent.Data.Text.pData);
							break;
						case	metaInstrument:
							printf("Instrument = '%s'",msg.MsgData.MetaEvent.Data.Text.pData);
							break;
						case	metaLyric:
							printf("Lyric = '%s'",msg.MsgData.MetaEvent.Data.Text.pData);
							break;
						case	metaMarker:
							printf("Marker = '%s'",msg.MsgData.MetaEvent.Data.Text.pData);
							break;
						case	metaCuePoint:
							printf("Cue point = '%s'",msg.MsgData.MetaEvent.Data.Text.pData);
							break;
						case	metaEndSequence:
							printf("End Sequence");
							break;
						case	metaSetTempo:
							printf("Tempo = %d",msg.MsgData.MetaEvent.Data.Tempo.iBPM);
							break;
						case	metaSMPTEOffset:
							printf("SMPTE offset = %d:%d:%d.%d %d",
								msg.MsgData.MetaEvent.Data.SMPTE.iHours,
								msg.MsgData.MetaEvent.Data.SMPTE.iMins,
								msg.MsgData.MetaEvent.Data.SMPTE.iSecs,
								msg.MsgData.MetaEvent.Data.SMPTE.iFrames,
								msg.MsgData.MetaEvent.Data.SMPTE.iFF
								);
							break;
						case	metaTimeSig:
							printf("Time sig = %d/%d",msg.MsgData.MetaEvent.Data.TimeSig.iNom,
								msg.MsgData.MetaEvent.Data.TimeSig.iDenom/MIDI_NOTE_CROCHET);
							break;
						case	metaKeySig:
							if (muGetKeySigName(str, msg.MsgData.MetaEvent.Data.KeySig.iKey))
								printf("Key sig = %s", str);
							break;

						case	metaSequencerSpecific:
							printf("Sequencer specific = ");
							HexList(msg.MsgData.MetaEvent.Data.Sequencer.pData, msg.MsgData.MetaEvent.Data.Sequencer.iSize); // ok
							printf("\r\n");
							break;
						}
						break;

					case	msgSysEx1:
					case	msgSysEx2:
						printf("Sysex = ");
						HexList(msg.MsgData.SysEx.pData, msg.MsgData.SysEx.iSize); // ok
						break;
					}

					if (ev == msgSysEx1 || ev == msgSysEx1 || (ev==msgMetaEvent && msg.MsgData.MetaEvent.iType==metaSequencerSpecific))
					{
						/* Already done a hex dump */
					}
					else
					{
						printf("  [");
						if (msg.bImpliedMsg) printf("%X!", msg.iImpliedMsg);
						for(j=0;j<msg.iMsgSize;j++)
							printf("%X ", msg.data[j]);
						printf("]\r\n");
					}
				}
			}
		}

		midiReadFreeMessage(&msg);
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
	SetupSystem();        // System-Einstellungen


	playMidiFile("overworld_deep3.mid");
	//playMidiFile("jammind3.mid");


    // loop forever
    while(1);
}






















