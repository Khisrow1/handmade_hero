/*  +======| File Info |===============================================================+
    |                                                                                  |
    |     Subdirectory:  /src                                                          |
    |    Creation date:  1/4/2021 7:30:49 AM                                           |
    |    Last Modified:                                                                |
    |                                                                                  |
    +=====================| Sayed Abid Hashimi, Copyright � All rights reserved |======+  */

#if !defined(HANDMADE_RENDER_GROUP_CPP)

inline v4
SRGB255ToLinear1(v4 C)
{
	v4 Result;

	real32 Inv255 = 1.0f / 255.0f;

	Result.r = Square(Inv255*C.r);
	Result.g = Square(Inv255*C.g);
	Result.b = Square(Inv255*C.b);
	Result.a = Inv255*C.a;

	return Result;
}

inline v4
Linear1ToSRGB255(v4 C)
{
	v4 Result;

	real32 One255 = 255.0f;

	Result.r = One255*SquareRoot(C.r);
	Result.g = One255*SquareRoot(C.g);
	Result.b = One255*SquareRoot(C.b);
	Result.a = One255*C.a;

	return Result;
}

inline v4
UnscaleAndBiasNormal(v4 Normal)
{
	v4 Result;

	real32 Inv255 = 1.0f / 255.0f;

	Result.x = -1.0f + 2.0f*(Inv255*Normal.x);
	Result.y = -1.0f + 2.0f*(Inv255*Normal.y);
	Result.z = -1.0f + 2.0f*(Inv255*Normal.z);

	Result.w = Inv255*Normal.w;

	return Result;
}

internal void
DrawRectangle(loaded_bitmap *Buffer, v2 vMin, v2 vMax, v4 Color)
{
	real32 A = Color.a;
	real32 R = Color.r;
	real32 G = Color.g;
	real32 B = Color.b;

	int32_t MinX = RoundReal32ToInt32(vMin.x);
	int32_t MinY = RoundReal32ToInt32(vMin.y);
	int32_t MaxX = RoundReal32ToInt32(vMax.x);
	int32_t MaxY = RoundReal32ToInt32(vMax.y);

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

	uint32_t Color32 = ((RoundReal32ToUInt32(A * 255.0f) << 24) |
					  	(RoundReal32ToUInt32(R * 255.0f) << 16) |
					  	(RoundReal32ToUInt32(G * 255.0f) << 8)  |
					  	(RoundReal32ToUInt32(B * 255.0f) << 0));

	uint8_t *Row = ((uint8_t *)Buffer->Memory +
							   MinX*BITMAP_BYTES_PER_PIXEL +
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
			*Pixel++ = Color32;
		}	

		Row += Buffer->Pitch;
	}
}

inline v4
Unpack4x8(uint32_t Packed)
{
	v4 Result = {};

	Result = {(real32)((Packed >> 16) & 0xFF),
			  (real32)((Packed >> 8) & 0xFF),
			  (real32)((Packed >> 0) & 0xFF),
			  (real32)((Packed >> 24) & 0xFF)};

	return Result;
}

struct bilinear_sample
{
	uint32_t A, B, C, D;
};
inline bilinear_sample
BilinearSample(loaded_bitmap *Texture, int32_t X, int32_t Y)
{
	bilinear_sample Result;

	uint8_t *TexelPtr = ((uint8_t *)Texture->Memory) + Y*Texture->Pitch + X*sizeof(uint32_t);
	Result.A = *(uint32_t *)(TexelPtr);
	Result.B = *(uint32_t *)(TexelPtr + sizeof(uint32_t));
	Result.C = *(uint32_t *)(TexelPtr + Texture->Pitch);
	Result.D = *(uint32_t *)(TexelPtr + Texture->Pitch + sizeof(uint32_t));

	return Result;
}

inline v4
SRGBBilinearBlend(bilinear_sample BilinearSample, real32 fX, real32 fY)
{
	v4 TexelA = Unpack4x8(BilinearSample.A);
	v4 TexelB = Unpack4x8(BilinearSample.B);
	v4 TexelC = Unpack4x8(BilinearSample.C);
	v4 TexelD = Unpack4x8(BilinearSample.D);

	// NOTE sRGB to "linear" brightness space conversion (remove gamma curve)
	TexelA = SRGB255ToLinear1(TexelA);
	TexelB = SRGB255ToLinear1(TexelB);
	TexelC = SRGB255ToLinear1(TexelC);
	TexelD = SRGB255ToLinear1(TexelD);

	v4 Result = Lerp(Lerp(TexelA, fX, TexelB),
					 fY,
					 Lerp(TexelC, fX, TexelD));

	return Result;
}

inline v3
SampleEnvironmentMap(v2 ScreenSpaceUV, v3 SampleDirection, real32 Roughness, environment_map *Map,
					 real32 DistanceFromMapinZ)
{
	/* NOTE(Khisrow):

	   ScreenSpaceUV tells us where the ray is being case _from_ in 
	   normalized screen coordinates.

	   SampleDirection tells us what direction the cast is going - it
	   does not have to be normalized.

	   Roughness says which LODs of Map we sample from.

	   DistanceFromMapInZ says how far the map is from the sample point in Z,
	   given in meters
	*/

	// NOTE(Khisrow): Pick which LODs we have to sample from
	uint32_t LODIndex = (uint32_t)(Roughness*(real32)(ArrayCount(Map->LOD) - 1) + 0.5f);
	Assert(LODIndex < ArrayCount(Map->LOD));

	loaded_bitmap *LOD = Map->LOD + LODIndex;

	// NOTE(Khisrow): Compute the distance to the map and the scaling
	// factor for the meters-to-UVs
	real32 UVsPerMeter = 0.05f; // TODO(Khisrow): Parameterize this!
	real32 C = (UVsPerMeter*DistanceFromMapinZ) / SampleDirection.y;
	v2 Offset = C * V2(SampleDirection.x, SampleDirection.z);

	// NOTE(Khisrow): Find the intersection point
	v2 UV = ScreenSpaceUV + Offset;

	// NOTE(Khisrow): Clamp to the valid range
	UV.x = Clamp01(UV.x);
	UV.y = Clamp01(UV.y);

	// NOTE(Khisrow): Bilinear Sample
	real32 tX = ((UV.x*(real32)(LOD->Width - 2)));
	real32 tY = ((UV.y*(real32)(LOD->Height - 2)));

	int32_t X = (int32_t)tX;
	int32_t Y = (int32_t)tY;

	real32 fX = tX - (real32)X;
	real32 fY = tY - (real32)Y;

	Assert((X >= 0) && (X < LOD->Width));
	Assert((Y >= 0) && (Y < LOD->Height));

	// NOTE(Khisrow): Turn this on to see where on the sampling we are sampling from.
#if 0
	uint8_t *TexelPtr = ((uint8_t *)LOD->Memory + Y*LOD->Pitch + X*sizeof(uint32_t));
	*(uint32_t *)TexelPtr = 0xFFFFFFFF;
#endif

	bilinear_sample Sample = BilinearSample(LOD, X, Y);
	v3 Result = SRGBBilinearBlend(Sample, fX, fY).xyz;

	return Result;
}

internal void
DrawRectangleSlowly(loaded_bitmap *Buffer, v2 Origin, v2 XAxis, v2 YAxis, v4 Color,
					loaded_bitmap *Texture, loaded_bitmap *NormalMap,
					environment_map *Top, environment_map *Middle, environment_map *Bottom,
					real32 PixelsToMeters)
{
	BEGIN_TIMED_BLOCK(DrawRectangleSlowly);
	// NOTE Premultiply color up front
	Color.rgb *= Color.a;

	real32 XAxisLength = Length(XAxis);
	real32 YAxisLength = Length(YAxis);

	v2 NxAxis = (YAxisLength / XAxisLength) * XAxis;
	v2 NyAxis = (XAxisLength / YAxisLength) * YAxis;
	real32 NzScale = 0.5f*(YAxisLength + XAxisLength);

	real32 InvXAxisLengthSq = 1.0f / LengthSq(XAxis);
	real32 InvYAxisLengthSq = 1.0f / LengthSq(YAxis);

	uint32_t Color32 = ((RoundReal32ToUInt32(Color.a * 255.0f) << 24) |
						(RoundReal32ToUInt32(Color.r * 255.0f) << 16) |
						(RoundReal32ToUInt32(Color.g * 255.0f) << 8)  |
						(RoundReal32ToUInt32(Color.b * 255.0f) << 0));

	int32_t WidthMax = (Buffer->Width - 1);
	int32_t HeightMax = (Buffer->Height - 1);

	real32 InvWidthMax = 1.0f / (real32)WidthMax;
	real32 InvHeightMax = 1.0f / (real32)HeightMax;

	// TODO(Khisrow): This will need to be specified separately!!!
	real32 OriginZ = 0.0f;
	real32 OriginY = (Origin + 0.5f*XAxis + 0.5f*YAxis).y;
	real32 FixedCastY = InvHeightMax*OriginY;

	int32_t XMin = WidthMax;
	int32_t XMax = 0;

	int32_t YMin = HeightMax;
	int32_t YMax = 0;

	v2 P[4] = {Origin, Origin + XAxis, Origin + XAxis + YAxis, Origin + YAxis};
	for(int32_t PIndex = 0;
		PIndex < ArrayCount(P);
		++PIndex)
	{
		v2 TestP = P[PIndex];
		int32_t FloorX = FloorReal32ToInt32(TestP.x);
		int32_t CeilX = CeilReal32ToInt32(TestP.x);
		int32_t FloorY = FloorReal32ToInt32(TestP.y);
		int32_t CeilY = CeilReal32ToInt32(TestP.y);

		if(XMin > FloorX) {XMin = FloorX;}
		if(YMin > FloorY) {YMin = FloorY;}
		if(XMax < CeilX) {XMax = CeilX;}
		if(YMax < CeilY) {YMax = CeilY;}
	}

	if(XMin < 0) {XMin = 0;}
	if(YMin < 0) {YMin = 0;}
	if(XMax > WidthMax) {XMax = WidthMax;}
	if(YMax > HeightMax) {YMax = HeightMax;}

	uint8_t *Row = ((uint8_t *)Buffer->Memory +
							   XMin*BITMAP_BYTES_PER_PIXEL +
							   YMin*Buffer->Pitch);

	for (int Y = YMin;
		 Y <= YMax;
		 ++Y)
	{
		uint32_t *Pixel = (uint32_t *)Row;
		for (int X = XMin;
			 X <= XMax;
			 ++X)
		{
			BEGIN_TIMED_BLOCK(TestPixel);

			v2 PixelP = V2i(X, Y);
			v2 d = PixelP - Origin;
			// TODO(Khisrow): PerpInner
			// TODO(Khisrow): Simpler Origin
			real32 Edge0 = Inner(d, -Perp(XAxis));
			real32 Edge1 = Inner(d - XAxis, -Perp(YAxis));
			real32 Edge2 = Inner(d - XAxis - YAxis, Perp(XAxis));
			real32 Edge3 = Inner(d - YAxis, Perp(YAxis));

			if((Edge0 < 0) &&
			   (Edge1 < 0) &&
			   (Edge2 < 0) &&
			   (Edge3 < 0))
			{

				BEGIN_TIMED_BLOCK(FillPixel);
#if 1
				v2 ScreenSpaceUV = {(real32)X*InvWidthMax, FixedCastY};
				real32 ZDiff = PixelsToMeters*((real32)Y - OriginY);
#else
				v2 ScreenSpaceUV = {(real32)X*InvWidthMax, (real32)Y*InvHeightMax};
				real32 ZDiff = 0.0f;
#endif

				real32 U = InvXAxisLengthSq*Inner(d, XAxis);
				real32 V = InvYAxisLengthSq*Inner(d, YAxis);
#if 0
				Assert((U >= 0.0f) && (U <= 1.0f));
				Assert((V >= 0.0f) && (V <= 1.0f));
#endif
				real32 tX = ((U*(real32)(Texture->Width - 2)));
				real32 tY = ((V*(real32)(Texture->Height - 2)));

				int32_t iX = (int32_t)tX;
				int32_t iY = (int32_t)tY;

				real32 fX = tX - (real32)iX;
				real32 fY = tY - (real32)iY;

				Assert((iX >= 0) && (iX < Texture->Width));
				Assert((iY >= 0) && (iY < Texture->Height));

				bilinear_sample TexelSample = BilinearSample(Texture, iX, iY);
				v4 Texel = SRGBBilinearBlend(TexelSample, fX, fY);
#if 0
				if(NormalMap)
				{
					bilinear_sample NormalSample = BilinearSample(NormalMap, iX, iY);

					v4 NormalA = Unpack4x8(NormalSample.A);
					v4 NormalB = Unpack4x8(NormalSample.B);
					v4 NormalC = Unpack4x8(NormalSample.C);
					v4 NormalD = Unpack4x8(NormalSample.D);

					v4 Normal = Lerp(Lerp(NormalA, fX, NormalB),
									 fY,
									 Lerp(NormalC, fX, NormalD));

					Normal = UnscaleAndBiasNormal(Normal);

					// NOTE(Khisrow): Take into the account the non-uniform scaling and rotations
					Normal.xy = Normal.x * NxAxis + Normal.y * NyAxis;

					// NOTE(Khisrow): NzScale could be a parameter if we want people to
					// have control over the amount of scaling in the Z direction that
					// the normals appear to have.
					Normal.z *= NzScale;
					Normal.xyz = Normalize(Normal.xyz);

					// NOTE(Khisrow): The eye vector is always assumed to be [0, 0, 1]
					// NOTE(Khisrow): Dot Product Ex*Nx + Ey*Ny + Ez*Nz and so Dot Product = Nz
					//                             ^->0    ^->0    ^->1
					// NOTE(Khisrow): This is a simplified version of the reflection -e + 2e^T N N
					v3 BounceDirection = 2.0f*Normal.z*Normal.xyz;
					BounceDirection.z -= 1.0f;

					// TODO(Khisrow): Eventually we need to support two mapping,
					// one for top-down view (which we don't do now) and one
					// for sideways, which is what's happening here.
					BounceDirection.z = -BounceDirection.z;

					environment_map *FarMap = 0;
					real32 Pz = OriginZ + ZDiff;
					real32 MapZ = 2.0f;
					real32 tEnvMap = BounceDirection.y;
					real32 tFarMap = 0.0f;
					if(tEnvMap < -0.5f)
					{
						FarMap = Bottom;
						tFarMap = -1.0f - 2.0f*tEnvMap;
					}
					else if(tEnvMap > 0.5f)
					{
						FarMap = Top;
						tFarMap = 2.0f*(tEnvMap - 0.5f);
					}

					tFarMap *= tFarMap;
					tFarMap *= tFarMap;

					v3 LightColor = {0, 0, 0};
					if(FarMap)
					{
						real32 DistanceFromMapInZ = FarMap->Pz - Pz;
						v3 FarMapColor = SampleEnvironmentMap(ScreenSpaceUV, BounceDirection, Normal.w, FarMap,
															  DistanceFromMapInZ);
						LightColor = Lerp(LightColor, tFarMap, FarMapColor);
					}

					Texel.rgb = Texel.rgb + Texel.a*LightColor;
#if 0
					Texel.rgb = V3(0.5f, 0.5f, 0.5f) + 0.5f*BounceDirection;
					Texel.rgb *= Texel.a;
#endif
				}
#endif
				Texel = Hadamard(Texel, Color);
				Texel.r = Clamp01(Texel.r);
				Texel.g = Clamp01(Texel.g);
				Texel.b = Clamp01(Texel.b);

				v4 Dest = {(real32)((*Pixel >> 16) & 0xFF),
						   (real32)((*Pixel >> 8) & 0xFF),
						   (real32)((*Pixel >> 0) & 0xFF),
						   (real32)((*Pixel >> 24) & 0xFF)};

				// NOTE sRGB to "linear" brightness space conversion (add gamma curve)
				Dest = SRGB255ToLinear1(Dest);

				v4 Blended = (1.0f-Texel.a)*Dest + Texel;

				// NOTE "linear" brightness to sRGB conversion (add the gamma curve)
				v4 Blended255 = Linear1ToSRGB255(Blended);

				*Pixel = (((uint32_t)(Blended255.a + 0.5f) << 24) |
						  ((uint32_t)(Blended255.r + 0.5f) << 16) |
						  ((uint32_t)(Blended255.g + 0.5f) << 8)  |
						  ((uint32_t)(Blended255.b + 0.5f) << 0));

				END_TIMED_BLOCK(FillPixel);
			}
			++Pixel;

			END_TIMED_BLOCK(TestPixel);
		}	

		Row += Buffer->Pitch;
	}

	END_TIMED_BLOCK(DrawRectangleSlowly)
}

internal void
DrawBitmap(loaded_bitmap *Buffer, loaded_bitmap *Bitmap,
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

	
	int32_t BytesPerPixel = BITMAP_BYTES_PER_PIXEL;
	uint8_t *SourceRow = (uint8_t *)Bitmap->Memory + SourceOffsetY*Bitmap->Pitch + BytesPerPixel*SourceOffsetX;
	uint8_t *DestRow = ((uint8_t *)Buffer->Memory +
								   MinX*BytesPerPixel +
								   MinY*Buffer->Pitch);

	for(int Y = MinY;
		Y < MaxY;
		++Y)
	{
		uint32_t *Dest = (uint32_t *)DestRow;
		uint32_t *Source = (uint32_t *)SourceRow;
		for(int X = MinX;
			X < MaxX;
			++X)
		{
			v4 Texel = {(real32)((*Source >> 16) & 0xFF),
						(real32)((*Source >> 8) & 0xFF),
						(real32)((*Source >> 0) & 0xFF),
						(real32)((*Source >> 24) & 0xFF)};

			Texel = SRGB255ToLinear1(Texel);

			Texel *= CAlpha;

			v4 D = {(real32)((*Dest >> 16) & 0xFF),
					(real32)((*Dest >> 8) & 0xFF),
					(real32)((*Dest >> 0) & 0xFF),
					(real32)((*Dest >> 24) & 0xFF)};

			D = SRGB255ToLinear1(D);

			v4 Result = (1.0f-Texel.a)*D + Texel;

			Result = Linear1ToSRGB255(Result);

			*Dest = (((uint32_t)(Result.a + 0.5f) << 24) |
					 ((uint32_t)(Result.r + 0.5f) << 16) |
					 ((uint32_t)(Result.g + 0.5f) << 8)  |
					 ((uint32_t)(Result.b + 0.5f) << 0));
			++Dest;
			++Source;
		}

		DestRow += Buffer->Pitch;
		SourceRow += Bitmap->Pitch;
	}
}

internal void
ChangeSaturation(loaded_bitmap *Buffer, real32 Level)
{
	uint8_t *DestRow = (uint8_t *)Buffer->Memory;
	for(int Y = 0;
		Y < Buffer->Height;
		++Y)
	{
		uint32_t *Dest = (uint32_t *)DestRow;
		for(int X = 0;
			X < Buffer->Width;
			++X)
		{
			v4 D = {(real32)((*Dest >> 16) & 0xFF),
					(real32)((*Dest >> 8) & 0xFF),
					(real32)((*Dest >> 0) & 0xFF),
					(real32)((*Dest >> 24) & 0xFF)};

			D = SRGB255ToLinear1(D);

			real32 Avg = (1.0f/3.0f) * (D.r + D.g + D.b);
			v3 Delta = V3(D.r - Avg, D.g - Avg, D.b - Avg);

			v4 Result = V4(V3(Avg, Avg, Avg) + Level*Delta, D.a);

			Result = Linear1ToSRGB255(Result);

			*Dest = (((uint32_t)(Result.a + 0.5f) << 24) |
					 ((uint32_t)(Result.r + 0.5f) << 16) |
					 ((uint32_t)(Result.g + 0.5f) << 8)  |
					 ((uint32_t)(Result.b + 0.5f) << 0));
			++Dest;
		}

		DestRow += Buffer->Pitch;
	}
}
internal void
DrawRectangleOutline(loaded_bitmap *Buffer, v2 vMin, v2 vMax, v3 Color, real32 R = 2.0f)
{
	// NOTE Top and Bottom
	DrawRectangle(Buffer, V2(vMin.x - R, vMin.y - R), V2(vMax.x + R, vMin.y + R), V4(Color, 1.0f));
	DrawRectangle(Buffer, V2(vMin.x - R, vMax.y - R), V2(vMax.x + R, vMax.y + R), V4(Color, 1.0f));

	// NOTE Left and Right
	DrawRectangle(Buffer, V2(vMin.x - R, vMin.y - R), V2(vMin.x + R, vMax.y + R), V4(Color, 1.0f));
	DrawRectangle(Buffer, V2(vMax.x - R, vMin.y - R), V2(vMax.x + R, vMax.y + R), V4(Color, 1.0f));
}

internal void
DrawMatte(loaded_bitmap *Buffer, loaded_bitmap *Bitmap,
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

	
	int32_t BytesPerPixel = BITMAP_BYTES_PER_PIXEL;
	uint8_t *SourceRow = (uint8_t *)Bitmap->Memory + SourceOffsetY*Bitmap->Pitch + BytesPerPixel*SourceOffsetX;
	uint8_t *DestRow = ((uint8_t *)Buffer->Memory +
								   MinX*BytesPerPixel +
								   MinY*Buffer->Pitch);

	for(int Y = MinY;
		Y < MaxY;
		++Y)
	{
		uint32_t *Dest = (uint32_t *)DestRow;
		uint32_t *Source = (uint32_t *)SourceRow;
		for(int X = MinX;
			X < MaxX;
			++X)
		{
			real32 SA = (real32)((*Source >> 24) & 0xFF);
			real32 RSA = (SA / 255.0f) * CAlpha;
			real32 SR = CAlpha*(real32)((*Source >> 16) & 0xFF);
			real32 SG = CAlpha*(real32)((*Source >> 8) & 0xFF);
			real32 SB = CAlpha*(real32)((*Source >> 0) & 0xFF);

			real32 DA = (real32)((*Dest >> 24) & 0xFF);
			real32 DR = (real32)((*Dest >> 16) & 0xFF);
			real32 DG = (real32)((*Dest >> 8) & 0xFF);
			real32 DB = (real32)((*Dest >> 0) & 0xFF);
			real32 RDA = (DA / 255.0f);

			real32 InvRSA = (1.0f-RSA);
			// TODO Check this for math errors
			// real32 A = 255.0f*(RSA + RDA - RSA*RDA);
			real32 A = DA*InvRSA;
			real32 R = InvRSA*DR;
			real32 G = InvRSA*DG;
			real32 B = InvRSA*DB;

			*Dest = (((uint32_t)(A + 0.5f) << 24) |
					 ((uint32_t)(R + 0.5f) << 16) |
					 ((uint32_t)(G + 0.5f) << 8)  |
					 ((uint32_t)(B + 0.5f) << 0));
			++Dest;
			++Source;
		}

		DestRow += Buffer->Pitch;
		SourceRow += Bitmap->Pitch;
	}
}

struct entity_basis_p_result
{
	v2 P;
	real32 Scale;
	bool32 Valid;
};

inline entity_basis_p_result
GetRenderEntityBasisP(render_group *RenderGroup, render_entity_basis *EntityBasis, v2 ScreenDim)
{
	v2 ScreenCenter = 0.5f*ScreenDim;

	entity_basis_p_result Result = {};

	v3 EntityBaseP = EntityBasis->Basis->P;

	real32 DistanceToPZ = (RenderGroup->RenderCamera.DistanceAboveTarget - EntityBaseP.z);
	real32 NearClipPlane = 0.2f;

	v3 RawXY = V3(EntityBaseP.xy + EntityBasis->Offset.xy, 1.0f);

	if(DistanceToPZ > NearClipPlane)
	{
		v3 ProjectedXY = (1.0f / DistanceToPZ) * RenderGroup->RenderCamera.FocalLength*RawXY;
		Result.P = ScreenCenter + RenderGroup->MetersToPixels*ProjectedXY.xy;
		Result.Scale = RenderGroup->MetersToPixels*ProjectedXY.z;
		Result.Valid = true;
	}

	return Result;
}

internal void
RenderGroupToOutput(render_group *RenderGroup, loaded_bitmap *OutputTarget)
{
	BEGIN_TIMED_BLOCK(RenderGroupToOutput);

	v2 ScreenDim = {(real32)OutputTarget->Width,
					(real32)OutputTarget->Height};

	real32 PixelsToMeters = 1.0f / RenderGroup->MetersToPixels;

	for(uint32_t BaseAddress = 0;
		BaseAddress < RenderGroup->PushBufferSize;
		)
	{
		render_group_entry_header *Header = (render_group_entry_header *)(RenderGroup->PushBufferBase + BaseAddress);
		BaseAddress += sizeof(*Header);
		void *Data = (uint8_t *)Header + sizeof(*Header);
		switch(Header->Type)
		{
			case RenderGroupEntryType_render_entry_clear:
			{
				render_entry_clear *Entry = (render_entry_clear *)Data;

				DrawRectangle(OutputTarget, V2(0, 0),
							  V2((real32)OutputTarget->Width, (real32)OutputTarget->Height),
							  Entry->Color);

				BaseAddress += sizeof(*Entry);
			} break;
#if 0
			case RenderGroupEntryType_render_entry_saturation:
			{
				render_entry_saturation *Entry = (render_entry_saturation *)Data;

				ChangeSaturation(OutputTarget, Entry->Level);

				BaseAddress += sizeof(*Entry);
			} break;
#endif
			case RenderGroupEntryType_render_entry_bitmap:
			{
				render_entry_bitmap *Entry = (render_entry_bitmap *)Data;
				entity_basis_p_result Basis = GetRenderEntityBasisP(RenderGroup, &Entry->EntityBasis, ScreenDim);
				Assert(Entry->Bitmap);
#if 0
				DrawBitmap(OutputTarget, Entry->Bitmap, P.x, P.y, Entry->Color.a);

#else
				DrawRectangleSlowly(OutputTarget, Basis.P,
									Basis.Scale*V2(Entry->Size.x, 0),
									Basis.Scale*V2(0, Entry->Size.y), Entry->Color,
									Entry->Bitmap, 0, 0, 0, 0, PixelsToMeters);
#endif

				BaseAddress += sizeof(*Entry);
			} break;

			case RenderGroupEntryType_render_entry_rectangle:
			{
				render_entry_rectangle *Entry = (render_entry_rectangle *)Data;
				entity_basis_p_result Basis = GetRenderEntityBasisP(RenderGroup, &Entry->EntityBasis, ScreenDim);
				DrawRectangle(OutputTarget, Basis.P, Basis.P + Basis.Scale*Entry->Dim, Entry->Color);

				BaseAddress += sizeof(*Entry);
			} break;

			case RenderGroupEntryType_render_entry_coordinate_system:
			{
				render_entry_coordinate_system *Entry = (render_entry_coordinate_system *)Data;
				DrawRectangleSlowly(OutputTarget,
									Entry->Origin,
									Entry->XAxis,
									Entry->YAxis,
									Entry->Color,
									Entry->Texture,
									Entry->NormalMap,
									Entry->Top, Entry->Middle, Entry->Bottom,
									PixelsToMeters);

				v4 Color = {1, 1, 0, 1};
				v2 Dim = {2, 2};
				v2 P = Entry->Origin;
				DrawRectangle(OutputTarget, P - Dim, P + Dim, Color);

				P = Entry->Origin + Entry->XAxis;
				DrawRectangle(OutputTarget, P - Dim, P + Dim, Color);

				P = Entry->Origin + Entry->YAxis;
				DrawRectangle(OutputTarget, P - Dim, P + Dim, Color);

				v2 vMax = Entry->Origin + Entry->YAxis + Entry->XAxis;
				DrawRectangle(OutputTarget, vMax - Dim, vMax + Dim, Color);
#if 0
				for(uint32_t PIndex = 0;
					PIndex < ArrayCount(Entry->Points);
					++PIndex)
				{
					P = Entry->Points[PIndex];
					P = Entry->Origin + P.x*Entry->XAxis + P.y*Entry->YAxis;
					DrawRectangle(OutputTarget, P - Dim, P + Dim, Entry->Color.r, Entry->Color.g, Entry->Color.b);
				}
#endif
				BaseAddress += sizeof(*Entry);
			} break;

			InvalidDefaultCase;
		}
	}

	END_TIMED_BLOCK(RenderGroupToOutput);
}

internal render_group *
AllocateRenderGroup(memory_arena *Arena, uint32_t MaxPushBufferSize,
					uint32_t ResolutionPixelsX, uint32_t ResolutionPixelsY)
{
	render_group *Result = PushStruct(Arena, render_group);
	Result->PushBufferBase = (uint8_t *)PushSize(Arena, MaxPushBufferSize);

	Result->DefaultBasis = PushStruct(Arena, render_basis);
	Result->DefaultBasis->P = V3(0, 0, 0);

	Result->MaxPushBufferSize = MaxPushBufferSize;
	Result->PushBufferSize = 0;

	Result->GameCamera.FocalLength = 0.6f; // NOTE(Khisrow): Meters the person is sitting from their monitor
	Result->GameCamera.DistanceAboveTarget = 9.0f;
	Result->RenderCamera = Result->GameCamera;
	Result->RenderCamera.DistanceAboveTarget = 9.0f;

	Result->GlobalAlpha = 1.0f;

	// TODO(Khisrow): Need to adjust this based on buffer size
	real32 WidthOfMonitor = 0.635f; // NOTE(Khisrow): Horizontal measurement of monitor in meters
	Result->MetersToPixels = (real32)ResolutionPixelsX*WidthOfMonitor;

	real32 PixelsToMeters = 1.0f / Result->MetersToPixels;
	Result->MonitorHalfDimInMeters = {0.5f*ResolutionPixelsX*PixelsToMeters,
									  0.5f*ResolutionPixelsY*PixelsToMeters};

	return Result;
}

#define PushRenderElement(Group, type) (type *)PushRenderElement_(Group, sizeof(type), RenderGroupEntryType_##type)
inline void *
PushRenderElement_(render_group *RenderGroup, uint32_t Size, render_group_entry_type Type)
{
	void *Result = 0;

	Size += sizeof(render_group_entry_header);

	if((RenderGroup->PushBufferSize + Size) < RenderGroup->MaxPushBufferSize)
	{
		render_group_entry_header *Header = (render_group_entry_header *)(RenderGroup->PushBufferBase + RenderGroup->PushBufferSize);
		Header->Type = Type;
		Result = (uint8_t *)Header + sizeof(render_group_entry_header);
		RenderGroup->PushBufferSize += Size;
	}
	else
	{
		InvalidCodePath;
	}

	return Result;
}

inline void
PushBitmap(render_group *Group, loaded_bitmap *Bitmap, real32 Height, v3 Offset, v4 Color = V4(1, 1, 1, 1))
{
	render_entry_bitmap *Entry = PushRenderElement(Group, render_entry_bitmap);
	if(Entry)
	{
		Entry->EntityBasis.Basis = Group->DefaultBasis;
		Entry->Bitmap = Bitmap;
		v2 Size = V2(Height*Bitmap->WidthOverHeight, Height);
		v2 Align = Hadamard(Bitmap->AlignPercentage, Size);
		Entry->EntityBasis.Offset = Offset - V3(Align, 0);
		Entry->Color = Color*Group->GlobalAlpha;
		Entry->Size = Size;
	}
}

inline void
PushRect(render_group *Group, v3 Offset, v2 Dim, v4 Color = V4(1, 1, 1, 1))
{
	render_entry_rectangle *Piece = PushRenderElement(Group, render_entry_rectangle);
	if(Piece)
	{
		Piece->EntityBasis.Basis = Group->DefaultBasis;
		Piece->EntityBasis.Offset = (Offset - V3(0.5f*Dim, 0));
		Piece->Color = Color;
		Piece->Dim = Dim;
	}
}

inline void
PushRectOutline(render_group *Group, v3 Offset, v2 Dim, v4 Color = V4(1, 1, 1, 1))
{
	real32 Thickness = 0.1f;

	// NOTE Top and Bottom
	PushRect(Group, Offset - V3(0, 0.5f*Dim.y, 0), V2(Dim.x, Thickness), Color);
	PushRect(Group, Offset + V3(0, 0.5f*Dim.y, 0), V2(Dim.x, Thickness), Color);

	// NOTE Left and Right
	PushRect(Group, Offset - V3(0.5f*Dim.x, 0, 0), V2(Thickness, Dim.y), Color);
	PushRect(Group, Offset + V3(0.5f*Dim.x, 0, 0), V2(Thickness, Dim.y), Color);
}

inline void
Clear(render_group *Group, v4 Color)
{
	render_entry_clear *Entry = PushRenderElement(Group, render_entry_clear);
	if(Entry)
	{
		Entry->Color = Color;
	}
}

#if 0
inline void
Saturation(render_group *Group, real32 Level)
{
	render_entry_saturation *Entry = PushRenderElement(Group, render_entry_saturation);
	if(Entry)
	{
		Entry->Level = Level;
	}
}
#endif

inline render_entry_coordinate_system *
CoordinateSystem(render_group *Group, v2 Origin, v2 XAxis, v2 YAxis, v4 Color,
				 loaded_bitmap *Texture, loaded_bitmap *NormalMap,
				 environment_map *Top, environment_map *Middle, environment_map *Bottom)
{
	render_entry_coordinate_system *Entry = PushRenderElement(Group, render_entry_coordinate_system);
	if(Entry)
	{
		Entry->Color = Color;
		Entry->Origin = Origin;
		Entry->XAxis = XAxis;
		Entry->YAxis = YAxis;
		Entry->Texture = Texture;
		Entry->NormalMap = NormalMap;
		Entry->Top = Top;
		Entry->Middle = Middle;
		Entry->Bottom = Bottom;
	}

	return Entry;
}

inline v2
Unproject(render_group * RenderGroup, v2 ProjectedXY, real32 AtDistanceFromCamera)
{
	v2 WorldXY = (AtDistanceFromCamera / RenderGroup->GameCamera.FocalLength)*ProjectedXY;

	return WorldXY;
}

inline rectangle2
GetCameraRectangleAtDistance(render_group * Group, real32 DistanceFromCamera)
{
	v2 RawXY = Unproject(Group, Group->MonitorHalfDimInMeters, DistanceFromCamera);

	rectangle2 Result = RectCenterHalfDim(V2(0, 0), RawXY);

	return Result;
}

inline rectangle2
GetCameraRectangleAtTarget(render_group * Group)
{
	rectangle2 Result = GetCameraRectangleAtDistance(Group, Group->GameCamera.DistanceAboveTarget);

	return Result;
}


#define HANDMADE_RENDER_GROUP_CPP
#endif
