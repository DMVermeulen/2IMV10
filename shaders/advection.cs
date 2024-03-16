#version 430

layout (local_size_x = 128, local_size_y = 1, local_size_z = 1) in;

layout(binding = 2) buffer denseMap {
    float denseMapData[];
};

layout(binding = 0) buffer oriTubes {
    float oriTubesData[];
};

layout(binding = 4) buffer updatedTubes {
    float updatedTubesData[];
};

layout(binding = 9) buffer tempNormals {
    float tempNormalsData[];
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
    // Check if global ID is within the valid range of the buffer
    if (globalID < totalSize) {
		int R2 = kernelR * kernelR;
		int kernelWidth = 2 * kernelR + 1;
        //vec3 grad = vec3(0);
		vec3 point = vec3(oriTubesData[globalID*3],oriTubesData[globalID*3+1],oriTubesData[globalID*3+2]);
		vec3 deltaP = point - aabbMin;
		int X = int(deltaP.x / voxelUnitSize);
		int Y = int(deltaP.y / voxelUnitSize);
		int Z = int(deltaP.z / voxelUnitSize);
		X = min(X, int(nVoxels_X - 1));
		Y = min(Y, int(nVoxels_Y - 1));
		Z = min(Z, int(nVoxels_Z - 1));
/* 		int indexA= nVoxels_X * nVoxels_Y*Z + nVoxels_X * Y + X;
		indexA = min(indexA,totalVoxels-1); */
/* 		for (int dx = -kernelR; dx < kernelR; dx++) {
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

					vec3 dir = vec3(dx, dy, dz);
					float diffPos = dot(dir, dir) / R2;
					if (diffPos < 1e-5)
						continue;
					if (diffPos > 1)
						continue;
					int indexB = nVoxels_X * nVoxels_Y*nz + nVoxels_X * ny + nx;
					indexB = min(indexB, totalVoxels);
					float diffDense = denseMapData[indexB] - denseMapData[indexA];

					grad += normalize(dir)*diffDense*exp(-diffPos);
					//debug
					//if(diffDense>diff){
					//	grad = normalize(dir)*diffDense*exp(-diffPos);
					//	diff = diffDense;
					//}
				}
			}
		} */
		
/* 		grad = grad/float(kernelWidth * kernelWidth*kernelWidth);
		vec3 delta;
		if (dot(grad, grad) < 1e-5)
			delta = vec3(0);
		else
		    delta = kernelR*voxelUnitSize * normalize(grad)/1.5; */
		
		//project grad to normal vector
		//vec3 normal = vec3(tempNormalsData[globalID*3],tempNormalsData[globalID*3+1],tempNormalsData[globalID*3+2]);
		//delta = dot(normal,delta)*normal;
		
		//X-direction
		int X1 = max(X-1,0);
		int X2 = min(X+1,int(nVoxels_X - 1));
		int indexLEFT = nVoxels_X * nVoxels_Y*Z + nVoxels_X * Y + X1;
		indexLEFT = min(indexLEFT, totalVoxels-1);
		indexLEFT = max(indexLEFT,0);
		int indexRIGHT = nVoxels_X * nVoxels_Y*Z + nVoxels_X * Y + X2;
		indexRIGHT = min(indexRIGHT, totalVoxels-1);
		indexRIGHT = max(indexRIGHT,0);
		float gradX = denseMapData[indexRIGHT] - denseMapData[indexLEFT];
		
	    //Y-direction
		int Y1 = max(Y-1,0);
		int Y2 = min(Y+1,int(nVoxels_Y - 1));
		int indexBACK = nVoxels_X * nVoxels_Y*Z + nVoxels_X * Y1 + X;
		indexBACK = min(indexBACK, totalVoxels-1);
		indexBACK = max(0,indexBACK);
		int indexFRONT = nVoxels_X * nVoxels_Y*Z + nVoxels_X * Y2 + X;
		indexFRONT = min(indexFRONT, totalVoxels-1);
		indexFRONT = max(0,indexFRONT);
		float gradY = denseMapData[indexFRONT] - denseMapData[indexBACK];
		
	    //Z-direction
		int Z1 = max(Z-1,0);
		int Z2 = min(Z+1,int(nVoxels_Z - 1));
		int indexDOWN = nVoxels_X * nVoxels_Y*Z1 + nVoxels_X * Y + X;
		indexBACK = min(indexBACK, totalVoxels-1);
		indexBACK = max(0,indexBACK);
		int indexUP = nVoxels_X * nVoxels_Y*Z2 + nVoxels_X * Y + X;
		indexUP = min(indexUP, totalVoxels-1);
		indexUP = max(0,indexUP);
		float gradZ = denseMapData[indexUP] - denseMapData[indexDOWN];
		
		vec3 grad = vec3(gradX,gradY,gradZ)/(2*voxelUnitSize);
		vec3 delta;
		if (dot(grad, grad) < 1e-9)
		  delta = vec3(0);
	    else
		  delta = normalize(grad) * kernelR * voxelUnitSize;

		updatedTubesData[globalID*3] = oriTubesData[globalID*3]+delta.x;
		updatedTubesData[globalID*3+1] = oriTubesData[globalID*3+1]+delta.y;
		updatedTubesData[globalID*3+2] = oriTubesData[globalID*3+2]+delta.z;
		
    }

}