/*-----------------------------------------------------------------------*/
/*  (C) Copyright Institute of Astronomy ETHZ 8092 Zuerich Switzerland   */
/*-----------------------------------------------------------------------*/
/*  Programmname: callisto.cpp                                           */
/*                                                                       */
/*  Revision: V1.1     Date: 23.09.2003    Autor: Chr. Monstein          */
/*                                                                       */
/*  Purpose: Hostcontroller Radiospectrometer CALLISTO                   */
/*                                                                       */
/*  Compiler: BORLAND C++Builder 6                                       */
/*-----------------------------------------------------------------------*/

// Created by: Chr. Monstein 05.05.2003
// Updated by: Chr. Monstein 23.09.2003 scheduler read process implemented

//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop
USEFORM("mainunit.cpp", MainForm);
USEFORM("XY.cpp", Form1);
USEFORM("Yt.cpp", Form3);
USEFORM("XYZ.cpp", Form2);
USEFORM("Info.cpp", Form7);
//---------------------------------------------------------------------------
WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    try
    {
        Application->Initialize();
        Application->Title = "Callisto Control";
        Application->CreateForm(__classid(TMainForm), &MainForm);
                 Application->CreateForm(__classid(TForm1), &Form1);
                 Application->CreateForm(__classid(TForm2), &Form2);
                 Application->CreateForm(__classid(TForm3), &Form3);
                 Application->CreateForm(__classid(TForm7), &Form7);
                 Application->Run();
    }
    catch (Exception &exception)
    {
        Application->ShowException(&exception);
    }
    return 0;
}
//---------------------------------------------------------------------------


