/*
Copyright (c) 2012, Intel Corporation

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
    * Neither the name of Intel Corporation nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#define G_M1_SQUARED 1750.0f
#define MAX_VELOCITY 500.0f
#define MIN_DISTANCE_SQUARED 1.0f // no acceleration within this range

// compute gravitational force given particle position
float3 Acceleration(float3 in_pos)
{
    // just using distance from 0
    float dSquared = (in_pos.x * in_pos.x) + (in_pos.y * in_pos.y) + (in_pos.z * in_pos.z);
    if (dSquared < MIN_DISTANCE_SQUARED) return (float3)(0,0,0);
    // F = G*m1*m2 / (r*r), F/m1 = a, so a = G * M1 / (r*r)
    float a = G_M1_SQUARED / dSquared;
    // convert position into direction
    float3 accelVector = -normalize(in_pos);
    accelVector *= a;
    return accelVector;
}

float3 Velocity(float3 in_velocity, float3 in_accelVector, float in_deltaT)
{
    float3 dV = in_accelVector * in_deltaT;
    float3 velocity = in_velocity + dV;

    if (length(velocity) > MAX_VELOCITY)
    {
        velocity = normalize(velocity);
        velocity *= MAX_VELOCITY;
    }

    return velocity;
}

__kernel void Simulate(
    __global float4* in_points,     // array of positions per particle
    __global float4* in_velocities, // array of velocities per particle
    float in_range,
    float in_deltaT)
{
    unsigned int gid = get_global_id(0);

    float3 velocity = in_velocities[gid].xyz;
    float3 position = in_points[gid].xyz;

    // if a particle reaches the edge of the universe, redirect it.
    // the cross product of two positions in space forms a tangent to the
    // center of space. Bouncing that (~random) direction looks interesting.
    if (length(position) > in_range)
    {
        // direction from center to particle
        float3 fromCenter = normalize(position);
        // component of velocity in outward direction
        float vFromCenter = dot(fromCenter, velocity);

        unsigned int nid = gid + 1;
        if (get_global_size(0) <= nid)
        {
            nid = gid - 1;
        }
        float3 pos2 = in_points[nid].xyz;
        float3 dir = cross(fromCenter, normalize(pos2));
        velocity = normalize(dir) * length(velocity);
        velocity *= 0.995f; // add drag...

        // ... but preserve any accumulated acceleration toward center
        if (vFromCenter < 0)
        {
            velocity += vFromCenter * fromCenter;
        }

        //position = in_range * fromCenter; // clamp position to max range
    }

    // compute acceleration from gravity based on particle position
    float3 accelVector = Acceleration(position);
    // update velocity
    velocity = Velocity(velocity, accelVector, in_deltaT);
    // update position
    float3 deltaPos = (velocity * in_deltaT) + (0.5f * accelVector* in_deltaT * in_deltaT);
    position = position + deltaPos;

    in_velocities[gid] = (float4)(velocity, 0);

    float normalizedSpeed = length(velocity) / MAX_VELOCITY;
    in_points[gid] = (float4)(position, normalizedSpeed);
}
