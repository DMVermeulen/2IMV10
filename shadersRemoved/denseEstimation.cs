#version 430

layout (local_size_x = 128, local_size_y = 1, local_size_z = 1) in;

layout(binding = 1) buffer voxelCount {
    uint voxelCountData[];
};

layout(binding = 2) buffer denseMap {
    float denseMapData[];
};


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

    // Check if global ID is within the valid range of the buffer
    if (globalID < totalSize) {
		float dense = 0;
		int Z = globalID / (nVoxels_X*nVoxels_Y);
		int mod= globalID % (nVoxels_X*nVoxels_Y);
		int X = mod % nVoxels_X;
		int Y = mod / nVoxels_X;
		for (int dx = -kernelR; dx < kernelR; dx++) {
			for (int dy = -kernelR; dy < kernelR; dy++) {
				for (int dz = -kernelR; dz < kernelR; dz++) {
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
			}
		}
		
/* 		//X-direction
		for (int dx = -kernelR; dx < kernelR; dx++) {
			int dy=0;
			int dz=0;
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
			index = min(int(index), int(totalSize-1));
			index = max(0,index);
			int pointCnt = int(voxelCountData[index]);
			dense += pointCnt * (1 - dot/PR2);
		}
		
		//Y-direction
		for (int dy = -kernelR; dy < kernelR; dy++) {
			int dx=0;
			int dz=0;
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
			index = min(int(index), int(totalSize-1));
			index = max(0,index);
			int pointCnt = int(voxelCountData[index]);
			dense += pointCnt * (1 - dot/PR2);
		}
		
		//Z-direction
		for (int dz = -kernelR; dz < kernelR; dz++) {
			int dx=0;
			int dy=0;
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
			index = min(int(index), int(totalSize-1));
			index = max(0,index);
			int pointCnt = int(voxelCountData[index]);
			dense += pointCnt * (1 - dot/PR2);
		} */
		
		//dense -= 2*voxelCountData[nVoxels_X * nVoxels_Y*Z + nVoxels_X * Y + X]*1.0;
		
		
		denseMapData[globalID] = dense;
		
		
		//denseMapData[globalID] = voxelCountData[nVoxels_X * nVoxels_Y*Z + nVoxels_X * Y + X]*1.0;
		
		//TEST
		//if(0==voxelCountData[globalID])
		//	denseMapData[globalID] = 0;
		
    }
}
