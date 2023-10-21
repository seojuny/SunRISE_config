/*-----------------------------------------------------------------------*/
/*  (C) Copyright Institute of Astronomy ETHZ 8092 Zuerich Switzerland   */
/*-----------------------------------------------------------------------*/
/*  Programmname: rxrs232.cpp                                            */
/*                                                                       */
/*  Revision: V1.0     Date: 23.04.2008    Autor: Chr. Monstein          */
/*                                                                       */
/*  Purpose: Receiver control unit Radiospectrometer CALLISTO            */
/*                                                                       */
/*  Compiler: BORLAND C++Builder 6                                       */
/*-----------------------------------------------------------------------*/

// Created by: Chr. Monstein 05.05.2003
// Updated by: Chr. Monstein 17.06.2003
// Updated by: Chr. Monstein 26.11.2004 programmable baudrate, integration deleted
// Updated by: H.Meyer       01.06.2006 Divisor flag Vx for the RISC removed
// Updated by: Chr. Monstein 20.10.2006 AGC programming tuner
// Updated by: Chr. Monstein 23.04.2008 COM9...COM18
// Updated by: Chr. Monstein 20.11.2010 fixed baudrate
// Updated by: Chr. Monstein 08.02.2011 test-output in GetRX*
// Updated by: Chr. Monstein 22.03.2011 status enforcement to identify CD1316LS/IV
// Updated by: Chr. Monstein 21.03.2012 send '?' to check firmware version
// Updated by: Chr. Monstein 26.04.2014 back to old wsc* due to incompatibility with FEDORA

//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#define WIN32

#include <stdio.h> // sprintf

#include "WSC.H"
#include "keycode.h"
#include "RXRS232.h"
#include "mainunit.h"

#pragma package(smart_init)
//---------------------------------------------------------------------------

TWrite *RX_SerialThread;
char rxcomport[40];
int RX_Port; // Receiver control unit RCU
int RX_Baud = WSC_Baud115200; //  constant

Byte RX_ringbuff[RXbuflen];
long RX_in=0, RX_out=0;
//---------------------------------------------------------------------------
extern int rows;             // import from mainunit.cpp, number of measurement points per sweep
extern int nsps;             // import from mainunit.cpp, number of sweeps per second
extern int clocksource;      // import from mainunit.cpp, RISC-timer source
extern int agclevel;         // import from mainunit.cpp, tuner agc level
extern bool response;        // import from mainunit.cpp

//---------------------------------------------------------------------------
__fastcall TWrite::TWrite(bool CreateSuspended) : TThread(CreateSuspended)
{
}
//---------------------------------------------------------------------------
bool __fastcall TWrite::Init(void)
{
    int Code;
    bool result;
    if (strstr(rxcomport,"COM1") ) RX_Port=COM1;
    if (strstr(rxcomport,"COM2") ) RX_Port=COM2;
    if (strstr(rxcomport,"COM3") ) RX_Port=COM3;
    if (strstr(rxcomport,"COM4") ) RX_Port=COM4;
    if (strstr(rxcomport,"COM5") ) RX_Port=COM5;
    if (strstr(rxcomport,"COM6") ) RX_Port=COM6;
    if (strstr(rxcomport,"COM7") ) RX_Port=COM7;
    if (strstr(rxcomport,"COM8") ) RX_Port=COM8;
    if (strstr(rxcomport,"COM9") ) RX_Port=COM9;
    if (strstr(rxcomport,"COM10")) RX_Port=COM10;
    if (strstr(rxcomport,"COM11")) RX_Port=COM11;
    if (strstr(rxcomport,"COM12")) RX_Port=COM12;
    if (strstr(rxcomport,"COM13")) RX_Port=COM13;
    if (strstr(rxcomport,"COM14")) RX_Port=COM14;
    if (strstr(rxcomport,"COM15")) RX_Port=COM15;
    if (strstr(rxcomport,"COM16")) RX_Port=COM16;
    if (strstr(rxcomport,"COM17")) RX_Port=COM17;
    if (strstr(rxcomport,"COM18")) RX_Port=COM18;
    if (strstr(rxcomport,"COM19")) RX_Port=COM19;
    if (strstr(rxcomport,"COM20")) RX_Port=COM20;
    if (strstr(rxcomport,"COM21")) RX_Port=COM21;
    if (strstr(rxcomport,"COM22")) RX_Port=COM22;
    if (strstr(rxcomport,"COM23")) RX_Port=COM23;
    if (strstr(rxcomport,"COM24")) RX_Port=COM24;

    // Code = SioDebug('R'); // power USB-device

    //SioKeyCode(WSC_KEY_CODE); /* used for newer version of wsc -> FEDORA incompatibility */
    Code = SioReset(RX_Port,8192,1024); // COM, RX-buffer, TX-buffer
    if(Code<0)
    {
        MainForm->Label5->Caption = "Can't open RCU-port";
        result = false;
    }
    else
    {
        SioParms(RX_Port,WSC_NoParity,WSC_OneStopBit,WSC_WordLength8);
        SioBaud(RX_Port,RX_Baud);
        SioDTR(RX_Port,'S');
        SioRTS(RX_Port,'S');
        MainForm->Label5->Caption = "COM-port available";
        result = true;
    }
    return (result);
}
//---------------------------------------------------------------------------
void __fastcall TWrite::Parametrisation(void) // startup
{
    char st[20];
    int  interndiv,externdiv;

    Send("D0\r"); // no debug
    Send("?\r"); // get status to identify new firmare V1.8 with 25.43 MHz quartz

    sprintf(st,"O%03u\r",agclevel); Send(st); // Set tuner gain (agc)

    sprintf(st,"L%u\r",rows); Send(st); // actual sweeplenght

    int nenner = rows*nsps;
    if (nenner < 1)
        nenner = 1;
    if (clocksource==1)
    {
        interndiv = 86400/nenner - 1;
        sprintf(st,"GS%u\r",interndiv); // GS107=800pixel/s,GS71=1200pixel/s,GS53=1600pixel/s
        Send(st);
    }
    else
    {
        if (clocksource==2)
        {
            externdiv = 500000/nenner - 1;
            sprintf(st,"GA%u\r",externdiv); // GA1249=400pixel/s,GA624=800pixel/s,GA499=1000pixel/s
            Send(st);
        }
    }
    sprintf(st,"T%u\r",clocksource); Send(st); // clock source, resp. trigger source

    float myfreq = 45.0; // minimum frequency of tuner
    sprintf(st,"F0%05.1f\r",myfreq); Send(st); // clock source, resp. trigger source

    Send("?\r"); // enforce status-info to check tuner-type and serial connection

}
//---------------------------------------------------------------------------
void __fastcall TWrite::Transmit(char byte) // Send byte to client via RS
{
    SioPutc(RX_Port, byte);
}
//---------------------------------------------------------------------------
void __fastcall TWrite::Send(char * cmd) // Send byte to client via RS
{
    SioPuts(RX_Port, (LPSTR)cmd, strlen(cmd)); // string to receiver
}
//---------------------------------------------------------------------------
void __fastcall TWrite::GetRXRS232Data(void) // Private activities
{
    int cnt;
    #define local_size 1024
    char buffer[local_size]; // we expect 4KByte/sec = 80Byte/20msec
    cnt = SioGets(RX_Port,(LPSTR)buffer,local_size);  // (LPSTR) = mit null beendeter String
    for (int i=0; i<cnt; i++) // fill up RX ringbuffer
    {
        RX_ringbuff[RX_in] = buffer[i];
        RX_in=(RX_in+1)%RXbuflen;
    }
    if (cnt>0) response = true;
    //MainForm->Label2->Caption=cnt; // default circa 100 bytes/20msec_cycle
}
//---------------------------------------------------------------------------
void __fastcall TWrite::Execute() // Main job extra-thread
{
    while(1) // almost forever...
    {
        if(Terminated) return; // exit thread
        GetRXRS232Data();
        Sleep(20); // wait
    }
}
//---------------------------------------------------------------------------
void __fastcall TWrite::Close() // termination
{
    Send("S0\r"); // stop state machine
    Sleep(50);
    SioDone(RX_Port);
    Terminate();
    WaitFor();
}
//---------------------------------------------------------------------------

