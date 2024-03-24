#version 430 core

layout (local_size_x = 128, local_size_y = 1, local_size_z = 1) in;
layout (binding = 0) buffer tracks {
    float tracksData[];
};

layout (binding = 1) buffer lines {
    float linesData[];
};

layout (binding = 2) buffer isFiberBegin {
    int isFiberBeginData[];
};

layout (binding = 3) buffer isFiberEnd {
    int isFiberEndData[];
};

layout (binding = 4) buffer trackId {
    int trackIdData[];
};

uniform int totalSize;

void main() {
	int globalID = int(gl_GlobalInvocationID.x);
		
	if (globalID < totalSize) {
		if(1==isFiberBeginData[globalID]){
			int index = 2*globalID-2*trackIdData[globalID];
			linesData[3*index] = tracksData[3*globalID];
			linesData[3*index+1] = tracksData[3*globalID+1];
			linesData[3*index+2] = tracksData[3*globalID+2];
		}
		else if(1==isFiberEndData[globalID]){
			int index = 2*globalID-2*trackIdData[globalID]-1;
			linesData[3*index] = tracksData[3*globalID];
			linesData[3*index+1] = tracksData[3*globalID+1];
			linesData[3*index+2] = tracksData[3*globalID+2];
		}
		else {
			int index1 = 2*globalID-2*trackIdData[globalID]-1;
			int index2 = index1+1;
			
			linesData[3*index1] = tracksData[3*globalID];
			linesData[3*index1+1] = tracksData[3*globalID+1];
			linesData[3*index1+2] = tracksData[3*globalID+2];
			
			linesData[3*index2] = tracksData[3*globalID];
			linesData[3*index2+1] = tracksData[3*globalID+1];
			linesData[3*index2+2] = tracksData[3*globalID+2];
		}
	}
	
}