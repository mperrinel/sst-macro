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

#include "emberfft3d.h"

// TODO: parse sst arguments to fill a data struct
void init(Data& m_data) {

  // from static initializer
  m_loopIndex = -1;
  m_forwardStart = 0;
  m_forwardStop = 0;
  m_backwardStop = 0;
  m_forwardTotal = 0;
  m_backwardTotal = 0;

  m_data.np0 = 100;
  m_data.np1 = 100;
  m_data.np2 = 100;
  m_data.nprow = 1;

  m_iterations = 1;

  m_nsPerElement = 1;

  m_transCostPer[0] = 1;
  m_transCostPer[1] = 1;
  m_transCostPer[2] = 1;
  m_transCostPer[3] = 1;
  m_transCostPer[4] = 1;
  m_transCostPer[5] = 1;

  MPI_Init(nullptr, nullptr);
  comm = MPI_COMM_WORLD;
  MPI_Comm_rank(comm, &comm_rank);
  MPI_Comm_size(comm, &comm_size);
  MPI_Comm_group(comm, &group_world);

  configure();
}

void configure()
{
    m_data.npcol = comm_size / m_data.nprow;

    //assert( 0 == (comm_size % m_data.nprow) );

    unsigned myRow = comm_rank % m_data.nprow;
    unsigned myCol = comm_rank / m_data.nprow;
//    verbose(CALL_INFO, 2, 0, "%d: nx=%d ny=%d nx=%d nprow=%d "
//        "npcol=%d myRow=%d myCol=%d\n",
//            comm_rank, m_data.np0, m_data.np1, m_data.np2,
//                m_data.nprow, m_data.npcol, myRow, myCol );

    initTimes( comm_size, m_data.np0, m_data.np1, m_data.np2,
                    m_nsPerElement, m_transCostPer );

    m_rowGrpRanks.resize( m_data.npcol );
    m_colGrpRanks.resize( m_data.nprow );

    std::ostringstream tmp; 

    for ( unsigned int i = 0; i < m_rowGrpRanks.size(); i++ ) {
        m_rowGrpRanks[i] = myRow + i * m_data.nprow;
        tmp << m_rowGrpRanks[i] << " " ;
    }
//    verbose(CALL_INFO, 2, 0,"row grp [%s]\n", tmp.str().c_str() );
    tmp.str("");
    tmp.clear();
    for ( unsigned int i = 0; i < m_colGrpRanks.size(); i++ ) {
        m_colGrpRanks[i] = myCol * m_data.nprow + i;
        tmp << m_colGrpRanks[i] << " " ;
    }
//    verbose(CALL_INFO, 2, 0,"col grp [%s]\n", tmp.str().c_str() );

    m_data.np0loc_row.resize(m_data.nprow);
    m_data.np1loc_row.resize(m_data.nprow);
    m_data.np1loc_col.resize(m_data.npcol);
    m_data.np2loc_col.resize(m_data.npcol);

    {
        const int np0half = m_data.np0/2 + 1;
        m_data.np0half = np0half;
        const int np0loc = np0half/m_data.nprow;

        for ( unsigned int i = 0; i < m_data.nprow; i++ ) {
            m_data.np0loc_row[i] = np0loc; 
        }
        for ( unsigned int i = 0; i < np0half % m_data.nprow; ++i ) {
            ++m_data.np0loc_row[i];
        }
        m_data.np0loc = m_data.np0loc_row[ myRow ];
    }

    {
        const int np1loc = m_data.np1/m_data.nprow;
        for ( unsigned int i = 0; i < m_data.nprow; i++ ) {
            m_data.np1loc_row[i] = np1loc; 
        }
        for ( unsigned int i = 0; i < m_data.np1 % m_data.nprow; i++ ) {
            ++m_data.np1loc_row[i];
        }
        m_data.np1locf = m_data.np1loc_row[myRow];
    }

    {
        const int np1loc = m_data.np1/m_data.npcol;
        for ( unsigned int i=0; i < m_data.npcol; i++ ) {
            m_data.np1loc_col[i] = np1loc;
        }
        for ( unsigned int i = 0; i < m_data.np1 % m_data.npcol; i++ ) {
            ++m_data.np1loc_col[i];
        }
        m_data.np1locb = m_data.np1loc_col[myCol];
    }
    
    {
        const int np2loc = m_data.np2/m_data.npcol;
        for ( unsigned int i=0; i < m_data.npcol; i++ ) {
            m_data.np2loc_col[i] = np2loc;
        }
        for ( unsigned int i=0; i < m_data.np2 % m_data.npcol; i++ ) {
            ++m_data.np2loc_col[i];
        }
        m_data.np2loc = m_data.np2loc_col[myCol];
    }
    m_data.ntrans = m_data.np1locf * m_data.np2loc;

//    verbose(CALL_INFO, 2, 0, "np0half=%d np0loc=%d np1locf_=%d "
//            "np1locb_%d np2loc_=%d\n",
//        m_data.np0half, m_data.np0loc, m_data.np1locf, m_data.np1locb, m_data.np2loc );

    m_rowSendCnts.resize(m_data.npcol);
    m_rowSendDsp.resize(m_data.npcol);
    m_rowRecvCnts.resize(m_data.npcol);
    m_rowRecvDsp.resize(m_data.npcol);

    int soffset = 0, roffset = 0;
    for ( unsigned i = 0; i < m_data.npcol; i++ ) {

        int sendblk = 2*m_data.np0loc * m_data.np1loc_col[i] *m_data.np2loc;
        int recvblk = 2*m_data.np0loc *m_data.np1locb * m_data.np2loc_col[i];

//        verbose(CALL_INFO, 2, 0,"row, sendblk=%d soffset=%d "
//                "recvblk=%d roffset=%d\n",sendblk,soffset,recvblk,roffset);
        m_rowSendCnts[i] = sendblk;
        m_rowRecvCnts[i] = recvblk;
        m_rowSendDsp[i] = soffset; 
        m_rowRecvDsp[i] = roffset;
        soffset += sendblk;
        roffset += recvblk;
    }

    m_colSendCnts_f.resize( m_data.nprow );
    m_colSendDsp_f.resize( m_data.nprow );
    m_colRecvCnts_f.resize( m_data.nprow );
    m_colRecvDsp_f.resize( m_data.nprow );

    soffset = roffset = 0;
    for ( unsigned i = 0; i < m_data.nprow; i++ ) {

        int sendblk = 2 * m_data.np0loc_row[i] *m_data.np1locf *m_data.np2loc;
        int recvblk = 2 *m_data.np0loc * m_data.np1loc_row[i] *m_data.np2loc;
//        verbose(CALL_INFO, 2, 0,"col_f, sendblk=%d soffset=%d "
//                "recvblk=%d roffset=%d\n",sendblk,soffset,recvblk,roffset);

        m_colSendCnts_f[i] = sendblk;
        m_colRecvCnts_f[i] = recvblk; 
        m_colSendDsp_f[i] = soffset; 
        m_colRecvDsp_f[i] = roffset;
        soffset += sendblk;
        roffset += recvblk;
    }

    m_colSendCnts_b.resize( m_data.nprow );
    m_colSendDsp_b.resize( m_data.nprow );
    m_colRecvCnts_b.resize( m_data.nprow );
    m_colRecvDsp_b.resize( m_data.nprow );

    soffset = roffset = 0;
    for ( unsigned i = 0; i < m_data.nprow; i++ ) {

        int sendblk = 2 *m_data.np0loc * m_data.np1loc_row[i] *m_data.np2loc;
        int recvblk = 2 * m_data.np0loc_row[i] *m_data.np1locf *m_data.np2loc;

//        verbose(CALL_INFO, 2, 0,"col_b, sendblk=%d soffset=%d "
//                "recvblk=%d roffset=%d\n",sendblk,soffset,recvblk,roffset);

        m_colSendCnts_b[i] = sendblk;
        m_colRecvCnts_b[i] = recvblk; 
        m_colSendDsp_b[i] = soffset; 
        m_colRecvDsp_b[i] = roffset;
        soffset += sendblk;
        roffset += recvblk;
    }

    int size1 = m_data.np0 *m_data.np1locf *m_data.np2loc;
    int size2 = m_data.np1 * m_data.np0 *m_data.np2loc  / m_data.nprow;
    int size3 = m_data.np2 *m_data.np1locb * m_data.np0 / m_data.nprow;

    int maxsize = (size1 > size2 ? size1 : size2);
    maxsize = (maxsize > size3 ? maxsize : size3);
    // TODO: better way to allocate?
    m_sendBuf =   malloc( maxsize * sizeof(MPI_COMPLEX));
    m_recvBuf =   malloc( maxsize * sizeof(MPI_COMPLEX));
//    verbose(CALL_INFO, 2, 0,"maxsize=%d\n",maxsize);

//    verbose(CALL_INFO, 2, 0,"np0=%d np1=%d np2=%d nprow=%d npcol=%d\n",
//                m_data.np0, m_data.np1, m_data.np2, m_data.nprow, m_data.npcol );
//    verbose(CALL_INFO, 2, 0,"np0half=%d np1loc=%d np1locf=%d no1locb=%d np2loc=%d\n",
//        m_data.np0half, m_data.np0loc, m_data.np1locf, m_data.np1locb, m_data.np2loc );

}

void initTimes( int numPe, int x, int y, int z, float nsPerElement,
                std::vector<float>& transCostPer )
{

    double cost = nsPerElement * x * ((y * z)/numPe); 
//	if ( 0 == comm_rank ) {
//	   	output("%s: nsPerElement=%.5f %.2f %.2f %.2f"
//			" %.2f %.2f %.2f\n", getMotifName().c_str(), nsPerElement,
//			transCostPer[0], transCostPer[1], transCostPer[2],
//			transCostPer[3], transCostPer[4], transCostPer[5]);
//	}
    //assert( cost > 0.0 );
    m_fwdTime[0] =  m_fwdTime[1] = m_fwdTime[2] = cost; 
    m_bwdTime[0] =  m_bwdTime[1] = m_bwdTime[2] = cost; 
    m_bwdTime[2] *= 2;

    m_fwdTime[0] *= transCostPer[0];  
    m_fwdTime[1] *= transCostPer[1];  
    m_fwdTime[2] *= transCostPer[2];  

    m_bwdTime[0] *= transCostPer[3];  
    m_bwdTime[1] *= transCostPer[4];  
    m_bwdTime[2] *= transCostPer[5];  
}

bool generate()
{
//    verbose(CALL_INFO, 1, 0, "loop=%d\n", m_loopIndex );

    m_forwardTotal += (m_forwardStop - m_forwardStart);
    m_backwardTotal += (m_backwardStop - m_forwardStop);

    if (  m_loopIndex == (signed) m_iterations ) {
        if ( 0 == comm_rank ) {
            printf(": nRanks=%d fwd time %f sec\n", comm_size,
                ((double) m_forwardTotal / 1000000000.0) / m_iterations );
            printf(": rRanks=%d bwd time %f sec\n", comm_size,
                ((double) m_backwardTotal / 1000000000.0) / m_iterations );
        }
        return true;
    }

    if (  m_loopIndex < 0 ) {
      //TODO: Does this replacement work?
      MPI_Group_incl(group_world, static_cast<int>(m_rowGrpRanks.size()), m_rowGrpRanks.data(), &m_rowGroup);
      MPI_Comm_create(comm, m_rowGroup, &m_rowComm);

      MPI_Group_incl(group_world, static_cast<int>(m_colGrpRanks.size()), m_colGrpRanks.data(), &m_colGroup);
      MPI_Comm_create(comm, m_rowGroup, &m_colComm);

//        enQ_commCreate( evQ, GroupWorld, m_rowGrpRanks, &m_rowComm );
//        enQ_commCreate( evQ, GroupWorld, m_colGrpRanks, &m_colComm );
        ++m_loopIndex;
        return false;
    }

    // TODO: replace with a compute directive
    //enQ_compute( evQ, (uint64_t) ((double) calcFwdFFT1() ) );

    MPI_Alltoallv(m_sendBuf, &m_colSendCnts_f[0], &m_colSendDsp_f[0], MPI_DOUBLE,
        m_recvBuf, &m_colRecvCnts_f[0], &m_colRecvDsp_f[0], MPI_DOUBLE,
        m_colComm );
  
//    enQ_compute( evQ, (uint64_t) ((double) calcFwdFFT2() ) );
    MPI_Alltoallv(m_sendBuf, &m_rowSendCnts[0], &m_rowSendDsp[0], MPI_DOUBLE,
        m_recvBuf, &m_rowRecvCnts[0], &m_rowRecvDsp[0], MPI_DOUBLE,
        m_rowComm );

//TODO: replace with a compute directive
//    enQ_compute( evQ, (uint64_t) ((double) calcFwdFFT3()  ) );
    
    MPI_Barrier(comm);

    //TODO: replace with a compute directive
//    enQ_compute( evQ, (uint64_t) ((double) calcBwdFFT1() ) );

    MPI_Alltoallv( m_sendBuf, &m_rowSendCnts[0], &m_rowSendDsp[0], MPI_DOUBLE,
                   m_recvBuf, &m_rowRecvCnts[0], &m_rowRecvDsp[0], MPI_DOUBLE,
                   m_rowComm );
    
//TODO: replace with a compute directive
//    enQ_compute( evQ, (uint64_t) ((double) calcBwdFFT2() ) );

    MPI_Alltoallv(m_sendBuf, &m_colSendCnts_b[0], &m_colSendDsp_b[0], MPI_DOUBLE,
        m_recvBuf, &m_colRecvCnts_b[0], &m_colRecvDsp_b[0], MPI_DOUBLE,
        m_colComm );

    //TODO: replace with a compute directive
//    enQ_compute( evQ, (uint64_t) ((double) calcBwdFFT3() ) );

    MPI_Barrier(comm);
//    enQ_barrier( evQ, GroupWorld );
//    enQ_getTime( evQ, &m_backwardStop );

    if ( ++m_loopIndex == (signed) m_iterations ) {
      MPI_Comm_free(&m_rowComm);
      MPI_Comm_free(&m_colComm);
//        enQ_commDestroy( evQ, m_rowComm );
//        enQ_commDestroy( evQ, m_colComm );
    }

    return false;
}

int main(int argc, char** argv)
{
  init(m_data);
  generate();
  MPI_Finalize();
  return 0;
}
