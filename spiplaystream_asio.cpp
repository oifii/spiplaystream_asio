////////////////////////////////////////////////////////////////
//nakedsoftware.org, spi@oifii.org or stephane.poirier@oifii.org
//
//
//2012julyXX, creation for playing a stereo wav file using bass24
//nakedsoftware.org, spi@oifii.org or stephane.poirier@oifii.org
////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string>
using namespace std;
#include <ctime>
#include <iostream>
#include <assert.h>
#include <windows.h>

#include "bassasio.h"
#include "bass.h"
#include <map>
HSTREAM global_HSTREAM;
map<string,int> global_asiodevicemap;

//The event signaled when the app should be terminated.
HANDLE g_hTerminateEvent = NULL;
//Handles events that would normally terminate a console application. 
BOOL WINAPI ConsoleCtrlHandler(DWORD dwCtrlType);
int Terminate();

HWND global_hwndconsole;


// display error messages
void Error(const char *es)
{
	char mes[200];
	sprintf(mes,"%s\n(error code: %d)",es,BASS_ErrorGetCode());
	//MessageBox(win,mes,0,0);
	cout << mes << endl;
}

// ASIO function
DWORD CALLBACK AsioProc(BOOL input, DWORD channel, void *buffer, DWORD length, DWORD user)
{
	DWORD c=BASS_ChannelGetData(user,buffer,length);
	if (c==-1) c=0; // an error, no data
	return c;
}

//////
//main
//////
int main(int argc, char *argv[]);
int main(int argc, char *argv[])
{
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
	outputAudioChannelSelectors[0] = 6; // on emu patchmix ASIO device channel 15 (left)
	outputAudioChannelSelectors[1] = 7; // on emu patchmix ASIO device channel 16 (right)
	if(argc>4)
	{
		outputAudioChannelSelectors[0]=atoi(argv[4]); //0 for first asio channel (left) or 2, 4, 6 and 8 for spi (maxed out at 10 asio output channel)
	}
	if(argc>5)
	{
		outputAudioChannelSelectors[1]=atoi(argv[5]); //1 for second asio channel (right) or 3, 5, 7 and 9 for spi (maxed out at 10 asio output channel)
	}

    //Auto-reset, initially non-signaled event 
    g_hTerminateEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
    //Add the break handler
    ::SetConsoleCtrlHandler(ConsoleCtrlHandler, TRUE);

	global_hwndconsole = GetConsoleWindow();

	BASS_ASIO_DEVICEINFO myBASS_ASIO_DEVICEINFO;
	for (int i=0;BASS_ASIO_GetDeviceInfo(i,&myBASS_ASIO_DEVICEINFO);i++)
	{
		//SendDlgItemMessage(h,10,LB_ADDSTRING,0,(LPARAM)i.name);
		string devicenamestring = myBASS_ASIO_DEVICEINFO.name;
		global_asiodevicemap.insert(pair<string,int>(devicenamestring,i));
	}
	DWORD deviceid=0;
	
	map<string,int>::iterator it;
	it = global_asiodevicemap.find(audiodevicename);
	if(it!=global_asiodevicemap.end())
	{
		deviceid = (*it).second;
		printf("%s maps to %d\n", audiodevicename.c_str(), deviceid);
		//deviceInfo = Pa_GetDeviceInfo(deviceid);
		//deviceInfo->maxInputChannels
		//assert(outputAudioChannelSelectors[0]<deviceInfo->maxOutputChannels);
		//assert(outputAudioChannelSelectors[1]<deviceInfo->maxOutputChannels);
	}
	else
	{
		assert(false);
		Terminate();
	}

	// not playing anything via BASS, so don't need an update thread
	BASS_SetConfig(BASS_CONFIG_UPDATEPERIOD,0);
	// initialize BASS - "no sound" device
	if (!BASS_Init(0,44100,0,0,NULL))
	//if (!BASS_Init(0,48000,0,0,NULL))
	{
		Error("Can't initialize device");
		Terminate();
	}
	if (!BASS_ASIO_Init(deviceid,0)) 
	{
		Error("Can't initialize ASIO device");
		Terminate();
	}
	// set the ASIO device to work with
	BASS_ASIO_SetDevice(deviceid); 

	/*
	BASS_ASIO_Stop(); // stop the device
	BASS_ASIO_ChannelReset(0,-1,BASS_ASIO_RESET_ENABLE|BASS_ASIO_RESET_JOIN); // disable & unjoin all output channels
	BASS_StreamFree(global_HSTREAM);
	*/
	//global_HSTREAM=BASS_StreamCreateFile(FALSE,filename.c_str(),0,0,BASS_SAMPLE_LOOP); 
	global_HSTREAM=BASS_StreamCreateFile(FALSE,filename.c_str(),0,0,BASS_SAMPLE_LOOP|BASS_SAMPLE_FLOAT|BASS_STREAM_DECODE);
	if(!global_HSTREAM)
	{
		Error("Can't open stream");
		Terminate();
	}

	/////////////////
	//play
	/////////////////
	//BASS_ChannelPlay(global_HSTREAM,TRUE); // play the stream from the start
	// start ASIO output
	BASS_CHANNELINFO myBASS_CHANNELINFO;
	BASS_ChannelGetInfo(global_HSTREAM,&myBASS_CHANNELINFO);
	BASS_ASIO_ChannelEnable(0,outputAudioChannelSelectors[0],(ASIOPROC (__stdcall *))AsioProc,(void*)global_HSTREAM); // enable 1st output channel...
	//for (a=1;a<i.chans;a++)
	//	BASS_ASIO_ChannelJoin(0,a,0); // and join the next channels to it
	BASS_ASIO_ChannelJoin(0,outputAudioChannelSelectors[1],outputAudioChannelSelectors[0]);
	BASS_ASIO_ChannelSetFormat(0,0,BASS_ASIO_FORMAT_FLOAT); // set the source format (float)
	BASS_ASIO_ChannelSetRate(0,0,myBASS_CHANNELINFO.freq); // set the channel sample rate
	BASS_ASIO_SetRate(myBASS_CHANNELINFO.freq); // try to set the device rate too (saves resampling)
	if (!BASS_ASIO_Start(0)) // start output using default buffer/latency
		Error("Can't start ASIO output");

	
	QWORD length_byte=BASS_ChannelGetLength(global_HSTREAM, BASS_POS_BYTE); // the length in bytes
	double length_s=BASS_ChannelBytes2Seconds(global_HSTREAM, length_byte); // the length in seconds
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
	return Terminate();
}

int Terminate()
{
	BASS_StreamFree(global_HSTREAM); // free the stream
	BASS_ASIO_Free(); //one call for each asio device open, in here only one asio device at a time
	BASS_Free(); // close output
	return 0; //success
	//return -1; //error
}

//Called by the operating system in a separate thread to handle an app-terminating event. 
BOOL WINAPI ConsoleCtrlHandler(DWORD dwCtrlType)
{
    if (dwCtrlType == CTRL_C_EVENT ||
        dwCtrlType == CTRL_BREAK_EVENT ||
        dwCtrlType == CTRL_CLOSE_EVENT)
    {
        // CTRL_C_EVENT - Ctrl+C was pressed 
        // CTRL_BREAK_EVENT - Ctrl+Break was pressed 
        // CTRL_CLOSE_EVENT - Console window was closed 
		Terminate();
        // Tell the main thread to exit the app 
        ::SetEvent(g_hTerminateEvent);
        return TRUE;
    }

    //Not an event handled by this function.
    //The only events that should be able to
	//reach this line of code are events that
    //should only be sent to services. 
    return FALSE;
}
