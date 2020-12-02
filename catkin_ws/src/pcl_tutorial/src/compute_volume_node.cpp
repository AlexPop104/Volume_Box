#include <pcl/ModelCoefficients.h>
#include <pcl/point_types.h>
#include <pcl/io/pcd_io.h>
#include <pcl/filters/extract_indices.h>
#include <pcl/filters/voxel_grid.h>
#include <pcl/features/normal_3d.h>
#include <pcl/kdtree/kdtree.h>
#include <pcl/sample_consensus/method_types.h>
#include <pcl/sample_consensus/model_types.h>
#include <pcl/segmentation/sac_segmentation.h>
#include <pcl/segmentation/extract_clusters.h>
#include <pcl/filters/project_inliers.h>

#include <ros/ros.h>
// PCL specific includes
#include <sensor_msgs/PointCloud2.h>
#include <pcl_conversions/pcl_conversions.h>
#include <pcl/point_cloud.h>
#include <pcl/point_types.h>
#include <visualization_msgs/Marker.h>

#include<pcl_tutorial/compute_volume_nodeConfig.h>
#include <dynamic_reconfigure/server.h>

class ComputeVolumeNode
{
public:
  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

  ComputeVolumeNode()
  {

    bool ok2;

    pcl::PointCloud<pcl::PointXYZ>::Ptr cloud(new pcl::PointCloud<pcl::PointXYZ>);
    pcl::PointCloud<pcl::PointXYZ>::Ptr cloud_f(new pcl::PointCloud<pcl::PointXYZ>);
    pcl::PointCloud<pcl::PointXYZ>::Ptr cloud_filtered(new pcl::PointCloud<pcl::PointXYZ>);

    pcl::PointCloud<pcl::PointXYZ>::Ptr cloud_final(new pcl::PointCloud<pcl::PointXYZ>);
    pcl::PointCloud<pcl::PointXYZ>::Ptr cloud_linii(new pcl::PointCloud<pcl::PointXYZ>);
    pcl::PointCloud<pcl::PointXYZ>::Ptr cloud_proiectii(new pcl::PointCloud<pcl::PointXYZ>);

    pcl::SACSegmentation<pcl::PointXYZ> seg;
    pcl::PointIndices::Ptr inliers(new pcl::PointIndices);
    pcl::ModelCoefficients::Ptr coefficients(new pcl::ModelCoefficients);
    pcl::PointCloud<pcl::PointXYZ>::Ptr cloud_plane(new pcl::PointCloud<pcl::PointXYZ>());

    pcl::PointCloud<pcl::PointXYZ> all_lines[4][4];
    pcl::PointCloud<pcl::PointXYZ>::Ptr all_planes[4];
    pcl::PointCloud<pcl::PointXYZ>::Ptr all_projected_lines[4][4];

    float Coeficients[3][4];

    float Volum = 1;

    pub1_ = nh_.advertise<sensor_msgs::PointCloud2>("/output_plan", 1);
    pub2_ = nh_.advertise<sensor_msgs::PointCloud2>("/output_proiectii", 1);
    



    sub_ = nh_.subscribe("/pf_out", 1, &ComputeVolumeNode::cloudCallback, this);

    config_server_.setCallback(boost::bind(&ComputeVolumeNode::dynReconfCallback, this, _1, _2));

    vis_pub = nh_.advertise<visualization_msgs::Marker>("/Volum_final", 0);
    vis2_pub = nh_.advertise<visualization_msgs::Marker>("/Nr_of_planes", 0);

    
  }

  ~ComputeVolumeNode() {}

  void planar_segmenting_single_time(pcl::PointCloud<pcl::PointXYZ>::Ptr cloud,
                                     pcl::PointCloud<pcl::PointXYZ>::Ptr cloud_segmented,
                                     pcl::ModelCoefficients::Ptr coefficients)
  {
    pcl::PointIndices::Ptr inliers(new pcl::PointIndices);

    pcl::PointCloud<pcl::PointXYZ>::Ptr outliers(new pcl::PointCloud<pcl::PointXYZ>);
    pcl::PointCloud<pcl::PointXYZ>::Ptr outliers_segmented(new pcl::PointCloud<pcl::PointXYZ>);
    pcl::SACSegmentation<pcl::PointXYZ> seg;

    // Optional
    seg.setOptimizeCoefficients(true);
    // Mandatory
    seg.setModelType(pcl::SACMODEL_PLANE);
    seg.setMethodType(pcl::SAC_RANSAC);
    seg.setDistanceThreshold(0.01);

    // Segment dominant plane
    seg.setInputCloud(cloud);
    seg.segment(*inliers, *coefficients);
    pcl::copyPointCloud<pcl::PointXYZ>(*cloud, *inliers, *cloud_segmented);

    if (inliers->indices.size() == 0)
    {

      PCL_ERROR("Could not estimate a planar model for the given dataset.");
      ok2 = 0;
    }
  }



  void euclidean_segmenting(pcl::PointCloud<pcl::PointXYZ>::Ptr cloud,
                            pcl::PointCloud<pcl::PointXYZ>::Ptr cloud_f,
                            bool &ok2)
  {

    // Create the filtering object: downsample the dataset using a leaf size of 1cm
    pcl::VoxelGrid<pcl::PointXYZ> vg;
    pcl::SACSegmentation<pcl::PointXYZ> seg;

    pcl::PointCloud<pcl::PointXYZ>::Ptr cloud_filtered(new pcl::PointCloud<pcl::PointXYZ>);
    pcl::PointIndices::Ptr inliers(new pcl::PointIndices);
    pcl::ModelCoefficients::Ptr coefficients(new pcl::ModelCoefficients);
    pcl::PointCloud<pcl::PointXYZ>::Ptr cloud_plane(new pcl::PointCloud<pcl::PointXYZ>());

    vg.setInputCloud(cloud);
    vg.setLeafSize(0.01f, 0.01f, 0.01f);
    vg.filter(*cloud_filtered);
    // std::cout << "PointCloud after filtering has: " << cloud_filtered->points.size() << " data points." << std::endl; //*

    // Create the segmentation object for the planar model and set all the parameters

    seg.setOptimizeCoefficients(true);
    seg.setModelType(pcl::SACMODEL_PLANE);
    seg.setMethodType(pcl::SAC_RANSAC);
    seg.setMaxIterations(100);
    seg.setDistanceThreshold(0.02);

    int i = 0, nr_points = (int)cloud_filtered->points.size();

    // Segment the largest planar component from the remaining cloud
    seg.setInputCloud(cloud_filtered);
    seg.segment(*inliers, *coefficients);
    if (inliers->indices.size() == 0)
    {
      std::cout << "Could not estimate a planar model for the given dataset." << std::endl;
      //////
      ///////////
      //////////
      // TREBUIE FUNCTIE DE OPRIRE
      /////////////
    }

    // Extract the planar inliers from the input cloud
    pcl::ExtractIndices<pcl::PointXYZ> extract;
    extract.setInputCloud(cloud_filtered);
    extract.setIndices(inliers);
    extract.setNegative(false);

    // Write the planar inliers to disk
    extract.filter(*cloud_plane);
    //std::cout << "PointCloud representing the planar component: " << cloud_plane->points.size() << " data points." << std::endl;

    // Remove the planar inliers, extract the rest
    extract.setNegative(true);
    extract.filter(*cloud_f);
    *cloud_filtered = *cloud_f; //     HERE IS THE CLOUD FILTERED EXTRACTED

    if (cloud_filtered->size() != 0)
    {
      // Creating the KdTree object for the search method of the extraction
      pcl::search::KdTree<pcl::PointXYZ>::Ptr tree(new pcl::search::KdTree<pcl::PointXYZ>);
      tree->setInputCloud(cloud_filtered);

      std::vector<pcl::PointIndices> cluster_indices;
      pcl::EuclideanClusterExtraction<pcl::PointXYZ> ec;
      ec.setClusterTolerance(0.02); // 2cm
      ec.setMinClusterSize(100);
      ec.setMaxClusterSize(25000);
      ec.setSearchMethod(tree);
      ec.setInputCloud(cloud_filtered);
      ec.extract(cluster_indices);

      for (std::vector<pcl::PointIndices>::const_iterator it = cluster_indices.begin(); it != cluster_indices.end(); ++it)
      {
        pcl::PointCloud<pcl::PointXYZ>::Ptr cloud_cluster(new pcl::PointCloud<pcl::PointXYZ>);
        for (std::vector<int>::const_iterator pit = it->indices.begin(); pit != it->indices.end(); pit++)
          cloud_cluster->points.push_back(cloud_filtered->points[*pit]); //*
        cloud_cluster->width = cloud_cluster->points.size();
        cloud_cluster->height = 1;
        cloud_cluster->is_dense = true;

        cloud = cloud_cluster;
        ok2 = 1;
      }

      ////////////////////////////////////
    }
    else
    {
      ok2 = 0;
    }
  }

   void planar_segmenting(pcl::PointCloud<pcl::PointXYZ>::Ptr cloud,
                         float Coeficients[3][4],
                         pcl::PointCloud<pcl::PointXYZ>::Ptr all_planes[4],
                         pcl::PointCloud<pcl::PointXYZ>::Ptr cloud_final,
                         int t,
                         bool &ok2)
  {
    pcl::PointIndices::Ptr inliers(new pcl::PointIndices);
    pcl::ModelCoefficients::Ptr coefficients(new pcl::ModelCoefficients);
    pcl::PointCloud<pcl::PointXYZ>::Ptr cloud_segmented(new pcl::PointCloud<pcl::PointXYZ>);
    pcl::PointCloud<pcl::PointXYZ>::Ptr outliers(new pcl::PointCloud<pcl::PointXYZ>);
    pcl::PointCloud<pcl::PointXYZ>::Ptr outliers_segmented(new pcl::PointCloud<pcl::PointXYZ>);
    pcl::SACSegmentation<pcl::PointXYZ> seg;

    // Optional
    seg.setOptimizeCoefficients(true);
    // Mandatory
    seg.setModelType(pcl::SACMODEL_PLANE);
    seg.setMethodType(pcl::SAC_RANSAC);
    seg.setDistanceThreshold(0.01);

    // Segment dominant plane
    seg.setInputCloud(cloud);
    seg.segment(*inliers, *coefficients);
    pcl::copyPointCloud<pcl::PointXYZ>(*cloud, *inliers, *cloud_segmented);

    if (inliers->indices.size() == 0)
    {

      PCL_ERROR("Could not estimate a planar model for the given dataset.");
      ok2 = 0;
    }

    else
    {

      Coeficients[t - 1][0] = coefficients->values[0];
      Coeficients[t - 1][1] = coefficients->values[1];
      Coeficients[t - 1][2] = coefficients->values[2];
      Coeficients[t - 1][3] = coefficients->values[3];

      *cloud_final += *cloud_segmented;

      all_planes[t] = cloud_segmented;

      ok2 = 1;
    }
  }

  void compute_all(pcl::PointCloud<pcl::PointXYZ>::Ptr cloud,
                   pcl::PointCloud<pcl::PointXYZ>::Ptr cloud_floor,
                   pcl::PointCloud<pcl::PointXYZ>::Ptr cloud_final,
                   pcl::PointCloud<pcl::PointXYZ>::Ptr cloud_proiectii,
                   pcl::PointCloud<pcl::PointXYZ>::Ptr cloud_linii,
                   float &Volum, int &p)
  {

    pcl::PointCloud<pcl::PointXYZ>::Ptr cloud_f(new pcl::PointCloud<pcl::PointXYZ>);

    float Coeficients[3][4];

    pcl::PointCloud<pcl::PointXYZ> all_lines[4][4];
    pcl::PointCloud<pcl::PointXYZ>::Ptr all_planes[4];
    pcl::PointCloud<pcl::PointXYZ>::Ptr all_projected_lines[4][4];

    pcl::ModelCoefficients::Ptr coefficients_floor(new pcl::ModelCoefficients);

    Eigen::Vector3f normal_floor;

    bool ok = 1;

    bool ok2 = 1;

    pcl::PCDWriter writer;

    if (cloud->size() < nivel_initial)
    {
      ok = 0;
      p = 0; // NO PLANE
    }
    else
    {
      planar_segmenting_single_time(cloud, cloud_floor, coefficients_floor);

      normal_floor << coefficients_floor->values[0], coefficients_floor->values[1], coefficients_floor->values[2];
      if (coefficients_floor->values[3] < 0)
      {
        normal_floor *= -1;
      }
      p=1;
    }

    for (int t = 1; (t < 4) && ok; t++)
    {

      euclidean_segmenting(cloud, cloud_f, ok2);

      if (cloud_f->size() == 0)
      {
        ok = 0; //No more planes to cut out
      }

      if (ok2 != 0)
      {
        planar_segmenting(cloud_f, Coeficients, all_planes, cloud_final, t, ok2); //cloud_f is global, so the modifications stay

        cloud = cloud_f; // Cloud is now the extracted pointcloud

        if (cloud->size() < nivel_initial/3)
        {
          ok = 0;
          if (t == 1)
          {
            p = 1; // Only the Ground Plane
          }
        }
       
          p = t + 1;
        
        
      }
    }



  }
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
  void
  dynReconfCallback(pcl_tutorial::compute_volume_nodeConfig &config, uint32_t level)
  {
     nivel_initial=config.nr_points_initial;
  }

  void
  cloudCallback(const sensor_msgs::PointCloud2ConstPtr &cloud_msg)
  {
    float Volum = 1;
    int p = 0;

    pcl::PointCloud<pcl::PointXYZ>::Ptr cloud_final(new pcl::PointCloud<pcl::PointXYZ>);
    pcl::PointCloud<pcl::PointXYZ>::Ptr cloud_proiectii(new pcl::PointCloud<pcl::PointXYZ>);
    pcl::PointCloud<pcl::PointXYZ>::Ptr cloud_linii(new pcl::PointCloud<pcl::PointXYZ>);

    pcl::PointCloud<pcl::PointXYZ>::Ptr cloud_floor(new pcl::PointCloud<pcl::PointXYZ>);

    pcl::PointCloud<pcl::PointXYZ> cloud_Test;
    pcl::fromROSMsg(*cloud_msg, cloud_Test);

    pcl::PointCloud<pcl::PointXYZ>::Ptr cloudPTR(new pcl::PointCloud<pcl::PointXYZ>);
    *cloudPTR = cloud_Test;

    compute_all(cloudPTR, cloud_floor, cloud_final, cloud_proiectii, cloud_linii, Volum, p);

    sensor_msgs::PointCloud2 tempROSMsg;
    sensor_msgs::PointCloud2 tempROSMsg2;
    sensor_msgs::PointCloud2 tempROSMsg3;

    pcl::toROSMsg(*cloud_final, tempROSMsg);
    pcl::toROSMsg(*cloud_proiectii, tempROSMsg2);
    pcl::toROSMsg(*cloud_floor, tempROSMsg3);

    tempROSMsg.header.frame_id = "camera_depth_optical_frame";
    tempROSMsg2.header.frame_id = "camera_depth_optical_frame";
    tempROSMsg3.header.frame_id = "camera_depth_optical_frame";

    //Message Marker Volume
    ////////////////////////////////////////
    std::stringstream ss;

    ss << "Volumul este " << Volum << " m3";

    visualization_msgs::Marker marker;
    marker.header.frame_id = "camera_depth_optical_frame";
    marker.header.stamp = ros::Time::now();
    marker.pose.position.x = 1;
    marker.type = visualization_msgs::Marker::TEXT_VIEW_FACING;
    marker.text = ss.str();
    marker.action = visualization_msgs::Marker::ADD;

    marker.pose.position.y = 1;
    marker.pose.position.z = 1;
    marker.pose.orientation.x = 0.0;
    marker.pose.orientation.y = 0.0;
    marker.pose.orientation.z = 0.0;
    marker.pose.orientation.w = 1.0;
    marker.scale.x = 1;
    marker.scale.y = 0.1;
    marker.scale.z = 0.1;
    marker.color.a = 1.0; // Don't forget to set the alpha! Otherwise it is invisible
    marker.color.r = 0.0;
    marker.color.g = 1.0;
    marker.color.b = 0.0;
    marker.lifetime = ros::Duration();

    //////////////////////////////////////////////////////////////////////////////////

    //Message Marker Volume
    ////////////////////////////////////////
    std::stringstream ss2;

    switch (p)
    {
    case 0:
      ss2 << "No plane detected"
          << "\n";
      break;
    case 1:
      ss2 << "Only ground plane detected"
          << "\n";
      break;
    case 2:
      ss2 << "Ground plane and 1 plane detected"
          << "\n";
      break;
    case 3:
      ss2 << "Ground plane and 2 planes detected"
          << "\n";
      break;
    case 4:
      ss2 << "Ground plane and 3 planes detected"
          << "\n";
      break;
    }

    visualization_msgs::Marker marker2;
    marker2.header.frame_id = "camera_depth_optical_frame";
    marker2.header.stamp = ros::Time::now();
    marker2.pose.position.x = 1;
    marker2.type = visualization_msgs::Marker::TEXT_VIEW_FACING;
    marker2.text = ss2.str();
    marker2.action = visualization_msgs::Marker::ADD;

    marker2.pose.position.y = 1;
    marker2.pose.position.z = 1;
    marker2.pose.orientation.x = 0.0;
    marker2.pose.orientation.y = 0.0;
    marker2.pose.orientation.z = 0.0;
    marker2.pose.orientation.w = 1.0;
    marker2.scale.x = 1;
    marker2.scale.y = 0.1;
    marker2.scale.z = 0.1;
    marker2.color.a = 1.0; // Don't forget to set the alpha! Otherwise it is invisible
    marker2.color.r = 0.0;
    marker2.color.g = 1.0;
    marker2.color.b = 0.0;
    marker2.lifetime = ros::Duration();

    //////////////////////////////////////////////////////////////////////////////////

    //Publish the data

    pub1_.publish(tempROSMsg);
    pub2_.publish(tempROSMsg2);

    vis_pub.publish(marker);
    vis2_pub.publish(marker2);

    cloud_final->clear();
    cloud_linii->clear();
    cloud_proiectii->clear();

    Volum = 1;
  }

private:
  bool ok2;

  pcl::PointCloud<pcl::PointXYZ>::Ptr cloud;
  pcl::PointCloud<pcl::PointXYZ>::Ptr cloud_f;
  pcl::PointCloud<pcl::PointXYZ>::Ptr cloud_filtered;

  pcl::PointCloud<pcl::PointXYZ>::Ptr cloud_final;
  pcl::PointCloud<pcl::PointXYZ>::Ptr cloud_linii;
  pcl::PointCloud<pcl::PointXYZ>::Ptr cloud_proiectii;

  pcl::SACSegmentation<pcl::PointXYZ> seg;
  pcl::PointIndices::Ptr inliers;
  pcl::ModelCoefficients::Ptr coefficients;
  pcl::PointCloud<pcl::PointXYZ>::Ptr cloud_plane;

  pcl::PointCloud<pcl::PointXYZ> all_lines[4][4];
  pcl::PointCloud<pcl::PointXYZ>::Ptr all_planes[4];
  pcl::PointCloud<pcl::PointXYZ>::Ptr all_projected_lines[4][4];

  pcl::PointCloud<pcl::PointXYZ>::Ptr cloud_floor;

  float Coeficients[3][4];

  float Volum;

  dynamic_reconfigure::Server<pcl_tutorial::compute_volume_nodeConfig> config_server_;

  double nivel_initial;

  ros::NodeHandle nh_;
  ros::Subscriber sub_;
  ros::Publisher pub1_;
  ros::Publisher pub2_;
 

  ros::Publisher vis_pub;
  ros::Publisher vis2_pub;
  //dynamic_reconfigure::Server<pcl_tutorial::compute_volume_nodeConfig> config_server_;
};

int main(int argc, char **argv)
{
  ros::init(argc, argv, "compute_volume_node");

  ComputeVolumeNode vf;

  ros::spin();
}
