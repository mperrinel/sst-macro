#include <sprockit/keyword_registration.h>

namespace sstmac {
  static const char* main_sstmac_valid_keywords[] = {
  "id",
  "negligible_size",
  "name",
  "model",
  "mtu",
  "nsockets",
  "nworkers",
  "switch_name",
  "pipeline_speedup",
  "type",
  "epoch",
  "bin_size",
  "frequency",
  "latency",
  "bandwidth",
  "arbitrator",
  "mpi_max_num_requests",
  "debug_startup",  
  "debug",  
  "congestion_model",  
  "event_manager",  
  "event_calendar_max_time",  
  "event_calendar_epoch_length",  
  "event_calendar_search_window",  
  "timestamp_resolution",  
  "network_name",  
  "param_name",  
  "network_name",  
  "topology_name",  
  "sst_nthread",  
  "serialization_buffer_size",  
  "serialization_num_bufs_allocation",  
  "partition",  
  "network_name",  
  "interconnect",
  "runtime",
  "stop_time",  
  "sst_nproc",
  "sst_nthread",  
  "nic_name",
  "router_seed",  
  "router",
  "radix",
  "ugal_threshold",
  "network_spyplot",    
  "compute_scheduler", 
  "pisces_injection_latency",  
  "pisces_injection_bandwidth",        
  "node_cores",  
  "node_sockets",  
  "node_pipeline_speedup",
  "parallelism",
  "node_frequency",    
  "ping_timeout",  
  "activity_monitor",  
  "sumi_eager_cutoff",
  "immediate_nack",  
  "timestamp_resolution",  
  "env",   
  "num_messages",   
  "host_compute_modeling",  
  "launch_coordinate_file",   
  "coordinate_file",
  "launch_hostname_map",
  "hostname_map",
  "random_allocation_seed",
  "launch_hostname_list",  
  "hostname_list",
  "random_indexer_seed",
  };
  static int main_sstmac_num_valid_keywords = sizeof(main_sstmac_valid_keywords) / sizeof(const char*);
sprockit::StaticKeywordRegister _main_static_keyword_init_(main_sstmac_num_valid_keywords, main_sstmac_valid_keywords);
}
