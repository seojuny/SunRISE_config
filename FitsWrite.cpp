/*-----------------------------------------------------------------------*/
/*  (C) Copyright Institute of Astronomy ETHZ 8092 Zuerich Switzerland   */
/*-----------------------------------------------------------------------*/
/*  Programmname: FitsWrite.cpp                                          */
/*                                                                       */
/*  Revision: V1.1     Date: 07.04.2008    Autor: Chr. Monstein          */
/*                                                                       */
/*  Purpose: save data as FITS file                                      */
/*                                                                       */
/*  Compiler: BORLAND C++Builder 6                                       */
/*-----------------------------------------------------------------------*/

// Created by: Chr. Monstein 01.10.2004
// Updated by: Chr. Monstein 13.11.2004
// Updated by: Chr. Monstein 30.11.2004 adapted to CALLISTO
// Updated by: Chr. Monstein 02.12.2004 int pszBuffer -> Byte pszBuffer
// Updated by: Chr. Monstein 05.12.2004 labels 16bit -> 8bit, rep. filename
// Updated by: Chr. Monstein 06.12.2004 more keys inserted
// Updated by: Chr. Monstein 02.06.2005 x-axis as double, new origin, locosc
// Updated by: Chr. Monstein 07.04.2008 axis-keywords updated
// Updated by: Chr. Monstein 11.06.2009 calibration process
// Updated by: Chr. Monstein 23.06.2009 object=Sun -> SFU (JaveViewer)
// Updated by: Chr. Monstein 24.06.2009 calibration combined with mmode=2
// Updated by: Chr. Monstein 20.11.2010 delte keyword COMMENT, HISTORY
// Updated by: Chr. Monstein 30.05.2014 new calibration mode (1) in Kelvin
//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#include "fitsio.h"
#include <fstream.h>
#include <math.h>
#include "FitsWrite.h"
#include "mainunit.h"
#include "Info.h"
#include "eeprom.h"

//---------------------------------------------------------------------------
extern Byte *pszBuffer_RX1;        // import from mainunit.cpp
extern long column;                // import from mainunit.cpp
extern int rows;                   // import from mainunit.cpp
extern int nsps;                   // import from mainunit.cpp
extern long filenumber;            // import from mainunit.cpp
extern TDateTime dtOld;            // import from mainunit.cpp
extern char datapath[120];         // import from mainunit.cpp
extern char instrument[80];        // import from mainunit.cpp
extern char origin[80];            // import from mainunit.cpp
extern char mmode;                 // import from mainunit.cpp

extern char frqfilename[80];       // import from mainunit.cpp
extern int agclevel;               // import from mainunit.cpp

extern float freqinlist[501];      // import from EEPROM.cpp
extern float locosc;               // import from EEPROM.cpp
extern Tcal CF[500];               // import from EEPROM.cpp
extern bool calibration;           // import from EEPROM.cpp

extern TGPSdata MyGPS;             // import from GPSRS232.cpp

//---------------------------------------------------------------------------
int WriteFits(int rx, int fc)
{
    Word year, month, day, hh, mm, ss, MSec;
    int hh2, mm2, ss2;
    double dt, DT, t1, t2;
    char obsst[80], object[80];
    long columns = column+1; // actual number of columns (0...7 => 8!)

    DecodeDate(dtOld, year, month, day);
    DecodeTime(dtOld, hh, mm, ss, MSec);

    if (rows>8)
    {
        long ii,jj,q;
        int m,n;
        for (ii=0; ii<columns; ii++)
        {
          m = 0;
          n = 0;
          for (jj=0; jj<rows-8; jj++)
          {
            q = ii*rows + (rows-1-jj);   // address = Transpose(fits-address) low freq top
            m = m + pszBuffer_RX1[q];
            n++;
          }
          m = (float)m/(float)n + 0.5; // average over N-8 channels

          jj=rows-8;
          {
            q = ii*rows + (rows-1-jj);   // average
            pszBuffer_RX1[q]=int(m);
          }

          jj=rows-7;
          {
            q = ii*rows + (rows-1-jj);   // agclevel
            pszBuffer_RX1[q]=agclevel;
          }

          for (jj=rows-6; jj<rows; jj++)
          {
            q = ii*rows + (rows-1-jj);   // staircase (column)
            pszBuffer_RX1[q]=ii%255;
          }
        }
    }

    dt  = 1.000/(double)nsps; // time per sweep [sec]
    DT  = columns*dt; // time per file  [sec]
    t1  = hh*3600 + mm*60 + ss;
    t2  = t1 + DT;
    hh2 = int(t2/3600);
    mm2 = int(t2/60 - hh2*60);
    ss2 = int(t2 - hh2*3600 - mm2*60);
    sprintf(obsst,instrument); // =f(obsnr)  LAB, ZSTS, BLEN, NRAO,...
    sprintf(object,"Sun"); // sorgt alleine dafuer, dass der JavaViewer als unit SFU anzeigt...

    // fits-stuff ...
    //double *parray = new double [(long)rows*columns];  // fits-data buffer
    unsigned short int *parray = new unsigned short int [(long)rows*columns];  // fits-data buffer
    long naxes[2] = {columns, (long)rows}; // image direction and dimensions
    long p, q; // index of fits-array
    fitsfile *fptr; // required by every program that uses CFITSIO
    long ii, jj;
    int status = 0;
    long fpixel = 1, naxis = 2, nelements = columns * (long)rows; // number of pixels to write;
    double arg;
    char st[240], fitfilename[240];
    long iarg;
    char carg;

    // evaluate fitfilename here
    sprintf(fitfilename,"%s%s_%04u%02u%02u_%02u%02u%02u_%02u.fit",datapath,obsst,year,month,day,hh,mm,ss,fc);

    DeleteFile(fitfilename); // erase old file, if any
    fits_create_file(&fptr, fitfilename, &status); // create new file

    //fits_create_img(fptr, SHORT_IMG, naxis, naxes, &status); //create the primary array image (16-bit short integer pixels
    fits_create_img(fptr, BYTE_IMG, naxis, naxes, &status); //create the primary array image (8-bit pixels
    // write header ....
    sprintf(st,"%04u-%02u-%02u",year,month,day);
    fits_update_key(fptr,TSTRING, "DATE"     , st, "Time of observation", &status);
    //sprintf(st,"%04u/%02u/%02u %02u:%02u:%02u Radio flux density 8bit",year,month,day, hh,mm,ss);
    sprintf(st,"%04u/%02u/%02u  Radio flux density, e-CALLISTO (%s)",year,month,day,obsst);

    fits_update_key(fptr,TSTRING, "CONTENT"  , st, "Title of image", &status);
    fits_update_key(fptr,TSTRING, "ORIGIN"   , origin, "Organization name", &status); // default=ETH Zurich Switzerland
    fits_update_key(fptr,TSTRING, "TELESCOP" , "Radio Spectrometer", "Type of instrument", &status);

    fits_update_key(fptr,TSTRING, "INSTRUME" , obsst, "Name of the spectrometer", &status);
    fits_update_key(fptr,TSTRING, "OBJECT"   , object,"object description", &status);

    sprintf(st,"%04u/%02u/%02u",year,month,day);
    fits_update_key(fptr,TSTRING, "DATE-OBS" , st, "Date observation starts", &status);
    sprintf(st,"%02u:%02u:%02u.%03u",hh,mm,ss,MSec);
    fits_update_key(fptr,TSTRING, "TIME-OBS" , st, "Time observation starts", &status);

    sprintf(st,"%04u/%02u/%02u",year,month,day);
    fits_update_key(fptr,TSTRING, "DATE-END" , st, "date observation ends", &status);

    sprintf(st,"%02u:%02u:%02u",hh2,mm2,ss2); // to be recalculated!
    fits_update_key(fptr,TSTRING, "TIME-END" , st, "time observation ends", &status);

    // transpose data array
    if (calibration && mmode=='2') // calnnnnn.par must be present and mmode must be set for calibration [SFU]
    {
        float adc, exponent, Tant, s;
        for (ii=0; ii<columns; ii++)
        {
          for (jj=0; jj<rows; jj++)
          {
            p = jj*columns + ii;   // fit-address
            q = ii*rows + (rows-1-jj);   // address = Transpose(fits-address) low freq top
            adc = (float)pszBuffer_RX1[q];
            exponent = (adc - CF[jj].a)/CF[jj].b;
            if (exponent < 0.43) exponent = 0.43; // = 2.7K
            if (exponent > 5.65) exponent = 5.65; // = 450'000K
            Tant = pow(10.0,exponent);  // delogarithm
            s = CF[jj].cf * (Tant - CF[jj].Tb); // calibration
            if (s < -9.0) s = -9.0; // prevent infinite low value
            Tant = 45.0*log10(s + 10.0); // log compression
            parray[p] = (unsigned short int)Tant;
          }
        }
    }

    if (calibration && mmode=='1') // calnnnnn.par must be present and mmode must be set for calibration [Kelvin]
    {
        float adc, exponent, Tant, Y;
        for (ii=0; ii<columns; ii++)
        {
          for (jj=0; jj<rows; jj++)
          {
            p = jj*columns + ii;   // fit-address
            q = ii*rows + (rows-1-jj);   // address = Transpose(fits-address) low freq top
            adc = (float)pszBuffer_RX1[q];
            exponent = (adc - CF[jj].a)/CF[jj].b;
            if (exponent < 0.43) exponent = 0.43; // = 2.7K
            if (exponent > 6.30) exponent = 6.30; // = 1MK
            Y = pow(10.0,exponent);  // delogarithm
            Tant = Y*290.0 - CF[jj].Tb; // calibration
            if (Tant < 2.7) Tant = 2.7; // prevent infinite low value
            Tant = 40.0*log10(Tant) + 0.5; // log compression
            parray[p] = (unsigned short int)Tant;
          }
        }
    }

    if (mmode=='3') // just raw data (digits)
    {
        for (ii=0; ii<columns; ii++)
        {
          for (jj=0; jj<rows; jj++)
          {
            p = jj*columns + ii;   // fit-address
            q = ii*rows + (rows-1-jj);   // address = Transpose(fits-address) low freq top
            parray[p] = (unsigned short int)pszBuffer_RX1[q];
          }
        }
    }

    int mini=255, maxi=0;
    for (p=0; p<nelements; p++)  // check all pixels
    {
        if (parray[p] < mini)
            mini = parray[p];
        if (parray[p] > maxi)
            maxi = parray[p];
    }
    //float bscale = ((float)maxi-(float)mini)/(255.0-0.0);
    //float bzero  = (float)mini;

   // arg=bzero;  fits_update_key(fptr,TDOUBLE, "BZERO"    , &arg, "scaling offset"   , &status);
   // arg=bscale; fits_update_key(fptr,TDOUBLE, "BSCALE"   , &arg, "scaling factor"   , &status);
    arg=0.0;  fits_update_key(fptr,TDOUBLE, "BZERO"    , &arg, "scaling offset"   , &status);
    arg=1.0; fits_update_key(fptr,TDOUBLE, "BSCALE"   , &arg, "scaling factor"   , &status);
    // to get real SFU with JavaViewer bzero MST be 0 and bscale MUST be 1!!!
    // otherwise JavaDisplay recalculates flux twice (= bad function o NASAs mrdfits)
    if (calibration && mmode=='2')
       fits_update_key(fptr,TSTRING, "BUNIT"    , "45*log(sfu+10)", "z-axis title" , &status);

    if (calibration && mmode=='1')
       fits_update_key(fptr,TSTRING, "BUNIT"    , "40*log(Tant)", "z-axis title" , &status);

    if (mmode=='3')
       fits_update_key(fptr,TSTRING, "BUNIT"    , "digits", "z-axis title" , &status);

    iarg=mini;  fits_update_key(fptr,TLONG  , "DATAMIN"  , &iarg, "minimum element in image", &status);
    iarg=maxi;  fits_update_key(fptr,TLONG  , "DATAMAX"  , &iarg, "maximum element in image", &status);

    arg=t1;     fits_update_key(fptr,TDOUBLE, "CRVAL1"   , &arg, "value on axis 1 at reference pixel [sec of day]"      , &status);
    arg=0;      fits_update_key(fptr,TLONG  , "CRPIX1"   , &arg, "reference pixel of axis 1"                            , &status);
                fits_update_key(fptr,TSTRING, "CTYPE1"   , "Time [UT]", "title of axis 1"                       , &status);
    arg=dt;     fits_update_key(fptr,TDOUBLE, "CDELT1"   , &arg, "step between first and second element in x-axis [sec]", &status);

    arg=(double)rows;
                fits_update_key(fptr,TDOUBLE, "CRVAL2"   , &arg, "value on axis 2 at the reference pixel"       , &status);
    iarg=0;     fits_update_key(fptr,TLONG  , "CRPIX2"   , &iarg, "reference pixel of axis 2"                   , &status);
                fits_update_key(fptr,TSTRING, "CTYPE2"   , "Frequency [MHz]", "title of axis 2"             , &status);
    arg=-1;     fits_update_key(fptr,TDOUBLE, "CDELT2"   , &arg, "step between first and second element in axis", &status); //~(fmax-fmin)/(rows-1)

                //fits_update_key(fptr,TSTRING, "COMMENT"  , "Warning: the value of CDELT1 may be rounded!", ""   , &status);
                //fits_update_key(fptr,TSTRING, "COMMENT"  , "Warning: the frequency axis may not be regular!", "", &status);
                //fits_update_key(fptr,TSTRING, "COMMENT"  , "Warning: the value of CDELT2 may be rounded!", ""   , &status);
                //fits_update_key(fptr,TSTRING, "COMMENT"  , "FITS Definition document #100 and other FITS information", "", &status);

    // environmental parameters
    arg=MyGPS.Latitude;  fits_update_key(fptr,TDOUBLE, "OBS_LAT" , &arg, "observatory latitude in degree"   , &status);
    sprintf(st,"%c",MyGPS.LatCode);
                         fits_update_key(fptr,TSTRING, "OBS_LAC" , st  , "observatory latitude code {N,S}"  , &status);
    arg=MyGPS.Longitude; fits_update_key(fptr,TDOUBLE, "OBS_LON" , &arg, "observatory longitude in degree"  , &status);
    sprintf(st,"%c",MyGPS.LonCode);
                         fits_update_key(fptr,TSTRING, "OBS_LOC" , st  , "observatory longitude code {E,W}" , &status);
    arg=MyGPS.Height;    fits_update_key(fptr,TDOUBLE, "OBS_ALT", &arg , "observatory altitude in meter asl", &status);

                         fits_update_key(fptr,TSTRING, "FRQFILE" , frqfilename, "name of frequency file" , &status);
    iarg=agclevel;       fits_update_key(fptr,TLONG  , "PWM_VAL" , &iarg, "PWM value to control tuner gain", &status);

                //fits_update_key(fptr,TSTRING, "HISTORY"  , "", "", &status);

    //fits_write_img(fptr,TDOUBLE,fpixel,nelements,parray,&status); // write the array of the integers to the image
    fits_write_img(fptr,TUSHORT,fpixel,nelements,parray,&status); // write the array of the integers to the image

    // write axes ..., header first (very important, given by cfitsio!)
    char* tType[2] = {"TIME", "FREQUENCY"};
    char* tForm[2];
    tForm[0] = new char[30]; // time
    tForm[1] = new char[30]; // frequency

    sprintf(tForm[0],"%dD8.3",columns); // double x-axis (time)
    sprintf(tForm[1],"%dD8.3",rows);    // double y-axis (frequency)

    fits_create_tbl(fptr, BINARY_TBL, 0, 2, tType, tForm, NULL, NULL, &status);

    double newTScale = 1.0;
    fits_write_key(fptr, TDOUBLE, "TSCAL1", &newTScale, NULL, &status);
    double newTZero = 0.0;
    fits_write_key(fptr, TDOUBLE, "TZERO1", &newTZero, NULL, &status);

    double newFScale = 1.0;
    fits_write_key(fptr, TDOUBLE, "TSCAL2", &newFScale, NULL, &status);
    double newFZero = 0.0;
    fits_write_key(fptr, TDOUBLE, "TZERO2", &newFZero, NULL, &status);

    // write time axis...
    double *timeAxis = new double[columns];
    for (long i=0; i<columns; i++)
       timeAxis[i]=i*dt;
    fits_write_col(fptr, TDOUBLE, 1 ,1 ,1, columns, timeAxis, &status);

    // write frequency axis...
    double *freqAxis = new double[rows];
    for (long i=0; i<rows; i++)
      freqAxis[rows-1-i]=(double) freqinlist[i]; // reverse axis!

    fits_write_col(fptr, TDOUBLE, 2, 1, 1, rows, freqAxis, &status);

    fits_close_file(fptr,&status); // close the file

    delete [] parray;    // free fits-buffer
    delete [] freqAxis;  // free y-axis
    delete [] timeAxis;  // free x-axis
    delete [] tForm[0];
    delete [] tForm[1];

    sprintf(st,"Actual FITS %s written; %urows x %ucols",fitfilename,rows,columns);
    Form7->Label11->Caption = st;

    char drive = UpCase(datapath[0]);
    int n = drive - 65 + 1;
    __int64 AmtFree = DiskFree(n);  // (0 = aktuelles Laufwerk, 1 = A, 2 = B usw.).
    __int64 Total = DiskSize(n);    //( 0 = aktuelles Laufwerk, 1 = A, 2 = B usw.).
    AnsiString S;
    S.sprintf("%6.3f percent of the space on drive %c: is free: %I64d KB", (float)AmtFree*100.0/(float)Total, drive, AmtFree/1024 );
    Form7->Label15->Caption = S;  // see also Infopanel

    ++filenumber;
    sprintf(st,"$HST:FITS #%u %urows x %ucols",filenumber,rows,columns);
    MainForm->Logging(st);

    return(status);
}
//---------------------------------------------------------------------------

