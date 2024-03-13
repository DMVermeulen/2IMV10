#version 430

layout (local_size_x = 128, local_size_y = 1, local_size_z = 1) in;

layout(binding = 0) buffer tempTubes {
    float tempTubesData[];
};

layout(binding = 7) buffer smoothedTubes {
    float smoothedTubesData[];
};

layout(binding = 8) buffer relaxedTubes {
    float relaxedTubesData[];
};

uniform int totalSize;
uniform float relaxFactor;


void main() {
    // Get global thread ID
    int globalID = int(gl_GlobalInvocationID.x);

    // Check if global ID is within the valid range of the buffer
    if (globalID < totalSize) {		
		vec3 newPoint = vec3(smoothedTubesData[globalID*3],smoothedTubesData[globalID*3+1],smoothedTubesData[globalID*3+2]);
        vec3 oriPoint = vec3(tempTubesData[globalID*3],tempTubesData[globalID*3+1],tempTubesData[globalID*3+2]);
		
		//vec3 finalPoint = (1-relaxFactor)*oriPoint + relaxFactor*newPoint;
		vec3 finalPoint = (1-relaxFactor)*oriPoint + relaxFactor*newPoint;
		relaxedTubesData[globalID*3] = finalPoint.x;
		relaxedTubesData[globalID*3+1] = finalPoint.y;
		relaxedTubesData[globalID*3+2] = finalPoint.z;
		
		//DEBUG
		//vec3 debug = relaxFactor*newPoint;
		//relaxedTubesData[globalID*3] = debug.x;
		//relaxedTubesData[globalID*3+1] = debug.y;
		//relaxedTubesData[globalID*3+2] = debug.z;
    }

}