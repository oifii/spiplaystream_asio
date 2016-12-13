/*
 * Copyright (c) 2010-2016 Stephane Poirier
 *
 * stephane.poirier@oifii.org
 *
 * Stephane Poirier
 * 3532 rue Ste-Famille, #3
 * Montreal, QC, H2X 2L1
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <conio.h>
#include "bassasio.h"
#include "bass.h"


#include <assert.h>
#include <string>
using namespace std;
#include <map>
map<string,int> global_asiodevicemap;


// display error messages
void Error(const char *text) 
{
	printf("Error(%d/%d): %s\n",BASS_ErrorGetCode(),BASS_ASIO_ErrorGetCode(),text);
	BASS_ASIO_Free();
	BASS_Free();
	exit(0);
}

// ASIO function
DWORD CALLBACK AsioProc(BOOL input, DWORD channel, void *buffer, DWORD length, void *user)
{
	DWORD c=BASS_ChannelGetData((DWORD)user,buffer,length);
	if (c==-1) c=0; // an error, no data
	return c;
}

void main(int argc, char **argv)
{
	int nShowCmd = false;
	ShellExecuteA(NULL, "open", "begin.bat", "", NULL, nShowCmd);

	///////////////////
	//read in arguments
	///////////////////
	string filename = "testwav.wav"; //usage: spiplaystream testwav.wav
	if(argc>1)
	{
		//first argument is the filename
		filename = argv[1];
	}
	float fSecondsPlay = -1.0; //negative for playing only once
	if(argc>2)
	{
		fSecondsPlay = atof(argv[2]);
	}
	string audiodevicename="E-MU ASIO"; //"Speakers (2- E-MU E-DSP Audio Processor (WDM))"
	if(argc>3)
	{
		audiodevicename = argv[3]; //for spi, device name could be "E-MU ASIO", "Speakers (2- E-MU E-DSP Audio Processor (WDM))", etc.
	}
    int outputAudioChannelSelectors[2]; //int outputChannelSelectors[1];
	/*
	outputAudioChannelSelectors[0] = 0; // on emu patchmix ASIO device channel 1 (left)
	outputAudioChannelSelectors[1] = 1; // on emu patchmix ASIO device channel 2 (right)
	*/
	/*
	outputAudioChannelSelectors[0] = 2; // on emu patchmix ASIO device channel 3 (left)
	outputAudioChannelSelectors[1] = 3; // on emu patchmix ASIO device channel 4 (right)
	*/
	outputAudioChannelSelectors[0] = 2; // on emu patchmix ASIO device channel 15 (left)
	outputAudioChannelSelectors[1] = 3; // on emu patchmix ASIO device channel 16 (right)
	if(argc>4)
	{
		outputAudioChannelSelectors[0]=atoi(argv[4]); //0 for first asio channel (left) or 2, 4, 6 and 8 for spi (maxed out at 10 asio output channel)
	}
	if(argc>5)
	{
		outputAudioChannelSelectors[1]=atoi(argv[5]); //1 for second asio channel (right) or 3, 5, 7 and 9 for spi (maxed out at 10 asio output channel)
	}
	DWORD chan,time;
	BOOL ismod;
	QWORD pos;
	int a;//device=0;
	BASS_CHANNELINFO i;

	printf("Simple console mode BASS+ASIO example : MOD/MPx/OGG/WAV player\n"
			"--------------------------------------------------------------\n");

	// check the correct BASS was loaded
	if (HIWORD(BASS_GetVersion())!=BASSVERSION) 
	{
		printf("An incorrect version of BASS was loaded");
		return;
	}	
	/*
	if (argc==1) 
	{
		printf("\tusage: spiplaystream_asio <file> [devicename] [leftasiochannel] [rightasiochannel]\n");
		return;
	}
	*/
	BASS_ASIO_DEVICEINFO myBASS_ASIO_DEVICEINFO;
	for (int i=0;BASS_ASIO_GetDeviceInfo(i,&myBASS_ASIO_DEVICEINFO);i++)
	{
		string devicenamestring = myBASS_ASIO_DEVICEINFO.name;
		global_asiodevicemap.insert(pair<string,int>(devicenamestring,i));
	}
	int deviceid=0;
	map<string,int>::iterator it;
	it = global_asiodevicemap.find(audiodevicename);
	if(it!=global_asiodevicemap.end())
	{
		deviceid = (*it).second;
		printf("%s maps to %d\n", audiodevicename.c_str(), deviceid);
	}
	else
	{
		assert(false);
		//Terminate();
	}

	// not playing anything via BASS, so don't need an update thread
	BASS_SetConfig(BASS_CONFIG_UPDATEPERIOD,0);
	// setup BASS - "no sound" device
	//BASS_Init(0,48000,0,0,NULL);
	BASS_Init(0,44100,0,0,NULL);

	// try streaming the file/url
	if ((chan=BASS_StreamCreateFile(FALSE,filename.c_str(),0,0,BASS_SAMPLE_LOOP|BASS_STREAM_DECODE|BASS_SAMPLE_FLOAT))
		|| (chan=BASS_StreamCreateURL(filename.c_str(),0,BASS_SAMPLE_LOOP|BASS_STREAM_DECODE|BASS_SAMPLE_FLOAT,0,0))) 
	{
		pos=BASS_ChannelGetLength(chan,BASS_POS_BYTE);
		if (BASS_StreamGetFilePosition(chan,BASS_FILEPOS_DOWNLOAD)!=-1) 
		{
			// streaming from the internet
			if (pos!=-1)
				printf("streaming internet file [%I64u bytes]",pos);
			else
				printf("streaming internet file");
		} else
			printf("streaming file [%I64u bytes]",pos);
		ismod=FALSE;
	} 
	else 
	{
		// try loading the MOD (with looping, sensitive ramping, and calculate the duration)
		if (!(chan=BASS_MusicLoad(FALSE,filename.c_str(),0,0,BASS_SAMPLE_LOOP|BASS_STREAM_DECODE|BASS_SAMPLE_FLOAT|BASS_MUSIC_RAMPS|BASS_MUSIC_PRESCAN,0)))
			// not a MOD either
			Error("Can't play the file");
		{ // count channels
			float dummy;
			for (a=0;BASS_ChannelGetAttribute(chan,BASS_ATTRIB_MUSIC_VOL_CHAN+a,&dummy);a++);
		}
		printf("playing MOD music \"%s\" [%u chans, %u orders]",
			BASS_ChannelGetTags(chan,BASS_TAG_MUSIC_NAME),a,BASS_ChannelGetLength(chan,BASS_POS_MUSIC_ORDER));
		pos=BASS_ChannelGetLength(chan,BASS_POS_BYTE);
		ismod=TRUE;
	}

	// display the time length
	if (pos!=-1) 
	{
		time=(DWORD)BASS_ChannelBytes2Seconds(chan,pos);
		printf(" %u:%02u\n",time/60,time%60);
	} else // no time length available
		printf("\n");

	// setup ASIO stuff
	if (!BASS_ASIO_Init(deviceid,BASS_ASIO_THREAD))
		Error("Can't initialize ASIO device");
	BASS_ChannelGetInfo(chan,&i);
	//BASS_ASIO_ChannelEnable(0,0,&AsioProc,(void*)chan); // enable 1st output channel...
	BASS_ASIO_ChannelEnable(0,outputAudioChannelSelectors[0],&AsioProc,(void*)chan); // enable 1st output channel...
	//for (a=1;a<i.chans;a++)
	//	BASS_ASIO_ChannelJoin(0,a,0); // and join the next channels to it
	BASS_ASIO_ChannelJoin(0,outputAudioChannelSelectors[1],outputAudioChannelSelectors[0]);
	if (i.chans==1) BASS_ASIO_ChannelEnableMirror(1,0,0); // mirror mono channel to form stereo output
	//BASS_ASIO_ChannelSetFormat(0,0,BASS_ASIO_FORMAT_FLOAT); // set the source format (float)
	BASS_ASIO_ChannelSetFormat(0,outputAudioChannelSelectors[0],BASS_ASIO_FORMAT_FLOAT); // set the source format (float)
	//BASS_ASIO_ChannelSetRate(0,0,i.freq); // set the source rate
	BASS_ASIO_ChannelSetRate(0,outputAudioChannelSelectors[0],i.freq); // set the source rate
	BASS_ASIO_SetRate(i.freq); // try to set the device rate too (saves resampling)
	if (!BASS_ASIO_Start(0)) // start output using default buffer/latency
		Error("Can't start ASIO output");

	/*
	while (!_kbhit() && BASS_ChannelIsActive(chan)) 
	{
		// display some stuff and wait a bit
		pos=BASS_ChannelGetPosition(chan,BASS_POS_BYTE);
		time=BASS_ChannelBytes2Seconds(chan,pos);
		printf("pos %09I64u",pos);
		if (ismod) 
		{
			pos=BASS_ChannelGetPosition(chan,BASS_POS_MUSIC_ORDER);
			printf(" (%03u:%03u)",LOWORD(pos),HIWORD(pos));
		}
		printf(" - %u:%02u - L ",time/60,time%60);
		{
			//DWORD level=BASS_ASIO_ChannelGetLevel(0,0)*32768; // left channel level
			DWORD level=BASS_ASIO_ChannelGetLevel(0,outputAudioChannelSelectors[0])*32768; // left channel level
			for (a=27204;a>200;a=a*2/3) putchar(level>=a?'*':'-');
			putchar(' ');
			//if (BASS_ASIO_ChannelIsActive(0,1))
				//level=BASS_ASIO_ChannelGetLevel(0,1)*32768; // right channel level
			if (BASS_ASIO_ChannelIsActive(0,outputAudioChannelSelectors[1]))
				level=BASS_ASIO_ChannelGetLevel(0,outputAudioChannelSelectors[1])*32768; // right channel level
			for (a=210;a<32768;a=a*3/2) putchar(level>=a?'*':'-');
		}
		printf(" R - cpu %.2f%%  \r",BASS_ASIO_GetCPU());
		fflush(stdout);
		Sleep(50);
	}
	printf("                                                                             \r");

	{ // wind the frequency and volume down...
		float freq=BASS_ASIO_ChannelGetRate(0,0),f=1;
		for (;f>0;f-=0.04) 
		{
			BASS_ASIO_ChannelSetRate(0,0,freq*f); // set sample rate
			for (a=0;a<i.chans;a++) // set volume for all channels...
				BASS_ASIO_ChannelSetVolume(0,a,f);
			Sleep(20);
		}
	}
	*/
	QWORD length_byte=BASS_ChannelGetLength(chan, BASS_POS_BYTE); // the length in bytes
	double length_s=BASS_ChannelBytes2Seconds(chan, length_byte); // the length in seconds
	if(fSecondsPlay<0)
	{
		//play once
		Sleep(length_s*1000);
	}
	else
	{
		//loopplay until
		Sleep(fSecondsPlay*1000);
	}
	BASS_ASIO_Stop(); // stop the device

	BASS_ASIO_Free();
	BASS_Free();

	nShowCmd = false;
	ShellExecuteA(NULL, "open", "end.bat", "", NULL, nShowCmd);
}
