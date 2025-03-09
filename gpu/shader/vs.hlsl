struct VSInput {
	float4 position: POSITION;
	float2 texcoord: TEXCOORD;
};

struct VSOutput {
	float4 position: SV_POSITION;
	float2 texcoord: TEXCOORD;
	float4 mask: COLOR;
};

struct Entity {
	float4 trans;
	float4 scale;
	float4 mask;
};
StructuredBuffer<Entity> entities: register(t0);

cbuffer Camera : register(b0) {
	float4x4 proj;
};

VSOutput main(VSInput input, uint instIdx: SV_InstanceID) {
	VSOutput output;

	output.position = input.position;
	output.position *= entities[instIdx].scale;
	output.position += entities[instIdx].trans;
	output.position = mul(proj, output.position);

	output.texcoord = input.texcoord;
	output.mask = entities[instIdx].mask;

	return output;
}
