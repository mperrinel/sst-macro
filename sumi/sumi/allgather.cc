#include <sumi/allgather.h>
#include <sumi/partner_timeout.h>
#include <sumi/transport.h>
#include <sumi/domain.h>
#include <sprockit/output.h>
#include <cstring>

#define divide_by_2_round_up(x) \
  ((x/2) + (x%2))

#define divide_by_2_round_down(x) \
  (x/2)

using namespace sprockit::dbg;


namespace sumi
{

SpktRegister("bruck_allgather", dag_collective, bruck_allgather_collective);

void
bruck_allgather_actor::init_buffers(void* dst, void* src)
{
  bool in_place = dst == src;
  if (src){
    int block_size = nelems_ * type_size_;
    if (in_place){
      if (dense_me_ != 0){
        int inPlaceOffset = dense_me_* block_size;
        void* inPlaceSrc = ((char*)src + inPlaceOffset);
        std::memcpy(dst, inPlaceSrc, block_size);
      }
    } else {
      //put everything into the dst buffer to begin
      std::memcpy(dst, src, block_size);
    }
    long buffer_size = nelems_ * type_size_ * dom_->nproc();
    send_buffer_ = my_api_->make_public_buffer(dst, buffer_size);
    recv_buffer_ = send_buffer_;
    result_buffer_ = send_buffer_;
  }
}

void
bruck_allgather_actor::finalize_buffers()
{
  if (result_buffer_.ptr){
    long buffer_size = nelems_ * type_size_ * dom_->nproc();
    my_api_->unmake_public_buffer(send_buffer_, buffer_size);
  }
}

void
bruck_allgather_actor::init_dag()
{
  int log2nproc, midpoint, nprocs_extra_round, num_rounds;
  compute_tree(log2nproc, midpoint, num_rounds, nprocs_extra_round);

  debug_printf(sumi_collective,
    "Bruck %s: configured for %d rounds with an extra round exchanging %d proc segments on tag=%d ",
    rank_str().c_str(), log2nproc, nprocs_extra_round, tag_);

  //in the last round, we send half of total data to nearest neighbor
  //in the penultimate round, we send 1/4 data to neighbor at distance=2
  //and so on...

  int partner_gap = 1;
  int round_nelems = nelems_;
  int nproc = dense_nproc_;
  action *prev_send, *prev_recv;
  for (int i=0; i < num_rounds; ++i){
    int send_partner = (dense_me_ + nproc - partner_gap) % nproc;
    int recv_partner = (dense_me_ + partner_gap) % nproc;
    action* send_ac = new send_action(i, send_partner);
    action* recv_ac = new recv_action(i, recv_partner);
    send_ac->offset = 0;
    recv_ac->offset = round_nelems;
    send_ac->nelems = round_nelems;
    recv_ac->nelems = round_nelems;

    if (i == 0){
      add_initial_action(send_ac);
      add_initial_action(recv_ac);
    } else {
      add_dependency(prev_send, send_ac);
      add_dependency(prev_recv, send_ac);
      add_dependency(prev_send, recv_ac);
      add_dependency(prev_recv, recv_ac);
    }

    partner_gap *= 2;
    round_nelems *= 2;
    prev_send = send_ac;
    prev_recv = recv_ac;
  }

  if (nprocs_extra_round){
    int nelems_extra_round = nprocs_extra_round * nelems_;
    int send_partner = (dense_me_ + nproc - partner_gap) % nproc;
    int recv_partner = (dense_me_ + partner_gap) % nproc;
    action* send_ac = new send_action(num_rounds+1,send_partner);
    action* recv_ac = new recv_action(num_rounds+1,recv_partner);
    send_ac->offset = 0;
    recv_ac->offset = round_nelems;
    send_ac->nelems = nelems_extra_round;
    recv_ac->nelems = nelems_extra_round;

    add_dependency(prev_send, send_ac);
    add_dependency(prev_recv, send_ac);
    add_dependency(prev_send, recv_ac);
    add_dependency(prev_recv, recv_ac);
  }
}

void
bruck_allgather_actor::buffer_action(void *dst_buffer, void *msg_buffer, action* ac)
{
  std::memcpy(dst_buffer, msg_buffer, ac->nelems * type_size_);
}

void
bruck_allgather_actor::finalize()
{
  // rank 0 need not reorder
  // or no buffers
  if (dense_me_ == 0 || result_buffer_ == 0){
    return;
  }

  //we need to reorder things a bit
  //first, copy everything out
  int total_nelems = nelems_* dense_nproc_;
  int total_size = total_nelems * type_size_;
  char* tmp = new char[total_size];
  std::memcpy(tmp, result_buffer_, total_size);


  int my_offset = nelems_ * dense_me_;

  int copy_size = (total_nelems - my_offset) * type_size_;
  int copy_offset = my_offset * type_size_;

  void* src = tmp;
  void* dst = ((char*)result_buffer_) + copy_offset;
  std::memcpy(dst, src, copy_size);

  copy_size = my_offset * type_size_;
  copy_offset = (total_nelems - my_offset) * type_size_;
  src = tmp + copy_offset;
  dst = result_buffer_;
  std::memcpy(dst, src, copy_size);

  delete[] tmp;
}


}
