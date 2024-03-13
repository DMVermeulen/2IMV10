// compute.cu
#include <cuda_runtime.h>
#include<device_launch_parameters.h>
#include<math_functions.h>
#include<vector_types.h>
#include<vector_functions.h>
#include"helper_math.h"
#include<device_atomic_functions.h>
#include<device_functions.h>
#include<sm_20_atomic_functions.h>
#include<sm_60_atomic_functions.h>

typedef unsigned int uint;

//Kernels 
//voxel count
__global__ void voxelCountKernel(
    float* oriTubesData,     //read from
    int* voxelCountData,     //write to
    int totalSize, int nVoxels_X, int nVoxels_Y, int nVoxels_Z, float3 aabbMin, float voxelUnitSize  //other parameters
) {
    int globalID = blockIdx.x * blockDim.x + threadIdx.x;

    if (globalID < totalSize) {
        float3 point = make_float3(oriTubesData[globalID * 3],
            oriTubesData[globalID * 3 + 1],
            oriTubesData[globalID * 3 + 2]);
        float3 deltaP = point - aabbMin;
        int level_X = min(nVoxels_X - 1, static_cast<int>(deltaP.x / voxelUnitSize));
        int level_Y = min(nVoxels_Y - 1, static_cast<int>(deltaP.y / voxelUnitSize));
        int level_Z = min(nVoxels_Z - 1, static_cast<int>(deltaP.z / voxelUnitSize));
        int index = nVoxels_X * nVoxels_Y * level_Z + nVoxels_X * level_Y + level_X;
        atomicAdd(&voxelCountData[index], 1);
    }
}

__global__ void densityEstimationKernal(
    int* voxelCountData,
    float* denseMapData,
    int totalSize, int nVoxels_X, int nVoxels_Y, int nVoxels_Z, int kernelR, float voxelUnitSize 
    ) {
    // Get global thread ID
    int globalID = blockIdx.x * blockDim.x + threadIdx.x;

    // Check if global ID is within the valid range of the buffer
    if (globalID < totalSize) {
        float dense = 0;
        int Z = globalID / (nVoxels_X * nVoxels_Y);
        int mod = globalID % (nVoxels_X * nVoxels_Y);
        int X = mod % nVoxels_X;
        int Y = mod / nVoxels_X;
        for (int dx = -kernelR; dx < kernelR; dx++) {
            for (int dy = -kernelR; dy < kernelR; dy++) {
                for (int dz = -kernelR; dz < kernelR; dz++) {
                    int nx = X + dx;
                    nx = max(nx, 0);
                    nx = min(nx, nVoxels_X - 1);
                    int ny = Y + dy;
                    ny = max(ny, 0);
                    ny = min(ny, nVoxels_Y - 1);
                    int nz = Z + dz;
                    nz = max(nz, 0);
                    nz = min(nz, nVoxels_Z - 1);
                    float diff_x = voxelUnitSize * (nx - X);
                    float diff_y = voxelUnitSize * (ny - Y);
                    float diff_z = voxelUnitSize * (nz - Z);
                    float dot_product = diff_x * diff_x + diff_y * diff_y + diff_z * diff_z;
                    float PR2 = kernelR * voxelUnitSize * kernelR * voxelUnitSize;
                    if (dot_product > PR2)
                        continue;

                    int index = nVoxels_X * nVoxels_Y * nz + nVoxels_X * ny + nx;
                    index = min(index, totalSize);
                    int pointCnt = voxelCountData[index];

                    dense += pointCnt * (1 - dot_product / PR2);
                }
            }
        }

        denseMapData[globalID] = dense;
    }
}

__global__ void advectionKernel(
    float* oriTubesData, float* tempNormalsData, float* denseMapData,  //read from
    float* updatedTubesData,                       //write to
    int totalSize, int nVoxels_X, int nVoxels_Y, int nVoxels_Z, int kernelR, float voxelUnitSize, float3 aabbMin, int totalVoxels
 ) {
    int globalID = blockIdx.x * blockDim.x + threadIdx.x;

    if (globalID < totalSize) {
        float diff = 0;
        int R2 = kernelR * kernelR;
        int kernelWidth = 2 * kernelR + 1;
        float3 grad = make_float3(0);

        float3 point = make_float3(oriTubesData[globalID * 3],
            oriTubesData[globalID * 3 + 1],
            oriTubesData[globalID * 3 + 2]);
        float3 deltaP = point - aabbMin;
        int X = min(static_cast<int>(deltaP.x / voxelUnitSize), nVoxels_X - 1);
        int Y = min(static_cast<int>(deltaP.y / voxelUnitSize), nVoxels_Y - 1);
        int Z = min(static_cast<int>(deltaP.z / voxelUnitSize), nVoxels_Z - 1);
        int indexA = nVoxels_X * nVoxels_Y * Z + nVoxels_X * Y + X;

        for (int dx = -kernelR; dx < kernelR; dx++) {
            for (int dy = -kernelR; dy < kernelR; dy++) {
                for (int dz = -kernelR; dz < kernelR; dz++) {
                    int nx = X + dx;
                    nx = max(nx, 0);
                    nx = min(nx, nVoxels_X - 1);
                    int ny = Y + dy;
                    ny = max(ny, 0);
                    ny = min(ny, nVoxels_Y - 1);
                    int nz = Z + dz;
                    nz = max(nz, 0);
                    nz = min(nz, nVoxels_Z - 1);

                    float3 dir = make_float3(dx, dy, dz);
                    float diffPos = dot(dir, dir) / R2;
                    if (diffPos < 1e-5)
                        continue;
                    if (diffPos > 1)
                        continue;
                    int indexB = nVoxels_X * nVoxels_Y * nz + nVoxels_X * ny + nx;
                    indexB = min(indexB, totalVoxels);
                    float diffDense = denseMapData[indexB] - denseMapData[indexA];

                    if (diffDense > diff) {
                        grad = normalize(dir) * diffDense * exp(-diffPos);
                        diff = diffDense;
                    }
                }
            }
        }

        float3 delta;
        if (dot(grad, grad) < 1e-5)
            delta = make_float3(0);
        else
            delta = kernelR * voxelUnitSize * normalize(grad) / 1.5;

        //float3 normal = make_float3(tempNormalsData[globalID * 3],
        //    tempNormalsData[globalID * 3 + 1],
        //    tempNormalsData[globalID * 3 + 2]);
        //delta = dot(normal, delta) * normal;

        updatedTubesData[globalID * 3] = oriTubesData[globalID * 3] + delta.x;
        updatedTubesData[globalID * 3 + 1] = oriTubesData[globalID * 3 + 1] + delta.y;
        updatedTubesData[globalID * 3 + 2] = oriTubesData[globalID * 3 + 2] + delta.z;
    }
}

__global__ void relaxationKernel(
    float* tempTubesData, float* smoothedTubesData,
    float* relaxedTubesData,
    int totalSize, float relaxFactor
    ) {
    int globalID = blockIdx.x * blockDim.x + threadIdx.x;

    if (globalID < totalSize) {
        float3 newPoint = make_float3(smoothedTubesData[globalID * 3],
            smoothedTubesData[globalID * 3 + 1],
            smoothedTubesData[globalID * 3 + 2]);
        float3 oriPoint = make_float3(tempTubesData[globalID * 3],
            tempTubesData[globalID * 3 + 1],
            tempTubesData[globalID * 3 + 2]);

        float3 finalPoint = (1 - relaxFactor) * oriPoint + relaxFactor * newPoint;
        relaxedTubesData[globalID * 3] = finalPoint.x;
        relaxedTubesData[globalID * 3 + 1] = finalPoint.y;
        relaxedTubesData[globalID * 3 + 2] = finalPoint.z;
    }
}

//called by host 
extern "C" void cuda_voxelCount(float* oriTubesData,     //read from
    int* voxelCountData,    //write to
    int totalSize, int nVoxels_X, int nVoxels_Y, int nVoxels_Z, float3 aabbMin, float voxelUnitSize
) {
    int blockSize = 128; // Choose your desired block size
    int gridSize = (totalSize + blockSize - 1) / blockSize;
    voxelCountKernel << <gridSize, blockSize >> > (oriTubesData, voxelCountData, totalSize, nVoxels_X, nVoxels_Y, nVoxels_Z, aabbMin, voxelUnitSize);
}

extern "C" void cuda_densityEstimation(
    int* voxelCountData,      //read from
    float* denseMapData,      //write to
    int totalSize, int nVoxels_X, int nVoxels_Y, int nVoxels_Z, int kernelR, float voxelUnitSize
) {
    int blockSize = 128; // Choose your desired block size
    int gridSize = (totalSize + blockSize - 1) / blockSize;
    densityEstimationKernal << <gridSize, blockSize >> > (voxelCountData, denseMapData, totalSize, nVoxels_X, nVoxels_Y, nVoxels_Z, kernelR, voxelUnitSize);
}

extern "C" void cuda_advection(
    float* oriTubesData, float* tempNormalsData, float* denseMapData,  //read from
    float* updatedTubesData,                       //write to
    int totalSize, int nVoxels_X, int nVoxels_Y, int nVoxels_Z, int kernelR, float voxelUnitSize, float3 aabbMin, int totalVoxels
) {
    int blockSize = 128; // Choose your desired block size
    int gridSize = (totalSize + blockSize - 1) / blockSize;
    advectionKernel << <gridSize, blockSize >> > (oriTubesData, tempNormalsData, denseMapData, updatedTubesData, totalSize, nVoxels_X, nVoxels_Y, nVoxels_Z, kernelR, voxelUnitSize, aabbMin, totalVoxels);
}

extern "C" void cuda_relaxation(
    float* tempTubesData, float* smoothedTubesData,
    float* relaxedTubesData,
    int totalSize, float relaxFactor
) {
    int blockSize = 128; // Choose your desired block size
    int gridSize = (totalSize + blockSize - 1) / blockSize;
    relaxationKernel << <gridSize, blockSize >> > (tempTubesData, smoothedTubesData, relaxedTubesData, totalSize, relaxFactor);
}
