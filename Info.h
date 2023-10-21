/*-----------------------------------------------------------------------*/
/*  (C) Copyright Institute of Astronomy ETHZ 8092 Zuerich Switzerland   */
/*-----------------------------------------------------------------------*/
/*  Programmname: Info.h                                                 */
/*                                                                       */
/*  Revision: V1.1     Date: 29.11.2004    Autor: Chr. Monstein          */
/*                                                                       */
/*  Purpose: Info panel for Radiospectrometer CALLISTO                   */
/*                                                                       */
/*  Compiler: BORLAND C++Builder 6                                       */
/*-----------------------------------------------------------------------*/

// Created by: Chr. Monstein 29.11.2004
// Updated by: Chr. Monstein 30.11.2004  header included
//---------------------------------------------------------------------------

#ifndef InfoH
#define InfoH
//---------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
//---------------------------------------------------------------------------
class TForm7 : public TForm
{
__published:	// Von der IDE verwaltete Komponenten
    TLabel *Label1;
    TLabel *Label2;
    TLabel *Label3;
    TLabel *Label4;
    TLabel *Label5;
    TLabel *Label6;
    TLabel *Label8;
    TLabel *Label10;
    TLabel *Label11;
    TLabel *Label7;
    TLabel *Label9;
    TLabel *Label12;
    TLabel *Label13;
    TLabel *Label14;
    TLabel *Label15;
private:	// Anwender-Deklarationen
public:		// Anwender-Deklarationen
    __fastcall TForm7(TComponent* Owner);
};
//---------------------------------------------------------------------------
extern PACKAGE TForm7 *Form7;
//---------------------------------------------------------------------------
#endif
