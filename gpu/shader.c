#include "shader.h"
#include <stdio.h>



const char *psx_vertex_shader_source =
    "#version 450 core\n"

    "layout(location = 0) in vec3 position;\n"
    "layout(location = 1) in vec4 color;\n"
    "layout(location = 2) in vec2 uv;\n"
    "layout(location = 3) in uint render_mode;\n"
    "layout(location = 4) in uint texture_layer;\n"
    "layout(location = 5) in uint palette_id;\n"

    "out vec4 vertex_color;\n"
    "noperspective out vec2 vertex_uv;\n"
    "flat out uint vertex_render_mode;\n"
    "flat out uint vertex_texture_layer;\n"
    "flat out uint vertex_palette_id;\n"

    "void main()\n"
    "{\n"
    "    gl_Position = vec4(position, 1.0);\n"

    "    vertex_color = color;\n"
    "    vertex_uv = uv;\n"
    "    vertex_render_mode = render_mode;\n"
    "    vertex_texture_layer = texture_layer;\n"
    "    vertex_palette_id = palette_id;\n"
    "}\n";



/*   Kwantyzacja koloru do 5 bitów (32 stopnie jasności, od 0 do 31)
     Dokładnie tak, jak wyświetlał to prawdziwy PSX! 
     Tylko dla RENDER_MODE = 0 */


const char *psx_fragment_shader_source =
    "#version 450 core\n"

    "in vec4 vertex_color;\n"
    "noperspective in vec2 vertex_uv;\n"
    "flat in uint vertex_render_mode;\n"
    "flat in uint vertex_texture_layer;\n"
    "flat in uint vertex_palette_id;\n"

    "layout(binding = 1) uniform usampler2DArray uIndexTex;\n"
    "layout(binding = 0) uniform sampler2D uPaletteTex;\n"

    "out vec4 FragColor;\n"

    "void main()\n"
    "{\n"

    "    if (vertex_render_mode == 0u)\n"
    "    {\n"
    "        vec3 psx_rgb = floor(vertex_color.rgb * 31.0) / 31.0;\n"
    "        FragColor = vec4(psx_rgb, vertex_color.a);\n"
    "    }\n"
    "    else if (vertex_render_mode == 1u)\n"
    "    {\n"
    "        FragColor = vertex_color;\n"
    "    }\n"
    "    else if (vertex_render_mode == 2u)\n"
    "    {\n"
    "        ivec2 tex_size = textureSize(uIndexTex, 0).xy;\n"

    "        ivec2 texel = ivec2(vertex_uv * vec2(tex_size));\n"
    "        texel = clamp(texel, ivec2(0), tex_size - ivec2(1));\n"

    "        uint index = texelFetch(\n"
    "            uIndexTex,\n"
    "            ivec3(texel, int(vertex_texture_layer)),\n"
    "            0\n"
    "        ).r;\n"

    "        FragColor = texelFetch(\n"
    "            uPaletteTex,\n"
    "            ivec2(int(index), int(vertex_palette_id)),\n"
    "            0\n"
    "        );\n"
    "    }\n"
   "    else if (vertex_render_mode == 3u)\n"
    "    {\n"
    "        ivec2 tex_size = textureSize(uIndexTex, 0).xy;\n"

    "        ivec2 texel = ivec2(vertex_uv * vec2(tex_size));\n"
    "        texel = clamp(texel, ivec2(0), tex_size - ivec2(1));\n"

    "        uint index = texelFetch(\n"
    "            uIndexTex,\n"
    "            ivec3(texel, int(vertex_texture_layer)),\n"
    "            0\n"
    "        ).r;\n"

    "        vec4 tex_color = texelFetch(\n"
    "            uPaletteTex,\n"
    "            ivec2(int(index), int(vertex_palette_id)),\n"
    "            0\n"
    "        );\n"

    "        vec3 v = vertex_color.rgb;\n"
    "        vec3 light = v * 2.0 + max(v - 0.5, 0.0) * 4.0;\n"
    "        vec3 lit_rgb = clamp(tex_color.rgb * light, 0.0, 1.0);\n"

    "        FragColor = vec4(lit_rgb, tex_color.a * vertex_color.a);\n"
    "    }\n"
    "}\n";

    const char *postprocess_vertex_shader_source =
    "#version 450 core\n"

    "out vec2 uv;\n"

    "void main()\n"
    "{\n"
    "    vec2 positions[4] = vec2[](\n"
    "        vec2(-1.0, -1.0),\n"
    "        vec2( 1.0, -1.0),\n"
    "        vec2(-1.0,  1.0),\n"
    "        vec2( 1.0,  1.0)\n"
    "    );\n"

    "    vec2 texcoords[4] = vec2[](\n"
    "        vec2(0.0, 0.0),\n"
    "        vec2(1.0, 0.0),\n"
    "        vec2(0.0, 1.0),\n"
    "        vec2(1.0, 1.0)\n"
    "    );\n"

    "    gl_Position = vec4(positions[gl_VertexID], 0.0, 1.0);\n"
    "    uv = texcoords[gl_VertexID];\n"
    "}\n";


const char *postprocess_fragment_shader_source =
    "#version 450 core\n"
    "\n"
    "layout(binding = 0) uniform sampler2D uScene;\n"
    "in vec2 uv;\n"
    "out vec4 FragColor;\n"
    "\n"
    "// --- KONFIGURACJA EFEKTU CRT ---\n"
    "const float CURVATURE         = 0.12;  // Zakrzywienie szkla (0.0 = plaski ekran)\n"
    "const float SCANLINE_COUNT    = 240.0; // Liczba linii w pionie (240p = klasyk PS1/PS2)\n"
    "const float SCANLINE_STRENGTH = 0.45;  // Ciemnosc przerw miedzy liniami (0.0 - 1.0)\n"
    "const float BLOOM_STRENGTH    = 0.85;  // Sila poswiaty jasnych elementow\n"
    "const float BLOOM_SPREAD      = 3.5;   // Promien rozmycia poswiaty\n"
    "const float MASK_STRENGTH     = 0.25;  // Widocznosc siatki pikseli RGB (0.0 - 1.0)\n"
    "const float VIGNETTE_STRENGTH = 0.25;  // Przyciemnienie rogow ekranu\n"
    "\n"
    "// Zakrzywienie kineskopu (barrel distortion)\n"
    "vec2 curveUV(vec2 coord)\n"
    "{\n"
    "    coord = coord * 2.0 - 1.0;\n"
    "    vec2 offset = abs(coord.yx) * CURVATURE;\n"
    "    coord = coord + coord * offset * offset;\n"
    "    return coord * 0.5 + 0.5;\n"
    "}\n"
    "\n"
    "void main()\n"
    "{\n"
    "    // 1. Zakrzywienie ekranu i czarna ramka poza kineskopem\n"
    "    vec2 crtUV = (CURVATURE > 0.0) ? curveUV(uv) : uv;\n"
    "    if (crtUV.x < 0.0 || crtUV.x > 1.0 || crtUV.y < 0.0 || crtUV.y > 1.0)\n"
    "    {\n"
    "        FragColor = vec4(0.0, 0.0, 0.0, 1.0);\n"
    "        return;\n"
    "    }\n"
    "\n"
    "    vec2 texSize = vec2(textureSize(uScene, 0));\n"
    "    vec2 texel = 1.0 / texSize;\n"
    "    vec3 color = texture(uScene, crtUV).rgb;\n"
    "\n"
    "    // 2. Prawdziwy szeroki Bloom (probkowanie promieniowe + krzywa Gaussa)\n"
    "    vec3 bloom = vec3(0.0);\n"
    "    float totalWeight = 0.0;\n"
    "    for (int x = -3; x <= 3; x++)\n"
    "    {\n"
    "        for (int y = -3; y <= 3; y++)\n"
    "        {\n"
    "            vec2 offset = vec2(x, y) * texel * BLOOM_SPREAD;\n"
    "            vec2 sampleUV = clamp(crtUV + offset, texel * 0.5, vec2(1.0) - texel * 0.5);\n"
    "            vec3 sampleCol = texture(uScene, sampleUV).rgb;\n"
    "\n"
    "            float luma = dot(sampleCol, vec3(0.2126, 0.7152, 0.0722));\n"
    "            float mask = smoothstep(0.25, 0.85, luma);\n"
    "            float weight = exp(-float(x * x + y * y) / 8.0);\n"
    "\n"
    "            bloom += sampleCol * mask * weight;\n"
    "            totalWeight += weight;\n"
    "        }\n"
    "    }\n"
    "    bloom /= totalWeight;\n"
    "    color += bloom * BLOOM_STRENGTH;\n"
    "\n"
    "    // 3. Scanlines (adaptacyjne - jasne piksele lekko puchna jak w prawdziwym CRT)\n"
    "    float luma = clamp(dot(color, vec3(0.299, 0.587, 0.114)), 0.0, 1.0);\n"
    "    float scanWave = sin(crtUV.y * SCANLINE_COUNT * 6.2831853);\n"
    "    float scanMask = 0.5 + 0.5 * scanWave;\n"
    "    // Jasne linie sa grubsze i zlewaja sie bardziej niz ciemne\n"
    "    float dynamicScan = mix(SCANLINE_STRENGTH, SCANLINE_STRENGTH * 0.35, luma);\n"
    "    color *= mix(1.0 - dynamicScan, 1.0, pow(scanMask, 1.5));\n"
    "\n"
    "    // 4. Maska kineskopu (Aperture Grille - pionowe paski R, G, B)\n"
    "    float subpixel = mod(gl_FragCoord.x, 3.0);\n"
    "    vec3 grille = vec3(1.0 - MASK_STRENGTH);\n"
    "    if (subpixel < 1.0)      grille.r = 1.0 + MASK_STRENGTH * 0.5;\n"
    "    else if (subpixel < 2.0) grille.g = 1.0 + MASK_STRENGTH * 0.5;\n"
    "    else                     grille.b = 1.0 + MASK_STRENGTH * 0.5;\n"
    "    color *= grille;\n"
    "\n"
    "    // 5. Winieta i lekka kompensacja jasnosci (zeby obraz nie byl za ciemny)\n"
    "    vec2 vigUV = crtUV * (1.0 - crtUV.yx);\n"
    "    float vig = pow(vigUV.x * vigUV.y * 15.0, VIGNETTE_STRENGTH);\n"
    "    color *= clamp(vig, 0.0, 1.0) * 1.15;\n"
    "\n"
    "    FragColor = vec4(color, 1.0);\n"
    "}\n";

unsigned int compile_shader(GLenum type, const char* src)
{
	unsigned int s = glCreateShader(type);
	int ok;

	glShaderSource(s,1,&src,NULL);
	glCompileShader(s);

	glGetShaderiv(s,GL_COMPILE_STATUS,&ok);

	if(!ok) {
		char log[1024];
		glGetShaderInfoLog(s,sizeof(log),NULL,log);
		printf("Shader Error:\n%s\n",log);
		printf("Compiling shader type: 0x%x\n", type);
	}	

	return s;
}

unsigned int create_psx_program(void)
{
	unsigned int vs = compile_shader(GL_VERTEX_SHADER,
					psx_vertex_shader_source);
	unsigned int fs = compile_shader(GL_FRAGMENT_SHADER, 
					psx_fragment_shader_source);

	unsigned int p = glCreateProgram();

	glAttachShader(p,vs);
	glAttachShader(p,fs);

	glLinkProgram(p);

	glDetachShader(p, vs);
	glDetachShader(p, fs);

	glDeleteShader(vs);
	glDeleteShader(fs);

	return p;
}


void shader_use_program(unsigned int program)
{
	glUseProgram(program);
}

unsigned int create_postprocess_program(void)
{
    unsigned int vs;
    unsigned int fs;
    unsigned int program;
    int status;

    vs = compile_shader(
        GL_VERTEX_SHADER,
        postprocess_vertex_shader_source
    );

    fs = compile_shader(
        GL_FRAGMENT_SHADER,
        postprocess_fragment_shader_source
    );

    if (vs == 0 || fs == 0) {
        if (vs != 0)
            glDeleteShader(vs);

        if (fs != 0)
            glDeleteShader(fs);

        return 0;
    }

    program = glCreateProgram();

    glAttachShader(program, vs);
    glAttachShader(program, fs);

    glLinkProgram(program);

    glGetProgramiv(program, GL_LINK_STATUS, &status);

    glDetachShader(program, vs);
    glDetachShader(program, fs);

    glDeleteShader(vs);
    glDeleteShader(fs);

    if (!status) {
        char log[1024];

        glGetProgramInfoLog(program, sizeof(log), NULL, log);
        printf("Postprocess program link error:\n%s\n", log);

        glDeleteProgram(program);
        return 0;
    }

    return program;
}
