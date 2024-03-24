#version 430 core

layout (local_size_x = 128, local_size_y = 1, local_size_z = 1) in;
layout (binding = 0) buffer oriTubes {
    float oriTubesData[];
};

layout (binding = 1, std430) buffer voxelCount {
    uint voxelCountData[];
};

uniform int totalSize;
uniform int nVoxels_X;
uniform int nVoxels_Y;
uniform int nVoxels_Z;
uniform vec3 aabbMin;
uniform float voxelUnitSize;
uniform int totalVoxels;

void main() {
	int globalID = int(gl_GlobalInvocationID.x);
		
	//1-D buffer implementation
	if (globalID < totalSize) {
	    vec3 point = vec3(oriTubesData[globalID*3],oriTubesData[globalID*3+1],oriTubesData[globalID*3+2]);
		vec3 deltaP = point - aabbMin;
		int level_X = int(min(float(nVoxels_X - 1), deltaP.x / voxelUnitSize));
		int level_Y = int(min(float(nVoxels_Y - 1), deltaP.y / voxelUnitSize));
		int level_Z = int(min(float(nVoxels_Z - 1), deltaP.z / voxelUnitSize));
		int index = nVoxels_X * nVoxels_Y*level_Z + nVoxels_X * level_Y + level_X;
		index = min(index, totalVoxels-1);
		index = max(int(0),index);
		atomicAdd(voxelCountData[index], 1);
	}
	
}