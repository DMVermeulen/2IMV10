#version 430

#define FINITE 99999

layout (local_size_x = 128, local_size_y = 1, local_size_z = 1) in;

layout(binding = 0) buffer updated {
    float updatedData[];
};

uniform int totalSize;
uniform vec3 pos;
uniform vec3 dir;


void main() {
    int globalID = int(gl_GlobalInvocationID.x);

    if (globalID < totalSize) {	
        int id0 = 2*globalID;
        int id1 = id0+1;		
		vec3 point0 = vec3(updatedData[id0*3],updatedData[id0*3+1],updatedData[id0*3+2]);
        vec3 point1 = vec3(updatedData[id1*3],updatedData[id1*3+1],updatedData[id1*3+2]);
		
		bool isUp0 = dot(point0-pos,dir)>=0;
		bool isUp1 = dot(point1-pos,dir)>=0;
		
		if(isUp0 && isUp1){
		   updatedData[id0*3] = FINITE;
		   updatedData[id0*3+1] = FINITE;
		   updatedData[id0*3+2] = FINITE;
		   
		   updatedData[id1*3] = FINITE;
		   updatedData[id1*3+1] = FINITE;
		   updatedData[id1*3+2] = FINITE;
		}
		else if(!isUp0 && !isUp1){
		   updatedData[id0*3] = point0.x;
		   updatedData[id0*3+1] = point0.y;
		   updatedData[id0*3+2] = point0.z;
		   
		   updatedData[id1*3] = point1.x;
		   updatedData[id1*3+1] = point1.y;
		   updatedData[id1*3+2] = point1.z;
		}
		else{
		   //calculate intersecting point
		   float t = dot(pos-point0,dir)/dot(point1-point0,dir);
		   vec3 interPoint = point0+t*(point1-point0);
		   if(isUp0){
		        updatedData[id0*3] = interPoint.x;
		        updatedData[id0*3+1] = interPoint.y;
		        updatedData[id0*3+2] = interPoint.z;
				
			    updatedData[id1*3] = point1.x;
		        updatedData[id1*3+1] = point1.y;
		        updatedData[id1*3+2] = point1.z;
		   }
		   else if(isUp1){
		        updatedData[id0*3] = point0.x;
		        updatedData[id0*3+1] = point0.y;
		        updatedData[id0*3+2] = point0.z;
				
			    updatedData[id1*3] = interPoint.x;
		        updatedData[id1*3+1] = interPoint.y;
		        updatedData[id1*3+2] = interPoint.z;
		   }
		}
    }

}