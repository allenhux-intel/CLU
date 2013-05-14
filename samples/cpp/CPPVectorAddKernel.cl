kernel void vectorAdd(global const int *inputA, global const int *inputB, global int *output)
{
	output[get_global_id(0)] = inputA[get_global_id(0)] + inputB[get_global_id(0)];
}