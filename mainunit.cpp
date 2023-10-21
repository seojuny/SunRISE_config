/*-----------------------------------------------------------------------*/
/*  (C) Copyright Institute of Astronomy ETHZ 8092 Zuerich Switzerland   */
/*-----------------------------------------------------------------------*/
/*  Programmname: mainunit.cpp                                           */
/*                                                                       */
/*  Revision: V1.15    Date: 14.11.2011    Autor: Chr. Monstein          */
/*                                                                       */
/*  Purpose: Hostcontroller Radiospectrometer CALLISTO                   */
/*                                                                       */
/*  Compiler: BORLAND C++Builder 6                                       */
/*-----------------------------------------------------------------------*/

// Created by: Chr. Monstein 05.05.2003
// Updated by: Chr. Monstein 28.05.2003 progressbar included in mainform
// Updated by: Chr. Monstein 17.06.2003 all port related things shiftet to there modul
// Updated by: Chr. Monstein 23.09.2003 scheduler read process implemented
// Updated by: Chr. Monstein 16.10.2003 some procedure exported to their modules
// Updated by: Chr. Monstein 27.11.2003 UploadThread included
// Updated by: Chr. Monstein 05.12.2003 Peripherals in separate forms
// Updated by: Chr. Monstein 27.04.2004 timer constants included at NRAO
// Updated by: Chr. Monstein 19.05.2004 different minor changes (plot, info, logfile)
// Updated by: Chr. Monstein 02.07.2004 optimization display, new datafilename
// Updated by: Chr. Monstein 26.11.2004 new parameters (gps, fpu, hygrowin)
// Updated by: Chr. Monstein 01.12.2004 RAW file deleted, FITS inserted
// Updated by: Chr. Monstein 06.12.2004 FCx and FCy linked
// Updated by: Chr. Monstein 02.06.2005 'origin' from cfg-file
// Updated by: Chr. Monstein 03.06.2005 check logpath and datapath
// Updated by: Chr. Monstein 05.04.2006 Hygrowin, GPS and FPU units deleted
// Updated by: Chr. Monstein 05.04.2006 > 2nd generation callisto created
// Updated by: Chr. Monstein 25.04.2006 Version management in Info.cpp, Hints&Tricks del.
// Updated by: H. Meyer      08.08.2006 code changed to e-callisto
// Updated by: Chr. Monstein 20.02.2007 OV load reduced
// Updated by: Chr. Monstein 11.06.2009 Radiomonitoring, more infos, +calibration
// Updated by: Chr. Monstein 23.06.2009 new: title comment
// Updated by: Chr. Monstein 17.04.2009 new light curves
// Updated by: Chr. Monstein 06.08.2009 updated light curve processes
// Updated by: Chr. Monstein 30.12.2009 hide plots if button is pressed twice
// Updated by: Chr. Monstein 13.01.2010 more (5) light curves possible
// Updated by: Chr. Monstein 04.03.2010 plots more often + auto stop at data loss
// Updated by: Chr. Monstein 20.11.2010 allow autostart
// Updated by: Chr. Monstein 08.02.2011 several small improvements
// Updated by: Chr. Monstein 18.02.2011 format OVS fixed
// Updated by: Chr. Monstein 11.04.2011 pwm-programming only when value changes
// Updated by: Chr. Monstein 14.11.2011 improved Help->options (Links to documents Reeve)
// Updated by: Chr. Monstein 21.12.2012 check firmware for V1.8 (new 25.43 MHz quartz)
// Updated by: Chr. Monstein 01.09.2012 repair bug in light curve calibration
// Updated by: Chr. Monstein 26.06.2013 new buttons DeviceManager and Taskmanager + priorty selection
// Updated by: Chr. Monstein 26.04.2014 back to old wsc* due to incompatibility with FEDORA
// Updated by: Chr. Monstein 27.12.2014 reload frqfilename blocked, repair memory alloc for char-arrays
// Updated by: Chr. Monstein 12.08.2017 minor updates (text, version 119->120)
// Updated by: Chr. Monstein 15.09.2017 Yt below MainForm to cope with low resolution monitors
// Updated by: Chr. Monstein 02.10.2017 add FCx to OVS-files and suppress re-read of FCx from callisto.cfg

//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#include <math.h>
#include <stdio.h>
#include <fstream.h>
#include <DateUtils.hpp>
#include <conio.h>

#define WIN32
#include "mainunit.h"
#include "WSC.H"
#include "RXRS232.H"
#include "XY.h"
#include "XYZ.h"
#include "EEPROM.h"
#include "Yt.h"
#include "Scheduler.h"
#include "Info.h"
#include "FitsWrite.h"

#pragma package(smart_init)
#pragma resource "*.dfm"

//---------------------------------------------------------------------------
// import form RXRS232.cpp
extern char rxcomport[40];
extern TWrite *RX_SerialThread; // RX activities
extern Byte RX_ringbuff[RXbuflen];
extern long RX_in, RX_out;
//---------------------------------------------------------------------------
// import from EEPROM.cpp
extern EEPROM *UploadThread;
extern TTP testfrequency;
extern Tolc OlcList[MaxNolc];  // table containing optional light curve infos
extern int Nolc;               // number of light curves
extern bool calibration;       // import from EEPROM.cpp
extern Tcal CF[500];           // import from EEPROM.cpp

//---------------------------------------------------------------------------
// import from Scheduler.cpp
extern bool scheduler_once;
//---------------------------------------------------------------------------
// data buffer for light curve plot Yt
extern int *Yt_ringbuff;
extern long Yt_in, Yt_out;
//---------------------------------------------------------------------------
// data buffer for spectrum plot XY
extern int *XY_ringbuff;
extern long XY_in, XY_out;
extern float *BGbuffer; // background buffer
//---------------------------------------------------------------------------
// data buffer for spectrum plot XYZ
extern Byte *XYZ_ringbuff;
extern long XYZ_in, XYZ_out;
//---------------------------------------------------------------------------

// some global variables
long datcount = 0;    // counts all measured data points
Byte HiDat;           // measured data  (8 bit)

Byte *pszBuffer_RX1;  // data buffer between main- and FITS-unit
int rows;             // default number of Measurement Points Per Sweep
int row;              // actual measurement point
long column;          // actual column (sweep)
long columns;         // max number of column
int nsps;             // default number of Sweeps Per Second
int FCx;              // actual focuscodes

long YTbuflen;        // lenght of light curve plot buffer
long XYbuflen;        // lenght of time domain plot buffer
long XYZbuflen;       // lenght of time domain plot buffer

FILE *LogFileID;      // to save all receiver and host messages
char logfilename[240]; // actual (last) logfile
FILE *OverviewFileID;      // to save all receiver and host messages (prn)
FILE *OverviewFileID2;      // to save all receiver and host messages (csv)
char overviewfilename[240]; // actual (last) logfile (prn)
char overviewfilename2[240]; // actual (last) logfile (csv)
AnsiString TimerSt;   // to be logged...
AnsiString DatumSt;   // to be logged...
Word Year, Month, Day, Hour, Mint, Sect, MSec;  // goobal timing
float Tact, old_T;
TDateTime dtOld;      // Starttime structure for FITS

char frqfilename[80]="no file sel.";      // actual frequency file
char frqpath[120]="";     // path to frequencyfile
char logpath[120]="";     // default place to store logfiles
char datapath[120]="";    // default place to store data
char LCpath[120]="";      // default place to store light curves
char OVSpath[120]="";     // default place to store spectral overview
long filenumber;          // counts fitsfiles
int fitsenable;           // which fits files are to be saved
float low_band, mid_band; // tuner barrier frequencies
float detector_sens=25.4; // default 8700 sens mV/dB
float if_init = (10.70+27.0); // IF-frequency #1 = IF-frequency #2 + Quarz-frequency [MHz]
int chargepump=1;         // tuner configuration
int agclevel=100;         // tuner gain
int old_agclevel=agclevel;// tuner gain
int db_scale=6;           // dB per division
int autostart=0;          // automatic start after power on default=off

int timerinterval;    // main timing interupt time
int timerpreread;     // time to stop early enough to start again at the right time
int timeouthexdata;   // time to collect all data from buffers

TMainForm *MainForm;  // mainunit of Borland C-Builder Program
int measurestate = 0; // status of measurement, default=0=idle

int observatory;      // Code numerical
char instrument[80];  // dito ASCII
char titlecomment[80]="LPD";// additional info on top of API
char origin[80];      // default ETH Zurich Switzerland
int clocksource;      // RISC-timer source
int fpucontrolcode;   // How to control the FPU, if any
long filetime;        // Length of one FITS-file expressed in seconds
char mmode;           // measurementmode
int ackcnt = 0;       // acknowledge byte counter
int adc_divider=4;    // assuming 10bit instrument -> FIT = data/adc_divider

bool upload           = false; // Do not upload frequency table into EEPROM per se
bool eepromuploaded   = false; // process synchronisation, load job finished?
bool eepromCycleReady = false; // process synchronisation to accelerate upload
bool transfer_once    = true;
bool Callisto_8bit    = false; // assume we have a 10bit device. 8bit is identified by 'ChargePump'
bool response         = false; // need to prove that Callisto is responding

float rxvoltage;       // ATmega16 voltage
int yield;             // Overview counter

TGPSdata MyGPS;        // structure holds GPS time, date and some other data
bool overview = false; // to separate overview data into its own file
bool lightcurves = true; // save lightcurves, if in FRQ enabled
float MV[MaxNolc]={0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0}; // 10 averaged light curves
int lcc = 0;           // counter for averaging LC

// Project->Option->Version needs to be updated also!!!
char version[40] = "SW-version 1.24, e-Callisto";
char author[80]  = "Author: Chr. Monstein, ETH Zurich, 2019-06-26";
//---------------------------------------------------------------------------
__fastcall TMainForm::TMainForm(TComponent* Owner) : TForm(Owner)
{
    char tmp[80];

    ReadConfig(true); // get all default stuff

    Timer->Interval = timerinterval; // set main timer IRQ-time (default 50msec)
    Timer1->Enabled = false; // data loss procedure inactive
    Timer->Enabled = false;

    OpenLogging();
    Logging("$HST:Application just created");
    sprintf(tmp,"$HST:%s",version);
    Logging(tmp);
    Logging("$HST:Configurationfile read");

    RadioButton1Click(0); // goto automatic mode

    RX_SerialThread = new TWrite(false);
    Sleep(200);  // bisher 200
    bool result = RX_SerialThread->Init(); // Connect RX-interface Port
    // parametrisation will take place within UploadThread->Upload(true) later on
    if (!result)
    {
        Application->MessageBox("Check COM-port/USB-adapter","Serial port is not responding", MB_OK);
        RX_SerialThread->Close();
        Application->Terminate();
    }
    Timer->Enabled = true;

    Logging("$HST:ATmega16 pre-configured");
    //--------------------------------------------------------------
    UploadThread = new EEPROM(false); // upload frequency file into eeprom
    Sleep(500);  // bisher 500ms ??????
    UploadThread->Init(); // Prepare EEPROM upload
    UploadThread->Upload(true);
    sprintf(tmp,"$HST:Frequencyfile %s",frqfilename);
    Logging(tmp);

    Logging("$HST:Callisto ready now...");
 /*   if (!response)
    {
        Timer->Enabled = false;
        Timer1->Enabled = false;
        RX_SerialThread->Close();
        UploadThread->Close();
        Application->MessageBox("Check Callisto power and serial connection","Callisto is not responding", MB_OK);
        Application->Terminate();
    }   */

}
//---------------------------------------------------------------------------
void __fastcall TMainForm::MyClock(void)
{
    TDateTime dtPresent = Now();
    DecodeDate(dtPresent, Year, Month, Day);
    DatumSt = DateToStr(dtPresent);
    DecodeTime(dtPresent, Hour, Mint, Sect, MSec);
    TimerSt = TimeToStr(Time()); // needed for logfiles
    Tact = Hour + ((float)Mint)/60.0 + ((float)Sect)/3600.0 + ((float)MSec)/3600.0/1000.0;
}
//---------------------------------------------------------------------------
void __fastcall TMainForm::OpenLogging(void)
{
    MyClock(); // get time & date
    sprintf(logfilename,"%sLOG_%s_%s_%04u%02u%02u_%02u%02u%02u.txt",logpath,instrument,titlecomment,Year,Month,Day,Hour,Mint,Sect);
    LogFileID = fopen(logfilename, "at"); // append logfile in text format
}
//---------------------------------------------------------------------------
void __fastcall TMainForm::Logging(char *par)
{
    char buff[240];
                                                                                                                  
    sprintf(buff,"%s,%s,%s\n",DatumSt,TimerSt,par);
    fputs(buff,LogFileID);  // write to logfile
    sprintf(buff,"%s,%s",TimerSt,par);
    Receivermessages->Lines->Add(buff); // Memo
}
//---------------------------------------------------------------------------
void __fastcall TMainForm::CloseLogging(void)
{
    fflush(LogFileID);
    fclose(LogFileID);
}
//---------------------------------------------------------------------------
void __fastcall TMainForm::SaveLightcurves(float t, float M1, float M2, float M3, float M4, float M5, float M6, float M7, float M8, float M9, float M10)
{
    char st[240];
    FILE *out;

    if (calibration && mmode=='2')
       sprintf(st,"%sLC%04u%02u%02u_SFU_%s.txt",LCpath,Year,Month,Day,titlecomment);
    else
       sprintf(st,"%sLC%04u%02u%02u_ADU_%s.txt",LCpath,Year,Month,Day,titlecomment);

    if (!FileExists(st))
      lightcurves = true; // write new header if LC was deleted
    else
    if (FileExists(st) && (lightcurves==true))
      lightcurves = false; // prevent new header after restart of application

    try
    {
        out = fopen(st,"a");
        if (lightcurves)
        {
            fprintf(out,"Time_UT,%6.3fMHz,%6.3fMHz,%6.3fMHz,%6.3fMHz,%6.3fMHz,%6.3fMHz,%6.3fMHz,%6.3fMHz,%6.3fMHz,%6.3fMHz,%s,pwm=%i\n",
               OlcList[0].MHz,OlcList[1].MHz,OlcList[2].MHz,OlcList[3].MHz,OlcList[4].MHz,
               OlcList[5].MHz,OlcList[6].MHz,OlcList[7].MHz,OlcList[8].MHz,OlcList[9].MHz,version,agclevel );
            lightcurves = false;
        }
        fprintf(out,"%8.4f,%9.3f,%9.3f,%9.3f,%9.3f,%9.3f,%9.3f,%9.3f,%9.3f,%9.3f,%9.3f\n",t,M1,M2,M3,M4,M5,M6,M7,M8,M9,M10);
        fclose (out);
    }
    catch (Exception &exception)
    {
        Logging("$HST:Error writing lightcurve");

    }
}
//---------------------------------------------------------------------------
void __fastcall TMainForm::OpenOverview(void)
{
    MyClock(); // get time & date
    sprintf(overviewfilename ,"%sOVS_%s_%s_%04u%02u%02u_%02u%02u%02u_%02u.prn",OVSpath,instrument,titlecomment,Year,Month,Day,Hour,Mint,Sect,FCx);
    sprintf(overviewfilename2,"%sOVS_%s_%s_%04u%02u%02u_%02u%02u%02u_%02u.csv",OVSpath,instrument,titlecomment,Year,Month,Day,Hour,Mint,Sect,FCx);
    OverviewFileID  = fopen(overviewfilename , "at"); // append overviewfile in text format (prn)
    OverviewFileID2 = fopen(overviewfilename2, "at"); // append overviewfile in text format (csv)
    char buff[80];
    sprintf(buff,"Frequency[MHz];Amplitude RX1[mV] at pwm=%u;%s\n",agclevel,version);  // write legend to overview-file
    fputs(buff,OverviewFileID );  // write legend to overview-file (prn)
    fputs(buff,OverviewFileID2);  // write legend to overview-file (csv)
    yield = 0; // overview counter
}
//---------------------------------------------------------------------------
void __fastcall TMainForm::SaveOverview(char *par)
{
    char buff[80];
    static int state = 0;
    float freq = 0;
    int ampl1  = 0;
    int p;

    sscanf(par,"$CRX:%f,%d",&freq,&ampl1);
    if ((freq >= 45.0) && (freq <= 870.0) && (ampl1 >50.0) && (ampl1 <2500.0))
    {
      sprintf(buff,"%7.3f;%u\n",freq,ampl1);
      fputs(buff,OverviewFileID );  // write to overview-file (prn)
      fputs(buff,OverviewFileID2);  // write to overview-file (prn)

      yield++;
      if ((state++%10)==0) // sometimes ...
      {
         p = (float)yield/132.0; // =/13200.0*100.0
         ProgressBar1->Position=p;
         sprintf(buff,"Yield =%4u%%",p);
         Percent->Caption = buff;
      }
    }
}
//---------------------------------------------------------------------------
void __fastcall TMainForm::CloseOverview(void)
{
    fflush(OverviewFileID);    // (prn)
    fflush(OverviewFileID2);   // (csv)

    fclose(OverviewFileID);    // (prn)
    fclose(OverviewFileID2);   // (csv)
}
//---------------------------------------------------------------------------
void __fastcall TMainForm::DisplayMmode(char mode)
{
  char st[80];
  switch(UpCase(mode)) // evaluate measurement mode
  {
      case '0': // empty mode
      {
          Logging("$HST:Measurement idle");
          break;
      }
      case '2': // calibration mode
      {
          Logging("$HST:Receiver calibration");
          break;
      }
      case '3': // steady / continuous recording
      {
          Logging("$HST:Continuous measurement");
          break;
      }
      case '8': // auto OV
      {
          Logging("$HST:Automatic overview");
          break;
      }
      default: // rest
      {
          sprintf(st,"$HST:Unsupported mode: (%c)",mmode);
          Logging(st);
         break;
      }
  }
} // end check measurement mode
//---------------------------------------------------------------------------
void __fastcall TMainForm::EraseBuffer(void)  // Evaluate receiver data
{
    long i;
    for (i=0; i<YTbuflen; i++)
    {
        Yt_ringbuff[i] = 0;
    }
    Yt_in  = 0;
    Yt_out = 0;

    for (i=0; i<XYbuflen; i++)
        XY_ringbuff[i] = 0;
    XY_in  = 0;
    XY_out = 0;

    for (i=0; i<XYZbuflen; i++)
        XYZ_ringbuff[i] = 0;
    XYZ_in = 0;
    XYZ_out= 0;
}
//---------------------------------------------------------------------------
void __fastcall TMainForm::EvalRX(void)  // Evaluate receiver data
{
    // lenght of message buffer (only)
    #define rxBufflen 800
    char tmp[80], st[80];
    static char rx[rxBufflen];  // space for messages from RCU
    static char data[5];  // buffer for LSB+MSB in Hex -> 4 bytes+ terminator
    static Byte hexcount  = 0;
    static Byte datastate = 0;
    static int labelcount = 0; // how often is this procedure called
    Byte c;
    static long i;
    static plotstate = 0;
    int tempd; // measureing value for RX
    int testdat; // temporary
    int dp; // difference of buffer pointers
    float p; // actual ercentage of buffer load
    static float pmean = 0; // mean percentage of buffer load
    float f; // counter overview

    static bool saveit; // controll flag for normal operation
    static bool saverest; // control flag for aborting running measurements
    static Word old_Sec;
    static int substate = 0;
    static bool datatermination = false;
    static bool disp_once = false;

    if ((Sect%2)==0) // display sometimes
    {
      sprintf(st,"Statemachine mainstate = %u/%u",measurestate,6);
      Form7->Label8->Caption = st;
      sprintf(st,"Measurement code = %c",mmode);
      Form7->Label13->Caption = st;
    }

    switch (measurestate) // what shall we do now?
    {
        case 0: // halt -> idle (get housekeeping data)
        {
          saveit = false; // don't save FITS data at all
          Logging("$HST:Measurement idle");
          RX_SerialThread->Send("U2\r"); // tuner gain voltage
          RX_SerialThread->Send("U4\r"); // emitter voltage BF199
          RX_SerialThread->Send("U6\r"); // input voltage
          measurestate++;
          disp_once = true;
          break;
        }
        case 1: // enable measurement, wait for Start-Button (measurestate=2)
        {
          if (disp_once)
          {
            Logging("$HST:Measurement enable");
            disp_once = false;
          }
          break; // messages not possible and also not desired and not allowed
        }
        case 2: // parametrisation <- entry Point for START-button
        {
          EEPROMButton->Enabled = false; // eeprom button inactive
          StartButton->Enabled  = false; // START-button inactive
          Overview->Enabled     = false;
          switch(substate) // to not overload RISC command buffer [TBC}
          {
            case 0: // set focuscode
            {
                if (eepromuploaded)
                {
                  if (transfer_once)
                  {
                    Logging("$HST:Param. RCU due to START");

                    sprintf(st,"FS%02u%02u\r",(Byte)FCx,(Byte)FCx-1);
                    RX_SerialThread->Send(st); // Send focuscode via risc parallel to FPU

                    sprintf(st,"$HST:Actual focuscode %02u",(Byte)FCx);
                    Logging(st);

                    sprintf(st,"L%u\r",rows);
                    RX_SerialThread->Send(st); // Send sweep length

                    RX_SerialThread->Send("S1\r"); // Send start event to risc state machine
                  }
                  substate++;
                }
                break;
            }
            case 1: // enable datatransfer
            {
                Logging("$HST:Enable data transfer");
                if (transfer_once)
                {
                    RX_SerialThread->Send("GE\r"); // Enable data transfer risc->host
                    transfer_once = false;
                }
                substate = 0; // prepare next start
                measurestate++; // begin saving of data allowed
                DisplayMmode(mmode);
                break;
            }
          } // end substates
          break;
        }
        case 3: // open a new file
        {
          Logging("$HST:Start new FITS-file");
          datcount = 0; // global datacounter
          saveit   = true; // save binary data from now on
          saverest = false;
          measurestate++; // begin saving binary data
          StopButton->Enabled = true;  // STOP -button active
          disp_once = true;
          dtOld = EncodeDateTime(Year, Month, Day, Hour, Mint, Sect, MSec); // 1st timestamp-> FITS
          break;
        }
        case 4: // running measurement -> write continously
        {
          if (disp_once)
          {
            Logging("$HST:Continous measurement"); // stay here and save HEX data until STOP occures
            disp_once = false;
          }
          break;
        }
        case 5: // disable measurement -> start timeout, <- entry point for STOP-Button
        {
          Logging("$HST:Measurement stop");
          datatermination = false;
          saverest = true;
          RX_SerialThread->Send("GD\r"); // disable datatransfer (not stopping the state machine)

          measurestate++; // -> terminate collecting data
          old_Sec = 0; // Reset timeout (collecting data)
          disp_once = true;
          break;
        }
        case 6: // after timeout -> stop
        {
          old_Sec = old_Sec+Timer->Interval; // expressed in msec
          if (disp_once)
          {
            Logging("$HST:Measurement wait...");
            disp_once = false;
          }
          if ((old_Sec>(timeouthexdata/nsps)) || (datatermination)) // check timeout (collecting data) [TBC]
          {
            Logging("$HST:Measurement halted");
            RX_SerialThread->Send("S0\r");   // disable state machine
            transfer_once = true;
            saveit = false; // no more data to be saved
            EEPROMButton->Enabled = true; // eeprom button allowed
            StartButton->Enabled  = true;  // START-button active
            StopButton->Enabled   = false;  // STOP - button inactive
            Overview->Enabled     = true;   // allow overview
            measurestate = 0; // -> stop now, go into idle state
            scheduler_once = true;
          }
          break;
        }
    } // end switch(measurestate);

    dp = abs(RX_in-RX_out); // difference pointer
    if (dp > 0)
    {
        do
        {
           c = RX_ringbuff[RX_out];
           RX_out = (RX_out+1)%RXbuflen;
           if (c==']')  datastate = 0; // ready upload eeprom
           if (c=='$')  datastate = 1; // start of message
           if (c=='\r') datastate = 3; // end of message
           if (c==STX)  datastate = 4; // start of (HEX-)transmission
           if (c=='&')  datastate = 6; // end of (HEX-)transmission
           switch(datastate) // what shall be done with incoming data?
           {
              case 0: // ACK detection to control eeprom upload
              {
                 eepromCycleReady = true; // block written into eeprom, continue
                 break; // nothing to do for the moment
              }
              case 1: // '$' detected, start message
              {
                 i = 0;
                 rx[i] = (char)c;
                 i = (i+1)%rxBufflen;
                 datastate = 2;
                 break;
              }
              case 2: // message running
              {
                 rx[i] = (char)c;
                 i = (i+1)%rxBufflen;
                 break;
              }
              case 3: // '\r' detected, end of message
              {
                // '\r' has already been spent to switch into this case
                rx[i] = '\0'; // terminate string
                if (!overview) Logging(rx); // almost all ...
                if (strstr(rx,"$CRX:")) response = true;
                if (strstr(rx,"$CRX:U2(+12V)"))
                {
                  sscanf(rx,"$CRX:U2(+12V)=%fV",&rxvoltage);
                }
                else
                if ((strstr(rx,"$CRX:")) && (overview)) // overview running
                {
                   SaveOverview(rx);
                   f = 0;
                   sscanf(rx,"$CRX:%f",&f);
                   if (f >= 869.999) // end of P2 overview
                   {
                      StartButton->Enabled  = true; // allow start again
                      EEPROMButton->Enabled = true; // allow freq selection again
                      RadioButton1->Enabled = true;

                      Logging("$HST:End spectral overview...");
                      Logging("$HST:Data in log-directory");
                      overview = false;
                      CloseOverview();
                      Overview->Enabled = true; // allow overview again
                   }
                }

                if (strstr(rx,"Callisto")) // Auto-RESET detected
                {
                  Logging("$HST:Watchdog triggered -> Reset");
                  RX_SerialThread->Parametrisation();
                  Timer1->Enabled = true; // start data loss proces

                } else
                if (strstr(rx,"ChargePump")) // this keyword identifies old version CD1316 (8bit)
                {
                  Logging("$HST:CD1316LV/IV detected");
                  Callisto_8bit = true;
                  adc_divider = 1;
                } else
                if (strstr(rx,"Frequency")) // response prooves system connected
                {
                  Logging("$HST:Receiver connected");
                  Label5->Caption = "Receiver connected";
                } else
                if (strstr(rx,"$CRX:V1.8")) // response prooves new firmware 25MHz
                {
                  Logging("$HST:Found LO=25.43 MHz");
                  if_init = (10.70+25.43); // 25.43 MHz LO for firmare >= 1.8
                }
                datastate++; // assume HEX-data are coming ... (?)
                break;
              } // end '\r'
              case 4: // STX detected, start of HEX-data
              {
                 hexcount = 0;
                 datastate++;
                 break; // forget STX Start-Byte
              }
              case 5: // continuous stream of HEX-data
              {
                data[hexcount] = (char)c; // fill buffer
                data[4] = '\0'; // string termination
 
                if (hexcount==3) // full house of the form 'FFFF' (HEX ASCII)
                {
                    if (saveit || saverest)
                    {
                        sscanf(data,"%04X",&tempd);
                        //LoDat = Byte(tempd/256);  // empty
                        //HiDat = Byte(tempd%256);  // RX
                        testdat = tempd/256/adc_divider; // should always be 0

                        /*
                        if (CheckBox1->Checked) // just to simulate data loss
                        {
                            CheckBox1->Checked=false;
                            testdat = 7;
                        } */

                        if ((testdat > 0) && (measurestate==4)) // we have data loss
                        {
                           Timer1->Enabled = true; // start data loss proces
                        }

                        row    = datcount%rows; // actual point
                        column = datcount/rows; // sweepnumber

                        Byte dfit =  Byte((tempd/adc_divider)%256); // enforce 8 bit for FIT and XYZ
                        
                        pszBuffer_RX1[datcount] = dfit;

                        if (row == testfrequency.poltp) // OnLineTestPoint OLTP
                        {
                            Yt_ringbuff[Yt_in] = tempd;
                            Yt_in = (Yt_in + 1)%YTbuflen;
                        }
                        // channel=0...199 (99)
                        if (Nolc > 0) // are there light curves needed?
                        {
                          if (OlcList[0].channel == row) // Option 151MHz light curve ON?
                          {
                             if (OlcList[0].count > 0)
                             {
                                MV[0] = MV[0] + float(tempd);
                                lcc++;
                             }
                          } else
                          {
                            if (OlcList[1].channel == row) // Option 245MHz light curve ON?
                            {
                               if (OlcList[1].count > 0) MV[1] = MV[1] + float(tempd);
                            } else
                            {
                              if (OlcList[2].channel == row) // Option 327MHz light curve ON?
                              {
                                 if (OlcList[2].count > 0) MV[2] = MV[2] + float(tempd);
                              } else
                              {
                                if (OlcList[3].channel == row) // Option 410MHz light curve ON?
                                {
                                   if (OlcList[3].count > 0) MV[3] = MV[3] + float(tempd);
                                } else
                                {
                                  if (OlcList[4].channel == row) // Option 608MHz light curve ON?
                                  {
                                     if (OlcList[4].count > 0) MV[4] = MV[4] + float(tempd);
                                  } else
                                  {
                                    if (OlcList[5].channel == row) // Option ???MHz light curve ON?
                                    {
                                       if (OlcList[5].count > 0) MV[5] = MV[5] + float(tempd);
                                    } else
                                    {
                                      if (OlcList[6].channel == row) // Option ???MHz light curve ON?
                                      {
                                         if (OlcList[6].count > 0) MV[6] = MV[6] + float(tempd);
                                      } else
                                      {
                                        if (OlcList[7].channel == row) // Option ???MHz light curve ON?
                                        {
                                           if (OlcList[7].count > 0) MV[7] = MV[7] + float(tempd);
                                        } else
                                         {
                                          if (OlcList[8].channel == row) // Option ???MHz light curve ON?
                                          {
                                             if (OlcList[8].count > 0) MV[8] = MV[8] + float(tempd);
                                          } else
                                          {
                                            if (OlcList[9].channel == row) // Option ???MHz light curve ON?
                                            {
                                               if (OlcList[9].count > 0) MV[9] = MV[9] + float(tempd);
                                            }
                                          } // end
                                        } // end
                                      } // end
                                    } // end
                                  } // end
                                } // end
                              } // end
                            } // end
                          } // end

                          if (lcc >= OlcList[0].count) // the other counts are ignored ...
                          {
                            if (calibration && mmode=='2') // calnnnnn.par must be present and mmode must be set for calibration
                               SaveLightcurves(Tact, Digit2SFU(0), Digit2SFU(1), Digit2SFU(2), Digit2SFU(3), Digit2SFU(4),
                                                     Digit2SFU(5), Digit2SFU(6), Digit2SFU(6), Digit2SFU(8), Digit2SFU(9) );
                            else // no calibration, just raw data
                               SaveLightcurves(Tact, MV[0]/lcc, MV[1]/lcc, MV[2]/lcc, MV[3]/lcc, MV[4]/lcc,
                                                     MV[5]/lcc, MV[6]/lcc, MV[7]/lcc, MV[8]/lcc, MV[9]/lcc );

                            for (int i=0; i<MaxNolc; i++)
                              MV[i] = 0.0;
                            lcc = 0; // restart LC integration
                          }
                        } // end optional light curve

                        XY_ringbuff[XY_in]   = tempd; XY_in  = (XY_in+1)%XYbuflen; // 8...10 bit

                        XYZ_ringbuff[XYZ_in] = dfit; XYZ_in = (XYZ_in+1)%XYZbuflen; // only 8 bit


                        if ((datcount%(rows*nsps))==0) // inversly proportional to clockrate = 1sec
                        {
                          //if (Form1->Visible) Form1->XY(); // plot frequency domain
                          //if (Form2->Visible) Form2->XYZ(); // plot time&frequency domain
                          //if (Form3->Visible) Form3->YT(); // plot time domain
                          switch (plotstate)
                          {
                              case 0:  // less time consuming
                              {
                                  if (Form3->Visible)
                                     Form3->YT(); // plot time domain
                                  if (Form1->Visible)
                                     Form1->XY(); // plot frequency domain
                                  plotstate++;
                                  break;
                              }
                              case 1: // very time consuming ...
                              {
                                  if (Form2->Visible)
                                     Form2->XYZ(); // plot time&frequency domain
                                  plotstate = 0; // back to Yt
                                  break;
                              }
                          }
                        }

                        if (datcount==0)
                            dtOld = EncodeDateTime(Year, Month, Day, Hour, Mint, Sect, MSec); // xth time-stamp-> next FITS

                        datcount++; // global data counter to switch for a new file
                        if ( (datcount >= (long)(rows*nsps*filetime)) || saverest) // buffer full or abort -> FITS-file
                        {
                            switch(fitsenable)
                            {
                                case 1: // RX1
                                {
                                    WriteFits(1,FCx);
                                    break;
                                }
                                default:  // no RX
                                {
                                    break;
                                }
                            }
                            datcount = 0;
                            saverest = false;
                        }

                        if (((datcount-1)%(5*rows))==0) // once per sweep => column
                        {
                            p = 100.0*(float)column/(float)columns;
                            sprintf(tmp,"Column %u of %u columns (=%5.1f%% of %usec filetime)",column,columns,p,filetime);
                            Form7->Label10->Caption = tmp; // show buffer pointers
                        }
                    }
                }
                hexcount = (hexcount+1)%4;
                break;
              } // end STX
              case 6: // EOT detected, end of HEX-data
              {
                 datastate = 0; // forget EOT data byte
                 datatermination = true;
                 Logging("$HST:EOT (&) detected");
                 break;
              }
           } // end switch(datastate)
        } while (RX_in!=RX_out);

        if ((labelcount++%8)==0)
        {
          if (RX_in>=RX_out)
             p = (float)dp / (float)RXbuflen * 100.0;
          else
             p = float(RX_in+(RXbuflen-RX_out)) / (float)RXbuflen * 100.0;

          if (eepromuploaded)
          {
            pmean = (10*pmean+p)/11.0;
            if (!overview)
            {
               sprintf(st,"Load =%4.0f%%",pmean);
               Percent->Caption = st;
               ProgressBar1->Position = pmean;
            }
            //sprintf(tmp,"Pin=%u Pout=%u",RX_in,RX_out);
            //MainForm->Label12->Caption=tmp; // show buffer pointers
          }
        }
    } // end if
}
//---------------------------------------------------------------------------
float __fastcall TMainForm::Digit2SFU(int curvenumber) // ADU -> Flux
{
  int index = rows - OlcList[curvenumber].channel -1; // ???

  float adc = MV[curvenumber]/lcc / adc_divider; // 0...1023 -> 0...255
  float exponent = (adc - CF[index].a)/CF[index].b;
  if (exponent < 0.43) exponent = 0.43; // = 2.7K
  if (exponent > 5.65) exponent = 5.65; // = 450'000K

  float Tant = pow(10.0,exponent);  // delogarithm
  float s = CF[index].cf * (Tant - CF[index].Tb); // calibration
  if (s <   -10.0) s =   -10.0;
  if (s > 10000.0) s = 10000.0; // S corresponds to ~ 450000K @ 7m-dish

  if (OlcList[curvenumber].count > 0)
    return(s); // calibrated light curve
  else
    return(0.0); // no light curve
}
//---------------------------------------------------------------------------
void __fastcall TMainForm::TimerTimer(TObject *Sender) // some periodic activity  50msec
{
    #define states 9
    static int state    = 0;
    static int cfgread  = 0;
    static bool first   = true;
    char st[80];

    MyClock(); // get time & date
    if (first) // first estimation of old_T
    {
        old_T = Tact;
        first = false;
        UploadThread->Upload(true);  // to guarantee correct frequencies 2012-04-02

    }

    switch(state)  // every ~60 msec
    {
        case 0:
        {
            Label1->Caption = "Time: " + TimerSt;
            EvalRX (); // read RX buffer and analyze data
            state++;
            break;
        }
        case 1:
        {
            if ((Tact-old_T)<0) // at midnight
            {
                Receivermessages->Clear();  // needed for Win9x with max 64K buffer size
                Logging("$HST:Midnight -> memo cleared");
                Logging("$HST:EOF(logfile)");
                CloseLogging();
                filenumber = 0; // reset FITS- file counter
                OpenLogging();
                Logging("$HST:STX(logfile)");
                lightcurves = true;
            }
            old_T = Tact;

            if (cfgread++%20 == 0) // every 50msec*3*20=3 sec
                ReadConfig(false); // reload

            state++;
            break;
        }
        case 2:
        {
            if (RadioButton1->Checked) // auto only
            {
                ReadScheduler(); // read scheduler file
                EvalScheduler(); // read scheduler data and analyze it
            }
            state = 0;
            break;
        }
    } // end switch(state)
}
//---------------------------------------------------------------------------
void __fastcall TMainForm::ReadConfig(bool first)
{
  char buff[240];
  AnsiString st;
  unsigned int priority=0;

  ifstream infile("callisto.cfg");
  while (infile)
  {
      infile.getline(buff,240);
      st = LowerCase(AnsiString(buff));
      if (buff[0]=='[')
      {
        if (first)
        {
           if (strstr(st.c_str(),"[rxcomport]="))    sscanf(buff,"[rxcomport]=%s"      ,&rxcomport);
           if (strstr(st.c_str(),"[frqfile]="))      sscanf(buff,"[frqfile]=%12c"      ,&frqfilename);
        }

        if (strstr(st.c_str(),"[observatory]="))     sscanf(buff,"[observatory]=%u"    ,&observatory);
        if (strstr(st.c_str(),"[instrument]="))      sscanf(buff,"[instrument]=%s"     ,&instrument);
        if (strstr(st.c_str(),"[titlecomment]="))    sscanf(buff,"[titlecomment]=%s"   ,&titlecomment);
        if (strstr(st.c_str(),"[origin]="))          sscanf(buff,"[origin]=%s"         ,&origin);
        if (strstr(st.c_str(),"[longitude]="))       sscanf(buff,"[longitude]=%c,%f",&MyGPS.LonCode, &MyGPS.Longitude);
        if (strstr(st.c_str(),"[latitude]="))        sscanf(buff,"[latitude]=%c,%f" ,&MyGPS.LatCode, &MyGPS.Latitude);
        if (strstr(st.c_str(),"[height]="))          sscanf(buff,"[height]=%f"      ,&MyGPS.Height);
        if (strstr(st.c_str(),"[clocksource]="))     sscanf(buff,"[clocksource]=%u"    ,&clocksource);
        if (strstr(st.c_str(),"[filetime]="))        sscanf(buff,"[filetime]=%u"       ,&filetime);

        if (first)
        {
           if (strstr(st.c_str(),"[focuscode]="))       sscanf(buff,"[focuscode]=%u"      ,&FCx);
        }
        if (strstr(st.c_str(),"[mmode]="))           sscanf(buff,"[mmode]=%c"          ,&mmode);

        YTbuflen  =  1000;  // 1000
        XYbuflen  =  2000;  // 2000
        XYZbuflen = 20000;  // 20000

        timerinterval  =   60;
        timerpreread   =    2;
        timeouthexdata = 1000;

        if (strstr(st.c_str(),"[fitsenable]="))      sscanf(buff,"[fitsenable]=%u"     ,&fitsenable);
        if (strstr(st.c_str(),"[datapath]="))        sscanf(buff,"[datapath]=%s"       ,&datapath);
        if (strstr(st.c_str(),"[logpath]="))         sscanf(buff,"[logpath]=%s"        ,&logpath);
        if (strstr(st.c_str(),"[lcpath]="))          sscanf(buff,"[lcpath]=%s"         ,&LCpath);
        if (strstr(st.c_str(),"[ovspath]="))         sscanf(buff,"[ovspath]=%s"        ,&OVSpath);

        low_band = 171.0;
        mid_band = 450.0;

        if (strstr(st.c_str(),"[chargepump]="))      sscanf(buff,"[chargepump]=%u"     ,&chargepump);
        if (strstr(st.c_str(),"[agclevel]="))        sscanf(buff,"[agclevel]=%u"       ,&agclevel);

        if (strstr(st.c_str(),"[detector_sens]="))   sscanf(buff,"[detector_sens]=%f"  ,&detector_sens);
        if (strstr(st.c_str(),"[db_scale]="))        sscanf(buff,"[db_scale]=%u"       ,&db_scale);

        if (strstr(st.c_str(),"[autostart]="))       sscanf(buff,"[autostart]=%u"      ,&autostart);

        if (strstr(st.c_str(),"[priority]="))       sscanf(buff,"[priority]=%u"      ,&priority);
      }
  }
  infile.close();

  if (first) // only at the beginning
  {
    switch (priority)
    {
       case 0:
       {
          HANDLE hCurrentProcess = GetCurrentProcess();
          SetPriorityClass(hCurrentProcess, NORMAL_PRIORITY_CLASS);
          break;
       }
       case 1:
       {
          HANDLE hCurrentProcess = GetCurrentProcess();
          SetPriorityClass(hCurrentProcess, ABOVE_NORMAL_PRIORITY_CLASS);
          break;
       }
       case 2:
       {
          HANDLE hCurrentProcess = GetCurrentProcess();
          SetPriorityClass(hCurrentProcess, HIGH_PRIORITY_CLASS);
          break;
       }
       case 3:
       {
          HANDLE hCurrentProcess = GetCurrentProcess();
          SetPriorityClass(hCurrentProcess, REALTIME_PRIORITY_CLASS);
          break;
       }
       default:
       {
          HANDLE hCurrentProcess = GetCurrentProcess();
          SetPriorityClass(hCurrentProcess, NORMAL_PRIORITY_CLASS);
          break;
       }
    }
  }

  if (!DirectoryExists(AnsiString(logpath)))
  {
    logpath[0]='\0';  // set to local
    Application->MessageBox("Path does not exist", "Logfile path error", MB_OK);
  }
  if (!DirectoryExists(AnsiString(datapath)))
  {
    datapath[0]='\0';  // set to local
    Application->MessageBox("Path does not exist", "Datafile path error", MB_OK);
  }
  if (!DirectoryExists(AnsiString(LCpath)))
  {
    LCpath[0]='\0';  // set to local
    Application->MessageBox("Path does not exist", "Light curve path error", MB_OK);
  }
  if (!DirectoryExists(AnsiString(OVSpath)))
  {
    OVSpath[0]='\0';  // set to local
    Application->MessageBox("Path does not exist", "Spectral overview path error", MB_OK);
  }

  // prepare global data buffers....
  if (first) // init only
  {
    Yt_ringbuff  = new int[YTbuflen];  // light curve
    XY_ringbuff  = new int[XYbuflen];  // 1D spectrum
    BGbuffer     = new float[XYbuflen]; // background buffer
    XYZ_ringbuff = new Byte[XYZbuflen]; // 2D spectrum
  }
  if (!first) // relaod only
  {
    if (agclevel!=old_agclevel)
    {
       sprintf(buff,"O%03u\r",agclevel);
       RX_SerialThread->Send(buff); // Set tuner gain (agc)
       old_agclevel = agclevel;
    }
  }

  sprintf(buff,"e-Callisto Radiospectrometer  %s",titlecomment);
  MainForm->Caption = buff;

  sprintf(buff,"e-Callisto  %s",titlecomment);
  Application->Title = buff;
}
//---------------------------------------------------------------------------
void __fastcall TMainForm::EEPROMButtonClick(TObject *Sender) // Define frequency program manually and read it
{
    AnsiString filename, fpath;

    if(OpenDialog1->Execute())
    {
      filename = LowerCase(ExtractFileName(OpenDialog1->FileName)); // filename only
      fpath    = LowerCase(ExtractFilePath(OpenDialog1->FileName)); // filename path
      sprintf(frqfilename,"%s",StrUpper(filename.c_str())); // save filename into global variable
      sprintf(frqpath    ,"%s",fpath.c_str()); // save path into global variable

      // global filename will be taken either from callisto.cfg or scheduler.cfg
      // or it can be entered via manual selection
      Logging("$HST:Manual frequencyfile sel.");
      UploadThread->Upload(true);
    }
}
//---------------------------------------------------------------------------
void __fastcall TMainForm::StartButtonClick(TObject *Sender) // Start
{
    measurestate = 2; // 0=idle, 1=enable, 2=parameter, 3=new file, 4=measure, 5=disable, 6=stop
}
//---------------------------------------------------------------------------
void __fastcall TMainForm::StopButtonClick(TObject *Sender) // Stop transmitting data
{
    measurestate = 5; // 0=idle, 1=enable, 2=parameter, 3=new file, 4=measure, 5=disable, 6=stop
}
//---------------------------------------------------------------------------
void __fastcall TMainForm::SpectrumButton2DClick(TObject *Sender)
{
    if (Form1->Visible==false)
    {
       Form1->Show(); // XY-plot
       Logging("$HST:XY plot selected");
    } else
    if (Form1->Visible)
    {
       Form1->Hide(); // XY-plot
       Logging("$HST:XY plot hidden");
    }
}
//---------------------------------------------------------------------------
void __fastcall TMainForm::SpectrumButton3DClick(TObject *Sender)
{
    if (Form2->Visible==false)
    {
       Form2->Show(); // XYZ-plot
       Logging("$HST:XYZ plot selected");
    } else
    if (Form2->Visible)
    {
       Form2->Hide(); // XYZ-plot
       Logging("$HST:XYZ plot hidden");
    }
}
//---------------------------------------------------------------------------
void __fastcall TMainForm::Button5Click(TObject *Sender)
{
    if (Form3->Visible==false)
    {
       Form3->Show(); // Yt-plot
       Logging("$HST:Yt plot selected");
    } else
    if (Form3->Visible)
    {
       Form3->Hide(); // XYZ-plot
       Logging("$HST:Yt plot hidden");
    }
}
//---------------------------------------------------------------------------
void __fastcall TMainForm::Edit1Click(TObject *Sender)
{
    PopupMenu1->Popup(MainForm->Left+50, MainForm->Top+50);
}
//---------------------------------------------------------------------------
void __fastcall TMainForm::FileView1Click(TObject *Sender)
{
    WinExec(("notepad.exe callisto.cfg"),SW_SHOW);
    Logging("$HST:callisto.cfg watched");
}
//---------------------------------------------------------------------------
void __fastcall TMainForm::Schedulercfg1Click(TObject *Sender)
{
    WinExec(("notepad.exe scheduler.cfg"),SW_SHOWNORMAL);
    Logging("$HST:scheduler.cfg watched");
}
//---------------------------------------------------------------------------
void __fastcall TMainForm::FormClose(TObject *Sender, TCloseAction &Action)
{
    RX_SerialThread->Close();
    UploadThread->Close();

    Logging("$HST:All threads terminated");
    Logging("$HST:This application closed");

    CloseLogging();

    delete [] Yt_ringbuff;
    delete [] XY_ringbuff;
    delete [] BGbuffer;
    delete [] XYZ_ringbuff;
    delete [] pszBuffer_RX1;

    Application->Terminate();
}
//---------------------------------------------------------------------------
void __fastcall TMainForm::RadioButton1Click(TObject *Sender) // automatic
{
    StartButton->Enabled  = false;
    StopButton->Enabled   = false;
    EEPROMButton->Enabled = false;
    Overview->Enabled     = false;
    Label20->Enabled      = true;  // scheduler text
    Logging("$HST:Switched to automatic");
}
//---------------------------------------------------------------------------
void __fastcall TMainForm::RadioButton2Click(TObject *Sender) // manual
{
    if (measurestate<2) // halt or idle
    {
       StartButton->Enabled  = true;
       EEPROMButton->Enabled = true;
       Overview->Enabled     = true;
    }
    else StopButton->Enabled = true;

    Label20->Enabled  = false;  // scheduler text
    Logging("$HST:Switched to manual");
}
//---------------------------------------------------------------------------
void __fastcall TMainForm::Help1Click(TObject *Sender)
{
    PopupMenu2->Popup(MainForm->Left+50, MainForm->Top+50);
}
//---------------------------------------------------------------------------
void __fastcall TMainForm::OperatingManual1Click(TObject *Sender)
{
    ShellExecute(Application->Handle,"open","acrord32.exe","eCallistoManual.pdf",NULL,SW_SHOWNORMAL);
    Logging("$HST:eCallistoManual.pdf watched");
}
//---------------------------------------------------------------------------
void __fastcall TMainForm::InfoUpdates(void)
{
    char st[120];
    sprintf(st,"Filename frequency program: %s",frqfilename);
    Form7->Label1->Caption = st;

    sprintf(st,"Number of channels per sweep: %u",rows);
    Form7->Label2->Caption = st;

    sprintf(st,"Number of sweeps per second: %u",nsps);
    Form7->Label3->Caption = st;

    sprintf(st,"Resolution in time domain: %6.1fmsec",1000.0/(float)nsps);
    Form7->Label4->Caption = st;

    sprintf(st,"Code FPU (focal plane unit): %u",FCx);
    Form7->Label12->Caption = st;

    MainForm->Label2->Caption = DatumSt;

    switch(clocksource)
    {
        case 0: // software
        {
          sprintf(st,"Measurement time per pixel: %6.2fmsec (clock = %uHz derived from software)",
                      1000.0/(float)nsps/(float)rows,rows*nsps);
          break;
        }
        case 1: // intern
        {
          sprintf(st,"Measurement time per pixel: %6.2fmsec (clock = %uHz derived from internal quartz)",
                      1000.0/(float)nsps/(float)rows,rows*nsps);
          break;
        }
        case 2: // external
        {
          sprintf(st,"Measurement time per pixel: %6.2fmsec (clock = %uHz derived from external 1MHz)",
                      1000.0/(float)nsps/(float)rows,rows*nsps);
          break;
        }
    }
    Form7->Label5->Caption = st;

    sprintf(st,"Actual logfile: %s",logfilename);
    Form7->Label6->Caption = st;
}
//---------------------------------------------------------------------------
void __fastcall TMainForm::Info1Click(TObject *Sender)
{
    Form7->Show(); // activate separate panel
    Logging("$HST:Info panel opened");
}
//---------------------------------------------------------------------------
void __fastcall TMainForm::OverviewClick(TObject *Sender)
{
    Overview->Enabled     = false;  // prevent recursive OV
    StartButton->Enabled  = false;  // prevent start
    EEPROMButton->Enabled = false;  // prevent new frq loading
    RadioButton1->Enabled = false;  // prevent auto
    Logging("$HST:Start overview...");
    Logging("$HST:Wait ~45sec please!");
    RX_SerialThread->Send("T0\r"); // software trigger
    RX_SerialThread->Send("M2\r"); // delay 1 msec  min, for long comm. cable 2 msec
    RX_SerialThread->Send("%5\r"); // Format=Frequency;MilliVolts
    RX_SerialThread->Send("F0045.0\r"); // start frequency
    RX_SerialThread->Send("L13200\r"); // all channels
    RX_SerialThread->Send("P2\r"); // go
    overview = true;
    OpenOverview();
    // the end is awaited in datastate==3 of EvalRX()
}
//---------------------------------------------------------------------------
void __fastcall TMainForm::Timer1Timer(TObject *Sender) // data loss proces
{
    static int state = 0;
    switch (state)
    {
        case 0:
        {
            StopButtonClick(0);
            Logging("$HST:Auto stop due to data loss.");
            state++;
            break;
        }
        case 1:
        {
            Logging("$HST:Check RS232-connection!");
            state++;
            break;
        }
        case 2:
        {
            Logging("$HST:Attempting Auto-Start");
            StartButtonClick(0);
            Timer1->Enabled = false;
            EraseBuffer();
            datcount = 0;
            row      = 0;
            column   = 0;
            state    = 0;
            break;
        }
    }
}
//---------------------------------------------------------------------------
void __fastcall TMainForm::Timer2Timer(TObject *Sender) // autostart process
{
    static int state = 0;
    char buff[20];
    switch (state)
    {
        case 0:
        {
            Logging("$HST:Auto start due to PON");
            state++;
            break;
        }
        case 1:
        {
            ProgressBar1->Position = Sect;
            sprintf(buff,"Timer =%3usec",60-Sect);
            Percent->Caption = buff;
            if (Sect == 0)
               state++;
            break;
        }
        case 2:
        {
            StartButtonClick(0);
            Timer2->Enabled = false;
            break;
        }
    }
}
//---------------------------------------------------------------------------
void __fastcall TMainForm::test1Click(TObject *Sender)
{
    try
    {
      ShellExecute(Application->Handle,
        "open", "http://www.reeve.com/Documents/CALLISTO/CALLISTOSoftwareSetup.pdf",
        NULL, NULL, SW_SHOWDEFAULT); // funktioniert
      Logging("$HST:Download SW-manual");
    }
    catch (Exception &exception)
    {
      Logging("$HST:Cannot open browser");
    }
}
//---------------------------------------------------------------------------
void __fastcall TMainForm::DownloadLatestHardwareManual1Click(
      TObject *Sender)
{
    try
    {
      ShellExecute(Application->Handle,
        "open", "http://www.reeve.com/Documents/CALLISTO/CALLISTOConstruction.pdf",
        NULL, NULL, SW_SHOWDEFAULT); // funktioniert
      Logging("$HST:Download HW-manual");
    }
    catch (Exception &exception)
    {
      Logging("$HST:Cannot open browser");
    }
}
//---------------------------------------------------------------------------
void __fastcall TMainForm::GotoCallistoFolder1Click(TObject *Sender)
{
    try
    {
      ShellExecute(Application->Handle, "explore", NULL, NULL, NULL, SW_SHOWNORMAL); // o.k.
      Logging("$HST:Open Explorer");
    }
    catch (Exception &exception)
    {
      Logging("$HST:Cannot open folder");
    }
}
//---------------------------------------------------------------------------
void __fastcall TMainForm::OnlineInfos1Click(TObject *Sender)
{
    Form7->Show(); // activate separate panel
    Logging("$HST:Info panel opened");
}
//---------------------------------------------------------------------------
void __fastcall TMainForm::DeviceManager1Click(TObject *Sender)
{
    system("devmgmt.msc");
    Logging("$HST:DevManager opened");
}
//---------------------------------------------------------------------------
void __fastcall TMainForm::TaskManager1Click(TObject *Sender)
{
    WinExec("taskmgr.exe",SW_NORMAL);
    Logging("$HST:TaskManager opened");
}
//---------------------------------------------------------------------------

void __fastcall TMainForm::Timer3Timer(TObject *Sender)  // check RS-232
{
  static int counter=0;
  char buff[50];
  sprintf(buff,"Awaiting response from Callisto:%i",counter);
  Receivermessages->Lines->Add(buff); // Memo
  if ((counter++ > 5) && (response==false))
  {
        Timer->Enabled = false;
        Timer1->Enabled = false;
        Timer3->Enabled = false;
        RX_SerialThread->Close();
        UploadThread->Close();
        Application->MessageBox("Check Callisto power and serial connection","Callisto is not responding", MB_OK);
        Application->Terminate();
  }
  if (response)
      Timer3->Enabled = false;

}
//---------------------------------------------------------------------------

