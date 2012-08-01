This tone mapping example shows:
1. how the CLU generator handles header files that are #included by .cl files,
   in this case a shared header file between .cl and .cpp
2. how to use cluNDRange2() to help launch a 2-dimensional kernel with a specific
   workgroup size
3. how CLU could be used in a "utility" manner -- CLU is initialized to
   do a device query, then released, then later initialized again for computation
