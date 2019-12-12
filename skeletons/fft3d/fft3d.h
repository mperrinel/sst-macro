// Copyright 2009-2019 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2019, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef H_EMBER_FFT_3D
#define H_EMBER_FFT_3D

#include <vector>
#include <inttypes.h>
#include <queue>
#include <mpi.h>
#include <assert.h>
#include <sstream>
#include <iostream>

struct Data {
  int np0;
  int np1;
  int np2;
  int iterations;
  unsigned int nprow;
  unsigned int npcol;
  int np0loc;
  int np0half;
  int np1locf;
  int np1locb;
  int np2loc;
  int ntrans;
  std::vector<int> np0loc_row;
  std::vector<int> np1loc_row;
  std::vector<int> np1loc_col;
  std::vector<int> np2loc_col;
} m_data;

std::vector< uint64_t > m_fwdTime(3,0);
std::vector< uint64_t > m_bwdTime(3, 0);
uint64_t calcFwdFFT1() { return m_fwdTime[0]; }
uint64_t calcFwdFFT2() { return m_fwdTime[1]; }
uint64_t calcFwdFFT3() { return m_fwdTime[2]; }
uint64_t calcBwdFFT1() { return m_bwdTime[0]; }
uint64_t calcBwdFFT2() { return m_bwdTime[1]; }
uint64_t calcBwdFFT3() { return m_bwdTime[2]; }

void initTimes( int numPe, int x, int y, int z, float nsPerElement,
                std::vector<float>& transCostPer );

int32_t m_loopIndex;

uint32_t m_iterations;

uint64_t    m_forwardStart;
uint64_t    m_forwardStop;
uint64_t    m_backwardStop;
uint64_t    m_forwardTotal;
uint64_t    m_backwardTotal;

std::vector< uint32_t >  m_sendCnts;
std::vector< uint32_t >  m_recvCnts;
std::vector< uint32_t >  m_sendDsp;
std::vector< uint32_t >  m_recvDsp;

MPI_Comm comm;
MPI_Comm m_rowComm;
MPI_Comm m_colComm;
MPI_Group group_world;
MPI_Group m_rowGroup;
MPI_Group m_colGroup;
int comm_size;
int comm_rank;

std::vector<int>    m_rowGrpRanks;
std::vector<int>    m_colGrpRanks;

std::vector<int>    m_rowSendCnts;
std::vector<int>    m_rowSendDsp;
std::vector<int>    m_rowRecvCnts;
std::vector<int>    m_rowRecvDsp;

std::vector<int>    m_colSendCnts_f;
std::vector<int>    m_colSendDsp_f;
std::vector<int>    m_colRecvCnts_f;
std::vector<int>    m_colRecvDsp_f;

std::vector<int>    m_colSendCnts_b;
std::vector<int>    m_colSendDsp_b;
std::vector<int>    m_colRecvCnts_b;
std::vector<int>    m_colRecvDsp_b;
void*               g_m_sendBuf;
void*               g_m_recvBuf;
float              m_nsPerElement;
std::vector<float> m_transCostPer(6);
int buffsize;

int parseArgs(int, char**, Data&);
void init(Data&);
void configure();
void run();
#endif
