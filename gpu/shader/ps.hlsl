struct PSInput {
	float4 position: SV_POSITION;
	float2 texcoord: TEXCOORD;
	float4 mask: COLOR;
};

struct PSOutput {
	float4 color: SV_TARGET0;
};

Texture2D tex: register(t1);
SamplerState smplr: register(s0);

PSOutput main(PSInput input) {
	PSOutput output;

	output.color = input.mask * tex.Sample(smplr, input.texcoord).a;

	return output;
}
