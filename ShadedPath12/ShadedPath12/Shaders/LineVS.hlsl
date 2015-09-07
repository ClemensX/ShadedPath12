#define MyRS1 "RootFlags( ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT), " \
              "CBV(b0, space = 0), " \
              "DescriptorTable( CBV(b1)), "


[RootSignature(MyRS1)]

float4 main( float4 pos : POSITION ) : SV_POSITION
{
	return pos;
}