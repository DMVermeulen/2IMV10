#version 430

layout (local_size_x = 128, local_size_y = 1, local_size_z = 1) in;

layout(r32f, binding = 0) uniform image3D denseMap;

layout(binding = 0) buffer oriTracks {
    float oriTracksData[];
};

layout(binding = 1) buffer updatedTracks {
    float updatedTracksData[];
};

layout(binding = 2) buffer directions {
    float directionsData[];
};


uniform int totalSize;
uniform int nVoxels_X;
uniform int nVoxels_Y;
uniform int nVoxels_Z;
uniform int kernelR;
uniform float voxelUnitSize;
uniform vec3 aabbMin;
uniform int totalVoxels;

// Compute shader entry point
void main() {
    // Get global thread ID
    int globalID = int(gl_GlobalInvocationID.x);
	float diff = 0;

	//3D image texture impl (denseMap)
    if (globalID < totalSize) {
		int R2 = kernelR * kernelR;
		int kernelWidth = 2 * kernelR + 1;
        //vec3 grad = vec3(0);
		vec3 point = vec3(oriTracksData[globalID*3],oriTracksData[globalID*3+1],oriTracksData[globalID*3+2]);
		vec3 deltaP = point - aabbMin;
		int X = int(deltaP.x / voxelUnitSize);
		int Y = int(deltaP.y / voxelUnitSize);
		int Z = int(deltaP.z / voxelUnitSize);
		X = min(X, int(nVoxels_X - 1));
		Y = min(Y, int(nVoxels_Y - 1));
		Z = min(Z, int(nVoxels_Z - 1));
		
		//project grad to normal vector
		//vec3 normal = vec3(tempNormalsData[globalID*3],tempNormalsData[globalID*3+1],tempNormalsData[globalID*3+2]);
		//delta = dot(normal,delta)*normal;
		
		//X-direction
		int X1 = max(X-1,0);
		int X2 = min(X+1,int(nVoxels_X - 1));
		float gradX = imageLoad(denseMap, ivec3(X2,Y,Z)).r - imageLoad(denseMap, ivec3(X1,Y,Z)).r;
		
	    //Y-direction
		int Y1 = max(Y-1,0);
		int Y2 = min(Y+1,int(nVoxels_Y - 1));
		float gradY = imageLoad(denseMap, ivec3(X,Y2,Z)).r - imageLoad(denseMap, ivec3(X,Y1,Z)).r;
		
	    //Z-direction
		int Z1 = max(Z-1,0);
		int Z2 = min(Z+1,int(nVoxels_Z - 1));
		float gradZ = imageLoad(denseMap, ivec3(X,Y,Z2)).r - imageLoad(denseMap, ivec3(X,Y,Z1)).r;
		
		vec3 grad = vec3(gradX,gradY,gradZ)/(2*voxelUnitSize);
		vec3 delta;
		if (dot(grad, grad) < 1e-9)
		  delta = vec3(0);
	    else{
			// constrained advection, project moving vector delta to the orthogonal vector
			delta = normalize(grad) * kernelR * voxelUnitSize;

			vec3 dir = vec3(directionsData[globalID*3],directionsData[globalID*3+1],directionsData[globalID*3+2]);
			vec3 u = cross(dir,cross(delta,dir));
			u = normalize(u);
			delta = u*dot(delta,u);
		}
		 
		updatedTracksData[globalID*3] = oriTracksData[globalID*3]+delta.x;
		updatedTracksData[globalID*3+1] = oriTracksData[globalID*3+1]+delta.y;
		updatedTracksData[globalID*3+2] = oriTracksData[globalID*3+2]+delta.z;
    }
}