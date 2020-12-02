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
#include <string>

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
    pub3_ = nh_.advertise<sensor_msgs::PointCloud2>("/cloud_floor", 1);
    sub_ = nh_.subscribe("/pf_out", 1, &ComputeVolumeNode::cloudCallback, this);

    vis_pub = nh_.advertise<visualization_msgs::Marker>("/Volum_final", 0);
    vis2_pub = nh_.advertise<visualization_msgs::Marker>("/Nr_of_planes", 0);
    vis3_pub = nh_.advertise<visualization_msgs::Marker>("/Mesaj_planuri", 0);
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
                         int dimension_cloud,
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

    if (inliers->indices.size() < 100)
    {
      ok2 = 0;

      if (inliers->indices.size() == 0)
      {

        //PCL_ERROR("Could not estimate a planar model for the given dataset.");
        std::cout << "Could not estimate a planar model for the given dataset."
                  << "\n";
      }
      else
      {
        //PCL_ERROR("Very few points in principal plane");
        std::cout << "Very few points in principal plane"
                  << "\n";
      }
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

  void create_lines(float Coeficients[3][4],
                    pcl::PointCloud<pcl::PointXYZ>::Ptr all_planes[4],
                    pcl::PointCloud<pcl::PointXYZ> all_lines[4][4],
                    pcl::PointCloud<pcl::PointXYZ>::Ptr cloud_linii,
                    bool &ok2)
  {

    pcl::PointCloud<pcl::PointXYZ>::Ptr cloud(new pcl::PointCloud<pcl::PointXYZ>);

    std::string str;
    std::string str2;

    pcl::PointCloud<pcl::PointXYZ>::Ptr cloud_projected(new pcl::PointCloud<pcl::PointXYZ>);

    int i, j;

    if (ok2 != 0)
    {

      for (i = 1; i < 4; i++)
      {

        cloud = all_planes[i];

        // Create a set of planar coefficients with X=Y=0,Z=1
        pcl::ModelCoefficients::Ptr coefficients(new pcl::ModelCoefficients());
        coefficients->values.resize(4);

        for (j = 1; j < 4; j++)
        {
          if (j != i)
          {
            /*
        std::cout << "\n";
        std::cout << "plan " << i << "\n";
         */
            coefficients->values[0] = Coeficients[j - 1][0];
            coefficients->values[1] = Coeficients[j - 1][1];
            coefficients->values[2] = Coeficients[j - 1][2];
            coefficients->values[3] = Coeficients[j - 1][3];

            /*  
        std::cout << "Projecting plane " << i << " to plane " << j << "\n";
        std::cout << "Saving line " << i << "_" << j << "\n";
         */
            // Create the filtering object
            pcl::ProjectInliers<pcl::PointXYZ> proj;
            proj.setModelType(pcl::SACMODEL_PLANE);
            proj.setInputCloud(cloud);
            proj.setModelCoefficients(coefficients);
            proj.filter(*cloud_projected);

            //PCL_INFO("Saving the projected Pointcloud \n");

            all_lines[i][j] = *cloud_projected;

            *cloud_linii = *cloud_linii + *cloud_projected;
          }
        }
      }
    }
    else
    {
      std::cout << "Cannot segment"
                << "\n";
    }
  }

  void project_line_2_plane_single(pcl::PointCloud<pcl::PointXYZ>::Ptr line,
                                   pcl::ModelCoefficients::Ptr coefficients2,
                                   pcl::PointCloud<pcl::PointXYZ>::Ptr projection)
  {
    
    

          //pcl::PointCloud<pcl::PointXYZ>::Ptr cloud_c(new pcl::PointCloud<pcl::PointXYZ>);
          pcl::PointCloud<pcl::PointXYZ>::Ptr cloud_projected(new pcl::PointCloud<pcl::PointXYZ>);

            // Create the filtering object
            pcl::ProjectInliers<pcl::PointXYZ> proj;
            proj.setModelType(pcl::SACMODEL_PLANE);
            proj.setInputCloud(cloud);
            proj.setModelCoefficients(coefficients2);
            proj.filter(*cloud_projected);

            *projection += *cloud;
            *projection += *cloud_projected;

            //*cloud_proiectii += *projection;
          

  }

  void project_line_2_plane(float Coeficients[3][4],
                            pcl::PointCloud<pcl::PointXYZ>::Ptr all_planes[4],
                            pcl::PointCloud<pcl::PointXYZ> all_lines[4][4],
                            pcl::PointCloud<pcl::PointXYZ>::Ptr all_projected_lines[4][4],
                            pcl::PointCloud<pcl::PointXYZ>::Ptr cloud_proiectii)
  {

    pcl::PointCloud<pcl::PointXYZ>::Ptr cloud(new pcl::PointCloud<pcl::PointXYZ>);

    int i, j;

    int aux;

    

    for (i = 1; i < 3; i++)
    {
      for (j = i; j < 4; j++)
      {

        if (i != j)
        {

          aux = (6 - i - j);


          

          // Create a set of planar coefficients with X=Y=0,Z=1
          pcl::ModelCoefficients::Ptr coefficients2(new pcl::ModelCoefficients());
          coefficients2->values.resize(4);

          coefficients2->values[0] = Coeficients[aux - 1][0];
          coefficients2->values[1] = Coeficients[aux - 1][1];
          coefficients2->values[2] = Coeficients[aux - 1][2];
          coefficients2->values[3] = Coeficients[aux - 1][3];

          int aux2;
          pcl::PointCloud<pcl::PointXYZ>::Ptr cloud_c(new pcl::PointCloud<pcl::PointXYZ>);

          for (int z = 1; z < 3; z++)
          {

            aux2 = i;
            i = j;
            j = aux2;

            ////////////

            *cloud = all_lines[i][j];

            project_line_2_plane_single(cloud,coefficients2,cloud_c);

            *cloud_proiectii += *cloud_c;

            /////////////

            /*

            pcl::PointCloud<pcl::PointXYZ>::Ptr cloud_projected(new pcl::PointCloud<pcl::PointXYZ>);

            *cloud = all_lines[i][j];

            // Create the filtering object
            pcl::ProjectInliers<pcl::PointXYZ> proj;
            proj.setModelType(pcl::SACMODEL_PLANE);
            proj.setInputCloud(cloud);
            proj.setModelCoefficients(coefficients2);
            proj.filter(*cloud_projected);

            *cloud_c += *cloud;
            *cloud_c += *cloud_projected;

            *cloud_proiectii += *cloud_c;

            */
          }

          all_projected_lines[i][j] = cloud_c;
        }
      }
    }
  }

  void compute_length_line(pcl::PointCloud<pcl::PointXYZ>::Ptr cloud,
                           float &distanta,
                           float &coordonate_punct_minim_x,
                           float &coordonate_punct_minim_y,
                           float &coordonate_punct_minim_z,
                           float &coordonate_punct_maxim_x,
                           float &coordonate_punct_maxim_y,
                           float &coordonate_punct_maxim_z)
  {

    float minim_x = cloud->points[0].x;
    int index_min_x = 0;

    float minim_y = cloud->points[0].y;
    int index_min_y = 0;

    float minim_z = cloud->points[0].z;
    int index_min_z = 0;

    float maxim_x = cloud->points[0].x;
    int index_max_x = 0;

    float maxim_y = cloud->points[0].y;
    int index_max_y = 0;

    float maxim_z = cloud->points[0].z;
    int index_max_z = 0;

    for (int nIndex = 0; nIndex < cloud->points.size(); nIndex++)
    {
      if (minim_x > cloud->points[nIndex].x)
      {
        minim_x = cloud->points[nIndex].x;
        index_min_x = nIndex;
      }

      if (minim_y > cloud->points[nIndex].y)
      {
        minim_y = cloud->points[nIndex].y;
        index_min_y = nIndex;
      }

      if (minim_z > cloud->points[nIndex].z)
      {
        minim_z = cloud->points[nIndex].z;
        index_min_z = nIndex;
      }

      if (maxim_x < cloud->points[nIndex].x)
      {
        maxim_x = cloud->points[nIndex].x;
        index_max_x = nIndex;
      }

      if (maxim_y < cloud->points[nIndex].y)
      {
        maxim_y = cloud->points[nIndex].y;
        index_max_y = nIndex;
      }

      if (maxim_z < cloud->points[nIndex].z)
      {
        maxim_z = cloud->points[nIndex].z;
        index_max_z = nIndex;
      }
    }

    float Sortare[3];

    Sortare[0] = abs(maxim_x - minim_x);
    Sortare[1] = abs(maxim_y - minim_y);
    Sortare[2] = abs(maxim_z - minim_z);

    float maximum = Sortare[0];

    float Puncte[2][3];

    int t = 0;

    Puncte[0][0] = index_min_x;
    Puncte[1][0] = index_max_x;
    Puncte[0][1] = index_min_y;
    Puncte[1][1] = index_max_y;
    Puncte[0][2] = index_min_z;
    Puncte[1][2] = index_max_z;

    for (int q = 0; q < 3; q++)
    {
      if (maximum < Sortare[q])
      {
        maximum = Sortare[q];
        t = q;
      }
    }

    int pozitie_min = Puncte[0][t];
    int pozitie_max = Puncte[1][t];

    float distanta_x = (cloud->points[pozitie_max].x - cloud->points[pozitie_min].x);

    distanta_x = distanta_x * distanta_x;

    float distanta_y = (cloud->points[pozitie_max].y - cloud->points[pozitie_min].y);

    distanta_y = distanta_y * distanta_y;

    float distanta_z = (cloud->points[pozitie_max].z - cloud->points[pozitie_min].z);

    distanta_z = distanta_z * distanta_z;

    distanta = distanta_x + distanta_y + distanta_z;

    distanta = sqrt(distanta_x + distanta_y + distanta_z);

    coordonate_punct_minim_x = cloud->points[pozitie_min].x;
    coordonate_punct_minim_y = cloud->points[pozitie_min].y;
    coordonate_punct_minim_z = cloud->points[pozitie_min].z;

    coordonate_punct_maxim_x = cloud->points[pozitie_max].x;
    coordonate_punct_maxim_y = cloud->points[pozitie_max].y;
    coordonate_punct_maxim_z = cloud->points[pozitie_max].z;
  }

  void compute_volume(pcl::PointCloud<pcl::PointXYZ>::Ptr all_projected_lines[4][4],
                      float &Volum)
  {

    pcl::PointCloud<pcl::PointXYZ>::Ptr cloud(new pcl::PointCloud<pcl::PointXYZ>);

    float muchii[3];

    int i, j;

    for (i = 1; i < 3; i++)
    {
      for (j = 1; j < 4; j++)
      {

        if (i < j)
        {
          float distanta = 0;

          cloud = all_projected_lines[i][j];

          float coordonate_punct_minim_x;
          float coordonate_punct_minim_y;
          float coordonate_punct_minim_z;
          float coordonate_punct_maxim_x;
          float coordonate_punct_maxim_y;
          float coordonate_punct_maxim_z;

          compute_length_line(cloud, distanta,
                              coordonate_punct_minim_x,
                              coordonate_punct_minim_y,
                              coordonate_punct_minim_z,
                              coordonate_punct_maxim_x,
                              coordonate_punct_maxim_y,
                              coordonate_punct_maxim_z);

          Volum = Volum * distanta;
        }
      }
    }

    // std::cout << "Volum final " << Volum << " m^3"
    //         << "\n";
  }

  void compute_third_perpendicular_plane(pcl::PointCloud<pcl::PointXYZ>::Ptr cloud_plane_1,
                                         pcl::PointCloud<pcl::PointXYZ>::Ptr cloud_plane_2,
                                         float Coeficients[3][4],
                                         pcl::PointCloud<pcl::PointXYZ> all_lines[4][4],
                                         pcl::PointCloud<pcl::PointXYZ>::Ptr cloud_linii)
  {
  }

  void compute_all(pcl::PointCloud<pcl::PointXYZ>::Ptr cloud,
                   pcl::PointCloud<pcl::PointXYZ>::Ptr cloud_floor,
                   pcl::PointCloud<pcl::PointXYZ>::Ptr cloud_final,
                   pcl::PointCloud<pcl::PointXYZ>::Ptr cloud_proiectii,
                   pcl::PointCloud<pcl::PointXYZ>::Ptr cloud_linii,
                   float &Volum,
                   int &p,
                   std::string &text_planuri)
  {

    pcl::PointCloud<pcl::PointXYZ>::Ptr cloud_f(new pcl::PointCloud<pcl::PointXYZ>);

    float Coeficients[3][4];

    pcl::PointCloud<pcl::PointXYZ> all_lines[4][4];
    pcl::PointCloud<pcl::PointXYZ>::Ptr all_planes[4];
    pcl::PointCloud<pcl::PointXYZ>::Ptr all_projected_lines[4][4];

    pcl::ModelCoefficients::Ptr coefficients_floor(new pcl::ModelCoefficients);

    bool ok = 1;

    bool ok2;

    if (cloud->size() == 0)
    {
      ok = 0;
      p = 0; // NO PLANE
    }

    int dimension_cloud = cloud->width * cloud->height;

    planar_segmenting_single_time(cloud, cloud_floor, coefficients_floor);

    Eigen::Vector3f normal_floor;
    normal_floor << coefficients_floor->values[0], coefficients_floor->values[1], coefficients_floor->values[2];
    if (coefficients_floor->values[3] < 0)
    {
      normal_floor *= -1;
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
        planar_segmenting(cloud_f, dimension_cloud, Coeficients, all_planes, cloud_final, t, ok2); //cloud_f is global, so the modifications stay

        cloud = cloud_f; // Cloud is now the extracted pointcloud

        if (cloud->size() == 0)
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

    if (ok && ok2)
    {
      create_lines(Coeficients, all_planes, all_lines, cloud_linii, ok2);

      project_line_2_plane(Coeficients, all_planes, all_lines, all_projected_lines, cloud_proiectii);

      compute_volume(all_projected_lines, Volum);
    }
    else
    {
      ///////////////////////////////////////////////////////////////////////////////////////////
      //Cases when the volume cannot be computed directly but otehr methods can be tried
      ////////////

      float epsilon_parallel = 0.5;
      float epsilon_perpendicular = 0.5;

      //   CAses where not enough planes
      if (p == 2)
      {
        // Ground plane and 1 plane
        Eigen::Vector3f normal_plane_1;
        normal_plane_1 << Coeficients[0][0], Coeficients[0][1], Coeficients[0][2];
        if (Coeficients[0][3] < 0)
        {
          normal_plane_1 *= -1;
        }

        float aux1 = abs(normal_floor(0) / normal_plane_1(0) - normal_floor(1) / normal_plane_1(1));
        float aux2 = abs(normal_floor(2) / normal_plane_1(2) - normal_floor(1) / normal_plane_1(1));
        float aux3 = abs(normal_floor(0) / normal_plane_1(0) - normal_floor(2) / normal_plane_1(2));

        if ((aux1 < epsilon_parallel) && (aux2 < epsilon_parallel) && (aux3 < epsilon_parallel))
        {

          text_planuri = "Planul este paralel cu podeaua \n";
        }

        float verificare_perpendicular = normal_floor(0) * normal_plane_1(0) + normal_floor(1) * normal_plane_1(1) + normal_floor(2) * normal_plane_1(2);

        if (abs(verificare_perpendicular) < epsilon_perpendicular)
        {
          text_planuri = "Planul este perpendicular cu podeaua \n";
        }
      }

      if (p == 3) // Sunt 2 PLanuri si Ground plane
      {
        Eigen::Vector3f normal_plane_1;
        normal_plane_1 << Coeficients[0][0], Coeficients[0][1], Coeficients[0][2];
        if (Coeficients[0][3] < 0)
        {
          normal_plane_1 *= -1;
        }

        Eigen::Vector3f normal_plane_2;
        normal_plane_2 << Coeficients[1][0], Coeficients[1][1], Coeficients[1][2];
        if (Coeficients[1][3] < 0)
        {
          normal_plane_2 *= -1;
        }

        text_planuri = "Sunt 2 planuri";
      }
    }
  }

  void
  dynReconfCallback()
  {
  }

  void
  cloudCallback(const sensor_msgs::PointCloud2ConstPtr &cloud_msg)
  {
    float Volum = 1;
    int p = 0;

    std::string text_planuri = "No text";

    pcl::PointCloud<pcl::PointXYZ>::Ptr cloud_final(new pcl::PointCloud<pcl::PointXYZ>);
    pcl::PointCloud<pcl::PointXYZ>::Ptr cloud_proiectii(new pcl::PointCloud<pcl::PointXYZ>);
    pcl::PointCloud<pcl::PointXYZ>::Ptr cloud_linii(new pcl::PointCloud<pcl::PointXYZ>);

    pcl::PointCloud<pcl::PointXYZ>::Ptr cloud_floor(new pcl::PointCloud<pcl::PointXYZ>);

    pcl::PointCloud<pcl::PointXYZ> cloud_Test;
    pcl::fromROSMsg(*cloud_msg, cloud_Test);

    pcl::PointCloud<pcl::PointXYZ>::Ptr cloudPTR(new pcl::PointCloud<pcl::PointXYZ>);
    *cloudPTR = cloud_Test;

    compute_all(cloudPTR, cloud_floor, cloud_final, cloud_proiectii, cloud_linii, Volum, p, text_planuri);

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

    visualization_msgs::Marker marker3;
    marker3.header.frame_id = "camera_depth_optical_frame";
    marker3.header.stamp = ros::Time::now();
    marker3.pose.position.x = 1;
    marker3.type = visualization_msgs::Marker::TEXT_VIEW_FACING;
    marker3.text = text_planuri;
    marker3.action = visualization_msgs::Marker::ADD;

    marker3.pose.position.y = 1;
    marker3.pose.position.z = 1;
    marker3.pose.orientation.x = 0.0;
    marker3.pose.orientation.y = 0.0;
    marker3.pose.orientation.z = 0.0;
    marker3.pose.orientation.w = 1.0;
    marker3.scale.x = 1;
    marker3.scale.y = 0.1;
    marker3.scale.z = 0.1;
    marker3.color.a = 1.0; // Don't forget to set the alpha! Otherwise it is invisible
    marker3.color.r = 0.0;
    marker3.color.g = 1.0;
    marker3.color.b = 0.0;
    marker3.lifetime = ros::Duration();

    ////////////////////////////////////////

    //Publish the data

    pub1_.publish(tempROSMsg);
    pub2_.publish(tempROSMsg2);
    pub3_.publish(tempROSMsg3);

    vis_pub.publish(marker);
    vis2_pub.publish(marker2);
    vis3_pub.publish(marker3);

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

  ros::NodeHandle nh_;
  ros::Subscriber sub_;
  ros::Publisher pub1_;
  ros::Publisher pub2_;
  ros::Publisher pub3_;

  ros::Publisher vis_pub;
  ros::Publisher vis2_pub;
  ros::Publisher vis3_pub;
  //dynamic_reconfigure::Server<pcl_tutorial::compute_volume_nodeConfig> config_server_;
};

int main(int argc, char **argv)
{
  ros::init(argc, argv, "compute_volume_node");

  ComputeVolumeNode vf;

  ros::spin();
}