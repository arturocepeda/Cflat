
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


class UClass {};

class UObject
{
public:
   UClass* GetClass(); // UObjectBase method, added to UObject for simplicity
   FName GetFName(); // UObjectBase method, added to UObject for simplicity
   FString GetName(); // UObjectUtilityBase method, added to UObject for simplicity
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
	 * Removes as many instances of Item as there are in the array, maintaining
	 * order but not indices.
	 *
	 * @param Item Item to remove from array.
	 * @returns Number of removed elements.
	 * @see Add, Insert, RemoveAll, RemoveAllSwap, RemoveSingle, RemoveSwap
	 */
   int32 Remove(const T& Item);

   T* begin();
   T* end();
};

#endif
