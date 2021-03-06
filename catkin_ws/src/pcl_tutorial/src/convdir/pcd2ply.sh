#!/bin/bash
rosrun pcl_ros pointcloud_to_pcd input:=/pf_out & sleep 1; kill $!
rm "$(stat -c "%Y:%n" * | sort -t: -n | tail -1 | cut -d: -f2-)"
for filename in *.pcd; do
    rm plycloud.ply
    pcl_pcd2ply "$filename" plycloud.ply
    rm "$filename"
done
