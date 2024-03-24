#version 430

layout (local_size_x = 128, local_size_y = 1, local_size_z = 1) in;

layout(binding = 10) buffer updatedDirections {
    float updatedDirectionsData[];
};

layout(binding = 11) buffer updatedNormals {
    float updatedNormalsData[];
};

layout(binding = 6) buffer isFiberEndpoint {
    int isFiberEndpointData[];
};

uniform int totalSize;


void main() {
    // Get global thread ID
    int globalID = int(gl_GlobalInvocationID.x);

    // Check if global ID is within the valid range of the buffer
    if (globalID < totalSize) {		
	    vec3 nextDir;
		if(globalID == totalSize-1)
			nextDir = vec3(0,0,1);
		else if(1==isFiberEndpointData[globalID] || 1==isFiberEndpointData[globalID+1]){
			nextDir = vec3(0,0,1);
		}
		else{
			int nextID = globalID+2;
			nextDir = vec3(updatedDirectionsData[nextID*3],updatedDirectionsData[nextID*3+1],updatedDirectionsData[nextID*3+2]);
		}
		
		vec3 nowDir = vec3(updatedDirectionsData[globalID*3],updatedDirectionsData[globalID*3+1],updatedDirectionsData[globalID*3+2]);
		
		vec3 left = cross(nextDir, nowDir);
		vec3 normal = normalize(cross(nowDir, left));
		
		updatedNormalsData[globalID*3] = normal.x;
		updatedNormalsData[globalID*3+1] = normal.y;
		updatedNormalsData[globalID*3+2] = normal.z;
    }

}