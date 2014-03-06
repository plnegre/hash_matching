#ifndef HASH_H
#define HASH_H

#include "opencv2/core/core.hpp"
#include "opencv2/features2d/features2d.hpp"
#include "hash_matching_base.h"
#include "opencv_utils.h"
#include <Eigen/Eigen>

using namespace std;
using namespace cv;

namespace hash_matching
{

class Hash
{

public:

  // Class contructor
  Hash();

  struct Params
  {
    //Default constructor sets all values to defaults.
    Params();

    // Class parameters
    int proj_num;                   //!> Number of projections required
    double features_max_value;      //!> maximum value for the feature components; for SIFT is 255.
    double n_levels;                //!> number of levels for the feature component value discretization

    // Default values
    static const int              DEFAULT_PROJ_NUM = 5;
    static const double           DEFAULT_F_MAX_VALUE = 255.0;
    static const double           DEFAULT_n_levels = 128.0;
  };

  // Set the parameter struct
  void setParams(const Params& params);

  // Return current parameters
  inline Params params() const { return params_; }

  // Initialize class
  bool initialize(Mat desc);
  bool initializeHyperplanes(Mat desc, int &region_size);

  // Compute the hash
  vector<uint> getHash1(Mat desc);
  vector<uint> getHash2(Mat desc);
  vector<float> getHash3(Mat desc);

  // Get the number of hyperplanes
  int getHyperplanes();

private:

  // Creates the combination table for hash
  void createCombinations();

  // Recursive function to create the combinations table for hash
  void recursiveCombinations(const vector< vector<string> > &all_vecs, 
                             size_t vec_idx, vector<string> combinations,
                             vector< vector<string> > &result);

  // Compute the hyperplanes
  void computeHyperplanes(Mat desc,
                          vector< vector<float> >& H, 
                          vector<float>& delta);

  // Divide descriptors by region
  vector< vector<int> > computeRegions(Mat desc,
                                       vector< vector<float> > H, 
                                       vector<float> delta);

  // Compute a random vector
  vector<float> compute_random_vector(uint seed, int size);

  // Stores parameters
  Params params_;

  // Properties
  vector<string> comb_;                     //!> Table of possible hash combinations
  vector< vector<float> > H_;               //!> Save the main H
  vector<float> delta_;                     //!> Save the main delta
  vector< vector< vector<float> > > sub_H_; //!> Save the sub-region H
  vector< vector<float> > sub_delta_;       //!> Save the sub-region deltas  
  int num_hyperplanes_;                     //!> Number of hyperplanes
  vector< vector<float> > r_;               //!> Vector of random values
};

} // namespace

#endif // HASH_H