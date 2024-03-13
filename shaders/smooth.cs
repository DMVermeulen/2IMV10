#version 430

layout (local_size_x = 128, local_size_y = 1, local_size_z = 1) in;

layout(binding = 4) buffer updatedTubes {
    float updatedTubesData[];
};

layout(binding = 6) buffer isFiberEndpoint {
    int isFiberEndpointData[];
};

layout(binding = 7) buffer smoothedTubes {
    float smoothedTubesData[];
};

uniform int totalSize;
uniform float smoothFactor;
uniform int smoothL;
uniform float voxelUnitSize;


void main() {
    // Get global thread ID
    int globalID = int(gl_GlobalInvocationID.x);

    // Check if global ID is within the valid range of the buffer
    if ((globalID < totalSize) && (globalID%2==1) && (0==isFiberEndpointData[globalID])) {
		vec3 sumX = vec3(0,0,0);
		
		//float L2 = dot(smoothL,smoothL);
		vec3 nowPoint = vec3(updatedTubesData[globalID*3],updatedTubesData[globalID*3+1],updatedTubesData[globalID*3+2]);
		
		//compute for left points
		int pointIndex = globalID;
		int cnt = 0;
		int totalCount=0;
		while(pointIndex>=0 && cnt<smoothL){
			if(1==isFiberEndpointData[pointIndex])
				break;
			//if(pointIndex == globalID){
			//	pointIndex -= 2;
			//	continue;
			//}
			vec3 lastPoint = vec3(updatedTubesData[pointIndex*3],updatedTubesData[pointIndex*3+1],updatedTubesData[pointIndex*3+2]);
			//vec3 delta = lastPoint-nowPoint;
			//float dis2 = dot(delta,delta);
			//if(dis2>L2)
			//	break;
			sumX += lastPoint;
			pointIndex -= 2;
			cnt++;
		}
		totalCount+=cnt;
		
		if(cnt<smoothL){
		    if(pointIndex<0)
			   sumX += vec3(updatedTubesData[0],updatedTubesData[1],updatedTubesData[2]);
		    else{
			   if(1!=isFiberEndpointData[pointIndex+1])
				sumX += vec3(updatedTubesData[pointIndex*3],updatedTubesData[pointIndex*3+1],updatedTubesData[pointIndex*3+2]);
			   else
				sumX += vec3(updatedTubesData[(pointIndex+1)*3],updatedTubesData[(1+pointIndex)*3+1],updatedTubesData[(1+pointIndex)*3+2]);
		    }
			totalCount++;
		}

		
		
		//compute for right points
		pointIndex = globalID;
		cnt = 0;
		while(pointIndex<=totalSize-1 && cnt<smoothL){
			if(1==isFiberEndpointData[pointIndex])
				break;
			vec3 nextPoint = vec3(updatedTubesData[pointIndex*3],updatedTubesData[pointIndex*3+1],updatedTubesData[pointIndex*3+2]);
			//vec3 delta = nextPoint-nowPoint;
			//float dis2 = dot(delta,delta);
			//if(dis2>L2)
			//	break;
			sumX += nextPoint;
			pointIndex += 2;
			cnt++;
		}
		totalCount+=cnt;
		
		if(cnt<smoothL){
		    if(pointIndex>totalSize-1)
			  sumX += vec3(updatedTubesData[3*(totalSize-1)],updatedTubesData[3*(totalSize-1)+1],updatedTubesData[3*(totalSize-1)+2]);
		    else{
			  if(1!=isFiberEndpointData[pointIndex-1])
				sumX += vec3(updatedTubesData[pointIndex*3],updatedTubesData[pointIndex*3+1],updatedTubesData[pointIndex*3+2]);
			  else
				sumX += vec3(updatedTubesData[(pointIndex-1)*3],updatedTubesData[(pointIndex-1)*3+1],updatedTubesData[(pointIndex-1)*3+2]);
		    }		
		  totalCount++;
		}

		sumX -= nowPoint;
		totalCount--;
		
		sumX = sumX/totalCount;
		vec3 smoothed = (1-smoothFactor)*nowPoint + smoothFactor*sumX;
		smoothedTubesData[globalID*3] = smoothed.x;
		smoothedTubesData[globalID*3+1] = smoothed.y;
		smoothedTubesData[globalID*3+2] = smoothed.z;
		
    }
	else if((globalID < totalSize) && (1==isFiberEndpointData[globalID])){
		vec3 nowPoint = vec3(updatedTubesData[globalID*3],updatedTubesData[globalID*3+1],updatedTubesData[globalID*3+2]);
		smoothedTubesData[globalID*3] = nowPoint.x;
		smoothedTubesData[globalID*3+1] = nowPoint.y;
		smoothedTubesData[globalID*3+2] = nowPoint.z;
	}
	else if((globalID < totalSize) && (globalID%2==0)){
		smoothedTubesData[globalID*3] = 0;
		smoothedTubesData[globalID*3+1] = 0;
		smoothedTubesData[globalID*3+2] = 0;
	}
}