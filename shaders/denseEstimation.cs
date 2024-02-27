#version 430

layout (local_size_x = 128, local_size_y = 1, local_size_z = 1) in;

layout(binding = 0) buffer voxelCount {
    uint voxelCountData[];
};

layout(binding = 1) buffer denseMap {
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
    uint globalID = gl_GlobalInvocationID.x;

    // Check if global ID is within the valid range of the buffer
    if (globalID < totalSize) {
		float dense = 0;
		uint Z = globalID / (nVoxels_X*nVoxels_Y);
		uint mod= globalID % (nVoxels_X*nVoxels_Y);
		uint X = mod % nVoxels_X;
		uint Y = mod / nVoxels_X;
		for (int dx = -kernelR; dx < kernelR; dx++) {
			for (int dy = -kernelR; dy < kernelR; dy++) {
				for (int dz = -kernelR; dz < kernelR; dz++) {
					uint nx = X + dx;
					nx = max(nx, 0);
					nx = min(nx, int(nVoxels_X - 1));
					uint ny = Y + dy;
					ny = max(ny, 0);
					ny = min(ny, int(nVoxels_Y - 1));
					uint nz = Z + dz;
					nz = max(nz, 0);
					nz = min(nz, int(nVoxels_Z - 1));
					vec3 diff = voxelUnitSize * (vec3(nx, ny, nz) - vec3(X, Y, Z));
					float dot = dot(diff, diff);
					float PR2 = kernelR * voxelUnitSize * kernelR * voxelUnitSize;
					if (dot > PR2)
						continue;

					uint index = nVoxels_X * nVoxels_Y*nz + nVoxels_X * ny + nx;
					index = min(int(index), int(totalSize));
					uint pointCnt = voxelCountData[index];
					
					dense += pointCnt * (1 - dot/PR2);
				}
			}
		}
		
		denseMapData[globalID] = dense;
    }
}
