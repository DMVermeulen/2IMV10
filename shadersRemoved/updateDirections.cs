#version 430

layout (local_size_x = 128, local_size_y = 1, local_size_z = 1) in;

layout(binding = 8) buffer relaxedTubes {
    float relaxedTubesData[];
};

layout(binding = 10) buffer updatedDirections {
    float updatedDirectionsData[];
};

uniform int totalSize;


void main() {
    // Get global thread ID
    int globalID = int(gl_GlobalInvocationID.x);

    // Check if global ID is within the valid range of the buffer
    if (globalID < totalSize) {		
	    int point1ID, point2ID;
		
		if(globalID%2==0){
			point1ID = globalID;
			point2ID = globalID+1;
		}
		else if(globalID%2==1){
		    point1ID = globalID-1;
			point2ID = globalID;
		}
		vec3 p1 = vec3(relaxedTubesData[point1ID*3],relaxedTubesData[point1ID*3+1],relaxedTubesData[point1ID*3+2]);
        vec3 p2 = vec3(relaxedTubesData[point2ID*3],relaxedTubesData[point2ID*3+1],relaxedTubesData[point2ID*3+2]);
		vec3 newDir = normalize(p1-p2);
		
		updatedDirectionsData[globalID*3] = newDir.x;
		updatedDirectionsData[globalID*3+1] = newDir.y;
		updatedDirectionsData[globalID*3+2] = newDir.z;
    }

}