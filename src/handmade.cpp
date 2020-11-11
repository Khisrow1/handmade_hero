#include "handmade.h"
#include "handmade_tile.cpp"
#include "handmade_random.h"

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
		   int32_t AlignX = 0, int32_t AlignY = 0,
		   real32 CAlpha = 1.0f)
{
	RealX -= (real32)AlignX;
	RealY -= (real32)AlignY;

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

internal high_entity *
MakeEntityHighFrequency(game_state *GameState, uint32_t LowIndex)
{
	high_entity *EntityHigh = 0;

	low_entity *EntityLow = &GameState->LowEntities[LowIndex];
	if(EntityLow->HighEntityIndex)
	{
		EntityHigh = GameState->HighEntities_ + EntityLow->HighEntityIndex;
	}
	else
	{
		if(GameState->HighEntityCount < ArrayCount(GameState->HighEntities_))
		{
			uint32_t HighIndex = GameState->HighEntityCount++;
			EntityHigh = GameState->HighEntities_ + HighIndex;

			// NOTE map the entity to camera space
			tile_map_difference Diff = Subtract(GameState->World->TileMap,
												&EntityLow->P, &GameState->CameraP);
			EntityHigh->P = Diff.dXY;
			EntityHigh->dP = v2{0, 0};
			EntityHigh->AbsTileZ = EntityLow->P.AbsTileZ;
			EntityHigh->FacingDirection = 0;
			EntityHigh->LowEntityIndex = LowIndex;

			EntityLow->HighEntityIndex = HighIndex;
		}
		else
		{
			InvalidCodePath;
		}
	}

	return EntityHigh;
}

inline entity
GetHighEntity(game_state *GameState, uint32_t LowIndex)
{
	entity Result = {};

	if((LowIndex > 0) && (LowIndex < GameState->LowEntityCount))
	{
		Result.LowIndex = LowIndex;
		Result.Low = GameState->LowEntities + LowIndex;
		Result.High = MakeEntityHighFrequency(GameState, LowIndex);
	}

	return Result;
}

inline void
MakeEntityLowFrequency(game_state *GameState, uint32_t LowIndex)
{
	low_entity *EntityLow = &GameState->LowEntities[LowIndex];
	uint32_t HighIndex = EntityLow->HighEntityIndex;
	if(HighIndex)
	{
		uint32_t LastHighIndex = GameState->HighEntityCount - 1;
		if(HighIndex != LastHighIndex)
		{
			high_entity *LastEntity = GameState->HighEntities_ + LastHighIndex;
			high_entity *DelEntity = GameState->HighEntities_ + HighIndex;

			*DelEntity = *LastEntity;
			GameState->LowEntities[LastEntity->LowEntityIndex].HighEntityIndex = HighIndex;
		}
		--GameState->HighEntityCount;
		EntityLow->HighEntityIndex = 0;
	}
}

inline void
OffsetAndCheckFrequencyByArea(game_state *GameState, v2 Offset, rectangle2 CameraBounds)
{
	for(uint32_t EntityIndex = 1;
		EntityIndex < GameState->HighEntityCount;
		)
	{
		high_entity *High = GameState->HighEntities_ + EntityIndex;

		High->P += Offset;
		if(IsInRectangle(CameraBounds, High->P))
		{
			++EntityIndex;
		}
		else
		{
			MakeEntityLowFrequency(GameState, High->LowEntityIndex);
		}
	}
}

inline low_entity *
GetLowEntity(game_state *GameState, uint32_t Index)
{
	low_entity *Result = 0;

	if((Index > 0) && (Index < GameState->LowEntityCount))
	{
		Result = GameState->LowEntities + Index;
	}

	return Result;
}

internal uint32_t
AddLowEntity(game_state *GameState, entity_type Type)
{
	Assert(GameState->LowEntityCount < ArrayCount(GameState->LowEntities));
	uint32_t EntityIndex = GameState->LowEntityCount++;

	GameState->LowEntities[EntityIndex] = {};
	GameState->LowEntities[EntityIndex].Type = Type;

	return EntityIndex;
}

internal uint32_t
AddPlayer(game_state *GameState)
{
	uint32_t EntityIndex = AddLowEntity(GameState, EntityType_Hero);
	low_entity *EntityLow = GetLowEntity(GameState, EntityIndex);

	EntityLow->P.AbsTileX = 6;
	EntityLow->P.AbsTileY = 6;
	EntityLow->P.AbsTileZ = 0;
	EntityLow->P.Offset_.X = 0;
	EntityLow->P.Offset_.Y = 0;
	EntityLow->Height = 0.5f;
	EntityLow->Width = 1.0f;
	EntityLow->Collides = true;

	if(GameState->CameraFollowingEntityIndex == 0)
	{
		GameState->CameraFollowingEntityIndex = EntityIndex;
	}

	return EntityIndex;
}

internal uint32_t
AddWall(game_state *GameState, uint32_t AbsTileX, uint32_t AbsTileY, uint32_t AbsTileZ)
{
	uint32_t EntityIndex = AddLowEntity(GameState, EntityType_Wall);
	low_entity *EntityLow = GetLowEntity(GameState, EntityIndex);

	EntityLow->P.AbsTileX = AbsTileX;
	EntityLow->P.AbsTileY = AbsTileY;
	EntityLow->P.AbsTileZ = AbsTileZ;
	EntityLow->Height = GameState->World->TileMap->TileSideInMeters;
	EntityLow->Width = EntityLow->Height;
	EntityLow->Collides = true;

	return EntityIndex;
}

internal bool32
TestWall(real32 WallX, real32 RelX, real32 RelY, real32 PlayerDeltaX, real32 PlayerDeltaY,
		 real32 *tMin, real32 MinY, real32 MaxY)
{
	bool32 Hit = false;
	real32 tEpsilon = 0.001f;
	if(PlayerDeltaX != 0.0f)
	{
		real32 tResult = (WallX - RelX) / PlayerDeltaX;
		real32 Y = RelY + tResult*PlayerDeltaY;
		if((tResult >= 0.0f) && (*tMin > tResult))
		{
			if((Y >= MinY ) && (Y <= MaxY))
			{
				*tMin = Maximum(0.0f, tResult - tEpsilon);
				Hit = true;
			}
		}
	}

	return Hit;
}

internal void
MovePlayer(game_state *GameState, entity Entity, real32 dt, v2 ddP)
{
	tile_map *TileMap = GameState->World->TileMap;

	real32 ddPLengthSq = LengthSq(ddP);
	if(ddPLengthSq > 1.0f)
	{
		ddP *= (1.0f / SquareRoot(ddPLengthSq));
	}

	real32 PlayerSpeed = 50.0f; // m/s^2
	ddP *= PlayerSpeed;

	// TODO ODE here!
	ddP += -8.0f * Entity.High->dP;

	v2 OldPlayerP = Entity.High->P;
	v2 PlayerDelta = (0.5f*ddP*Square(dt) + Entity.High->dP*dt);
	Entity.High->dP = ddP*dt + Entity.High->dP;
	v2 NewPlayerP = OldPlayerP + PlayerDelta;

	/*
	uint32_t MinTileX = Minimum(OldPlayerP.AbsTileX, NewPlayerP.AbsTileX);
	uint32_t MinTileY = Minimum(OldPlayerP.AbsTileY, NewPlayerP.AbsTileY);
	uint32_t MaxTileX = Maximum(OldPlayerP.AbsTileX, NewPlayerP.AbsTileX);
	uint32_t MaxTileY = Maximum(OldPlayerP.AbsTileY, NewPlayerP.AbsTileY);

	uint32_t EntityTileWidth = CeilReal32ToInt32(Entity.High->Width / TileMap->TileSideInMeters);
	uint32_t EntityTileHeight = CeilReal32ToInt32(Entity.High->Height / TileMap->TileSideInMeters);

	MinTileX -= EntityTileWidth;
	MinTileY -= EntityTileHeight;
	MaxTileX += EntityTileWidth;
	MaxTileY += EntityTileHeight;

	uint32_t AbsTileZ = Entity.High->P.AbsTileZ;
	*/

	for(uint32_t Iteration = 0;
		Iteration < 4;
		++Iteration)
	{
		real32 tMin = 1.0f;
		v2 WallNormal = {};
		uint32_t HitHighEntityIndex = 0;

		v2 DesiredPosition = Entity.High->P + PlayerDelta;

		for(uint32_t TestHighEntityIndex = 1;
			TestHighEntityIndex < GameState->HighEntityCount;
			++TestHighEntityIndex)
		{
			if(TestHighEntityIndex != Entity.Low->HighEntityIndex)
			{
				entity TestEntity;
				TestEntity.High = GameState->HighEntities_ + TestHighEntityIndex;
				TestEntity.LowIndex = TestEntity.High->LowEntityIndex;
				TestEntity.Low = GameState->LowEntities + TestEntity.LowIndex;

				if(TestEntity.Low->Collides)
				{
					real32 DiameterW = TestEntity.Low->Width + Entity.Low->Width;
					real32 DiameterH = TestEntity.Low->Height + Entity.Low->Height;

					v2 MinCorner = -0.5f*v2{DiameterW, DiameterH};
					v2 MaxCorner = 0.5f*v2{DiameterW, DiameterH};

					v2 Rel = Entity.High->P - TestEntity.High->P;

					// NOTE: when will the player hit the wall =>  t = (wx - p0x) / dx
					// NOTE: Test all four walls and take the minimum t of it.
					if(TestWall(MinCorner.X, Rel.X, Rel.Y, PlayerDelta.X, PlayerDelta.Y, &tMin,
								MinCorner.Y, MaxCorner.Y))
					{
						WallNormal = v2{-1, 0};
						HitHighEntityIndex = TestHighEntityIndex;
					}
					if(TestWall(MaxCorner.X, Rel.X, Rel.Y, PlayerDelta.X, PlayerDelta.Y, &tMin,
								MinCorner.Y, MaxCorner.Y))
					{
						WallNormal = v2{1, 0};
						HitHighEntityIndex = TestHighEntityIndex;
					}
					if(TestWall(MinCorner.Y, Rel.Y, Rel.X, PlayerDelta.Y, PlayerDelta.X, &tMin,
								MinCorner.X, MaxCorner.X))
					{
						WallNormal = v2{0, -1};
						HitHighEntityIndex = TestHighEntityIndex;
					}
					if(TestWall(MaxCorner.Y, Rel.Y, Rel.X, PlayerDelta.Y, PlayerDelta.X, &tMin,
								MinCorner.X, MaxCorner.X))
					{
						WallNormal = v2{0, 1};
						HitHighEntityIndex = TestHighEntityIndex;
					}
				}
			}
		}

		Entity.High->P += tMin*PlayerDelta;
		if(HitHighEntityIndex)
		{
			Entity.High->dP = Entity.High->dP - 1*Inner(Entity.High->dP, WallNormal)*WallNormal;
			PlayerDelta = DesiredPosition - Entity.High->P;
			PlayerDelta = PlayerDelta - 1*Inner(PlayerDelta, WallNormal)*WallNormal; 

			high_entity *HitHigh = GameState->HighEntities_ + HitHighEntityIndex;
			low_entity *HitLow = GameState->LowEntities + HitHigh->LowEntityIndex;
			Entity.High->AbsTileZ += HitLow->dAbsTileZ;
		}
		else
		{
			break;
		}
	}

	// TODO change this to use the acceleration vector for hero face
	if((Entity.High->dP.X == 0.0f ) > (Entity.High->dP.Y == 0.0f ))
	{
		// NOTE Should not be handled
	}

	else if(AbsoluteValue(Entity.High->dP.X) > AbsoluteValue(Entity.High->dP.Y))
	{
		if(Entity.High->dP.X > 0)
		{
			Entity.High->FacingDirection = 0;
		}
		else
		{
			Entity.High->FacingDirection = 2;
		}
	}
	else if(AbsoluteValue(Entity.High->dP.X) < AbsoluteValue(Entity.High->dP.Y))
	{
		if(Entity.High->dP.Y > 0)
		{
			Entity.High->FacingDirection = 1;
		}
		else
		{
			Entity.High->FacingDirection = 3;
		}
	}
	
	// TODO always write back a valid tile position
	Entity.Low->P = MapIntoTileSpace(GameState->World->TileMap, GameState->CameraP, Entity.High->P);
}

internal void
SetCamera(game_state *GameState, tile_map_position NewCameraP)
{
	tile_map *TileMap = GameState->World->TileMap;

	tile_map_difference dCameraP = Subtract(TileMap, &NewCameraP, &GameState->CameraP);
	GameState->CameraP = NewCameraP;

	uint32_t TileSpanX = 17*3;
	uint32_t TileSpanY = 9*3;;
	rectangle2 CameraBounds = RectCenterDim(v2{0, 0}, TileMap->TileSideInMeters*v2{(real32)TileSpanX, 
																				   (real32)TileSpanY});

	v2 EntityOffsetForFrame = -dCameraP.dXY;
	OffsetAndCheckFrequencyByArea(GameState, EntityOffsetForFrame, CameraBounds);

	// NOTE Move entities into high set here!
	// TODO solve wrapping
	uint32_t MinTileX = NewCameraP.AbsTileX - TileSpanX/2;
	uint32_t MaxTileX = NewCameraP.AbsTileX + TileSpanX/2;
	uint32_t MinTileY = NewCameraP.AbsTileY - TileSpanY/2;
	uint32_t MaxTileY = NewCameraP.AbsTileY + TileSpanY/2;
	for(uint32_t EntityIndex = 1;
		EntityIndex < GameState->LowEntityCount;
		++EntityIndex)
	{
		low_entity *Low = GameState->LowEntities + EntityIndex;

		if(Low->HighEntityIndex == 0)
		{
#if 0
			if((Low->P.AbsTileZ == NewCameraP.AbsTileZ) &&
			   (Low->P.AbsTileX >= MinTileX) &&
			   (Low->P.AbsTileX <= MaxTileX) &&
			   (Low->P.AbsTileY >= MinTileY) &&
			   (Low->P.AbsTileY <= MaxTileY))
#endif
			{
				MakeEntityHighFrequency(GameState, EntityIndex);
			}
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
		AddLowEntity(GameState, EntityType_Null);
		GameState->HighEntityCount = 1;

		GameState->BackDrop = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile,
				"handmade_hero_legacy_art/early_data/test/test_background.bmp" /* "structured_art.bmp"*/);

		GameState->Shadow = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile,
				"handmade_hero_legacy_art/early_data/test/test_hero_shadow.bmp" /* "structured_art.bmp"*/);
		hero_bitmaps *Bitmap;

		Bitmap = GameState->HeroBitmaps;
		Bitmap-> Head = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "handmade_hero_legacy_art/early_data/test/test_hero_right_head.bmp");
		Bitmap->Cape = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "handmade_hero_legacy_art/early_data/test/test_hero_right_cape.bmp");
		Bitmap->Torso = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "handmade_hero_legacy_art/early_data/test/test_hero_right_torso.bmp");
		Bitmap->AlignX = 72;
		Bitmap->AlignY = 182;
		++Bitmap;

		Bitmap->Head = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "handmade_hero_legacy_art/early_data/test/test_hero_back_head.bmp");
		Bitmap->Cape = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "handmade_hero_legacy_art/early_data/test/test_hero_back_cape.bmp");
		Bitmap->Torso = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "handmade_hero_legacy_art/early_data/test/test_hero_back_torso.bmp");
		Bitmap->AlignX = 72;
		Bitmap->AlignY = 182;
		++Bitmap;

		Bitmap->Head = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "handmade_hero_legacy_art/early_data/test/test_hero_left_head.bmp");
		Bitmap->Cape = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "handmade_hero_legacy_art/early_data/test/test_hero_left_cape.bmp");
		Bitmap->Torso = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "handmade_hero_legacy_art/early_data/test/test_hero_left_torso.bmp");
		Bitmap->AlignX = 72;
		Bitmap->AlignY = 182;
		++Bitmap;

		Bitmap->Head = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "handmade_hero_legacy_art/early_data/test/test_hero_front_head.bmp");
		Bitmap->Cape = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "handmade_hero_legacy_art/early_data/test/test_hero_front_cape.bmp");
		Bitmap->Torso = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "handmade_hero_legacy_art/early_data/test/test_hero_front_torso.bmp");
		Bitmap->AlignX = 72;
		Bitmap->AlignY = 182;
		++Bitmap;

		InitializeArena(&GameState->WorldArena, Memory->PermanentStorageSize - sizeof(game_state),
						(uint8_t *)Memory->PermanentStorage + sizeof(game_state));

		GameState->World = PushStruct(&GameState->WorldArena, world);
		world *World = GameState->World;
		World->TileMap = PushStruct(&GameState->WorldArena, tile_map);

		tile_map *TileMap = World->TileMap;

		TileMap->ChunkShift = 4;
		TileMap->ChunkMask = (1 << TileMap->ChunkShift) - 1;
		TileMap->ChunkDim = (1 << TileMap->ChunkShift);
		TileMap->TileSideInMeters = 1.4f;

		TileMap->TileChunkCountX = 128;
		TileMap->TileChunkCountY = 128;
		TileMap->TileChunkCountZ = 2;

		TileMap->TileChunks = PushArray(&GameState->WorldArena,
										TileMap->TileChunkCountX*
										TileMap->TileChunkCountX*
										TileMap->TileChunkCountZ,
										tile_chunk);

		uint32_t RandomNumberIndex = 0;
		uint32_t TilesPerWidth = 17;
		uint32_t TilesPerHeight = 9;
#if 0
		// TODO waiting for full sparseness;
		uint32_t ScreenX = INT32_MAX / 2;
		uint32_t ScreenY = INT32_MAX / 2;
#else
		uint32_t ScreenX = 0;
		uint32_t ScreenY = 0;
#endif
		uint32_t AbsTileZ = 0;

		bool32 DoorLeft = false;
		bool32 DoorRight = false;
		bool32 DoorTop = false;
		bool32 DoorBottom = false;
		bool32 DoorUp = false;
		bool32 DoorDown = false;
		for(uint32_t ScreenIndex = 0;
			ScreenIndex < 2;
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
				if(AbsTileZ == 0)
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

					SetTileValue(&GameState->WorldArena, World->TileMap, AbsTileX, AbsTileY, AbsTileZ, TileValue);
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
				if(AbsTileZ == 0)
				{
					AbsTileZ = 1;
				}
				else
				{
					AbsTileZ = 0;
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

		tile_map_position NewCameraP = {};
		NewCameraP.AbsTileX = 17/2;
		NewCameraP.AbsTileY = 9/2;
		SetCamera(GameState, NewCameraP);

		Memory->IsInitialized = true;
	}

	world *World = GameState->World;
	tile_map *TileMap = World->TileMap;

	int32_t TileSideInPixels = 60;
	real32 MetersToPixels = (real32)TileSideInPixels / (real32)TileMap->TileSideInMeters;

	real32 LowerLeftX = -(real32)TileSideInPixels / 2;
	real32 LowerLeftY = (real32)Buffer->Height;

	for (int ControllerIndex = 0;
		 ControllerIndex < ArrayCount(Input->Controllers);
		 ++ControllerIndex)
	{
		game_controller_input *Controller = GetController(Input, ControllerIndex);
		uint32_t LowIndex = GameState->PlayerIndexForController[ControllerIndex];
		if(LowIndex == 0)
		{
			if(Controller->Start.EndedDown)
			{
 				uint32_t EntityIndex = AddPlayer(GameState);
				GameState->PlayerIndexForController[ControllerIndex] = EntityIndex;
			}
		}
		else
		{
			entity ControllingEntity = GetHighEntity(GameState, LowIndex);
			v2 ddP = {};

			if(Controller->IsAnalog)
			{
				ddP = v2{Controller->StickAverageX, Controller->StickAverageY};
			} 
			else
			{
				// NOTE Digital input tunning
				if(Controller->MoveUp.EndedDown)
				{
					ddP.Y = 1.0f;
				}
				if(Controller->MoveDown.EndedDown)
				{
					ddP.Y = -1.0f;
				}
				if(Controller->MoveLeft.EndedDown)
				{
					ddP.X = -1.0f;
				}
				if(Controller->MoveRight.EndedDown)
				{
					ddP.X = 1.0f;
				}
			}

			if(Controller->ActionUp.EndedDown)
			{
				ControllingEntity.High->dZ = 4.0f;
			}

			MovePlayer(GameState, ControllingEntity, Input->dtForFrame, ddP);
		}
	}

	v2 EntityOffsetForFrame = {};
	entity CameraFollowingEntity = GetHighEntity(GameState, GameState->CameraFollowingEntityIndex);
	if(CameraFollowingEntity.High)
	{
		tile_map_position NewCameraP = GameState->CameraP;
		GameState->CameraP.AbsTileZ = CameraFollowingEntity.Low->P.AbsTileZ;
#if 1
		if(CameraFollowingEntity.High->P.X > (9.0f*TileMap->TileSideInMeters))
		{
			NewCameraP.AbsTileX += 17;
		}
		if(CameraFollowingEntity.High->P.X < -(9.0f*TileMap->TileSideInMeters))
		{
			NewCameraP.AbsTileX -= 17;
		}
		if(CameraFollowingEntity.High->P.Y > (5.0f*TileMap->TileSideInMeters))
		{
			NewCameraP.AbsTileY += 9;
		}
		if(CameraFollowingEntity.High->P.Y < -(5.0f*TileMap->TileSideInMeters))
		{
			NewCameraP.AbsTileY -= 9;
		}
#else
		if(CameraFollowingEntity.High->P.X > (1.0f*TileMap->TileSideInMeters))
		{
			NewCameraP.AbsTileX += 1;
		}
		if(CameraFollowingEntity.High->P.X < -(1.0f*TileMap->TileSideInMeters))
		{
			NewCameraP.AbsTileX -= 1;
		}
		if(CameraFollowingEntity.High->P.Y > (1.0f*TileMap->TileSideInMeters))
		{
			NewCameraP.AbsTileY += 1;
		}
		if(CameraFollowingEntity.High->P.Y < -(1.0f*TileMap->TileSideInMeters))
		{
			NewCameraP.AbsTileY -= 1;
		}
#endif
		SetCamera(GameState, NewCameraP);
	}

	// TODO Map new entities in and old entities out!!!
	// TODO Mapping tiles and stairs into the entity set!

	DrawBitmap(Buffer, &GameState->BackDrop, 0, 0);

	real32 ScreenCenterX = 0.5f * (real32)Buffer->Width;
	real32 ScreenCenterY = 0.5f * (real32)Buffer->Height;
#if 0
	for(int32_t RelRow = -10;
		RelRow < 10;
		++RelRow)
	{
		for(int32_t RelColumn = -20;
			RelColumn < 20;
			++RelColumn)
		{
			uint32_t Column = GameState->CameraP.AbsTileX + RelColumn;
			uint32_t Row = GameState->CameraP.AbsTileY + RelRow;
			uint32_t TileID = GetTileValue(TileMap, Column, Row, GameState->CameraP.AbsTileZ);
			if(TileID > 1)
			{
				real32 Gray = 0.5f;
				if(TileID == 2)
				{
					Gray = 1.0f;
				}

				if(TileID > 2)
				{
					Gray = 0.25f;
				}

				if((Column == GameState->CameraP.AbsTileX) &&
						(Row == GameState->CameraP.AbsTileY))
				{
					Gray = 0.f;
				}

				v2 TileSide = {0.5f*TileSideInPixels, 0.5f*TileSideInPixels};
				v2 Cen = {ScreenCenterX + ((real32)RelColumn)*TileSideInPixels - GameState->CameraP.Offset_.X*MetersToPixels,
						  ScreenCenterY - ((real32)RelRow)*TileSideInPixels + GameState->CameraP.Offset_.Y*MetersToPixels};
				v2 Min = Cen - TileSide;
				v2 Max = Cen + TileSide;
				DrawRectangle(Buffer, Min, Max, Gray, Gray, Gray);
			}
		}
	}
#endif
	for(uint32_t HighEntityIndex = 0;
		HighEntityIndex < GameState->HighEntityCount;
		++HighEntityIndex)
	{
		high_entity *HighEntity = GameState->HighEntities_ + HighEntityIndex;
		low_entity *LowEntity = GameState->LowEntities + HighEntity->LowEntityIndex;

		HighEntity->P += EntityOffsetForFrame;

		// WARNING Haphazard jump code inserted here
		real32 dt = Input->dtForFrame;
		real32 ddZ = -9.8f;
		HighEntity->Z = 0.5f*ddZ*Square(dt) + HighEntity->dZ*dt + HighEntity->Z;
		HighEntity->dZ = ddZ*dt + HighEntity->dZ;
		if(HighEntity->Z < 0)
		{
			HighEntity->Z = 0;
		}
		real32 CAlpha = 1.0f - 0.5f*HighEntity->Z;
		if(CAlpha < 0)
		{
			CAlpha = 0;
		}

		real32 PlayerR = 1.0f;
		real32 PlayerG = 1.0f;
		real32 PlayerB = 0.0f;
		real32 PlayerGroundPointX = ScreenCenterX + MetersToPixels*HighEntity->P.X; 
		real32 PlayerGroundPointY = ScreenCenterY - MetersToPixels*HighEntity->P.Y;
		real32 Z = -MetersToPixels*HighEntity->Z;
		v2 PlayerLeftTop = {PlayerGroundPointX - 0.5f*MetersToPixels*LowEntity->Width,
							PlayerGroundPointY - 0.5f*MetersToPixels*LowEntity->Height};
		v2 EntityWidthHeight = {LowEntity->Width, LowEntity->Height};

		if(LowEntity->Type == EntityType_Hero)
		{
			hero_bitmaps *HeroBitmaps = &GameState->HeroBitmaps[HighEntity->FacingDirection];
			DrawBitmap(Buffer, &GameState->Shadow, PlayerGroundPointX, PlayerGroundPointY, HeroBitmaps->AlignX, HeroBitmaps->AlignY, CAlpha);
			DrawBitmap(Buffer, &HeroBitmaps->Torso, PlayerGroundPointX, PlayerGroundPointY + Z, HeroBitmaps->AlignX, HeroBitmaps->AlignY);
			DrawBitmap(Buffer, &HeroBitmaps->Cape, PlayerGroundPointX, PlayerGroundPointY + Z, HeroBitmaps->AlignX, HeroBitmaps->AlignY);
			DrawBitmap(Buffer, &HeroBitmaps->Head, PlayerGroundPointX, PlayerGroundPointY + Z, HeroBitmaps->AlignX, HeroBitmaps->AlignY);
		}
		else
		{
			DrawRectangle(Buffer, 
					PlayerLeftTop,
					PlayerLeftTop + MetersToPixels*EntityWidthHeight,
					PlayerR, PlayerG, PlayerB);
		}
#if 0
#endif
	}
}

extern "C" GAME_GET_SOUND_SAMPLES(GameGetSoundSamples)
{
	game_state *GameState = (game_state *)Memory->PermanentStorage;
	GameOutputSound(GameState, SoundBuffer, 400);
}
