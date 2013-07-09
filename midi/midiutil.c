/*
 * midiutil.c - Auxiliary MIDI functionality. Requires Steevs MIDI Library
 *		        (midiinfo.h) for enumerations used in name mapping.
 * Version 1.4
 *
 *  AUTHOR: Steven Goodwin (StevenGoodwin@gmail.com)
 *			Copyright 2010, Steven Goodwin.
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of
 *  the License,or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "midifile.h"
#include "midiutil.h"


static char *szNoteName[] = {
	"C ",
	"Db",
	"D ",
	"Eb",
	"E ",
	"F ",
	"Gb",
	"G ",
	"Ab",
	"A ",
	"Bb",
	"B ",
};

static float fFreqlist[] = {
	261.63f,
	277.18f,
	293.66f,
	311.13f,
	329.63f,
	349.23f,
	369.99f,
	392.00f,
	415.30f,
	440.00f,
	466.16f,
	493.88f,
};

/*
** Name resolving functions
*/

BOOL muGetMIDIMsgName(char *pName, tMIDI_MSG iMsg)
{
	switch(iMsg)
		{
		case	msgNoteOff:
				strcpy(pName, "Note off");
				break;

		case	msgNoteOn:
				strcpy(pName, "Note on");
				break;

		case	msgNoteKeyPressure:
				strcpy(pName, "Note key pressure");
				break;

		case	msgSetParameter:
				strcpy(pName, "Set parameter");
				break;

		case	msgSetProgram:
				strcpy(pName, "Set program");
				break;

		case	msgChangePressure:
				strcpy(pName, "Change pressure");
				break;

		case	msgSetPitchWheel:
				strcpy(pName, "Set pitch wheel");
				break;

		case	msgMetaEvent:
				strcpy(pName, "Meta event");
				break;

		case	msgSysEx1:
				strcpy(pName, "SysEx1");
				break;

		case	msgSysEx2:
				strcpy(pName, "SysEx2");
				break;

		default:
				return FALSE;
		}
	return TRUE;
}


BOOL muGetKeySigName(char *pName, tMIDI_KEYSIG iKey)
{
static char *iKeysList[2][8] = {
/*#*/{"C ", "G ", "D ", "A ", "E ", "B ", "F#", "C#", },
/*b*/{"C ", "F ", "Bb", "Eb", "Ab", "Db", "Gb", "Cb", },
};

int iRootNum = (iKey&7);
int iFlats = (iKey&keyMaskNeg);
int iMin = (iKey&keyMaskMin);

	strcpy(pName,iKeysList[iFlats?1:0][iRootNum]);
	strcat(pName,iMin?" Min":" Maj");
	return TRUE;
}

BOOL muGetTextName(char *pName, tMIDI_TEXT iEvent)
{
	if (iEvent<1 || iEvent>7)	return FALSE;
	return muGetMetaName(pName, (tMIDI_META)iEvent);
}

BOOL muGetMetaName(char *pName, tMIDI_META iEvent)
{
	switch(iEvent)
		{
		case	metaSequenceNumber:	strcpy(pName, "Sequence Number");	break;
		case	metaTextEvent:		strcpy(pName, "Text Event");		break;
		case	metaCopyright:		strcpy(pName, "Copyright");			break;
		case	metaTrackName:		strcpy(pName, "Track Name");		break;
		case	metaInstrument:		strcpy(pName, "Instrument");		break;
		case	metaLyric:			strcpy(pName, "Lyric");				break;
		case	metaMarker:			strcpy(pName, "Marker");			break;
		case	metaCuePoint:		strcpy(pName, "Cue Point");			break;
		case	metaMIDIPort:		strcpy(pName, "MIDI Port");		 break;
		case	metaEndSequence:	strcpy(pName, "End Sequence");		break;
		case	metaSetTempo:		strcpy(pName, "Set Tempo");			break;
		case	metaSMPTEOffset:	strcpy(pName, "SMPTE Offset");		break;
		case	metaTimeSig:		strcpy(pName, "Time Sig");			break;
		case	metaKeySig:			strcpy(pName, "Key Sig");			break;
		case	metaSequencerSpecific:	strcpy(pName, "Sequencer Specific");	break;
		default:	return FALSE;
		}
	return TRUE;

}


/*
** Conversion Functions
*/
int muGetNoteFromName(const char *pName)
{
int note_map[] = {9, 11, 0, 2, 4, 5, 7};
char *p, cpy[16];
int note=0;

	strncpy(cpy, pName, 15);
	cpy[15] = '\0';
	p = cpy;

	while(!isalpha(*p) && *p)
		p++;
	
	if (*p)
		{
		note = toupper(*p)-'A';
		if (note >= 0 && note <= 7)
			{
			note = note_map[note];
			p++;
			if (*p == 'b')
				note--, p++;
			else if (*p == '#')
				note++, p++;
			
			note += atoi(p)*12+MIDI_NOTE_C0;
			}
		}
	
	return note;
}

char *muGetNameFromNote(char *pStr, int iNote)
{
	if (!pStr)		return NULL;

	if (iNote<0 || iNote>127)
		strcpy(pStr, "ERR");
	else
		sprintf(pStr, "%s%d", szNoteName[iNote%12], ((iNote-MIDI_NOTE_C0)/12));
	
	return pStr;
}

float muGetFreqFromNote(int iNote)
{
int oct = iNote/12-5;
float freq;

	if (iNote<0 || iNote>127)	return 0;

	freq = fFreqlist[iNote%12];
	
	while(oct > 0)
		freq *= 2.0f, oct--;
	
	while(oct < 0)
		freq /= 2.0f, oct++;
	
	return freq;
}

int muGetNoteFromFreq(float fFreq)
{
/* This is for completeness, I'm not sure of how often it
** will get used. Therefore, the code is un-optimised :)
*/
int iNote, iBestNote=0;
float fDiff=20000, f;

	for(iNote=0;iNote<127;++iNote)
		{
		f = muGetFreqFromNote(iNote);
		f -= fFreq; if (f<0) f=-f;
		if (f < fDiff)
			{
			fDiff = f; 
			iBestNote = iNote;
			}
		}
		
	return iBestNote;
}


int muGuessChord(const int *pNoteStatus, const int channel, const int lowRange, const int highRange) {
	int octave[24];
	int i;
	int lowestNote=999;
	int startNote = 999;
	int chordRoot = 0;
	int chordType = 0;
	int chordAdditions = 0;

	for(i=0;i<24;++i) {
		octave[i] = 0;
	}

	for(i=lowRange;i<=highRange;++i) {
		if (pNoteStatus[channel*128 + i]) {
			if (i<lowestNote) {
				lowestNote = i;
			}
			++octave[i%12];
			++octave[i%12+12];
			if ((i%12) < startNote) {
				startNote = i%12;
			}
		}
	}

	if (lowestNote == 999) {
		return -1;
	}

	/* Bring it into line with the 0-11 range */
	lowestNote %= 12;
	
	/* Majors */
	if (octave[startNote+3] && octave[startNote+8]) {
		chordRoot = startNote+8;
		chordType = CHORD_TYPE_MAJOR;
	} else if (octave[startNote+5] && octave[startNote+9]) {
		chordRoot = startNote+5;
		chordType = CHORD_TYPE_MAJOR;
	} else if (octave[startNote+4] && octave[startNote+7]) {
		chordRoot = startNote;
		chordType = CHORD_TYPE_MAJOR;

	/* Minor */
	} else if (octave[startNote+4] && octave[startNote+9]) {
		chordRoot = startNote+9;
		chordType = CHORD_TYPE_MINOR;
	} else if (octave[startNote+5] && octave[startNote+8]) {
		chordRoot = startNote+5;
		chordType = CHORD_TYPE_MINOR;
	} else if (octave[startNote+3] && octave[startNote+7]) {
		chordRoot = startNote;
		chordType = CHORD_TYPE_MINOR;

	/* Diminished */
	} else if (octave[startNote+3] && octave[startNote+6]) {
		chordRoot = lowestNote;
		chordType = CHORD_TYPE_DIM;
	} else if (octave[startNote+6] && octave[startNote+9]) {
		chordRoot = lowestNote;
		chordType = CHORD_TYPE_DIM;

	/* Augmented */
	} else if (octave[startNote+4] && octave[startNote+8]) {
		chordRoot = lowestNote;
		chordType = CHORD_TYPE_AUG;
	}

	if (octave[chordRoot + 10]) {
		chordAdditions |= CHORD_ADD_7TH;
	}
	if (octave[chordRoot + 11]) {
		chordAdditions |= CHORD_ADD_MAJ7TH;
	}
	if (octave[chordRoot + 2]) {
		chordAdditions |= CHORD_ADD_9TH;
	}

	chordRoot %= 12;

	if (chordType == 0) {
		return -1;
	}

	return chordRoot | chordType | chordAdditions | (lowestNote<<16);
}

char *muGetChordName(char *str, int chord) {
	int root = chord & CHORD_ROOT_MASK;
	int bass = (chord & CHORD_BASS_MASK) >> 16;

	if (root < 0 || root > 11) {
		root = 0;
	}

	if (bass < 0 || bass > 11) {
		bass = 0;
	}

	strcpy(str, szNoteName[root]);

	switch(chord & CHORD_TYPE_MASK) {
		case CHORD_TYPE_MAJOR:
			break;
		case CHORD_TYPE_MINOR:
			strcat(str, "m");
			break;
		case CHORD_TYPE_AUG:
			strcat(str, " aug");
			break;
		case CHORD_TYPE_DIM:
			strcat(str, " dim");
			break;
	}

	if (chord & CHORD_ADD_7TH) {
		strcat(str, "+7");
	}
	if (chord & CHORD_ADD_9TH) {
		strcat(str, "+9");
	}
	if (chord & CHORD_ADD_MAJ7TH) {
		strcat(str, "+7M");
	}
	
	if (bass != root) {
		strcat(str, "/");
		strcat(str, szNoteName[bass]);
	}

	return str;
}

