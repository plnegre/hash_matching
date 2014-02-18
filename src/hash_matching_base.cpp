#include "hash_matching_base.h"
#include "stereo_properties.h"
#include "opencv_utils.h"
#include <boost/shared_ptr.hpp>
#include <cv_bridge/cv_bridge.h>
#include <opencv2/opencv.hpp>
#include <iostream>
#include <fstream>
#include <boost/filesystem.hpp>

namespace fs=boost::filesystem;

hash_matching::HashMatchingBase::HashMatchingBase(
  ros::NodeHandle nh, ros::NodeHandle nhp) : nh_(nh), nh_private_(nhp)
{
  // Load parameters
  string ref_path, img_dir, desc_type, files_path;
  double desc_thresh;
  int num_hyperplanes, bucket_width, bucket_height, bucket_max, best_n;
  nh_private_.param("ref_path", ref_path, std::string(""));
  nh_private_.param("img_dir", img_dir, std::string(""));
  nh_private_.param("desc_type", desc_type, std::string("SIFT"));
  nh_private_.getParam("desc_thresh", desc_thresh);
  nh_private_.getParam("best_n", best_n);
  nh_private_.getParam("num_hyperplanes", num_hyperplanes);
  nh_private_.getParam("bucket_width", bucket_width);
  nh_private_.getParam("bucket_height", bucket_height);
  nh_private_.getParam("bucket_max", bucket_max);
  nh_private_.param("files_path", files_path, std::string("/home/user"));

  // Files path sanity check
  if (files_path[files_path.length()-1] != '/')
    files_path += "/";

  // Define image properties
  StereoProperties ref_prop, cur_prop;
  Hash hash_obj;

  // Setup the parameters
  hash_matching::StereoProperties::Params image_params;
  image_params.desc_type = desc_type;
  image_params.bucket_width = bucket_width;
  image_params.bucket_height = bucket_height;
  image_params.bucket_max = bucket_max;
  ref_prop.setParams(image_params);
  cur_prop.setParams(image_params);

  hash_matching::Hash::Params hash_params;
  hash_params.num_hyperplanes = num_hyperplanes;
  hash_obj.setParams(hash_params);

  // Sanity checks
  if (!boost::filesystem::exists(ref_path) || ref_path == "")
  {
    ROS_ERROR_STREAM("[HashMatching:] The reference image file does not exists: " << 
                     ref_path);
    return;
  }
  if (!fs::exists(img_dir) || !fs::is_directory(img_dir)) 
  {
    ROS_ERROR_STREAM("[HashMatching:] The image directory does not exists: " << 
                     img_dir);
    return;
  }

  // Initialize the output
  ostringstream output_csv;

  // Read the template image and extract kp and descriptors
  Mat img_temp = imread(ref_path, CV_LOAD_IMAGE_COLOR);
  ref_prop.setImage(img_temp);
  ROS_INFO_STREAM("Reference Keypoints Size: " << ref_prop.getKp().size());

  // Compute the reference hash
  if(!hash_obj.initialize(ref_prop.getDesc()))
  {
    ROS_ERROR("[HashMatchingBase:] Impossible to initialize the hyperplanes!");
    return;
  }
  vector<double> ref_hash = hash_obj.computeHash(ref_prop.getDesc());

  // Loop directory images
  fs::directory_iterator it(img_dir);
  fs::directory_iterator end;
  vector< pair<double,string> > dists;
  while (it!=end)
  {
    // Check if the directory entry is an directory.
    if (!fs::is_directory(it->status())) 
    {
      // Read image
      string path = img_dir + "/" + it->path().filename().string();
      Mat img_cur = imread(path, CV_LOAD_IMAGE_COLOR);
      cur_prop.setImage(img_cur);

      // Compute the hash
      vector<double> cur_hash;
      ros::WallTime start_time_hash = ros::WallTime::now();
      cur_hash = hash_obj.computeHash(cur_prop.getDesc());
      ros::WallDuration time_elapsed_hash = ros::WallTime::now() - start_time_hash;

      // Crosscheck matching
      vector<DMatch> matches;
      Mat match_mask;
      ros::WallTime start_time_desc = ros::WallTime::now();
      hash_matching::OpencvUtils::crossCheckThresholdMatching(ref_prop.getDesc(), 
                                                              cur_prop.getDesc(), 
                                                              desc_thresh, 
                                                              match_mask, matches);
      ros::WallDuration time_elapsed_desc = ros::WallTime::now() - start_time_desc;
      //ROS_INFO_STREAM("Computation time: " << time_elapsed_hash.toSec() << " | " << time_elapsed_desc.toSec());

      /*
      ROS_INFO_STREAM("--------------------------------------------- " << ref_hash.size() << " | " << cur_hash.size());
      for (uint t=0; t<ref_hash.size(); t++)
        cout << ref_hash[t] << ",";
      cout << endl;
      ROS_INFO("*******");
      for (uint t=0; t<cur_hash.size(); t++)
        cout << cur_hash[t] << ",";
      cout << endl;
      */

      // Compare hashes
      double matching = match(ref_hash, cur_hash);

      if (isfinite(matching))
      {
        // Log
        ROS_INFO_STREAM(it->path().filename().string() << " -> Hash: " <<
          matching << "\t | Desc. Matches: " << (int)matches.size());

        output_csv << matching << "," << matches.size() << endl;

        // Save hash into vector
        dists.push_back(make_pair(matching, it->path().filename().string()));
      }
    }
    // Next directory entry
    it++;
  }

  // Save data into file
  string out_file;
  out_file = files_path + "output.txt";
  fstream f_out(out_file.c_str(), ios::out | ios::trunc);
  f_out << output_csv.str();
  f_out.close();

  // Sort distances vector
  sort(dists.begin(), dists.end(), hash_matching::OpencvUtils::sortByDistance);

  // Show result
  for(int i=0; i<best_n; i++)
    ROS_INFO_STREAM("BEST MATCHING: " << dists[i].second << " (" << dists[i].first << ")");
}

double hash_matching::HashMatchingBase::match(vector<double> hash_1, vector<double> hash_2)
{
  ROS_ASSERT(hash_1.size() == hash_2.size()); // Sanity check

  // Compute the distance
  double sum = 0.0;
  for (uint i=0; i<hash_1.size(); i++)
    sum += fabs(hash_1[i] - hash_2[i]);

  return sum;
}