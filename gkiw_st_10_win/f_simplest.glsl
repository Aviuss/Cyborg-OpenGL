#version 330

uniform sampler2D textureMap0;
uniform sampler2D textureMap1;

in vec4 iC;
in vec4 l;
in vec4 n;
in vec4 v;
in vec2 iTexCoord0;
in vec2 iTexCoord1;

out vec4 pixelColor; //Zmienna wyjsciowa fragment shadera. Zapisuje sie do niej ostateczny (prawie) kolor piksela


void main(void) {
	vec4 kd = mix(texture(textureMap0, iTexCoord0), texture(textureMap1, iTexCoord1), 0);
	vec4 ks = vec4(1,1,1,1);

	vec4 ml = normalize(l);
	vec4 mn = normalize(n);
	vec4 mv = normalize(v);
	vec4 mr=reflect(-ml,mn); //Wektor odbity

	float nl = clamp(dot(mn, ml), 0, 1); //Kosinus kąta pomiędzy wektorami n i l.
	float rv = pow(clamp(dot(mr, mv), 0, 1), 25); // Kosinus kąta pomiędzy wektorami r i v podniesiony do 25 potęgi

	pixelColor = vec4(0.3, 0.3, 0.3, 1) + vec4(/*nl **/ 0.5* kd.rgb, kd.a) + vec4(ks.rgb*rv, 0); //Wyliczenie modelu oświetlenia (bez ambient);
	//pixelColor = vec4(1.0f, 1.0f, 0.0f, 1.0f);
}
