/*
*
* Vertex Shader --> Geometry Shader --> Pixel Shader
* entrypoint 'main' and profile are set as parameters in VS 13
*/

#include "Linetext.hlsli"

/* define point field as multiplication factors for du and dv around origin o, like this:

x x x     (-du,-2dv) ...
x x x     (-du, -dv) ...
x o x  == .
x x x     .
x x x     .

*/

static const float2 xpoints[] = {
	{ -1,  2 }, { 0,  2 }, { 1,  2 },
	{ -1,  1 }, { 0,  1 }, { 1,  1 },
	{ -1,  0 }, { 0,  0 }, { 1,  0 },
	{ -1, -1 }, { 0, -1 }, { 1, -1 },
	{ -1, -2 }, { 0, -2 }, { 1, -2 }
};

struct letter_el {
	int lines[14];
	int num;
};

static const letter_el letters[] = {
	{ { 0, 2, 2, 14, 14, 12, 12, 0, 0, 0, 0, 0, 0, 0 }, 8 },	// 0
	{ { 1, 13, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, 2 },		// 1
	{ { 0, 2, 2, 8, 8, 6, 6, 12, 12, 14, 0, 0, 0, 0 }, 10 },	// 2
	{ { 0, 2, 2, 8, 8, 6, 8, 14, 12, 14, 0, 0, 0, 0 }, 10 },	// 3
	{ { 0, 6, 6, 8, 4, 13, 0, 0, 0, 0, 0, 0, 0, 0 }, 6 },		// 4
	{ { 0, 2, 0, 6, 6, 8, 8, 14, 12, 14, 0, 0, 0, 0 }, 10 },	// 5
	{ { 0, 2, 0, 6, 6, 8, 8, 14, 12, 14, 6, 12, 0, 0 }, 12 },	// 6
	{ { 0, 2, 2, 12, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, 4 },		// 7
	{ { 0, 2, 0, 6, 6, 8, 8, 14, 12, 14, 6, 12, 2, 8 }, 14 },	// 8
	{ { 0, 2, 0, 6, 6, 8, 8, 14, 12, 14, 2, 8, 0, 0 }, 12 },	// 9
	{ { 1, 6, 1, 8, 6, 8, 6, 12, 8, 14, 0, 0, 0, 0 }, 10 },		// A
	//{ { 0, 1, 1, 5, 5, 11, 11, 13, 13, 12, 0, 12, 7, 8 }, 14 },	// B
	{ { 0, 1, 1, 5, 5, 7, 7, 11, 11, 13, 13, 12, 12, 0 }, 14 },	// B
	{ { 0, 2, 0, 12, 14, 12, 0, 0, 0, 0, 0, 0, 0, 0 }, 6 },		// C
	{ { 0, 1, 1, 5, 5, 11, 11, 13, 13, 12, 0, 12, 0, 0 }, 12 },	// D
	{ { 0, 2, 0, 6, 8, 6, 6, 12, 12, 14, 0, 0, 0, 0 }, 10 },	// E
	{ { 0, 2, 0, 6, 8, 6, 6, 12, 0, 0, 0, 0, 0, 0 }, 8 },		// F
	{ { 0, 2, 0, 12, 12, 14, 14, 8, 8, 7, 0, 0, 0, 0 }, 10 },	// G
	{ { 0, 12, 2, 14, 6, 8, 0, 0, 0, 0, 0, 0, 0, 0 }, 6 },		// H
	{ { 1, 13, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, 2 },		// I
	{ { 2, 11, 11, 13, 13, 13, 0, 0, 0, 0, 0, 0, 0, 0 }, 6 },	// J
	{ { 0, 12, 6, 2, 6, 14, 0, 0, 0, 0, 0, 0, 0, 0 }, 6 },		// K
	{ { 0, 12, 12, 14, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, 4 },		// L
	{ { 0, 12, 0, 10, 10, 2, 2, 14, 0, 0, 0, 0, 0, 0 }, 8 },	// M
	{ { 0, 12, 0, 14, 14, 2, 0, 0, 0, 0, 0, 0, 0, 0 }, 6 },		// N
	{ { 3, 1, 1, 5, 5, 11, 11, 13, 13, 9, 9, 3, 0, 0 }, 12 },	// O
	{ { 0, 2, 2, 8, 8, 6, 0, 12, 0, 0, 0, 0, 0, 0 }, 8 },		// P
	{ { 3, 1, 1, 5, 5, 11, 11, 13, 13, 9, 9, 3, 14, 10 }, 14 },	// Q
	{ { 12, 0, 0, 2, 2, 8, 8, 7, 7, 14, 0, 0, 0, 0 }, 10 },		// R
	{ { 0, 2, 0, 6, 6, 8, 8, 14, 12, 14, 0, 0, 0, 0 }, 10 },	// S (same as 5)
	{ { 0, 2, 1, 13, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, 4 },		// T
	{ { 0, 12, 12, 14, 14, 2, 0, 0, 0, 0, 0, 0, 0, 0 }, 6 },	// U
	{ { 0, 9, 9, 13, 13, 11, 11, 2, 0, 0, 0, 0, 0, 0 }, 8 },	// V
	{ { 0, 12, 12, 4, 4, 14, 14, 2, 0, 0, 0, 0, 0, 0 }, 8 },	// W
	{ { 0, 14, 2, 12, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, 4 },		// X
	{ { 0, 7, 7, 2, 7, 13, 0, 0, 0, 0, 0, 0, 0, 0 }, 6 },		// Y
	{ { 0, 2, 2, 12, 12, 14, 0, 0, 0, 0, 0, 0, 0, 0 }, 6 },		// Z
	{ { 6, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, 2 },		// -
	{ { 3, 8, 9, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, 4 },		// >
	{ { 9, 10, 10, 13, 13, 12, 12, 9, 0, 0, 0, 0, 0, 0 }, 8 },	// .
};

// draw a letter in the xy plane
void draw_letter_xy_plane(in float4 pos, in unsigned int ch, in int charpos, in int rotIndex, inout LineStream< GSOutput > output)
{
	float4x4 wvp = cbv.wvp;
	//int rotIndex = asuint(input[0].info.x);
	float4x4 rotList[4];
	rotList[0] = cbv.rot_xy;
	rotList[1] = cbv.rot_zy;
	rotList[2] = cbv.rot_yx;
	rotList[3] = cbv.rot_cs;
	float4x4 rot = rotList[rotIndex];
	float du = cbv.du;
	float dv = cbv.dv;
	GSOutput el;
	float4 addpos;
	// TODO
	//int indexesX[14] = letters[ch].lines;
	//int lenX = letters[ch].num;
	//[unroll]
	//for (int i = 0; i < lenX; i += 2)
	//{
	//	int pX = indexesX[i];
	//	float xtransX = charpos * 3 * du;
	//	addpos = float4(xtransX, 0, 0, 0);
	//	addpos.x += xpoints[pX].x * du;
	//	addpos.y = xpoints[pX].y * dv;
	//	addpos.w = 1;
	//	float4 endpos = float4(xtransX, 0, 0, 0);
	//	pX = indexesX[i+1];
	//	endpos.x += xpoints[pX].x * du;
	//	endpos.y = xpoints[pX].y * dv;
	//	endpos.w = 1;
	//	el.pos = mul(addpos, wvp);
	//	output.Append(el);
	//	el.pos = mul(endpos, wvp);
	//	output.Append(el);
	//	output.RestartStrip();
	//}
	//return;
	//
	int indexes[14] = letters[ch].lines;
	int len = letters[ch].num;
	float xtrans = charpos * 3 * du;
	[unroll]
	for (int i = 0; i < len; i += 2)
	{
		addpos = float4(xtrans, 0, 0, 0);
		int p = indexes[i];
		addpos.x += xpoints[p].x * du;
		addpos.y = xpoints[p].y * dv;
		addpos = mul(addpos, rot);
		addpos += pos;
		addpos.w = 1;
		el.pos = mul(addpos, wvp);
		output.Append(el);
		addpos = float4(xtrans, 0, 0, 0);
		p = indexes[i + 1];
		addpos.x += xpoints[p].x * du;
		addpos.y = xpoints[p].y * dv;
		addpos = mul(addpos, rot);
		addpos += pos;
		addpos.w = 1;
		el.pos = mul(addpos, wvp);
		output.Append(el);
		output.RestartStrip();
	}
}

[RootSignature(LinetextRS)]
[maxvertexcount(14)]
//[maxvertexcount(1024)]
void main(
	point TextElement input[1],
	inout LineStream< GSOutput > output
	)
{
	GSOutput el;
	float4 pos = input[0].pos;
	//draw_letter_xy_plane(pos, 1, 0, output);
	//draw_letter_xy_plane(pos, 0, 1, output);
	int letter = asuint(input[0].pos.w);
	int charpos = asuint(input[0].info.w);
	int rotIndex = asuint(input[0].info.x);
	pos.w = 1.0;
	draw_letter_xy_plane(pos, letter, charpos, rotIndex, output);
}


