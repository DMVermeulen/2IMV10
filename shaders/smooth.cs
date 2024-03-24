#version 430

layout (local_size_x = 128, local_size_y = 1, local_size_z = 1) in;

layout(binding = 0) buffer updatedTracks {
    float updatedTracksData[];
};

layout(binding = 1) buffer smoothedTracks {
    float smoothedTracksData[];
};

layout(binding = 2) buffer isFiberBegin {
    int isFiberBeginData[];
};

layout(binding = 3) buffer isFiberEnd {
    int isFiberEndData[];
};


uniform int totalSize;
uniform float smoothFactor;
uniform int smoothL;
uniform float voxelUnitSize;


void main() {
    // Get global thread ID
    int globalID = int(gl_GlobalInvocationID.x);

    // Check if global ID is within the valid range of the buffer
    if (globalID < totalSize) {
		int totalCount=0;
		vec3 sumX = vec3(0,0,0);
		vec3 nowPoint = vec3(updatedTracksData[globalID*3],updatedTracksData[globalID*3+1],updatedTracksData[globalID*3+2]);
		sumX += nowPoint;
		totalCount++;
		
		//compute for left points
		int pointIndex = globalID-1;
		int cnt = 0;
		while(pointIndex>=0 && cnt<smoothL){
			if(1==isFiberEndData[pointIndex])
				break;
			vec3 lastPoint = vec3(updatedTracksData[pointIndex*3],updatedTracksData[pointIndex*3+1],updatedTracksData[pointIndex*3+2]);
			sumX += lastPoint;
			pointIndex--;
			cnt++;
		}
		totalCount+=cnt;
		
		//compute for right points
		pointIndex = globalID+1;
		cnt = 0;
		while(pointIndex<=totalSize-1 && cnt<smoothL){
			if(1==isFiberBeginData[pointIndex])
				break;
			vec3 nextPoint = vec3(updatedTracksData[pointIndex*3],updatedTracksData[pointIndex*3+1],updatedTracksData[pointIndex*3+2]);
			sumX += nextPoint;
			pointIndex++;
			cnt++;
		}
		totalCount+=cnt;
				
		sumX = sumX/totalCount;
		vec3 smoothed = (1-smoothFactor)*nowPoint + smoothFactor*sumX;
		smoothedTracksData[globalID*3] = smoothed.x;
		smoothedTracksData[globalID*3+1] = smoothed.y;
		smoothedTracksData[globalID*3+2] = smoothed.z;
		
    }
}