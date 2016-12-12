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

#include "bass.h"
HSTREAM global_HSTREAM;

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

    //Auto-reset, initially non-signaled event 
    g_hTerminateEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
    //Add the break handler
    ::SetConsoleCtrlHandler(ConsoleCtrlHandler, TRUE);

	global_hwndconsole = GetConsoleWindow();

	// initialize default output device
	if (!BASS_Init(-1,44100,0,global_hwndconsole,NULL))
		Error("Can't initialize device");


	//global_HSTREAM=BASS_StreamCreateFile(FALSE,filename.c_str(),0,0,0); 
	global_HSTREAM=BASS_StreamCreateFile(FALSE,filename.c_str(),0,0,BASS_SAMPLE_LOOP); 
	if(!global_HSTREAM)
	{
		Error("Can't open stream");
		Terminate();
	}
	BASS_ChannelPlay(global_HSTREAM,TRUE); // play the stream from the start
	//BASS_ChannelStop(global_HSTREAM); // stop the stream

	/*
	QWORD length_byte=BASS_ChannelGetLength(global_HSTREAM, BASS_POS_BYTE); // the length in bytes
	double length_s=BASS_ChannelBytes2Seconds(global_HSTREAM, length_byte); // the length in seconds
	Sleep(length_s*1000);
	*/
	Sleep(fSecondsPlay*1000);
	return Terminate();
}

int Terminate()
{
	BASS_StreamFree(global_HSTREAM); // free the stream
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
