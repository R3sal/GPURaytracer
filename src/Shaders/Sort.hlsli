
struct CSInput
{
	uint3 GroupID : SV_GroupID;
	uint3 GroupThreadID : SV_GroupThreadID;
	uint3 GlobalThreadID : SV_DispatchThreadID;
	uint GroupThreadIndex : SV_GroupIndex;
};

struct SortInfo
{
	uint NumElements;
	uint SortPassIndex;
};