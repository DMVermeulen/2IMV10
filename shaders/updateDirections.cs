#version 430

layout (local_size_x = 128, local_size_y = 1, local_size_z = 1) in;

layout(binding = 0) buffer relaxedTracks {
    float relaxedTracksData[];
};

layout(binding = 1) buffer updatedDirections {
    float updatedDirectionsData[];
};

layout (binding = 2) buffer isFiberBegin {
    int isFiberBeginData[];
};

layout (binding = 3) buffer isFiberEnd {
    int isFiberEndData[];
};

uniform int totalSize;


void main() {
    // Get global thread ID
    int globalID = int(gl_GlobalInvocationID.x);

    // Check if global ID is within the valid range of the buffer
    if (globalID < totalSize) {		
	    int point1ID, point2ID;
		
		if(1==isFiberBeginData[globalID]){
			point1ID = globalID;
			point2ID = globalID+1;
		}
		else if(1==isFiberEndData[globalID]){
		    point1ID = globalID-1;
			point2ID = globalID;
		}
		else {
		    point1ID = globalID;
			point2ID = globalID+1;
		}
		vec3 p1 = vec3(relaxedTracksData[point1ID*3],relaxedTracksData[point1ID*3+1],relaxedTracksData[point1ID*3+2]);
        vec3 p2 = vec3(relaxedTracksData[point2ID*3],relaxedTracksData[point2ID*3+1],relaxedTracksData[point2ID*3+2]);
		vec3 newDir = normalize(p1-p2);
		
		updatedDirectionsData[globalID*3] = newDir.x;
		updatedDirectionsData[globalID*3+1] = newDir.y;
		updatedDirectionsData[globalID*3+2] = newDir.z;
    }

}