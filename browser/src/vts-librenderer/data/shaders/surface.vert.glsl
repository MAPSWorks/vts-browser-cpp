
uniform mat4 uniP;
uniform mat4 uniMv;
uniform mat3 uniUvMat;
uniform vec4 uniUvClip;
uniform ivec4 uniFlags; // mask, monochromatic, flat shading, uv source

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inUvInternal;
layout(location = 2) in vec2 inUvExternal;

out vec2 varUvTex;
out vec3 varViewPosition;

out highp float gl_ClipDistance[4];

void main()
{
    gl_ClipDistance[0] = (inUvExternal[0] - uniUvClip[0]) * +1.0;
    gl_ClipDistance[1] = (inUvExternal[1] - uniUvClip[1]) * +1.0;
    gl_ClipDistance[2] = (inUvExternal[0] - uniUvClip[2]) * -1.0;
    gl_ClipDistance[3] = (inUvExternal[1] - uniUvClip[3]) * -1.0;

    vec4 vp = uniMv * vec4(inPosition, 1.0);
    gl_Position = uniP * vp;
    varUvTex = vec2(uniUvMat * vec3(uniFlags.w > 0 ? inUvExternal
                                                   : inUvInternal, 1.0));
    varViewPosition = vp.xyz;
}

