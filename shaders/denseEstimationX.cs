#version 430 core

layout (local_size_x = 128, local_size_y = 1, local_size_z = 1) in;

layout(binding = 1) buffer voxelCount {
    uint voxelCountData[];
};

/* layout(binding = 10) buffer denseMap {
    float denseMapData[];
}; */

layout(r32f, binding = 1) uniform image3D denseMapX;


uniform int totalSize;
uniform int nVoxels_X;
uniform int nVoxels_Y;
uniform int nVoxels_Z;
uniform int kernelR;
uniform float voxelUnitSize;

// Compute shader entry point
void main() {
    // Get global thread ID
    int globalID = int(gl_GlobalInvocationID.x);

/*     // 1D buffer impl
    if (globalID < totalSize) {
		float dense = 0;
		int Z = globalID / (nVoxels_X*nVoxels_Y);
		int mod= globalID % (nVoxels_X*nVoxels_Y);
		int X = mod % nVoxels_X;
		int Y = mod / nVoxels_X;
		int dy=0;
		int dz=0;
		for (int dx = -kernelR; dx < kernelR; dx++) {
					int nx = X + dx;
					nx = max(nx, 0);
					nx = min(nx, int(nVoxels_X - 1));
					int ny = Y + dy;
					ny = max(ny, 0);
					ny = min(ny, int(nVoxels_Y - 1));
					int nz = Z + dz;
					nz = max(nz, 0);
					nz = min(nz, int(nVoxels_Z - 1));
					vec3 diff = voxelUnitSize * (vec3(nx, ny, nz) - vec3(X, Y, Z));
					float dot = dot(diff, diff);
					float PR2 = kernelR * voxelUnitSize * kernelR * voxelUnitSize;
					if (dot > PR2)
						continue;

					int index = nVoxels_X * nVoxels_Y*nz + nVoxels_X * ny + nx;
					index = min(int(index), int(totalSize));
					int pointCnt = int(voxelCountData[index]);
					
					dense += pointCnt * (1 - dot/PR2);
		}
		
		//dense -= 2*voxelCountData[nVoxels_X * nVoxels_Y*Z + nVoxels_X * Y + X]*1.0;
		
		
		denseMapData[globalID] = dense;
    } */
	
	// 3D image texture impl
    if (globalID < totalSize) {
		float dense = 0;
		int Z = globalID / (nVoxels_X*nVoxels_Y);
		int mod= globalID % (nVoxels_X*nVoxels_Y);
		int X = mod % nVoxels_X;
		int Y = mod / nVoxels_X;
		int dy=0;
		int dz=0;
		for (int dx = -kernelR; dx < kernelR; dx++) {
					int nx = X + dx;
					nx = max(nx, 0);
					nx = min(nx, int(nVoxels_X - 1));
					int ny = Y + dy;
					ny = max(ny, 0);
					ny = min(ny, int(nVoxels_Y - 1));
					int nz = Z + dz;
					nz = max(nz, 0);
					nz = min(nz, int(nVoxels_Z - 1));
					vec3 diff = voxelUnitSize * (vec3(nx, ny, nz) - vec3(X, Y, Z));
					float dot = dot(diff, diff);
					float PR2 = kernelR * voxelUnitSize * kernelR * voxelUnitSize;
					if (dot > PR2)
						continue;

					int index = nVoxels_X * nVoxels_Y*nz + nVoxels_X * ny + nx;
					index = min(int(index), int(totalSize));
					int pointCnt = int(voxelCountData[index]);
					
					dense += pointCnt * (1 - dot/PR2);
		}
		
		imageStore(denseMapX, ivec3(X,Y,Z),vec4(dense,0,0,1));
    }
}