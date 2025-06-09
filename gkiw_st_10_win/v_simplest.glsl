#version 330


//Zmienne jednorodne
uniform mat4 P;
uniform mat4 V;
uniform mat4 M;
uniform vec4 lp1;
uniform vec4 lp2;



//Atrybuty
in vec4 vertex; //wspolrzedne wierzcholka w przestrzeni modelu
//in vec4 color; //kolor wierzchołka
in vec4 normal; //wektor normalny wierzchołka w przestrzeni modelu
in vec2 texCoord0;


out vec4 n;
out vec4 v;
out vec2 iTexCoord0;

out vec4 worldVertex;

void main(void) {
    worldVertex = M * vertex;

    n = normalize(V * M * normal);//znormalizowany wektor normalny w przestrzeni oka
    v = normalize(vec4(0, 0, 0, 1) - V * worldVertex ); //Wektor do obserwatora w przestrzeni oka
    
    iTexCoord0 = texCoord0;

    gl_Position=P*V*M*vertex;
    
}
