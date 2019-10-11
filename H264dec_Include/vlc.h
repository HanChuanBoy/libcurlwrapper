/**********************************************************************
 * Software Copyright Licensing Disclaimer
 *
 * This software module was originally developed by contributors to the
 * course of the development of ISO/IEC 14496-10 for reference purposes
 * and its performance may not have been optimized.  This software
 * module is an implementation of one or more tools as specified by
 * ISO/IEC 14496-10.  ISO/IEC gives users free license to this software
 * module or modifications thereof. Those intending to use this software
 * module in products are advised that its use may infringe existing
 * patents.  ISO/IEC have no liability for use of this software module
 * or modifications thereof.  The original contributors retain full
 * rights to modify and use the code for their own purposes, and to
 * assign or donate the code to third-parties.
 *
 * This copyright notice must be included in all copies or derivative
 * works.  Copyright (c) ISO/IEC 2004.
 **********************************************************************/

/*!
 ************************************************************************
 * \file vlc.h
 *
 * \brief
 *    header for (CA)VLC coding functions
 *
 * \author
 *    Karsten Suehring
 *
 ************************************************************************
 */

#ifndef _VLC_H_
#define _VLC_H_
const byte NCBP[48][2]=
{
  {47, 0},{31,16},{15, 1},{ 0, 2},{23, 4},{27, 8},{29,32},{30, 3},{ 7, 5},{11,10},{13,12},{14,15},
  {39,47},{43, 7},{45,11},{46,13},{16,14},{ 3, 6},{ 5, 9},{10,31},{12,35},{19,37},{21,42},{26,44},
  {28,33},{35,34},{37,36},{42,40},{44,39},{ 1,43},{ 2,45},{ 4,46},{ 8,17},{17,18},{18,20},{20,24},
  {24,19},{ 6,21},{ 9,26},{22,28},{25,23},{32,27},{33,29},{34,30},{36,22},{40,25},{38,38},{41,41},
};

int se_v (char *tracestring, Bitstream *bitstream);
int ue_v (char *tracestring, Bitstream *bitstream);
int u_1 (char *tracestring, Bitstream *bitstream);
int u_v (int LenInBits, char *tracestring, Bitstream *bitstream);

// UVLC mapping
void linfo_ue(int len, int info, int *value1, int *dummy);
void linfo_se(int len, int info, int *value1, int *dummy);

void linfo_cbp_intra(int len,int info,int *cbp, int *dummy);
void linfo_cbp_inter(int len,int info,int *cbp, int *dummy);
void linfo_levrun_inter(int len,int info,int *level,int *irun);
void linfo_levrun_c2x2(int len,int info,int *level,int *irun);

int  readSyntaxElement_VLC (SyntaxElement *sym, Bitstream *currStream);
int  readSyntaxElement_UVLC(SyntaxElement *sym, struct img_par *img, struct inp_par *inp, struct datapartition *dp);
int  readSyntaxElement_Intra4x4PredictionMode(SyntaxElement *sym, struct img_par *img, struct inp_par *inp, struct datapartition *dp);

int  GetVLCSymbol (byte buffer[],int totbitoffset,int *info, int bytecount);
int  GetVLCSymbol_IntraMode (byte buffer[],int totbitoffset,int *info, int bytecount);

int readSyntaxElement_FLC(SyntaxElement *sym, Bitstream *currStream);
int readSyntaxElement_NumCoeffTrailingOnes(SyntaxElement *sym,  DataPartition *dP,
                                           char *type);
int readSyntaxElement_NumCoeffTrailingOnesChromaDC(SyntaxElement *sym,  DataPartition *dP);
int readSyntaxElement_Level_VLC0(SyntaxElement *sym, struct datapartition *dP);
int readSyntaxElement_Level_VLCN(SyntaxElement *sym, int vlc, struct datapartition *dP);
int readSyntaxElement_TotalZeros(SyntaxElement *sym,  DataPartition *dP);
int readSyntaxElement_TotalZerosChromaDC(SyntaxElement *sym,  DataPartition *dP);
int readSyntaxElement_Run(SyntaxElement *sym,  DataPartition *dP);
int GetBits (byte buffer[],int totbitoffset,int *info, int bytecount, 
             int numbits);
int ShowBits (byte buffer[],int totbitoffset,int bytecount, int numbits);


#endif

