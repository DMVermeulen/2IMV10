#version 430

layout (local_size_x = 128, local_size_y = 1, local_size_z = 1) in;

layout(binding = 0) buffer tempTracks {
    float tempTracksData[];
};

layout(binding = 1) buffer smoothedTracks {
    float smoothedTracksData[];
};

layout(binding = 2) buffer relaxedTracks {
    float relaxedTracksData[];
};

uniform int totalSize;
uniform float relaxFactor;


void main() {
    // Get global thread ID
    int globalID = int(gl_GlobalInvocationID.x);

    // Check if global ID is within the valid range of the buffer
    if (globalID < totalSize) {		
		vec3 newPoint = vec3(smoothedTracksData[globalID*3],smoothedTracksData[globalID*3+1],smoothedTracksData[globalID*3+2]);
        vec3 oriPoint = vec3(tempTracksData[globalID*3],tempTracksData[globalID*3+1],tempTracksData[globalID*3+2]);
		
		vec3 finalPoint = (1-relaxFactor)*oriPoint + relaxFactor*newPoint;
		relaxedTracksData[globalID*3] = finalPoint.x;
		relaxedTracksData[globalID*3+1] = finalPoint.y;
		relaxedTracksData[globalID*3+2] = finalPoint.z;
    }

}