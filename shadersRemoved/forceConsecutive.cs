#version 430

layout (local_size_x = 128, local_size_y = 1, local_size_z = 1) in;

layout(binding = 7) buffer smoothedTubes {
    float smoothedTubesData[];
};

layout(binding = 6) buffer isFiberEndpoint {
    int isFiberEndpointData[];
};

uniform int totalSize;


void main() {
    // Get global thread ID
    int globalID = int(gl_GlobalInvocationID.x);

    // Check if global ID is within the valid range of the buffer
    if ((globalID < totalSize) && (globalID%2==0) && (0==isFiberEndpointData[globalID])) {		
		int preID = globalID-1;
		smoothedTubesData[globalID*3] = smoothedTubesData[preID*3];
		smoothedTubesData[globalID*3+1] = smoothedTubesData[preID*3+1];
		smoothedTubesData[globalID*3+2] = smoothedTubesData[preID*3+2];
    }

}