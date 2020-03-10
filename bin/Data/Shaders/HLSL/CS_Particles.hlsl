/*
    CS_Particles
    
    Unsorted compute-shader particles.
*/

#define MAX_WIND_ZONES 16
#define MAX_COLLIDERS 16

cbuffer ParticleSetup : register(b0)
{
    float4 origin;      // XYZ = position, W = radius if sphere emitter
    float4 viewPos;     // XYZ = position
    float4 viewDir;     // XYZ = normalized dir
    float4 viewUp;      // XYZ = normalized dir
    float4 uvCoords;    // unused, just fill up a vertex-buffer with it all
    float4 startColor;  // Color used at 0.0 life-fraction, unused if COLOR_RAMP
    float4 endColor;    // Color used at 1.0 life-fraction, unused if COLOR_RAMP
    
    float timeDelta;    // time-delta for this simulation step
    float lifeTime;     // total lifetime of a particle
    float fadeOffTime;  // duration of time to fade off the size so the particle cleanly lerps sizes to nothing instead of blipping out of existence
    float gravity;      // constant gravity factor
    
    float dampen;       // velocity dampening factor
    float setup_unused1;
    float setup_unused2;
    float setup_unused3;
    
    float2 startSize;   // starting particle width/height
    float2 endSize;     // final particle width/height
    
    float curlNoiseRange;  // cube side dimensions
    float curlPower;       // scaling factor for curl noise
    float noiseSampleSize; // should be 1 / width
    float globalWindSquash;   // with GLOBAL_WIND uses sin-cos wave
    
    float4 initialVelocity; // XYZ = optional, W = scale of random velocity
    float4 constantVelocity; // XYZ = magnitude, W = scale of sin/cose vertical fake wind
    
    // WARNING: must have space for particleCount + addParticles in the buffers, that's the total
    uint particleCount;     // used for indicating where new particles get inserted
    uint addParticles;      // set to the number of particles to add in this simulation
    uint frameSeed;         // multipled into numbers for rand
    uint setup_unused4;
}

cbuffer WindSetup : register(b1)
{
    uint windZoneCt;
    uint wind_unused1;
    uint wind_unused2;
    uint wind_unused3;
    float4 windDir[MAX_WIND_ZONES]; // XYZ magnitude, W = centroid power
    float4 windPosRadius[MAX_WIND_ZONES];
}

cbuffer ColliderSetup : register(b2)
{    
    uint colliderCt;
    uint col_unused1;
    uint col_unused2;
    uint col_unused3;
    float4 colliderPosRadius[MAX_COLLIDERS];  // XYZ position, W = radius
    // could add an extents array for capsule and box support instead just spheres, contact normals though
}

#if defined(PARTICLE_SIMULATE)

RWBuffer<float4> positions : register(u1);  // XYZ = pos, W = time
RWBuffer<float4> velocities : register(u2); // XYZ = vel, W = ??? could be used for a sort-key, or a seed for color ramp V-coord ???

Texture2D noiseTex : register(t3);
SamplerState noiseSamp : register(s3);

Texture3D curlTex : register(t4);
SamplerState curlSamp : register(s4);

inline float3 FitRange(float3 srcVec, float range)
{
    // rebase around origin
    srcVec -= origin.xyz;
    
    // note srcVec.x - (-range) == srcVec.x + range
    // normalize by -range to range, sampler's responsibility after this clamp/wrap/etc
    return float3(
        (srcVec.x+range) / (range*2),
        (srcVec.y+range) / (range*2),
        (srcVec.z+range) / (range*2),
    );
}

inline float Rand(uint seed)
{
    return noiseTex.Sample(noiseSamp, float2(seed * noiseSampleSize, 0.0)).x * 2.0 - 1.0;
}

inline float3 RandRange(uint seed)
{
    return float3(
        Rand(seed * 372728),
        Rand(seed * 10002),
        Rand(seed * 65746)
    );
}

inline float4 NewVelocity(uint seed)
{
    return float4(initialVelocity.xyz + RandRange(seed * frameSeed * 291) * initialVelocity.w, 0.0);
}

void SpawnParticle(uint slot)
{
    #ifdef EMITTER_POINT
        positions[slot] = float4(origin.xyz, 0.0 /* lifetime */);
    #endif
    #ifdef EMITTER_SPHERE
        positions[slot] = float4(origin.xyz + RandRange(slot * frameSeed) * origin.w, 0.0);
    #endif
    velocities[slot] = NewVelocity(slot * frameSeed);
}

[numthreads(32,1,1)]
void CS(uint3 dispatchThreadId : SV_DispatchThreadID)
{     
    uint totalParticles = particleCount + addParticles; 
    if (dispatchThreadId.x < totalParticles)
    {
        const uint slot = dispatchThreadId.x;
    
        if (dispatchThreadId.x >= particleCount)
            SpawnParticle(slot);
            
        float4 p = positions[slot];        
        float4 vel = velocities[slot];
        
        vel.xyz *= 1.0 - (dampen * timeDelta);
        vel.xyz += constantVelocity.xyz * timeDelta;
        vel.y -= gravity * timeDelta;
        
        #if defined(VELOCITY_FIELD) || defined(VELOCITY_FIELD_ADD) || defined(VELOCITY_FIELD_DIRECT)
            #ifdef VELOCITY_FIELD_ADD // accumulates
                vel.xyz += curlTex.Sample(curlSamp, FitRange(p.xyz)).xyz * curlPower * timeDelta;
            #elif defined(VELOCITY_FIELD_DIRECT) // only manipulates the direction of velocity
                vel.xyz = lerp(
                    vel.xyz,
                    normalize(curlTex.Sample(curlSamp, FitRange(p.xyz)).xyz) * length(vel.xyz),
                    curlPower);
            #else // explicit
                vel.xyz = curlTex.Sample(curlSamp, FitRange(p.xyz)).xyz * curlPower * timeDelta;
            #endif
        #endif
        
        #ifdef WIND
        // optionally add wind to velocity
        // wind comes after the velocity field
        for (uint w = 0; w < windZoneCt; ++w)
        {
            const float3 toWind = p.xyz - windPosRadius[w].xyz;
            const float dist = length(toWind);
            if (dist < windPosRadius[w].w)
            {
                const float falloff = 1.0 - (dist / windPosRadius[w].w);
                vel.xyz += windDir[w].xyz * falloff * timeDelta;
                vel.xyz += toWind * windDir[w].w * falloff * timeDelta; // centroid power, an explosion basically
            }
        }
        #endif
        
        // global wind sin-cos wave, min because we want to avoid lift
        vel.y += min(sin(p.x * globalWindSquash), sin(p.y * globalWindSquash)) * constantVelocity.w * timeStep;
        
        #ifdef COLLISION
        for (uint c = 0; c < colliderCt; ++c)
        {
            const float3 sep = p.xyz - colliderPosRadius[c].xyz;
            const float dist = length(sep);
            if (dist < colliderPosRadius[c].w)
            {
                vel.xyz = reflect(vel.xyz, sep);
                p.xyz -= normalize(sep) * (colliderPosRadius[c].w - dist);
            }
        }
        #endif
        
        p.xyz += vel.xyz;
        p.w += timeDelta;
        
        #ifdef RADIUS_CONTAINED // prevent leaving the radius, just halts right there
        const float3 offsetPos = p.xyz - origin.xyz;
        if (length(offsetPos) > origin.w)
            p.xyz = normalize(offsetPos) * origin.w + origin.xyz;
        #endif
        
        #ifdef PING_PONG_CONTAINED // if we would go outside the radius clip then flip velocity so we bounce around
        const float3 offsetPos = p.xyz - origin.xyz;
        if (length(offsetPos) > origin.w)
        {
            p.xyz = normalize(offsetPos) * origin.w + origin.xyz;
            vel.xyz = -vel.xyz;
        }
        #endif
        
        #ifdef DAMPEN_CONTAINED // as we get farther from the origin we shed velocity until coming to a halt
        const float3 offsetPos = p.xyz - origin.xyz;
        float distFrac = length(offsetPos) / origin.w;
        distFrac = 1.0 - distFrac*distFrac; // square it and flip
        vel.xyz *= distFrac;
        #endif
        
        positions[slot] = p;
        velocities[slot] = vel;
    }
}

#endif

#if defined(PARTICLE_EXPAND_GEOMETRY)

// Particles * 4
RWBuffer<float4> positions : register(u1);
// Particles * 4
RWBuffer<float4> colors : register(u2);
// Do we need normals?

#ifdef COLOR_RAMP
    // instead of lerping between color use a gradient-ramp
    Texture2D colorRamp : register(t0);
    SamplerState colorSamp : register(s0);
#endif

Buffer<float4> particlePositions : register(t3);
Buffer<float4> particleVelocities : register(t4);


[numthreads(32,1,1)]
void CS(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    const uint slot = dispatchThreadId.x;
    if (slot < particleCount)
    {        
        float4 p = particlePositions[slot];
        const float lifeFraction = p.w / lifeTime;
        
        #ifdef FACE_VIEW_DIR
            const float3 viewRight = normalize(cross(viewDir, viewUp));
            const float3 upDir = viewUp.xyz;
        #else // face view position
            float3 toViewer = normalize(p.xyz - viewPos.xyz);
            float3 viewUp = cross(toViewer, viewRight);
            float3 viewRight = cross(viewUp, toViewer);
        #endif
        // TODO: add velocity scaled tracer mode to the above
        
        float2 size = lerp(startSize, endSize, lifeFraction);
        float2 halfSize = size * 0.5;
        if (p.w > lifeTime)
        {
            // lerp towards 0 size instead of blinking out of existence
            const float deathFade = (p.w-lifeTime) / fadeOffTime;
            halfSize = lerp(halfSize, float2(0.0, 0.0), deathFade);
            size = lerp(size, float2(0.0, 0.0), deathFade);
        }
        
        #ifdef EXPAND_FROM_BOTTOM // simplifies grass
            // top-left
            positions[slot*4] = p - viewRight*halfSize.x + upDir*size.y;
            // top-right
            positions[slot*4 + 1] = p + viewRight*halfSize.x + upDir*size.y;
            // bottom-left
            positions[slot*4 + 2] = p - viewRight*halfSize.x;
            // bottom-right
            positions[slot*4 + 3] = p + viewRight*halfSize.x;
        #else
            // top-left
            positions[slot*4] = p - viewRight*halfSize.x + upDir*halfSize.y;
            // top-right
            positions[slot*4 + 1] = p + viewRight*halfSize.x + upDir*halfSize.y;
            // bottom-left
            positions[slot*4 + 2] = p - viewRight*halfSize.x - upDir*halfSize.y;
            // bottom-right
            positions[slot*4 + 3] = p + viewRight*halfSize.x - upDir*halfSize.y;
        #endif
        
        #ifdef COLOR_RAMP
            const float4 c = colorRamp.Sample(colorSamp, float2(lifeFraction, 0.0));
        #else
            const float4 c = lerp(startColor, endColor, lifeFraction);
        #endif
        
        colors[slot*4] = c;
        colors[slot*4 + 1] = c;
        colors[slot*4 + 2] = c;
        colors[slot*4 + 3] = c;
    }
    else
    {
        // leave other data as is, collapsing to zero is good enough
        positions[slot] = float4(0.0, 0.0, 0.0, 0.0);
    }
}

#endif
