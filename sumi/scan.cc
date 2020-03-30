/**
Copyright 2009-2020 National Technology and Engineering Solutions of Sandia, 
LLC (NTESS).  Under the terms of Contract DE-NA-0003525, the U.S.  Government 
retains certain rights in this software.

Sandia National Laboratories is a multimission laboratory managed and operated
by National Technology and Engineering Solutions of Sandia, LLC., a wholly 
owned subsidiary of Honeywell International, Inc., for the U.S. Department of 
Energy's National Nuclear Security Administration under contract DE-NA0003525.

Copyright (c) 2009-2020, NTESS

All rights reserved.

Redistribution and use in source and binary forms, with or without modification, 
are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above
      copyright notice, this list of conditions and the following
      disclaimer in the documentation and/or other materials provided
      with the distribution.

    * Neither the name of the copyright holder nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

Questions? Contact sst-macro-help@sandia.gov
*/

#include <sumi/scan.h>
#include <sumi/transport.h>
#include <sumi/communicator.h>
#include <sprockit/output.h>
#include <sprockit/stl_string.h>
#include <cstring>


using namespace sprockit::dbg;

namespace sumi {


void
SimultaneousBtreeScanActor::finalizeBuffers()
{
  //nothing to do
  if (!result_buffer_) return;

  int size = nelems_ * type_size_;
  my_api_->unmakePublicBuffer(result_buffer_, size);
  my_api_->freePublicBuffer(send_buffer_, size);
  my_api_->freePublicBuffer(recv_buffer_, size);
}

void
SimultaneousBtreeScanActor::initBuffers()
{
  void* dst = result_buffer_;
  void* src = send_buffer_;
  if (!src) return;

  //if we need to do operations, then we need a temp buffer for doing sends
  int size = nelems_ * type_size_;

  result_buffer_ = my_api_->makePublicBuffer(dst, size);

  //but! we need a temporary send and recv buffers
  recv_buffer_ = my_api_->allocatePublicBuffer(size);
  send_buffer_ = my_api_->allocatePublicBuffer(size);

  //and put the initial set of values in
  std::memcpy(send_buffer_, src, size);
  std::memcpy(dst, src, size);
}

void
SimultaneousBtreeScanActor::initDag()
{
  slicer_->fxn = fxn_;

  int log2nproc, midpoint, virtual_nproc;
  RecursiveDoubling::computeTree(dom_nproc_, log2nproc, midpoint, virtual_nproc);

  int nproc = dom_nproc_;
  int me = dom_me_;
  int gap = 1;
  int send_partner = me + gap;
  int recv_partner = me - gap;
  Action *prev_send = nullptr, *prev_recv = nullptr, *prev_memcpy = nullptr;
  int rnd = 0;
  bool valid_send = send_partner < nproc;
  bool valid_recv = recv_partner >= 0;

  while (valid_send || valid_recv){
    Action* send_ac = nullptr, *recv_ac = nullptr, *memcpy_ac = nullptr;
    if (valid_send){
      send_ac = new SendAction(rnd, send_partner, SendAction::temp_send);
      send_ac->offset = 0; //we send the full amount every time
      send_ac->nelems = nelems_;
      addDependency(prev_send, send_ac);
      addDependency(prev_recv, send_ac);
      addDependency(prev_memcpy, send_ac);
    }
    if (valid_recv){
      recv_ac = new RecvAction(rnd, recv_partner, RecvAction::reduce);
      recv_ac->offset = 0;
      recv_ac->nelems = nelems_;
      addDependency(prev_send, recv_ac);
      addDependency(prev_recv, recv_ac);
      addDependency(prev_memcpy, recv_ac);
    }
    gap *= 2;
    send_partner = me + gap;
    recv_partner = me - gap;
    valid_send = send_partner < nproc;
    valid_recv = recv_partner >= 0;
    if (valid_send && recv_buffer_){
      //we need to memcpy the result buffer into the send buffer for the next round
      memcpy_ac = new ShuffleAction(rnd, 0);
      addDependency(send_ac, memcpy_ac);
      addDependency(recv_ac, memcpy_ac);
    }
    ++rnd;
    prev_send = send_ac;
    prev_recv = recv_ac;
    prev_memcpy = memcpy_ac;
  }
}

void
SimultaneousBtreeScanActor::bufferAction(void *dst_buffer, void *msg_buffer, Action* ac)
{
  (fxn_)(dst_buffer, msg_buffer, ac->nelems);
}

void
SimultaneousBtreeScanActor::startShuffle(Action * /*ac*/)
{
  int size = type_size_ * nelems_;
  ::memcpy(send_buffer_, result_buffer_, size);
}

}
