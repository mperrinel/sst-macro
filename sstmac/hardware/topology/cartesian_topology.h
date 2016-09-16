#ifndef CARTESIAN_TOPOLOGY_H
#define CARTESIAN_TOPOLOGY_H

#include <sstmac/hardware/topology/structured_topology.h>

namespace sstmac {
namespace hw {

/**
 * @brief The cartesian_topology class
 * Encapsulates a topology like torus that can be naturally mapped onto
 * an n-dimensional Cartesian (integer) coordinate system
 */
class cartesian_topology :
  public structured_topology
{
 public:
  /**
   * @brief configure_geometric_paths
   * For all possible geometric or structure paths, compute
   * their redundances in terms of number of ports that go
   * in the same geometric or structural direction, e.g.
   * might be 3 ports that all go +X on a router
   * @param [inout] redundancies
   */
  virtual void
  configure_geometric_paths(std::vector<int>& redundancies) = 0;

  node_id
  node_addr(const coordinates& coords) const;

  virtual switch_id
  switch_addr(const coordinates& coords) const = 0;

  coordinates
  node_coords(node_id nid) const;

  virtual coordinates
  switch_coords(switch_id) const = 0;

  int
  ndimensions() const {
    return dimensions_.size();
  }

  cartesian_topology*
  cart_topology() const override {
    return const_cast<cartesian_topology*>(this);
  }

  std::string
  label(switch_id sid) const override;

  std::string
  label(node_id nid) const override;

 protected:
  cartesian_topology(sprockit::sim_parameters* params,
                     InitMaxPortsIntra i1,
                     InitGeomEjectID i2);

  /**
   * The number of redundant links (ports) comprising a geometric
   * or structure direction in the topology
   */
  std::vector<int> red_;
  std::vector<int> dimensions_;

};

}
}

#endif // CARTESIAN_TOPOLOGY_H
