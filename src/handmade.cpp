/*  +======| File Info |===============================================================+
    |                                                                                  |
    |     Subdirectory:  /src                                                          |
    |    Creation date:  Undefined                                                     |
    |    Last Modified:  11/28/2020 5:57:10 AM                                         |
    |                                                                                  |
    +=====================| Sayed Abid Hashimi, Copyright � All rights reserved |======+  */

#include "handmade.h"
#include "handmade_random.h"
#include "handmade_entity.h"
#include "handmade_sim_region.h"

// +====| Unity Build |====+
#include "handmade_world.cpp"
#include "handmade_sim_region.cpp"
#include "handmade_entity.cpp"

internal void GameOutputSound(game_state *GameState, game_sound_output_buffer *SoundBuffer, int ToneHz)
{
	int16_t ToneVolume = 3000;
	int WavePeriod =SoundBuffer->SamplesPerSecond/ToneHz;

	int16_t *SampleOut = SoundBuffer->Samples;

	for (int SampleIndex = 0; 
			SampleIndex < SoundBuffer->SampleCountToOutput; 
			++SampleIndex) {
#if 0
		real32 SineValue = sinf(GameState->tSine);
		int16_t SampleValue = (int16_t)(SineValue * ToneVolume); 
#else
		int16_t SampleValue = 0;
#endif
		*SampleOut++ = SampleValue;
		*SampleOut++ = SampleValue;
#if 0
		GameState->tSine += 2.0f*PI32 * 1.0f / (real32)WavePeriod;
		if(GameState->tSine > 2.0f*PI32)
		{
			GameState->tSine -= 2.0f*PI32;
		}
#endif
	}
}

internal void
DrawRectangle(game_offscreen_buffer *Buffer, v2 vMin, v2 vMax, real32 R, real32 G, real32 B)
{
	int32_t MinX = RoundReal32ToInt32(vMin.X);
	int32_t MinY = RoundReal32ToInt32(vMin.Y);
	int32_t MaxX = RoundReal32ToInt32(vMax.X);
	int32_t MaxY = RoundReal32ToInt32(vMax.Y);

	if(MinX < 0)
	{
		MinX = 0;
	}
	if(MinY < 0)
	{
		MinY = 0;
	}
	if(MaxX > Buffer->Width)
	{
		MaxX = Buffer->Width;
	}
	if(MaxY > Buffer->Height)
	{
		MaxY = Buffer->Height;
	}

	uint32_t Color = (uint32_t)((RoundReal32ToUInt32(R * 255.0f) << 16) |
								(RoundReal32ToUInt32(G * 255.0f) << 8) |
								(RoundReal32ToUInt32(B * 255.0f) << 0));
	uint8_t *Row = ((uint8_t *)Buffer->Memory +
							   MinX*Buffer->BytesPerPixel +
							   MinY*Buffer->Pitch);

	for (int Y = MinY;
		 Y < MaxY;
		 ++Y)
	{
		uint32_t *Pixel = (uint32_t *)Row;
		for (int X = MinX;
			 X < MaxX;
			 ++X)
		{
			*Pixel++ = Color;
		}	
		Row += Buffer->Pitch;
	}
}

internal void
DrawBitmap(game_offscreen_buffer *Buffer, loaded_bitmap *Bitmap,
		   real32 RealX, real32 RealY,
		   real32 CAlpha = 1.0f)
{
	int32_t MinX = RoundReal32ToInt32(RealX);
	int32_t MinY = RoundReal32ToInt32(RealY);
	int32_t MaxX = MinX + Bitmap->Width;
	int32_t MaxY = MinY + Bitmap->Height;

	int32_t SourceOffsetX = 0;
	if(MinX < 0)
	{
		SourceOffsetX = -MinX;
		MinX = 0;
	}

	int32_t SourceOffsetY = 0;
	if(MinY < 0)
	{
		SourceOffsetY = -MinY;
		MinY = 0;
	}
	if(MaxX > Buffer->Width)
	{
		MaxX = Buffer->Width;
	}
	if(MaxY > Buffer->Height)
	{
		MaxY = Buffer->Height;
	}

	// TODO SourceRow needs to be changed for clipping
	uint32_t *SourceRow = Bitmap->Pixels + Bitmap->Width*(Bitmap->Height - 1);
	SourceRow += -SourceOffsetY*Bitmap->Width + SourceOffsetX;

	uint8_t *DestRow = ((uint8_t *)Buffer->Memory +
								   MinX*Buffer->BytesPerPixel +
								   MinY*Buffer->Pitch);

	for(int Y = MinY;
		Y < MaxY;
		++Y)
	{
		uint32_t *Dest = (uint32_t *)DestRow;
		uint32_t *Source = SourceRow;
		for(int X = MinX;
			X < MaxX;
			++X)
		{
			real32 A = (real32)((*Source >> 24) & 0xFF) / 255.0f;
			A *= CAlpha;
			real32 SR = (real32)((*Source >> 16) & 0xFF);
			real32 SG = (real32)((*Source >> 8) & 0xFF);
			real32 SB = (real32)((*Source >> 0) & 0xFF);

			real32 DR = (real32)((*Dest >> 16) & 0xFF);
			real32 DG = (real32)((*Dest >> 8) & 0xFF);
			real32 DB = (real32)((*Dest >> 0) & 0xFF);

			// TODO Premultiplied alpha
			real32 R = (1.0f - A)*DR + A*SR;
			real32 G = (1.0f - A)*DG + A*SG;
			real32 B = (1.0f - A)*DB + A*SB;

			*Dest = (((uint32_t)(R + 0.5f) << 16) |
					   ((uint32_t)(G + 0.5f) << 8) |
					   ((uint32_t)(B + 0.5f) << 0));
			++Dest;
			++Source;
		}

		DestRow += Buffer->Pitch;
		SourceRow -= Bitmap->Width;
	}
}

#pragma pack(push, 1)
struct bitmap_header
{
	uint16_t FileType; 
	uint32_t FileSize;
	uint16_t Reserved1;
	uint16_t Reserved2;
	uint32_t BitmapOffset;
	uint32_t Size; 
	int32_t Width;
	int32_t Height;
	uint16_t Planes;
	uint16_t BitsPerPixel;
	uint32_t Compression;
	uint32_t SizeOfBitmap;
	int32_t HorzResolution;
	int32_t VertResolution;
	uint32_t ColorsUsed;
	uint32_t ColorsImportant;

	uint32_t RedMask;
	uint32_t GreenMask;
	uint32_t BlueMask;
};
#pragma pack(pop)

internal loaded_bitmap
DEBUGLoadBMP(thread_context *Thread, debug_platform_read_entire_file *ReadEntireFile, char *Filename)
{
	loaded_bitmap Result = {};

	// NOTE Byte Order in memory is determined by the header itself, so we have to
	// read out the masks and convert the pixels.

	debug_read_file_result ReadResult = ReadEntireFile(Thread, Filename);
	if(ReadResult.ContentsSize != 0)
	{
		bitmap_header *Header = (bitmap_header *)ReadResult.Contents;
		uint32_t *Pixels = (uint32_t *)((uint8_t *)ReadResult.Contents + Header->BitmapOffset);
		Result.Pixels = Pixels;
		Result.Width = Header->Width;
		Result.Height = Header->Height;

		Assert(Header->Compression == 3);

		// NOTE BMP files van go both ways in terms of signage of the height;
		// height will be negative for top-down
		// Also there can be compression as well, so don't think this actual BMP loader.
		uint32_t RedMask = Header->RedMask;
		uint32_t GreenMask = Header->GreenMask;
		uint32_t BlueMask = Header->BlueMask;
		uint32_t AlphaMask = ~(RedMask | GreenMask | BlueMask);

		bit_scan_result RedScan = FindLeastSignificantSetBit(RedMask);
		bit_scan_result GreenScan = FindLeastSignificantSetBit(GreenMask);
		bit_scan_result BlueScan = FindLeastSignificantSetBit(BlueMask);
		bit_scan_result AlphaScan = FindLeastSignificantSetBit(AlphaMask);

		Assert(RedScan.Found);
		Assert(GreenScan.Found);
		Assert(BlueScan.Found);
		Assert(AlphaScan.Found);

		int32_t RedShift = 16 - (int32_t)RedScan.Index;
		int32_t GreenShift = 8 - (int32_t)GreenScan.Index;
		int32_t BlueShift = 0 - (int32_t)BlueScan.Index;
		int32_t AlphaShift = 24 - (int32_t)AlphaScan.Index;

		uint32_t *SourceDest = Pixels;
		for(int32_t Y = 0;
			Y < Header->Height;
			++Y)
		{
			for(int32_t X = 0;
				X < Header->Width;
				++X)
			{
				uint32_t C = *SourceDest;
#if 0
				*SourceDest++ = ((((C >> AlphaShift.Index) & 0xFF) << 24) |
								 (((C >> RedShift.Index) & 0xFF) << 16) |
								 (((C >> GreenShift.Index) & 0xFF) << 8) |
								 (((C >> BlueShift.Index) & 0xFF) << 0));
#else
				*SourceDest++ = (RotateLeft(C & RedMask, RedShift) |
								 RotateLeft(C & GreenMask, GreenShift) |
								 RotateLeft(C & BlueMask, BlueShift) |
								 RotateLeft(C & AlphaMask, AlphaShift));
#endif
			}
		}
	}

	return Result;
}

inline v2
GetCameraSpaceP(game_state *GameState, low_entity *EntityLow)
{
	world_difference Diff = Subtract(GameState->World, &EntityLow->P, &GameState->CameraP);
	v2 Result = Diff.dXY;

	return Result;
}

struct add_low_entity_result
{
	uint32_t LowIndex;
	low_entity *Low;
};

internal add_low_entity_result
AddLowEntity(game_state *GameState, entity_type Type, world_position P)
{
	Assert(GameState->LowEntityCount < ArrayCount(GameState->LowEntities));
	uint32_t EntityIndex = GameState->LowEntityCount++;

	low_entity *EntityLow = GameState->LowEntities + EntityIndex;
	*EntityLow = {};
	EntityLow->Sim.Type = Type;
	EntityLow->P = NullPosition();

	ChangeEntityLocation(&GameState->WorldArena, GameState->World, EntityIndex, EntityLow, P);

	add_low_entity_result Result;
	Result.Low = EntityLow;
	Result.LowIndex = EntityIndex;

	// TODO Do we need to have a begin/end paradigm for adding
	// entities so that they can brought into the high set when
	// they are added and when they are in the camera region.

	return Result;
}

internal void
InitHitpoints(low_entity * EntityLow, uint32_t HitPointCount)
{
	Assert(HitPointCount <= ArrayCount(EntityLow->Sim.HitPoint));
	EntityLow->Sim.HitPointMax = HitPointCount;
	for(uint32_t HitPointIndex = 0;
		HitPointIndex < EntityLow->Sim.HitPointMax;
		++HitPointIndex)
	{
		hit_point *HitPoint = EntityLow->Sim.HitPoint + HitPointIndex;
		HitPoint->Flags = 0;
		HitPoint->FilledAmount = HIT_POINT_SUB_COUNT;
	}
}

internal add_low_entity_result
AddSword(game_state *GameState)
{
	add_low_entity_result Entity  = AddLowEntity(GameState, EntityType_Sword, NullPosition());

	Entity.Low->Sim.Height = 0.5f;
	Entity.Low->Sim.Width = 1.0f;
	AddFlag(&Entity.Low->Sim, EntityFlag_Nonspatial);

	return Entity;
}

internal add_low_entity_result
AddPlayer(game_state *GameState)
{
	world_position P = GameState->CameraP;
	add_low_entity_result Entity = AddLowEntity(GameState, EntityType_Hero, P);

	Entity.Low->Sim.Height = 0.5f;
	Entity.Low->Sim.Width = 1.0f;
	AddFlag(&Entity.Low->Sim, EntityFlag_Collides);

	InitHitpoints(Entity.Low, 3);

	add_low_entity_result Sword = AddSword(GameState);
	Entity.Low->Sim.Sword.Index = Sword.LowIndex;

	if(GameState->CameraFollowingEntityIndex == 0)
	{
		GameState->CameraFollowingEntityIndex = Entity.LowIndex;
	}

	return Entity;
}

internal add_low_entity_result
AddFamiliar(game_state *GameState, uint32_t AbsTileX, uint32_t AbsTileY, uint32_t AbsTileZ)
{
	world_position P = ChunkPositionFromTilePosition(GameState->World, AbsTileX, AbsTileY, AbsTileZ);
	add_low_entity_result Entity  = AddLowEntity(GameState, EntityType_Familiar, P);

	Entity.Low->Sim.Height = 0.5f;
	Entity.Low->Sim.Width = 1.0f;
	AddFlag(&Entity.Low->Sim, EntityFlag_Collides);

	return Entity;
}

internal add_low_entity_result
AddMonstar(game_state *GameState, uint32_t AbsTileX, uint32_t AbsTileY, uint32_t AbsTileZ)
{
	world_position P = ChunkPositionFromTilePosition(GameState->World, AbsTileX, AbsTileY, AbsTileZ);
	add_low_entity_result Entity  = AddLowEntity(GameState, EntityType_Monstar, P);

	Entity.Low->Sim.Height = 0.5f;
	Entity.Low->Sim.Width = 1.0f;
	AddFlag(&Entity.Low->Sim, EntityFlag_Collides);

	InitHitpoints(Entity.Low, 3);

	return Entity;
}

internal add_low_entity_result
AddWall(game_state *GameState, uint32_t AbsTileX, uint32_t AbsTileY, uint32_t AbsTileZ)
{
	world_position P = ChunkPositionFromTilePosition(GameState->World, AbsTileX, AbsTileY, AbsTileZ);
	add_low_entity_result Entity = AddLowEntity(GameState, EntityType_Wall, P);

	Entity.Low->Sim.Height = GameState->World->TileSideInMeters;
	Entity.Low->Sim.Width = Entity.Low->Sim.Height;
	AddFlag(&Entity.Low->Sim, EntityFlag_Collides);

	return Entity;
}

inline void
PushPiece(entity_visible_piece_group *Group, loaded_bitmap *Bitmap,
		  v2 Offset, real32 OffsetZ, v2 Align, v2 Dim,
		  v4 Color, real32 EntityZC)
{
	Assert(Group->PieceCount < ArrayCount(Group->Pieces));
	entity_visible_piece *Piece = Group->Pieces + Group->PieceCount++;
	Piece->Bitmap = Bitmap;
	Piece->Offset = Group->GameState->MetersToPixels*V2(Offset.X, -Offset.Y) - Align;
	Piece->OffsetZ = Group->GameState->MetersToPixels*OffsetZ;
	Piece->EntityZC = EntityZC;
	Piece->R = Color.R;
	Piece->G = Color.G;
	Piece->B = Color.B;
	Piece->A = Color.A;
	Piece->Dim = Dim;
}

inline void
PushBitmap(entity_visible_piece_group *Group, loaded_bitmap *Bitmap,
		   v2 Offset, real32 OffsetZ, v2 Align, real32 Alpha = 1.0f, real32 EntityZC = 1.0f)
{
	PushPiece(Group, Bitmap, Offset, OffsetZ, Align, V2(0, 0), V4(1.0f, 1.0f, 1.0f, Alpha), EntityZC);
}

internal void
PushRect(entity_visible_piece_group *Group, v2 Offset, real32 OffsetZ,
		 v2 Dim, v4 Color, real32 EntityZC = 1.0f)
{
	PushPiece(Group, 0, Offset, OffsetZ, V2(0, 0), Dim, Color, EntityZC);
}

internal void
DrawHitpoints(sim_entity *Entity, entity_visible_piece_group *PieceGroup)
{
	if(Entity->HitPointMax >= 1)
	{
		v2 HealthDim = {0.2f, 0.2f};
		real32 SpacingX = 1.5f*HealthDim.X;
		v2 HitP = {-0.5f*(Entity->HitPointMax - 1)*SpacingX, -0.25f};
		v2 dHitP = {SpacingX, 0.0f};
		for(uint32_t HealthIndex = 0;
			HealthIndex < Entity->HitPointMax;
			++HealthIndex)
		{
			hit_point *HitPoint = Entity->HitPoint + HealthIndex;
			v4 Color = {1.0f, 0.0f, 0.0f, 1.0f};
			if(HitPoint->FilledAmount == 0)
			{
				Color = V4(0.2f, 0.2f, 0.2f, 1.0f);
			}

			PushRect(PieceGroup, HitP, 0, HealthDim, Color, 0.0f);
			HitP += dHitP;
		}
	}
}

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
	Assert((&Input->Controllers[0].Back - &Input->Controllers[0].Buttons[0]) ==
			(ArrayCount(Input->Controllers[0].Buttons) - 1));
	Assert(sizeof(game_state) <= Memory->PermanentStorageSize);

	game_state *GameState = (game_state *)Memory->PermanentStorage;
	if(!Memory->IsInitialized)
	{
		// NOTE Reserve the entity slot 0 for the NULL entity
		AddLowEntity(GameState, EntityType_Null, NullPosition());

		GameState->BackDrop = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile,
				"handmade_hero_legacy_art/early_data/test/test_background.bmp");

		GameState->Shadow = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile,
				"handmade_hero_legacy_art/early_data/test/test_hero_shadow.bmp");

		GameState->Tree = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile,
				"handmade_hero_legacy_art/early_data/test2/tree00.bmp");

		GameState->Sword = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile,
				"handmade_hero_legacy_art/early_data/test2/rock03.bmp");

		hero_bitmaps *Bitmap;

		Bitmap = GameState->HeroBitmaps;
		Bitmap-> Head = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "handmade_hero_legacy_art/early_data/test/test_hero_right_head.bmp");
		Bitmap->Cape = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "handmade_hero_legacy_art/early_data/test/test_hero_right_cape.bmp");
		Bitmap->Torso = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "handmade_hero_legacy_art/early_data/test/test_hero_right_torso.bmp");
		Bitmap->Align = V2(72 ,182);
		++Bitmap;

		Bitmap->Head = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "handmade_hero_legacy_art/early_data/test/test_hero_back_head.bmp");
		Bitmap->Cape = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "handmade_hero_legacy_art/early_data/test/test_hero_back_cape.bmp");
		Bitmap->Torso = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "handmade_hero_legacy_art/early_data/test/test_hero_back_torso.bmp");
		Bitmap->Align = V2(72 ,182);
		++Bitmap;

		Bitmap->Head = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "handmade_hero_legacy_art/early_data/test/test_hero_left_head.bmp");
		Bitmap->Cape = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "handmade_hero_legacy_art/early_data/test/test_hero_left_cape.bmp");
		Bitmap->Torso = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "handmade_hero_legacy_art/early_data/test/test_hero_left_torso.bmp");
		Bitmap->Align = V2(72 ,182);
		++Bitmap;

		Bitmap->Head = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "handmade_hero_legacy_art/early_data/test/test_hero_front_head.bmp");
		Bitmap->Cape = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "handmade_hero_legacy_art/early_data/test/test_hero_front_cape.bmp");
		Bitmap->Torso = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "handmade_hero_legacy_art/early_data/test/test_hero_front_torso.bmp");
		Bitmap->Align = V2(72 ,182);
		++Bitmap;

		InitializeArena(&GameState->WorldArena, Memory->PermanentStorageSize - sizeof(game_state),
						(uint8_t *)Memory->PermanentStorage + sizeof(game_state));

		GameState->World = PushStruct(&GameState->WorldArena, world);
		world *World = GameState->World;

		InitializeWorld(World, 1.4f);

		uint32_t RandomNumberIndex = 0;
		uint32_t TilesPerWidth = 17;
		uint32_t TilesPerHeight = 9;
		uint32_t ScreenBaseX = 0;
		uint32_t ScreenBaseY = 0;
		uint32_t ScreenBaseZ = 0;
		uint32_t ScreenX = ScreenBaseX;
		uint32_t ScreenY = ScreenBaseY;
		uint32_t AbsTileZ = ScreenBaseZ;

		bool32 DoorLeft = false;
		bool32 DoorRight = false;
		bool32 DoorTop = false;
		bool32 DoorBottom = false;
		bool32 DoorUp = false;
		bool32 DoorDown = false;
		for(uint32_t ScreenIndex = 0;
			ScreenIndex < 2000;
			++ScreenIndex)
		{
			Assert(RandomNumberIndex < ArrayCount(RandomNumberTable));
			uint32_t RandomChoice;
			if (DoorUp || DoorDown)
			{
				RandomChoice = RandomNumberTable[RandomNumberIndex++] % 2;
			}
			else
			{
				RandomChoice = RandomNumberTable[RandomNumberIndex++] % 3;
			}

			bool32 CreatedZDoor = false;
			if(RandomChoice == 2)
			{
				CreatedZDoor = true;
				if(AbsTileZ == ScreenBaseZ)
				{
					DoorUp = true;
				}
				else
				{
					DoorDown = true;
				}
			}
			else if(RandomChoice == 1)
			{
				DoorRight = true;
			}
			else
			{
				DoorTop = true;
			}

			for(uint32_t TileY = 0;
				TileY < TilesPerHeight;
				++TileY)
			{
				for(uint32_t TileX = 0;
					TileX < TilesPerWidth;
					++TileX)
				{
					uint32_t AbsTileX = ScreenX*TilesPerWidth + TileX;
					uint32_t AbsTileY = ScreenY*TilesPerHeight + TileY;

					uint32_t TileValue = 1;
					if((TileX == 0) && (!DoorLeft || (TileY != (TilesPerHeight/2))))
					{
						TileValue = 2;
					}
					if((TileX == (TilesPerWidth - 1)) && (!DoorRight || (TileY != (TilesPerHeight/2))))
					{
						TileValue = 2;
					}

					if((TileY == 0) && (!DoorBottom || (TileX != (TilesPerWidth/2))))
					{
						TileValue = 2;
					}
					if((TileY == TilesPerHeight - 1) && (!DoorTop || (TileX != (TilesPerWidth/2))))
					{
						TileValue = 2;
					}
					if((TileX == 10) && (TileY == 6))
					{
						if(DoorUp)
						{
							TileValue = 3;
						}
						else if(DoorDown)
						{
							TileValue = 4;
						}
					}

					if(TileValue == 2)
					{
						AddWall(GameState, AbsTileX, AbsTileY, AbsTileZ);
					}
				}
			}

			DoorLeft = DoorRight;
			DoorBottom = DoorTop;

			if(CreatedZDoor)
			{
				DoorDown = !DoorDown;
				DoorUp = !DoorUp;
			}
			else
			{
				DoorUp = false;
				DoorDown = false;
			}

			DoorRight = false;
			DoorTop = false;

			if(RandomChoice == 2)
			{
				if(AbsTileZ == ScreenBaseZ)
				{
					AbsTileZ = ScreenBaseZ + 1;
				}
				else
				{
					AbsTileZ = ScreenBaseZ;
				}
			}
			else if(RandomChoice == 1)
			{
				ScreenX += 1;
			}
			else
			{
				ScreenY += 1;
			}
		}
#if 0
		while(GameState->LowEntityCount < ArrayCount(GameState->LowEntities) - 16)
		{
			uint32_t Coordinate = 1024 + GameState->LowEntityCount;
			AddWall(GameState, Coordinate, Coordinate, Coordinate);
		}
#endif
		world_position NewCameraP = {};

		int32_t CameraTileX = ScreenBaseX*TilesPerWidth + 17/2;
		int32_t CameraTileY = ScreenBaseY*TilesPerHeight + 9/2;
		int32_t CameraTileZ = ScreenBaseZ;

		NewCameraP = ChunkPositionFromTilePosition(GameState->World, 
												   CameraTileX,
												   CameraTileY,
												   CameraTileZ);
		GameState->CameraP = NewCameraP;

		AddMonstar(GameState, CameraTileX+2, CameraTileY+2, CameraTileZ);
		AddFamiliar(GameState, CameraTileX-2, CameraTileY+2, CameraTileZ);

		Memory->IsInitialized = true;
	}

	world *World = GameState->World;

	int32_t TileSideInPixels = 60;
	GameState->MetersToPixels = (real32)TileSideInPixels / (real32)World->TileSideInMeters;
	real32 MetersToPixels = GameState->MetersToPixels;

	for (int ControllerIndex = 0;
		 ControllerIndex < ArrayCount(Input->Controllers);
		 ++ControllerIndex)
	{
		game_controller_input *Controller = GetController(Input, ControllerIndex);
		controlled_hero *ConHero = GameState->ControlledHeroes + ControllerIndex;
		if(ConHero->EntityIndex == 0)
		{
			if(Controller->Start.EndedDown)
			{
				*ConHero = {};
 				ConHero->EntityIndex = AddPlayer(GameState).LowIndex;
			}
		}
		else
		{
			ConHero->dZ = 0.0f;
			ConHero->ddP = {};
			ConHero->dSword = {};

			if(Controller->IsAnalog)
			{
				ConHero->ddP = v2{Controller->StickAverageX, Controller->StickAverageY};
			} 
			else
			{
				// NOTE Digital input tunning
				if(Controller->MoveUp.EndedDown)
				{
					ConHero->ddP.Y = 1.0f;
				}
				if(Controller->MoveDown.EndedDown)
				{
					ConHero->ddP.Y = -1.0f;
				}
				if(Controller->MoveLeft.EndedDown)
				{
					ConHero->ddP.X = -1.0f;
				}
				if(Controller->MoveRight.EndedDown)
				{
					ConHero->ddP.X = 1.0f;
				}
			}

			if(Controller->Start.EndedDown)
			{
				ConHero->dZ = 4.0f;
			}

			v2 dSword = {};
			if(Controller->ActionUp.EndedDown)
			{
				ConHero->dSword = V2(0.0f, 1.0f);
			}
			if(Controller->ActionDown.EndedDown)
			{
				ConHero->dSword = V2(0.0f, -1.0f);
			}
			if(Controller->ActionLeft.EndedDown)
			{
				ConHero->dSword = V2(-1.0f, 0.0f);
			}
			if(Controller->ActionRight.EndedDown)
			{
				ConHero->dSword = V2(1.0f, 0.0f);
			}

		}
	}

	uint32_t TileSpanX = 17*3;
	uint32_t TileSpanY = 9*3;;
	rectangle2 CameraBounds = RectCenterDim(v2{0, 0}, World->TileSideInMeters*v2{(real32)TileSpanX, 
																				 (real32)TileSpanY});

	memory_arena SimArena;
	InitializeArena(&SimArena, Memory->TransientStorageSize, Memory->TransientStorage);
	sim_region *SimRegion = BeginSim(&SimArena, GameState, GameState->World, GameState->CameraP, CameraBounds);

	//
	// NOTE Render
	//

#if 1
	DrawRectangle(Buffer, V2(0, 0), V2((real32)Buffer->Width, (real32)Buffer->Height), 0.5f, 0.5f, 0.5f);
#else
	DrawBitmap(Buffer, &GameState->BackDrop, 0, 0);
#endif
	real32 ScreenCenterX = 0.5f * (real32)Buffer->Width;
	real32 ScreenCenterY = 0.5f * (real32)Buffer->Height;

	entity_visible_piece_group PieceGroup;
	PieceGroup.GameState = GameState;
	sim_entity *Entity = SimRegion->Entities;
	for(uint32_t EntityIndex = 0;
		EntityIndex < SimRegion->EntityCount;
		++EntityIndex, ++Entity)
	{
		if(Entity->Updatable)
		{
			PieceGroup.PieceCount = 0;
			real32 dt = Input->dtForFrame;

			// TODO this is incorrect, should be computed after update!!!
			real32 ShadowAlpha = 1.0f - 0.5f*Entity->Z;
			if(ShadowAlpha < 0)
			{
				ShadowAlpha = 0;
			}

			move_spec MoveSpec = DefaultMoveSpec();
			v2 ddP = {};

			hero_bitmaps *HeroBitmaps = &GameState->HeroBitmaps[Entity->FacingDirection];
			switch(Entity->Type)
			{
				case EntityType_Hero:
				{
					for(uint32_t ControlIndex = 0; 
						ControlIndex < ArrayCount(GameState->ControlledHeroes);
						++ControlIndex)
					{
						controlled_hero *ConHero = GameState->ControlledHeroes + ControlIndex;
						if(Entity->StorageIndex == ConHero->EntityIndex)
						{
							if(ConHero->dZ != 0.0f)
							{
								Entity->dZ = ConHero->dZ;
							}

							MoveSpec.UnitMaxAccelVector = true;
							MoveSpec.Speed = 50.0f;
							MoveSpec.Drag = 8.0f;
							ddP = ConHero->ddP;
							if((ConHero->dSword.X != 0.0f) || (ConHero->dSword.Y != 0.0f))
							{
								sim_entity *Sword = Entity->Sword.Ptr;
								if(Sword && IsSet(Sword, EntityFlag_Nonspatial))
								{
									Sword->DistanceRemaining = 5.0f;
									MakeEntitySpatial(Sword, Entity->P, 5.0f*ConHero->dSword);
								}
							}
						}
					}

					PushBitmap(&PieceGroup, &GameState->Shadow, V2(0, 0), 0, HeroBitmaps->Align, ShadowAlpha, 0.0f);
					PushBitmap(&PieceGroup, &HeroBitmaps->Torso, V2(0, 0), 0, HeroBitmaps->Align);
					PushBitmap(&PieceGroup, &HeroBitmaps->Cape, V2(0, 0), 0, HeroBitmaps->Align);
					PushBitmap(&PieceGroup, &HeroBitmaps->Head, V2(0, 0), 0, HeroBitmaps->Align);

					DrawHitpoints(Entity, &PieceGroup);

				} break;
				case EntityType_Wall:
				{
#if 0
					DrawRectangle(Buffer, PlayerLeftTop, PlayerLeftTop + 0.9f*EntityWidthHeight*MetersToPixels,
								  1.0f, 1.0f, 0.0f);
#endif
					PushBitmap(&PieceGroup, &GameState->Tree, V2(0, 0), 0, V2(40, 80));
				} break;

				case EntityType_Familiar:
				{
					sim_entity *ClosestHero = 0;
					real32 ClosestHeroDSq = Square(10.f);

					sim_entity *TestEntity = SimRegion->Entities;
					for(uint32_t TestEntityIndex = 0;
						TestEntityIndex < SimRegion->MaxEntityCount;
						++TestEntityIndex, ++TestEntity)
					{
						if(TestEntity->Type == EntityType_Hero)
						{
							real32 TestDSq = LengthSq(TestEntity->P - Entity->P);
							if(TestEntity->Type == EntityType_Hero)
							{
								TestDSq *= 0.75f;
							}
							if(ClosestHeroDSq > TestDSq)
							{
								ClosestHero = TestEntity;
								ClosestHeroDSq = TestDSq;
							}
						}
					}

					if(ClosestHero && (ClosestHeroDSq > Square(3.01f)))
					{
						// TODO PULL THE SPEED OUT OF MOVE ENTITY
						real32 Accleration = 0.5f;
						real32 OneOverLength = Accleration / SquareRoot(ClosestHeroDSq);
						ddP = OneOverLength*(ClosestHero->P - Entity->P);
					}

					MoveSpec.Speed = 50.0f;
					MoveSpec.UnitMaxAccelVector = true;
					MoveSpec.Drag = 8.0f;

					Entity->tBob += dt;
					if(Entity->tBob > (2.0f*Pi32))
					{
						Entity->tBob -= (2.0f*Pi32);
					}
					real32 BobSin = Sin(2.0f*Entity->tBob);

					PushBitmap(&PieceGroup, &GameState->Shadow, V2(0, 0), 0, HeroBitmaps->Align, (0.5f*ShadowAlpha) + 0.2f*BobSin, 0.0f);
					PushBitmap(&PieceGroup, &HeroBitmaps->Head, V2(0, 0), 0.25f*BobSin, HeroBitmaps->Align);
				} break;

				case EntityType_Sword:
				{
					MoveSpec.UnitMaxAccelVector = false;
					MoveSpec.Speed = 0.0f;
					MoveSpec.Drag = 0.0f;

					// TODO Add the ability in the collision routines to understand
					// a movement limit for an entity, and then update this routine
					// to use that to know when to kill the sword.
					v2 OldP = Entity->P;
					real32 DistanceTraveled = Length(Entity->P - OldP);

					Entity->DistanceRemaining -= DistanceTraveled;
					if(Entity->DistanceRemaining < 0.0f)
					{
						MakeEntityNonSpatial(Entity);
					}

					PushBitmap(&PieceGroup, &GameState->Shadow, V2(0, 0), 0, HeroBitmaps->Align, ShadowAlpha, 0.0f);
					PushBitmap(&PieceGroup, &GameState->Sword, V2(0, 0), 0, V2(29, 10));
				} break;
				case EntityType_Monstar:
				{
					PushBitmap(&PieceGroup, &GameState->Shadow, V2(0, 0), 0, HeroBitmaps->Align, ShadowAlpha, 0.0f);
					PushBitmap(&PieceGroup, &HeroBitmaps->Torso, V2(0, 0), 0, HeroBitmaps->Align);

					DrawHitpoints(Entity, &PieceGroup);
				} break;

				default:
				{
					InvalidCodePath;
				} break;
			}

			if(!IsSet(Entity, EntityFlag_Nonspatial))
			{
				MoveEntity(SimRegion, Entity, Input->dtForFrame, &MoveSpec, ddP);
			}

			real32 EntityGroundPointX = ScreenCenterX + MetersToPixels*Entity->P.X; 
			real32 EntityGroundPointY = ScreenCenterY - MetersToPixels*Entity->P.Y;
			real32 EntityZ = -MetersToPixels*Entity->Z;
#if 0
			v2 PlayerLeftTop = {PlayerGroundPointX - 0.5f*MetersToPixels*LowEntity->Width,
				PlayerGroundPointY - 0.5f*MetersToPixels*LowEntity->Height};
			v2 EntityWidthHeight = {LowEntity->Width, LowEntity->Height};
#endif
			for(uint32_t PieceIndex = 0;
				PieceIndex < PieceGroup.PieceCount;
				++PieceIndex)
			{
				entity_visible_piece *Piece = PieceGroup.Pieces + PieceIndex;
				v2 Center = {EntityGroundPointX + Piece->Offset.X,
					EntityGroundPointY + Piece->Offset.Y + Piece->OffsetZ + Piece->EntityZC*EntityZ};
				if(Piece->Bitmap)
				{
					DrawBitmap(Buffer, Piece->Bitmap, Center.X, Center.Y, Piece->A);
				}
				else
				{
					v2 HalfDim = 0.5f*MetersToPixels*Piece->Dim;
					DrawRectangle(Buffer, Center - HalfDim, Center + HalfDim, Piece->R, Piece->G, Piece->B);
				}
			}
		}
	}

	EndSim(SimRegion, GameState);
}

extern "C" GAME_GET_SOUND_SAMPLES(GameGetSoundSamples)
{
	game_state *GameState = (game_state *)Memory->PermanentStorage;
	GameOutputSound(GameState, SoundBuffer, 400);
}
