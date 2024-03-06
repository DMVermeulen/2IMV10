#version 430

layout (local_size_x = 128, local_size_y = 1, local_size_z = 1) in;

layout(binding = 8) buffer relaxedTubes {
    float relaxedTubesData[];
};

layout(binding = 6) buffer isFiberEndpoint {
    int isFiberEndpointData[];
};

uniform int totalSize;


void main() {
    // Get global thread ID
    int globalID = int(gl_GlobalInvocationID.x);

    // Check if global ID is within the valid range of the buffer
    if (globalID < totalSize && globalID%2==0 && 0==isFiberEndpointData[globalID]) {		
		int preID = globalID-1;
		relaxedTubesData[globalID*3] = relaxedTubesData[preID*3];
		relaxedTubesData[globalID*3+1] = relaxedTubesData[preID*3+1];
		relaxedTubesData[globalID*3+2] = relaxedTubesData[preID*3+2];
    }

}