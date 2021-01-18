/*  +======| File Info |===============================================================+
    |                                                                                  |
    |     Subdirectory:  /src                                                          |
    |    Creation date:  1/4/2021 7:30:49 AM                                           |
    |    Last Modified:                                                                |
    |                                                                                  |
    +=====================| Sayed Abid Hashimi, Copyright � All rights reserved |======+  */

#if !defined(HANDMADE_RENDER_GROUP_H)

struct environment_map
{
	// NOTE (Khisrow): LOD[0] is 2^WidthPow2 x 2^HeightPow2
	uint32_t WidthPow2;
	uint32_t HeightPow2;

	loaded_bitmap *LOD[4];
};

struct render_basis
{
	v3 P;
};

struct render_entity_basis
{
	render_basis *Basis;
	v2 Offset;
	real32 OffsetZ;
	real32 EntityZC;
};

enum render_group_entry_type
{
	RenderGroupEntryType_render_entry_clear,
	RenderGroupEntryType_render_entry_bitmap,
	RenderGroupEntryType_render_entry_rectangle,
	RenderGroupEntryType_render_entry_coordinate_system,
};

struct render_group_entry_header
{
	render_group_entry_type Type;
};

struct render_entry_clear
{
	v4 Color;
};

struct render_entry_coordinate_system
{
	v2 Origin;
	v2 XAxis;
	v2 YAxis;
	v4 Color;
	loaded_bitmap *Texture;
	loaded_bitmap *NormalMap;

	environment_map *Top;
	environment_map *Middle;
	environment_map *Bottom;
};

struct render_entry_bitmap
{
	render_entity_basis EntityBasis;
	loaded_bitmap *Bitmap;
	real32 R, G, B, A;
};

struct render_entry_rectangle
{
	render_entity_basis EntityBasis;
	real32 R, G, B, A;
	v2 Dim;
};

struct render_group
{
	render_basis *DefaultBasis;
	real32 MetersToPixels;

	uint32_t MaxPushBufferSize;
	uint32_t PushBufferSize;
	uint8_t *PushBufferBase;
};

#define HANDMADE_RENDER_GROUP_H
#endif
