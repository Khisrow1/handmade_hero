/*  +======| File Info |===============================================================+
    |                                                                                  |
    |     Subdirectory:  /src                                                          |
    |    Creation date:  Undefined                                                     |
    |    Last Modified:  12/11/2020 5:43:37 PM                                         |
    |                                                                                  |
    +=====================| Sayed Abid Hashimi, Copyright � All rights reserved |======+  */

/*
	TODO:

	- Rendering
	- Straighten out all coordinate systems
	  - Screen
	  - World
	  - Texture
	- Optimization

	- Asset streaming

	- Debug code
  	  - Fonts
  	  - Logging
	  - Diagramming
	  - (A LITTLE GUI, but only a little!) Switches / sliders / etc.
	  - Thread Visualization

	ARCHITECTURE EXPLORATION
	- Z!
	  - Need to make a solid concept of ground levels to the camera
	  	can be freely placed in Z and have multiple ground levels
		in one sim region.
	  - Concept of ground in the collision loop so it can handle
	  	collisions coming onto AND OFF OF stairwells, for example.
	  - Go through and define how tall everything should be
	  - Make sure flying things can go through low walls
	  - How is this rendered?
	  - ZFudge!!!!!!
	- Collision Detection?
	  - Fix sword collision!
	  - Clean up perdicate proliferation! Can we make a nice clean
	  	set of flags/rules so that it's easy to understand how 
		things work in terms of special handling? This may involve
		making iteration handle everything instead of handling
		overlap outside and so on.
	  - Transient collision rules! Clear based on flag.
	    - Allow non-transient rules to override transient ones.
		- Entry/exit?
	  - What's the plan for robustness / shape definition?
	  - (Implement reprojection to handle interpenetration)
	- Implement multiple sim regions per frame
	  - Per-entity clocking
	  - Sim region merging? For multiple players?
	  - Simple zoom out view for testing
	  - Draw tile chunks so we can verify that things are aligned /
		in the chunks we want them to be in / etc...

	- Audio
	  - Sound effect triggers
	  - Ambient sounds
	  - Music

	- Metagame / save game?
	  - How do you enter "save slot"?
	  - Persistent unlocks/etc.
	  - Do we allow saved games? Probably yes, just only for "pausing",
	  * Continuous save for crash recovery?
	- Rudimentary world gen (no quality, just "what sorts of things" we do)
	  - Placement of background things
	  - Connectivity?
	  - Non-overlapping?
	  - Map display
	    - Magnets - how do they work?

	- AI
	  - Rudimentary monstar behanvior example
	  * Pathfinding
	  - AI "storage"

	* Animation, should probably lead into the rendering
	  - Skeletal animation
	  - Particle systems

	PRODUCTION
	-> GAME
	  - Entity system
	  - World generation
*/

#if !defined(HANDMADE_H)

#include "handmade_platform.h"

#define Minimum(A, B) ((A < B) ? (A) : (B))
#define Maximum(A, B) ((A > B) ? (A) : (B))

//
//
//

struct memory_arena
{
    memory_index Size;
    uint8 *Base;
    memory_index Used;

    int32 TempCount;
};

struct temporary_memory
{
    memory_arena *Arena;
    memory_index Used;
};

inline void
InitializeArena(memory_arena *Arena, memory_index Size, void *Base)
{
    Arena->Size = Size;
    Arena->Base = (uint8 *)Base;
    Arena->Used = 0;
    Arena->TempCount = 0;
}

#define PushStruct(Arena, type) (type *)PushSize_(Arena, sizeof(type))
#define PushArray(Arena, Count, type) (type *)PushSize_(Arena, (Count)*sizeof(type))
#define PushSize(Arena, Size) PushSize_(Arena, Size)
inline void *
PushSize_(memory_arena *Arena, memory_index Size)
{
    Assert((Arena->Used + Size) <= Arena->Size);
    void *Result = Arena->Base + Arena->Used;
    Arena->Used += Size;
    
    return(Result);
}

inline temporary_memory
BeginTemporaryMemory(memory_arena *Arena)
{
    temporary_memory Result;

    Result.Arena = Arena;
    Result.Used = Arena->Used;

    ++Arena->TempCount;

    return(Result);
}

inline void
EndTemporaryMemory(temporary_memory TempMem)
{
    memory_arena *Arena = TempMem.Arena;
    Assert(Arena->Used >= TempMem.Used);
    Arena->Used = TempMem.Used;
    Assert(Arena->TempCount > 0);
    --Arena->TempCount;
}

inline void
CheckArena(memory_arena *Arena)
{
    Assert(Arena->TempCount == 0);
}

#define ZeroStruct(Instance) ZeroSize(sizeof(Instance), &(Instance))
inline void
ZeroSize(memory_index Size, void *Ptr)
{
    // TODO(Khisrow): Check this guy for performance
    uint8 *Byte = (uint8 *)Ptr;
    while(Size--)
    {
        *Byte++ = 0;
    }
}

#include "handmade_instrinsics.h"
#include "handmade_math.h"
#include "handmade_world.h"
#include "handmade_sim_region.h"
#include "handmade_entity.h"
#include "handmade_render_group.h"

struct hero_bitmaps
{
    loaded_bitmap Head;
    loaded_bitmap Cape;
    loaded_bitmap Torso;
};

struct low_entity
{
    // TODO(Khisrow): It's kind of busted that P's can be invalid here,
    // AND we store whether they would be invalid in the flags field...
    // Can we do something better here?
    world_position P;
    sim_entity Sim;
};

struct controlled_hero
{
    uint32 EntityIndex;
    
    // NOTE(Khisrow): These are the controller requests for simulation
    v2 ddP;
    v2 dSword;
    real32 dZ;
};

struct pairwise_collision_rule
{
    bool32 CanCollide;
    uint32 StorageIndexA;
    uint32 StorageIndexB;
    
    pairwise_collision_rule *NextInHash;
};
struct game_state;
internal void AddCollisionRule(game_state *GameState, uint32 StorageIndexA, uint32 StorageIndexB, bool32 ShouldCollide);
internal void ClearCollisionRulesFor(game_state *GameState, uint32 StorageIndex);

struct ground_buffer
{
    // NOTE(Khisrow): An invalid P tells us that this ground_buffer has not been filled
    world_position P; // NOTE(Khisrow): This is the center of the bitmap
    loaded_bitmap Bitmap;
};

struct game_state
{
    memory_arena WorldArena;
    world *World;

    real32 TypicalFloorHeight;
    
    // TODO(Khisrow): Should we allow split-screen?
    uint32 CameraFollowingEntityIndex;
    world_position CameraP;

    controlled_hero ControlledHeroes[ArrayCount(((game_input *)0)->Controllers)];

    // TODO(Khisrow): Change the name to "stored entity"
    uint32 LowEntityCount;
    low_entity LowEntities[100000];

    loaded_bitmap Grass[2];
    loaded_bitmap Stone[4];
    loaded_bitmap Tuft[3];
    
    loaded_bitmap Backdrop;
    loaded_bitmap Shadow;
    hero_bitmaps HeroBitmaps[4];

    loaded_bitmap Tree;
    loaded_bitmap Sword;
    loaded_bitmap Stairwell;

    // TODO(Khisrow): Must be power of two
    pairwise_collision_rule *CollisionRuleHash[256];
    pairwise_collision_rule *FirstFreeCollisionRule;

    sim_entity_collision_volume_group *NullCollision;
    sim_entity_collision_volume_group *SwordCollision;
    sim_entity_collision_volume_group *StairCollision;
    sim_entity_collision_volume_group *PlayerCollision;
    sim_entity_collision_volume_group *MonstarCollision;
    sim_entity_collision_volume_group *FamiliarCollision;
    sim_entity_collision_volume_group *WallCollision;
    sim_entity_collision_volume_group *StandardRoomCollision;

    real32 Time;

    loaded_bitmap TestDiffuse; // TODO(Khisrow): Re-fill this guy with gray.
    loaded_bitmap TestNormal;
};

struct transient_state
{
    bool32 IsInitialized;
    memory_arena TranArena;    
    uint32 GroundBufferCount;
    ground_buffer *GroundBuffers;

    uint32 EnvMapWidth;
    uint32 EnvMapHeight;
    // NOTE(Khisrow): 0 is bottom, 1 is middle, 2 is top
    environment_map EnvMaps[3];
};

inline low_entity *
GetLowEntity(game_state *GameState, uint32 Index)
{
    low_entity *Result = 0;
    
    if((Index > 0) && (Index < GameState->LowEntityCount))
    {
        Result = GameState->LowEntities + Index;
    }

    return(Result);
}

#define HANDMADE_H
#endif
