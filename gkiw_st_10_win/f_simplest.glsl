#version 330

#define LIGHTSOURCES 51

uniform sampler2D textureMap0;
uniform sampler2D textureMap1;
uniform mat4 V;
uniform sampler2D textureMap2;

in vec4 iC;
in vec4 n;
in vec4 v;
in vec2 iTexCoord0;
in vec2 iTexCoord1;

uniform vec4 arrayLightsColor[LIGHTSOURCES ];
uniform vec4 arrayLights[LIGHTSOURCES];



uniform float fshift;

out vec4 pixelColor; //Zmienna wyjsciowa fragment shadera. Zapisuje sie do niej ostateczny (prawie) kolor piksela
in vec4 worldVertex;




// Fade function for smoothing
float fade(float t) {
    return t * t * t * (t * (t * 6.0 - 15.0) + 10.0);
}

vec2 fade(vec2 t) {
    return vec2(fade(t.x), fade(t.y));
}

// 2D hash function to get pseudorandom gradients
vec2 randomGradient(vec2 p) {
    float angle = fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453) * 6.28318;
    return vec2(cos(angle), sin(angle));
}

// Perlin Noise function
float perlin(vec2 p) {
    p += vec2(fshift*1, fshift*1);

    // Grid cell coordinates
    vec2 i0 = floor(p);
    vec2 f0 = fract(p);
    vec2 f = fade(f0);

    // Gradient vectors
    vec2 g00 = randomGradient(i0);
    vec2 g10 = randomGradient(i0 + vec2(1.0, 0.0));
    vec2 g01 = randomGradient(i0 + vec2(0.0, 1.0));
    vec2 g11 = randomGradient(i0 + vec2(1.0, 1.0));

    // Vectors from corners to point
    vec2 d00 = f0 - vec2(0.0, 0.0);
    vec2 d10 = f0 - vec2(1.0, 0.0);
    vec2 d01 = f0 - vec2(0.0, 1.0);
    vec2 d11 = f0 - vec2(1.0, 1.0);

    // Dot products
    float n00 = dot(g00, d00);
    float n10 = dot(g10, d10);
    float n01 = dot(g01, d01);
    float n11 = dot(g11, d11);

    // Bilinear interpolation
    float nx0 = mix(n00, n10, f.x);
    float nx1 = mix(n01, n11, f.x);
    float nxy = mix(nx0, nx1, f.y);

    return 0.5+0.5*nxy;
}

vec4 colorPixelByLightSource(vec4 lp, vec4 mn, vec4 mv, vec4 ks, vec4 kd, vec4 color, float lightDistance) {
    vec4 ml = normalize(lp);
	vec4 mr=reflect(-ml,mn); //Wektor odbity
    
	float nl = clamp(dot(mn, ml), 0, 1); //Kosinus kąta pomiędzy wektorami n i l1.
	float rv = pow(clamp(dot(mr, mv), 0, 1), 25); // Kosinus kąta pomiędzy wektorami r i v podniesiony do 25 potęgi
    
    lightDistance -= 0.55;

	return (vec4(nl * kd.rgb * color.rgb, kd.a) + vec4(ks.rgb*rv*color.rgb, 0)) / (0.04*lightDistance*lightDistance+0.1*lightDistance+0.5);
}

void main(void) {
    
    
    vec4 kd = texture(textureMap0, iTexCoord0);
	vec4 ks = texture(textureMap1, iTexCoord0);
	vec4 maskTexture = texture(textureMap2, iTexCoord0);
    
	vec4 mn = normalize(n);
	vec4 mv = normalize(v);

    
	pixelColor  = vec4(0.1 * kd.rgb, kd.a);//environmental lighting
    for (int i = 0; i < LIGHTSOURCES ; ++i) {
        pixelColor += colorPixelByLightSource(normalize(V* (arrayLights[i] - worldVertex )), mn, mv, ks, kd, arrayLightsColor[i], distance(arrayLights[i] , worldVertex));
    }

	// perlin mask for fluid like simulation
    float perlin_noise_frequency = 80;
    float perlin_val = perlin((iTexCoord0)*perlin_noise_frequency);
    
    vec4 fluidColor = vec4(0, perlin_val, 0, 0) + vec4(0,(1-perlin_val)*0.3,0,0);

	pixelColor = vec4(pixelColor.r * (1-maskTexture.r), pixelColor.g * (1-maskTexture.g), pixelColor.b * (1-maskTexture.b), pixelColor.a * (1-maskTexture.a) );
	pixelColor = pixelColor + vec4(fluidColor.r*maskTexture.r, fluidColor.g*maskTexture.g, fluidColor.b*maskTexture.b, fluidColor.a*maskTexture.a);
    
    //pixelColor = vec4(perlin_val, perlin_val, perlin_val, 1); // for testing
}
