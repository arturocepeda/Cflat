
///////////////////////////////////////////////////////////////////////////////
//
//  Cflat v0.50
//  Embeddable lightweight scripting language with C++ syntax
//
//  Copyright (c) 2019-2023 Arturo Cepeda Pérez
//
//  ---------------------------------------------------------------------------
//
//  This software is provided 'as-is', without any express or implied
//  warranty. In no event will the authors be held liable for any damages
//  arising from the use of this software.
//
//  Permission is granted to anyone to use this software for any purpose,
//  including commercial applications, and to alter it and redistribute it
//  freely, subject to the following restrictions:
//
//  1. The origin of this software must not be misrepresented; you must not
//     claim that you wrote the original software. If you use this software
//     in a product, an acknowledgment in the product documentation would be
//     appreciated but is not required.
//
//  2. Altered source versions must be plainly marked as such, and must not be
//     misrepresented as being the original software.
//
//  3. This notice may not be removed or altered from any source distribution.
//
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
//
//  Declarations and comments extracted from Unreal Engine's header files.
//
//  Copyright Epic Games, Inc. All Rights Reserved.
//
///////////////////////////////////////////////////////////////////////////////


#pragma once

#if defined (CFLAT_ENABLED)

#define UE_SMALL_NUMBER (1.e-8f)

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;
typedef unsigned long long uint64;
typedef char int8;
typedef short int16;
typedef int int32;
typedef long long int64;


/**
 * Public name, available to the world.  Names are stored as a combination of
 * an index into a table of unique strings and an instance number.
 * Names are case-insensitive, but case-preserving (when WITH_CASE_PRESERVING_NAME is 1)
 */
class FName
{
public:
   FName(const char* Name);

   bool operator==(FName Other) const;
   bool operator!=(FName Other) const;
};

/**
 * A dynamically sizeable string.
 * @see https://docs.unrealengine.com/latest/INT/Programming/UnrealArchitecture/StringHandling/FString/
 */
class FString
{
public:
   FString(const char* String);
};


/**
 * A vector in 3-D space composed of components (X, Y, Z) with floating point precision.
 */
struct FVector
{
   /** Vector's X component. */
   double X;
   /** Vector's Y component. */
   double Y;
   /** Vector's Z component. */
   double Z;

   /**
    * Constructor using initial values for each component.
    *
    * @param InX X Coordinate.
    * @param InY Y Coordinate.
    * @param InZ Z Coordinate.
    */
   FVector(double InX, double InY, double InZ);

   /**
    * Calculates normalized version of vector without checking for zero length.
    *
    * @return Normalized version of vector.
    * @see GetSafeNormal()
    */
   FVector GetUnsafeNormal() const;
   /**
    * Normalize this vector in-place if it is larger than a given tolerance. Leaves it unchanged if not.
    *
    * @param Tolerance Minimum squared length of vector for normalization.
    * @return true if the vector was normalized correctly, false otherwise.
    */
   bool Normalize(double Tolerance = UE_SMALL_NUMBER);
	/**
	 * Get the length (magnitude) of this vector.
	 *
	 * @return The length of this vector.
	 */
   double Length() const;
	/**
	 * Get the squared length of this vector.
	 *
	 * @return The squared length of this vector.
	 */
   double SquaredLength() const;

   FVector operator+(const FVector& V) const;
   FVector operator-(const FVector& V) const;
   FVector operator*(double Scale) const;
   FVector operator/(double Scale) const;

   FVector operator+=(const FVector& V);
   FVector operator-=(const FVector& V);
   FVector operator*=(const FVector& V);
   FVector operator/=(const FVector& V);
};

/**
 * A vector in 2-D space composed of components (X, Y) with floating point precision.
 */
struct FVector2D
{
   /** Vector's X component. */
   double X;
   /** Vector's Y component. */
   double Y;

	/**
	 * Constructor using initial values for each component.
	 *
	 * @param InX X coordinate.
	 * @param InY Y coordinate.
	 */
   FVector2D(double InX, double InY);
};

/**
 * Implements a container for rotation information.
 *
 * All rotation values are stored in degrees.
 *
 * The angles are interpreted as intrinsic rotations applied in the order Yaw, then Pitch, then Roll. I.e., an object would be rotated
 * first by the specified yaw around its up axis (with positive angles interpreted as clockwise when viewed from above, along -Z), 
 * then pitched around its (new) right axis (with positive angles interpreted as 'nose up', i.e. clockwise when viewed along +Y), 
 * and then finally rolled around its (final) forward axis (with positive angles interpreted as clockwise rotations when viewed along +X).
 *
 * Note that these conventions differ from quaternion axis/angle. UE Quat always considers a positive angle to be a left-handed rotation, 
 * whereas Rotator treats yaw as left-handed but pitch and roll as right-handed.
 * 
 */
struct FRotator
{
   /** Rotation around the right axis (around Y axis), Looking up and down (0=Straight Ahead, +Up, -Down) */
   double Pitch;
   /** Rotation around the up axis (around Z axis), Turning around (0=Forward, +Right, -Left)*/
   double Yaw;
   /** Rotation around the forward axis (around X axis), Tilting your head, (0=Straight, +Clockwise, -CCW) */
   double Roll;

	/**
	 * Constructor.
	 *
	 * @param InPitch Pitch in degrees.
	 * @param InYaw Yaw in degrees.
	 * @param InRoll Roll in degrees.
	 */
   FRotator(double InPitch, double InYaw, double InRoll);
};


//
//	FColor
//	Stores a color with 8 bits of precision per channel.  
//	Note: Linear color values should always be converted to gamma space before stored in an FColor, as 8 bits of precision is not enough to store linear space colors!
//	This can be done with FLinearColor::ToFColor(true) 
//
struct FColor
{
   uint8 R;
   uint8 G;
   uint8 B;
   uint8 A;

   FColor(uint8 InR, uint8 InG, uint8 InB, uint8 InA = 255u);
};

/**
 * A linear, 32-bit/component floating point RGBA color.
 */
struct FLinearColor
{
   float R;
   float G;
   float B;
   float A;

   FLinearColor(float InR, float InG, float InB, float InA = 1.0f);
};


template <typename T>
struct TObjectPtr
{
	T* Get() const;
};


class UClass;
class UWorld;

class UObject
{
public:
   UClass* GetClass() const; // UObjectBase method, added to UObject for simplicity
   FName GetFName() const; // UObjectBase method, added to UObject for simplicity
   FString GetName() const; // UObjectUtilityBase method, added to UObject for simplicity

   virtual UWorld* GetWorld() const;
};

class UActorComponent;
class USceneComponent;

/**
 * Actor is the base class for an Object that can be placed or spawned in a level.
 * Actors may contain a collection of ActorComponents, which can be used to control how actors move, how they are rendered, etc.
 * The other main function of an Actor is the replication of properties and function calls across the network during play.
 */
class AActor : public UObject
{
public:
   /** Returns the location of the RootComponent of this Actor*/
   FVector GetActorLocation() const;
   /** Returns the rotation of the RootComponent of this Actor */
   FRotator GetActorRotation() const;

	/** 
	 * Move the actor instantly to the specified location. 
	 * 
	 * @param NewLocation	The new location to teleport the Actor to.
	 * @return	Whether the location was successfully set if not swept, or whether movement occurred if swept.
	 */
   bool SetActorLocation(const FVector& NewLocation);
	/**
	 * Set the Actor's rotation instantly to the specified rotation.
	 *
	 * @param	NewRotation	The new rotation for the Actor.
	 * @return	Whether the rotation was successfully set.
	 */
   bool SetActorRotation(FRotator NewRotation);
	/** 
	 * Move the actor instantly to the specified location and rotation.
	 * 
	 * @param NewLocation		The new location to teleport the Actor to.
	 * @param NewRotation		The new rotation for the Actor.
	 * @return	Whether the rotation was successfully set.
	 */
   bool SetActorLocationAndRotation(FVector NewLocation, FRotator NewRotation);

   /** Returns this actor's root component. */
   USceneComponent* GetRootComponent();
   /** Searches components array and returns first encountered component of the specified class */
   UActorComponent* GetComponentByClass(UClass* ComponentClass);
};

/**
 * ActorComponent is the base class for components that define reusable behavior that can be added to different types of Actors.
 * ActorComponents that have a transform are known as SceneComponents and those that can be rendered are PrimitiveComponents.
 *
 * @see [ActorComponent](https://docs.unrealengine.com/latest/INT/Programming/UnrealArchitecture/Actors/Components/index.html#actorcomponents)
 * @see USceneComponent
 * @see UPrimitiveComponent
 */
class UActorComponent : public UObject
{
public:
   /** Follow the Outer chain to get the  AActor  that 'Owns' this component */
   AActor* GetOwner() const;
};

/**
 * A SceneComponent has a transform and supports attachment, but has no rendering or collision capabilities.
 * Useful as a 'dummy' component in the hierarchy to offset others.
 * @see [Scene Components](https://docs.unrealengine.com/latest/INT/Programming/UnrealArchitecture/Actors/Components/index.html#scenecomponents)
 */
class USceneComponent : public UActorComponent
{
public:
   static UClass* StaticClass();

	/** 
	 * Set visibility of the component, if during game use this to turn on/off
	 */
   void SetVisibility(bool bNewVisibility, bool bPropagateToChildren = false);
};

/** 
 * The line batch component buffers and draws lines (and some other line-based shapes) in a scene. 
 *	This can be useful for debug drawing, but is not very performant for runtime use.
 */
class ULineBatchComponent : public USceneComponent
{
public:
	virtual void DrawLine(
		const FVector& Start,
		const FVector& End,
		const FLinearColor& Color,
		uint8 DepthPriority,
		float Thickness = 0.0f,
		float LifeTime = 0.0f);
};


/**
 * Structure containing information about one hit of a trace, such as point of impact and surface normal at that point.
 */
struct FHitResult
{
	/** Face index we hit (for complex hits with triangle meshes). */
	int32 FaceIndex;

	/**
	 * 'Time' of impact along trace direction (ranging from 0.0 to 1.0) if there is a hit, indicating time between TraceStart and TraceEnd.
	 * For swept movement (but not queries) this may be pulled back slightly from the actual time of impact, to prevent precision problems with adjacent geometry.
	 */
	float Time;
	 
	/** The distance from the TraceStart to the Location in world space. This value is 0 if there was an initial overlap (trace started inside another colliding object). */
	float Distance;
	
	/**
	 * The location in world space where the moving shape would end up against the impacted object, if there is a hit. Equal to the point of impact for line tests.
	 * Example: for a sphere trace test, this is the point where the center of the sphere would be located when it touched the other object.
	 * For swept movement (but not queries) this may not equal the final location of the shape since hits are pulled back slightly to prevent precision issues from overlapping another surface.
	 */
	FVector Location;

	/**
	 * Location in world space of the actual contact of the trace shape (box, sphere, ray, etc) with the impacted object.
	 * Example: for a sphere trace test, this is the point where the surface of the sphere touches the other object.
	 * @note: In the case of initial overlap (bStartPenetrating=true), ImpactPoint will be the same as Location because there is no meaningful single impact point to report.
	 */
	FVector ImpactPoint;

	/**
	 * Normal of the hit in world space, for the object that was swept. Equal to ImpactNormal for line tests.
	 * This is computed for capsules and spheres, otherwise it will be the same as ImpactNormal.
	 * Example: for a sphere trace test, this is a normalized vector pointing in towards the center of the sphere at the point of impact.
	 */
	FVector Normal;

	/**
	 * Normal of the hit in world space, for the object that was hit by the sweep, if any.
	 * For example if a sphere hits a flat plane, this is a normalized vector pointing out from the plane.
	 * In the case of impact with a corner or edge of a surface, usually the "most opposing" normal (opposed to the query direction) is chosen.
	 */
	FVector ImpactNormal;

	/**
	 * Start location of the trace.
	 * For example if a sphere is swept against the world, this is the starting location of the center of the sphere.
	 */
	FVector TraceStart;

	/**
	 * End location of the trace; this is NOT where the impact occurred (if any), but the furthest point in the attempted sweep.
	 * For example if a sphere is swept against the world, this would be the center of the sphere if there was no blocking hit.
	 */
	FVector TraceEnd;

	FHitResult();

	/** Utility to return the Actor that owns the Component that was hit. */
	AActor* GetActor() const;
};

enum ECollisionChannel : int
{
	ECC_WorldStatic,
	ECC_WorldDynamic,
	ECC_Pawn,
	ECC_Visibility,
	ECC_Camera,
	ECC_PhysicsBody,
	ECC_Vehicle,
	ECC_Destructible
};

/** Structure that contains list of object types the query is intersted in.  */
struct FCollisionObjectQueryParams
{
	/** Set of object type queries that it is interested in **/
	int32 ObjectTypesToQuery;

	/** Extra filtering done during object query. See declaration for filtering logic */
	FMaskFilter IgnoreMask;

	FCollisionObjectQueryParams();
	FCollisionObjectQueryParams(ECollisionChannel QueryChannel);

	void AddObjectTypesToQuery(ECollisionChannel QueryChannel);
	void RemoveObjectTypesToQuery(ECollisionChannel QueryChannel);
};

enum class EQueryMobilityType
{
	Any,
	Static,	//Any shape that is considered static by physx (static mobility)
	Dynamic	//Any shape that is considered dynamic by physx (movable/stationary mobility)
};

/** Structure that defines parameters passed into collision function */
struct FCollisionQueryParams
{
	/** Tag used to provide extra information or filtering for debugging of the trace (e.g. Collision Analyzer) */
	FName TraceTag;

	/** Tag used to indicate an owner for this trace */
	FName OwnerTag;

	/** Whether we should trace against complex collision */
	bool bTraceComplex;

	/** Whether we want to find out initial overlap or not. If true, it will return if this was initial overlap. */
	bool bFindInitialOverlaps;

	/** Whether we want to return the triangle face index for complex static mesh traces */
	bool bReturnFaceIndex;

	/** Whether we want to include the physical material in the results. */
	bool bReturnPhysicalMaterial;

	/** Whether to ignore blocking results. */
	bool bIgnoreBlocks;

	/** Whether to ignore touch/overlap results. */
	bool bIgnoreTouches;

	/** Whether to skip narrow phase checks (only for overlaps). */
	bool bSkipNarrowPhase;

	/** Whether to ignore traces to the cluster union and trace against its children instead. */
	bool bTraceIntoSubComponents;

	/** Filters query by mobility types (static vs stationary/movable)*/
	EQueryMobilityType MobilityType;

	// Constructors
	FCollisionQueryParams();

	// Utils

	/** Add an actor for this trace to ignore */
	void AddIgnoredActor(const AActor* InIgnoreActor);
};

class UWorld final : public UObject
{
public:
	/** Line Batchers. All lines to be drawn in the world. */
	TObjectPtr<ULineBatchComponent> LineBatcher;

	/**
	 *  Trace a ray against the world using a specific channel and return the first blocking hit
	 *  @param  OutHit          First blocking hit found
	 *  @param  Start           Start location of the ray
	 *  @param  End             End location of the ray
	 *  @param  TraceChannel    The 'channel' that this ray is in, used to determine which components to hit
	 *  @param  Params          Additional parameters used for the trace
	 * 	@param 	ResponseParam	ResponseContainer to be used for this trace	 
	 *  @return TRUE if a blocking hit is found
	 */
	bool LineTraceSingleByChannel(struct FHitResult& OutHit,const FVector& Start,const FVector& End,ECollisionChannel TraceChannel,const FCollisionQueryParams& Params = FCollisionQueryParams::DefaultQueryParam) const;

	/**
	 *  Trace a ray against the world using object types and return the first blocking hit
	 *  @param  OutHit          First blocking hit found
	 *  @param  Start           Start location of the ray
	 *  @param  End             End location of the ray
	 *  @param	ObjectQueryParams	List of object types it's looking for
	 *  @param  Params          Additional parameters used for the trace
	 *  @return TRUE if any hit is found
	 */
	bool LineTraceSingleByObjectType(struct FHitResult& OutHit,const FVector& Start,const FVector& End,const FCollisionObjectQueryParams& ObjectQueryParams, const FCollisionQueryParams& Params = FCollisionQueryParams::DefaultQueryParam) const;

	/**
	 *  Trace a ray against the world using a specific channel and return overlapping hits and then first blocking hit
	 *  Results are sorted, so a blocking hit (if found) will be the last element of the array
	 *  Only the single closest blocking result will be generated, no tests will be done after that
	 *  @param  OutHits         Array of hits found between ray and the world
	 *  @param  Start           Start location of the ray
	 *  @param  End             End location of the ray
	 *  @param  TraceChannel    The 'channel' that this ray is in, used to determine which components to hit
	 *  @param  Params          Additional parameters used for the trace
	 * 	@param 	ResponseParam	ResponseContainer to be used for this trace	 
	 *  @return TRUE if OutHits contains any blocking hit entries
	 */
	bool LineTraceMultiByChannel(TArray<struct FHitResult>& OutHits,const FVector& Start,const FVector& End,ECollisionChannel TraceChannel,const FCollisionQueryParams& Params = FCollisionQueryParams::DefaultQueryParam) const;

	/**
	 *  Trace a ray against the world using object types and return overlapping hits and then first blocking hit
	 *  Results are sorted, so a blocking hit (if found) will be the last element of the array
	 *  Only the single closest blocking result will be generated, no tests will be done after that
	 *  @param  OutHits         Array of hits found between ray and the world
	 *  @param  Start           Start location of the ray
	 *  @param  End             End location of the ray
	 *	@param	ObjectQueryParams	List of object types it's looking for
	 *  @param  Params          Additional parameters used for the trace
	 *  @return TRUE if any hit is found
	 */
	bool LineTraceMultiByObjectType(TArray<struct FHitResult>& OutHits,const FVector& Start,const FVector& End,const FCollisionObjectQueryParams& ObjectQueryParams, const FCollisionQueryParams& Params = FCollisionQueryParams::DefaultQueryParam) const;	
};


/**
 * Templated dynamic array
 *
 * A dynamically sized array of typed elements.  Makes the assumption that your elements are relocate-able; 
 * i.e. that they can be transparently moved to new memory without a copy constructor.  The main implication 
 * is that pointers to elements in the TArray may be invalidated by adding or removing other elements to the array. 
 * Removal of elements is O(N) and invalidates the indices of subsequent elements.
 *
 * Caution: as noted below some methods are not safe for element types that require constructors.
 *
 **/
template<typename T>
class TArray
{
public:
	/**
	 * Returns true if the array is empty and contains no elements. 
	 *
	 * @returns True if the array is empty.
	 * @see Num
	 */
   bool IsEmpty() const;
	/**
	 * Returns number of elements in array.
	 *
	 * @returns Number of elements in array.
	 * @see GetSlack
	 */
   int32 Num() const;

	/**
	 * Reserves memory such that the array can contain at least Number elements.
	 *
	 * @param Number The number of elements that the array should be able to contain after allocation.
	 * @see Shrink
	 */
   void Reserve(int32 Number);
	/**
	 * Resizes array to given number of elements.
	 *
	 * @param NewNum New size of the array.
	 */
   void SetNum(int32 NewNum);
   	/**
	 * Resizes array to given number of elements, optionally shrinking it.
	 * New elements will be zeroed.
	 *
	 * @param NewNum New size of the array.
	 */
   void SetNumZeroed(int32 NewNum);
	/**
	 * Resizes array to given number of elements. New elements will be uninitialized.
	 *
	 * @param NewNum New size of the array.
	 */
   void SetNumUninitialized(int32 NewNum);
	/**
	 * Empties the array. It calls the destructors on held items if needed.
    */
   void Empty();

   T& operator[](int Index);

	/**
	 * Adds a new item to the end of the array, possibly reallocating the whole array to fit.
	 *
	 * Move semantics version.
	 *
	 * @param Item The item to add
	 * @return Index to the new item
	 * @see AddDefaulted, AddUnique, AddZeroed, Append, Insert
	 */
   void Add(const T& Item);
    /**
     * Removes an element (or elements) at given location, then shrinks
     * the array.
     *
     * @param Index Location in array of the element to remove.
     */
   void RemoveAt(int32 Index);

   T* begin();
   T* end();
};

#endif
