
///////////////////////////////////////////////////////////////////////////////
//
//  Cflat v0.60
//  Embeddable lightweight scripting language with C++ syntax
//
//  Copyright (c) 2019-2024 Arturo Cepeda Pérez and contributors
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

#define TEXT(x) L##x

#define UE_SMALL_NUMBER (1.e-8f)
#define UE_DOUBLE_SMALL_NUMBER (1.e-8)

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;
typedef unsigned long long uint64;
typedef char int8;
typedef short int16;
typedef int int32;
typedef long long int64;

typedef wchar_t TCHAR;


/**
 * A dynamically sizeable string.
 * @see https://docs.unrealengine.com/latest/INT/Programming/UnrealArchitecture/StringHandling/FString/
 */
class FString
{
public:
   FString(const char* String);
   FString(const TCHAR* String);
   FString();

   FString& Append(const char* String);
   FString& Append(const TCHAR* String);
   FString& Append(const FString& String);
   
   const TCHAR* operator*() const;

   static FString FromInt(int32 Num);
   static FString SanitizeFloat(double InFloat);
};

FString operator+(const FString& Lhs, const FString& Rhs);

/**
 * Public name, available to the world.  Names are stored as a combination of
 * an index into a table of unique strings and an instance number.
 * Names are case-insensitive, but case-preserving (when WITH_CASE_PRESERVING_NAME is 1)
 */
class FName
{
public:
   FName();
   FName(const char* Name);
   FName(FString Name);

   const FString& ToString() const;
   void ToString(FString& Out) const;

   bool operator==(FName Other) const;
   bool operator!=(FName Other) const;
};

class FText
{
public:
   static const FText& GetEmpty();

public:  
   /**
    * Generate an FText representing the passed in string
    */
   static FText FromString( const FString& String );
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

   /** A zero vector (0,0,0) */
   static const FVector ZeroVector;
   /** One vector (1,1,1) */
   static const FVector OneVector;
   /** Unreal up vector (0,0,1) */
   static const FVector UpVector;
   /** Unreal down vector (0,0,-1) */
   static const FVector DownVector;
   /** Unreal forward vector (1,0,0) */
   static const FVector ForwardVector;
   /** Unreal backward vector (-1,0,0) */
   static const FVector BackwardVector;
   /** Unreal right vector (0,1,0) */
   static const FVector RightVector;
   /** Unreal left vector (0,-1,0) */
   static const FVector LeftVector;
   /** Unit X axis vector (1,0,0) */
   static const FVector XAxisVector;
   /** Unit Y axis vector (0,1,0) */
   static const FVector YAxisVector;
   /** Unit Z axis vector (0,0,1) */
   static const FVector ZAxisVector;

   FVector();
   /**
    * Constructor using initial values for each component.
    *
    * @param InX X Coordinate.
    * @param InY Y Coordinate.
    * @param InZ Z Coordinate.
    */
   FVector(double InX, double InY, double InZ);

   /**
    * Set the values of the vector directly.
    *
    * @param InX New X coordinate.
    * @param InY New Y coordinate.
    * @param InZ New Z coordinate.
    */
   void Set(double InX, double InY, double InZ);
   /**
    * Calculate the dot product between this and another vector.
    *
    * @param V The other vector.
    * @return The dot product.
    */
   double Dot(const FVector& V) const;
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
   /**
    * Checks whether all components of the vector are exactly zero.
    *
    * @return true if the vector is exactly zero, false otherwise.
    */
   bool IsZero() const;
   /**
    * Checks whether vector is normalized.
    *
    * @return true if normalized, false otherwise.
    */
   bool IsNormalized() const;
   /**
    * Normalize this vector in-place if it is larger than a given tolerance. Leaves it unchanged if not.
    *
    * @param Tolerance Minimum squared length of vector for normalization.
    * @return true if the vector was normalized correctly, false otherwise.
    */
   bool Normalize(double Tolerance = UE_SMALL_NUMBER);
   /**
    * Calculates normalized version of vector without checking for zero length.
    *
    * @return Normalized version of vector.
    * @see GetSafeNormal()
    */
   FVector GetUnsafeNormal() const;
   /**
    * Euclidean distance between two points.
    *
    * @param V1 The first point.
    * @param V2 The second point.
    * @return The distance between two points.
    */
   static double Dist(const FVector &V1, const FVector &V2);
   static double Distance(const FVector &V1, const FVector &V2);
   /**
    * Squared distance between two points.
    *
    * @param V1 The first point.
    * @param V2 The second point.
    * @return The squared distance between two points.
    */
   static double DistSquared(const FVector &V1, const FVector &V2);

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

   FVector2D();
   /**
    * Constructor using initial values for each component.
    *
    * @param InX X coordinate.
    * @param InY Y coordinate.
    */
   FVector2D(double InX, double InY);
};

struct FQuat
{
   double X;
   double Y;
   double Z;
   double W;

public:
   FQuat();
   FQuat(double InX, double InY, double InZ, double InW);
};

/* Vector Typedefs */
typedef FVector FVector_NetQuantizeNormal;
typedef FVector FVector_NetQuantize;
typedef FVector FVector3d;
typedef FVector FVector3f;

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

   FRotator();
   /**
    * Constructor.
    *
    * @param InPitch Pitch in degrees.
    * @param InYaw Yaw in degrees.
    * @param InRoll Roll in degrees.
    */
   FRotator(double InPitch, double InYaw, double InRoll);
   /**
    * Get the result of adding a rotator to this.
    *
    * @param R The other rotator.
    * @return The result of adding a rotator to this.
    */
   FRotator operator+(const FRotator& R) const;
   /**
    * Get the result of subtracting a rotator from this.
    *
    * @param R The other rotator.
    * @return The result of subtracting a rotator from this.
    */
   FRotator operator-(const FRotator& R) const;
   /**
    * Get the result of scaling this rotator.
    *
    * @param Scale The scaling factor.
    * @return The result of scaling.
    */
   FRotator operator*(double Scale);
   /**
    * Multiply this rotator by a scaling factor.
    *
    * @param Scale The scaling factor.
    * @return Copy of the rotator after scaling.
    */
   FRotator operator*=(double Scale);
   /**
    * Checks whether two rotators are identical. This checks each component for exact equality.
    *
    * @param R The other rotator.
    * @return true if two rotators are identical, otherwise false.
    * @see Equals()
    */
   bool operator==(const FRotator& R) const;
   /**
    * Checks whether two rotators are different.
    *
    * @param V The other rotator.
    * @return true if two rotators are different, otherwise false.
    */
   bool operator!=(const FRotator& V) const;
   /**
    * Adds another rotator to this.
    *
    * @param R The other rotator.
    * @return Copy of rotator after addition.
    */
   FRotator operator+=(const FRotator& R);
   /**
    * Subtracts another rotator from this.
    *
    * @param R The other rotator.
    * @return Copy of rotator after subtraction.
    */
   FRotator operator-=(const FRotator& R);
   /**
    * Checks whether this has exactly zero rotation, when treated as an orientation.
    * This means that TRotator(0, 0, 360) is "zero", because it is the same final orientation as the zero rotator.
    *
    * @return true if this has exactly zero rotation, otherwise false.
    */
   bool IsZero() const;
   /**
    * Checks whether two rotators are equal within specified tolerance, when treated as an orientation.
    * This means that TRotator(0, 0, 360).Equals(TRotator(0,0,0)) is true, because they represent the same final orientation.
    * It can compare only wound rotators (i.e. multiples of 360 degrees) that end up in a same rotation
    * Rotators that represent the same final rotation, but get there via different intermediate rotations aren't equal
    * i.e. TRotator(0, 45, 0).Equals(TRotator(180, 135, 180)) is false
    *
    * @param R The other rotator.
    * @param Tolerance Error Tolerance.
    * @return true if two rotators are equal, within specified tolerance, otherwise false.
    */
   bool Equals(const FRotator& R) const;
   /**
    * Adds to each component of the rotator.
    *
    * @param DeltaPitch Change in pitch. (+/-)
    * @param DeltaYaw Change in yaw. (+/-)
    * @param DeltaRoll Change in roll. (+/-)
    * @return Copy of rotator after addition.
    */
   FRotator Add(double DeltaPitch, double DeltaYaw, double DeltaRoll);
   /**
    * Returns the inverse of the rotator.
    */
   FRotator GetInverse() const;
   /**
    * Convert a rotation into a unit vector facing in its direction.
    *
    * @return Rotation as a unit direction vector.
    */
   FVector Vector() const;
   /**
    * Get Rotation as a quaternion.
    *
    * @return Rotation as a quaternion.
    */
   FQuat Quaternion() const;
   /**
    * Convert a Rotator into floating-point Euler angles (in degrees). Rotator now stored in degrees.
    *
    * @return Rotation as a Euler angle vector.
    */
   FVector Euler() const;
   /**
    * Rotate a vector rotated by this rotator.
    *
    * @param V The vector to rotate.
    * @return The rotated vector.
    */
   FVector RotateVector(const FVector& V) const;
   /** 
    * Create a copy of this rotator and normalize, removes all winding and creates the "shortest route" rotation. 
    *
    * @return Normalized copy of this rotator
    */
   FRotator GetNormalized() const;
   /**
    * In-place normalize, removes all winding and creates the "shortest route" rotation.
    */
   void Normalize();
   /**
    * Convert a vector of floating-point Euler angles (in degrees) into a Rotator. Rotator now stored in degrees
    *
    * @param Euler Euler angle vector.
    * @return A rotator from a Euler angle.
    */
   static FRotator MakeFromEuler(const FVector& Euler);
};

struct FTransform
{
   /**
    * The identity transformation (Rotation = TQuat<T>::Identity, Translation = TVector<T>::ZeroVector, Scale3D = (1,1,1))
    */
   static const FTransform Identity;

   FTransform();

   FRotator Rotator() const;

   /**
    * Sets the rotation component
    * @param NewRotation The new value for the rotation component
    */
   void SetRotation(const FQuat& NewRotation);
   /**
    * Sets the translation component
    * @param NewTranslation The new value for the translation component
    */
   void SetTranslation(const FVector& NewTranslation);
   /**
    * Sets the Scale3D component
    * @param NewScale3D The new value for the Scale3D component
    */
   void SetScale3D(const FVector& NewScale3D);

   /**
    * Returns the rotation component
    *
    * @return The rotation component
    */
   FQuat GetRotation() const;
   /**
    * Returns the translation component
    *
    * @return The translation component
    */
   FVector GetTranslation() const;
   /**
    * Returns the Scale3D component
    *
    * @return The Scale3D component
    */
   FVector GetScale3D() const;
};


//
// FColor
// Stores a color with 8 bits of precision per channel.  
// Note: Linear color values should always be converted to gamma space before stored in an FColor, as 8 bits of precision is not enough to store linear space colors!
// This can be done with FLinearColor::ToFColor(true) 
//
struct FColor
{
   uint8 R;
   uint8 G;
   uint8 B;
   uint8 A;

   FColor();
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

   FLinearColor();
   FLinearColor(float InR, float InG, float InB, float InA = 1.0f);
};


template <typename T>
struct TObjectPtr
{
   TObjectPtr(T* Object);

   T* Get() const;
};

template <typename To, typename From>
To* Cast(From* Src);


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
	*
	* @param Slack (Optional) The expected usage size after empty operation. Default is 0.
	*/
	void Empty(int32 Slack = 0)

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

template<typename InElementType>
class TSet
{
public:
   /** Initialization constructor. */
   TSet();

   /**
    * Removes all elements from the set, potentially leaving space allocated for an expected number of elements about to be added.
    */
   void Empty();

   /**
    * Returns true if the sets is empty and contains no elements. 
    *
    * @returns True if the set is empty.
    * @see Num
    */
   bool IsEmpty() const;

   /** @return the number of elements. */
   int32 Num() const;

   /**
    * Adds an element to the set.
    *
    * @param   InElement               Element to add to set
    */
   void Add(const InElementType&  InElement);

   /**
    * Finds an element with the given key in the set.
    * @param Key - The key to search for.
    * @return A pointer to an element with the given key.  If no element in the set has the given key, this will return NULL.
    */
   InElementType* Find(const InElementType& Key);

   /**
    * Removes all elements from the set matching the specified key.
    * @param Key - The key to match elements against.
    * @return The number of elements removed.
    */
   int32 Remove(const InElementType& Key);

   /**
    * Checks if the element contains an element with the given key.
    * @param Key - The key to check for.
    * @return true if the set contains an element with the given key.
    */
   bool Contains(const InElementType& Key) const;

   template<bool bConst, bool bRangedFor = false>
   class TBaseIterator
   {
   public:
      TBaseIterator& operator++();
      InElementType& operator*() const;
      bool operator!=(const TBaseIterator& Rhs) const;
   };

   using TRangedForIterator      = TBaseIterator<false, true>;

   /**
    * DO NOT USE DIRECTLY
    * STL-like iterators to enable range-based for loop support.
    */
   TRangedForIterator      begin()       { return TRangedForIterator     (Elements.begin()); }
   TRangedForIterator      end()         { return TRangedForIterator     (Elements.end());   }
};


class UClass;
class UWorld;
class ULineBatchComponent;

class UObject
{
public:
   UClass* GetClass() const; // UObjectBase method, added to UObject for simplicity
   FName GetFName() const; // UObjectBase method, added to UObject for simplicity
   FString GetName() const; // UObjectUtilityBase method, added to UObject for simplicity

   virtual UWorld* GetWorld() const;
};

class UInterface : public UObject
{
};

bool IsValid(const UObject *Test);

template<typename T> 
T* LoadObject(UObject* Outer, const TCHAR* Name);

class UField : public UObject
{   
};

class UStruct : public UField
{   
};

class UClass : public UStruct
{
public:
   UObject* GetDefaultObject(bool bCreateIfNeeded = true) const;
};

class UScriptStruct : public UStruct
{
};

struct FSoftObjectPtr
{
   FSoftObjectPtr();
   FSoftObjectPtr(const UObject* Object);
   FSoftObjectPtr(TObjectPtr<UObject> Object);
   ~FSoftObjectPtr();

   UObject* LoadSynchronous() const;
   UObject* Get() const;
};

class AActor;
class APawn;

template <typename T>
class TSubclassOf
{
public:
	TSubclassOf(UClass* From);
	TSubclassOf& operator=(UClass* From);
	UClass* operator*() const;
	UClass* Get() const;
};

enum class ETeleportType : uint8
{
   /** Do not teleport physics body. This means velocity will reflect the movement between initial and final position, and collisions along the way will occur */
   None,
   /** Teleport physics body so that velocity remains the same and no collision occurs */
   TeleportPhysics,
   /** Teleport physics body and reset physics state completely */
   ResetPhysics,
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

/** This filter allows us to refine queries (channel, object) with an additional level of ignore by tagging entire classes of objects (e.g. "Red team", "Blue team")
    If(QueryIgnoreMask & ShapeFilter != 0) filter out */
typedef uint8 FMaskFilter;

enum ECollisionChannel;

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
   Static,  //Any shape that is considered static by physx (static mobility)
   Dynamic  //Any shape that is considered dynamic by physx (movable/stationary mobility)
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

   static const FCollisionQueryParams DefaultQueryParam;
};

/** Defines available strategies for handling the case where an actor is spawned in such a way that it penetrates blocking collision. */
enum class ESpawnActorCollisionHandlingMethod : uint8
{
   /** Fall back to default settings. */
   Undefined,
   /** Actor will spawn in desired location, regardless of collisions. */
   AlwaysSpawn,
   /** Actor will try to find a nearby non-colliding location (based on shape components), but will always spawn even if one cannot be found. */
   AdjustIfPossibleButAlwaysSpawn,
   /** Actor will try to find a nearby non-colliding location (based on shape components), but will NOT spawn unless one is found. */
   AdjustIfPossibleButDontSpawnIfColliding,
   /** Actor will fail to spawn. */
   DontSpawnIfColliding
};

/** Determines how the transform being passed into actor spawning methods interact with the actor's default root component */
enum class ESpawnActorScaleMethod : uint8
{
   /** Ignore the default scale in the actor's root component and hard-set it to the value of SpawnTransform Parameter */
   OverrideRootScale,
   /** Multiply value of the SpawnTransform Parameter with the default scale in the actor's root component */
   MultiplyWithRoot,
   SelectDefaultAtRuntime
};

/* Struct of optional parameters passed to SpawnActor function(s). */
struct FActorSpawnParameters
{
   FActorSpawnParameters();

   /* A name to assign as the Name of the Actor being spawned. If no value is specified, the name of the spawned Actor will be automatically generated using the form [Class]_[Number]. */
   FName Name;

   /* An Actor to use as a template when spawning the new Actor. The spawned Actor will be initialized using the property values of the template Actor. If left NULL the class default object (CDO) will be used to initialize the spawned Actor. */
   AActor* Template;

   /* The Actor that spawned this Actor. (Can be left as NULL). */
   AActor* Owner;

   /* The APawn that is responsible for damage done by the spawned Actor. (Can be left as NULL). */
   APawn* Instigator;

   /** Method for resolving collisions at the spawn point. Undefined means no override, use the actor's setting. */
   ESpawnActorCollisionHandlingMethod SpawnCollisionHandlingOverride;

   /** Determines whether to multiply or override root component with provided spawn transform */
   ESpawnActorScaleMethod TransformScaleMethod = ESpawnActorScaleMethod::MultiplyWithRoot;
};

/**
 * Generic implementation for most platforms
 */
struct FGenericPlatformMath
{
   /**
    * Performs a bit cast of the given float to an unsigned int of the same bit width.
    * @param F The float to bit cast to an unsigned integer.
   *  @return A bitwise copy of the float in a 32-bit unsigned integer value.
    */
   static inline uint32 AsUInt(float F);

   /** 
    * Performs a bit cast of the given double to an unsigned int of the same bit width.
    * @param D The double to bit cast to an unsigned integer.
   *  @return A bitwise copy of the double in a 64-bit unsigned integer value.
    */
   static inline uint64 AsUInt(double F);

   /** 
    * Performs a bit cast of the given unsigned int to float of the same bit width.
    * @param U The 32-bit unsigned int to bit cast to a 32-bit float.
   *  @return A bitwise copy of the 32-bit float in a 32-bit unsigned integer value.
    */
   static inline float AsFloat(uint32 U);

   /** 
    * Performs a bit cast of the given unsigned int to float of the same bit width.
    * @param U The 64-bit unsigned int to bit cast to a 64-bit float.
   *  @return A bitwise copy of the 64-bit float in a 64-bit unsigned integer value.
    */
   static inline double AsFloat(uint64 U);


   /**
    * Converts a float to an integer with truncation towards zero.
    * @param F    Floating point value to convert
    * @return     Truncated integer.
    */
   static int32 TruncToInt32(float F);
   static int32 TruncToInt32(double F);
   static int64 TruncToInt64(double F);

   static int32 TruncToInt(float F);
   static int64 TruncToInt(double F);

   /**
    * Converts a float to an integer value with truncation towards zero.
    * @param F    Floating point value to convert
    * @return     Truncated integer value.
    */
   static float TruncToFloat(float F);

   /**
    * Converts a double to an integer value with truncation towards zero.
    * @param F    Floating point value to convert
    * @return     Truncated integer value.
    */
   static double TruncToDouble(double F);

   static double TruncToFloat(double F);

   /**
    * Converts a float to a nearest less or equal integer.
    * @param F    Floating point value to convert
    * @return     An integer less or equal to 'F'.
    */
   static int32 FloorToInt32(float F);
   static int32 FloorToInt32(double F);
   static int64 FloorToInt64(double F);

   static int32 FloorToInt(float F);
   static int64 FloorToInt(double F);
   
   
   /**
   * Converts a float to the nearest less or equal integer.
   * @param F     Floating point value to convert
   * @return      An integer less or equal to 'F'.
   */
   static float FloorToFloat(float F);

   /**
   * Converts a double to a less or equal integer.
   * @param F     Floating point value to convert
   * @return      The nearest integer value to 'F'.
   */
   static double FloorToDouble(double F);

   static double FloorToFloat(double F);

   /**
    * Converts a float to the nearest integer. Rounds up when the fraction is .5
    * @param F    Floating point value to convert
    * @return     The nearest integer to 'F'.
    */
   static int32 RoundToInt32(float F);
   static int32 RoundToInt32(double F);
   static int64 RoundToInt64(double F);
   
   static int32 RoundToInt(float F);
   static int64 RoundToInt(double F);

   /**
   * Converts a float to the nearest integer. Rounds up when the fraction is .5
   * @param F     Floating point value to convert
   * @return      The nearest integer to 'F'.
   */
   static float RoundToFloat(float F);

   /**
   * Converts a double to the nearest integer. Rounds up when the fraction is .5
   * @param F     Floating point value to convert
   * @return      The nearest integer to 'F'.
   */
   static double RoundToDouble(double F);

   static double RoundToFloat(double F);

   /**
   * Converts a float to the nearest greater or equal integer.
   * @param F     Floating point value to convert
   * @return      An integer greater or equal to 'F'.
   */
   static int32 CeilToInt32(float F);
   static int32 CeilToInt32(double F);
   static int64 CeilToInt64(double F);

   static int32 CeilToInt(float F);
   static int64 CeilToInt(double F);

   /**
   * Converts a float to the nearest greater or equal integer.
   * @param F     Floating point value to convert
   * @return      An integer greater or equal to 'F'.
   */
   static float CeilToFloat(float F);

   /**
   * Converts a double to the nearest greater or equal integer.
   * @param F     Floating point value to convert
   * @return      An integer greater or equal to 'F'.
   */
   static double CeilToDouble(double F);

   static double CeilToFloat(double F);

   /**
    * Converts a double to nearest int64 with ties rounding to nearest even
    * May incur a performance penalty. Asserts on platforms that do not support this mode.
    * @param F    Double precision floating point value to convert
    * @return     The 64-bit integer closest to 'F', with ties going to the nearest even number
    */
   static int64 RoundToNearestTiesToEven(double F);

   /**
   * Returns signed fractional part of a float.
   * @param Value Floating point value to convert
   * @return      A float between >=0 and < 1 for nonnegative input. A float between >= -1 and < 0 for negative input.
   */
   static float Fractional(float Value);

   static double Fractional(double Value);

   /**
   * Returns the fractional part of a float.
   * @param Value Floating point value to convert
   * @return      A float between >=0 and < 1.
   */
   static float Frac(float Value);   

   static double Frac(double Value);

   /**
   * Breaks the given value into an integral and a fractional part.
   * @param InValue  Floating point value to convert
   * @param OutIntPart Floating point value that receives the integral part of the number.
   * @return         The fractional part of the number.
   */
   static float Modf(const float InValue, float* OutIntPart);

   /**
   * Breaks the given value into an integral and a fractional part.
   * @param InValue  Floating point value to convert
   * @param OutIntPart Floating point value that receives the integral part of the number.
   * @return         The fractional part of the number.
   */
   static double Modf(const double InValue, double* OutIntPart);

   // Returns e^Value
   static float Exp( float Value );
   static double Exp(double Value);

   // Returns 2^Value
   static float Exp2( float Value );
   static double Exp2(double Value);

   static float Loge( float Value );
   static double Loge(double Value);

   static float LogX( float Base, float Value );
   static double LogX(double Base, double Value);

   // 1.0 / Loge(2) = 1.4426950f
   static float Log2( float Value );
   // 1.0 / Loge(2) = 1.442695040888963387
   static double Log2(double Value);

   /**
    * Returns the floating-point remainder of X / Y
    * Warning: Always returns remainder toward 0, not toward the smaller multiple of Y.
    *       So for example Fmod(2.8f, 2) gives .8f as you would expect, however, Fmod(-2.8f, 2) gives -.8f, NOT 1.2f
    * Use Floor instead when snapping positions that can be negative to a grid
    *
    * This is forced to *NOT* inline so that divisions by constant Y does not get optimized in to an inverse scalar multiply,
    * which is not consistent with the intent nor with the vectorized version.
    */

   static float Fmod(float X, float Y);
   static double Fmod(double X, double Y);

   static float Sin( float Value );
   static double Sin( double Value );

   static float Asin( float Value );
   static double Asin( double Value );

   static float Sinh(float Value);
   static double Sinh(double Value);

   static float Cos( float Value );
   static double Cos( double Value );

   static float Acos( float Value );
   static double Acos( double Value );

   static float Cosh(float Value);
   static double Cosh(double Value);

   static float Tan( float Value );
   static double Tan( double Value );

   static float Atan( float Value );
   static double Atan( double Value );

   static float Tanh(float Value);
   static double Tanh(double Value);

   static float Atan2( float Y, float X );
   static double Atan2( double Y, double X );

   static float Sqrt( float Value );
   static double Sqrt( double Value );

   static float Pow( float A, float B );
   static double Pow( double A, double B );

   /** Computes a fully accurate inverse square root */
   static float InvSqrt( float F );
   static double InvSqrt( double F );

   /** Computes a faster but less accurate inverse square root */
   static float InvSqrtEst( float F );
   static double InvSqrtEst( double F );

   /** Return true if value is NaN (not a number). */
   static bool IsNaN( float A );
   static bool IsNaN(double A);

   /** Return true if value is finite (not NaN and not Infinity). */
   static bool IsFinite( float A );
   static bool IsFinite(double A);

   static bool IsNegativeOrNegativeZero(float A);

   static bool IsNegativeOrNegativeZero(double A);

   /** Returns a random integer between 0 and RAND_MAX, inclusive */
   static int32 Rand();

   /** Seeds global random number functions Rand() and FRand() */
   static void RandInit(int32 Seed);

   /** Returns a random float between 0 and 1, inclusive. */
   static float FRand();

   /** Seeds future calls to SRand() */
   static void SRandInit( int32 Seed );

   /** Returns the current seed for SRand(). */
   static int32 GetRandSeed();

   /** Returns a seeded random float in the range [0,1), using the seed from SRandInit(). */
   static float SRand();

   /**
    * Computes the base 2 logarithm for an integer value.
    * The result is rounded down to the nearest integer.
    *
    * @param Value      The value to compute the log of
    * @return        Log2 of Value. 0 if Value is 0.
    */   
   static uint32 FloorLog2(uint32 Value);

   /**
    * Computes the base 2 logarithm for a 64-bit value.
    * The result is rounded down to the nearest integer.
    *
    * @param Value      The value to compute the log of
    * @return        Log2 of Value. 0 if Value is 0.
    */   
   static uint64 FloorLog2_64(uint64 Value);

   /**
    * Counts the number of leading zeros in the bit representation of the 8-bit value
    *
    * @param Value the value to determine the number of leading zeros for
    *
    * @return the number of zeros before the first "on" bit
    */
   static uint8 CountLeadingZeros8(uint8 Value);

   /**
    * Counts the number of leading zeros in the bit representation of the 32-bit value
    *
    * @param Value the value to determine the number of leading zeros for
    *
    * @return the number of zeros before the first "on" bit
    */
   static uint32 CountLeadingZeros(uint32 Value);

   /**
    * Counts the number of leading zeros in the bit representation of the 64-bit value
    *
    * @param Value the value to determine the number of leading zeros for
    *
    * @return the number of zeros before the first "on" bit
    */
   static uint64 CountLeadingZeros64(uint64 Value);

   /**
    * Counts the number of trailing zeros in the bit representation of the value
    *
    * @param Value the value to determine the number of trailing zeros for
    *
    * @return the number of zeros after the last "on" bit
    */
   static uint32 CountTrailingZeros(uint32 Value);

   /**
    * Counts the number of trailing zeros in the bit representation of the value
    *
    * @param Value the value to determine the number of trailing zeros for
    *
    * @return the number of zeros after the last "on" bit
    */
   static uint64 CountTrailingZeros64(uint64 Value);

   /**
    * Returns smallest N such that (1<<N)>=Arg.
    * Note: CeilLogTwo(0)=0 
    */
   static uint32 CeilLogTwo( uint32 Arg );

   static uint64 CeilLogTwo64( uint64 Arg );

   /**
    * Returns the smallest N such that (1<<N)>=Arg. This is a less efficient version of CeilLogTwo, but written in a
    * way that can be evaluated at compile-time.
    */
   static uint8 ConstExprCeilLogTwo(size_t Arg);

   /** @return Rounds the given number up to the next highest power of two. */
   static uint32 RoundUpToPowerOfTwo(uint32 Arg);

   static uint64 RoundUpToPowerOfTwo64(uint64 V);

   /**
    * Returns value based on comparand. The main purpose of this function is to avoid
    * branching based on floating point comparison which can be avoided via compiler
    * intrinsics.
    *
    * Please note that we don't define what happens in the case of NaNs as there might
    * be platform specific differences.
    *
    * @param   Comparand      Comparand the results are based on
    * @param   ValueGEZero    Return value if Comparand >= 0
    * @param   ValueLTZero    Return value if Comparand < 0
    *
    * @return  ValueGEZero if Comparand >= 0, ValueLTZero otherwise
    */
   static float FloatSelect( float Comparand, float ValueGEZero, float ValueLTZero );

   /**
    * Returns value based on comparand. The main purpose of this function is to avoid
    * branching based on floating point comparison which can be avoided via compiler
    * intrinsics.
    *
    * Please note that we don't define what happens in the case of NaNs as there might
    * be platform specific differences.
    *
    * @param   Comparand      Comparand the results are based on
    * @param   ValueGEZero    Return value if Comparand >= 0
    * @param   ValueLTZero    Return value if Comparand < 0
    *
    * @return  ValueGEZero if Comparand >= 0, ValueLTZero otherwise
    */
   static double FloatSelect( double Comparand, double ValueGEZero, double ValueLTZero );

   /** Computes absolute value in a generic way */
   template< class T > 
   static T Abs( const T A );

   /** Returns 1, 0, or -1 depending on relation of T to 0 */
   template< class T > 
   static T Sign( const T A );

   /** Returns higher value in a generic way */
   template< class T > 
   static T Max( const T A, const T B );

   /** Returns lower value in a generic way */
   template< class T > 
   static T Min( const T A, const T B );

   /**
   * Min of Array
   * @param Array of templated type
   * @param Optional pointer for returning the index of the minimum element, if multiple minimum elements the first index is returned
   * @return   The min value found in the array or default value if the array was empty
   */
   template< class T >
   static T Min(const TArray<T>& Values, int32* MinIndex = nullptr);

   /**
   * Max of Array
   * @param Array of templated type
   * @param Optional pointer for returning the index of the maximum element, if multiple maximum elements the first index is returned
   * @return   The max value found in the array or default value if the array was empty
   */
   template< class T >
   static T Max(const TArray<T>& Values, int32* MaxIndex = nullptr);

   static int32 CountBits(uint64 Bits);
};

/**
 * Structure for all math helper functions, inherits from platform math to pick up platform-specific implementations
 * Check GenericPlatformMath.h for additional math functions
 */
struct FMath : public FGenericPlatformMath
{
   // Random Number Functions

   /** Helper function for rand implementations. Returns a random number in [0..A) */
   static int32 RandHelper(int32 A);
   static int64 RandHelper64(int64 A);

   /** Helper function for rand implementations. Returns a random number >= Min and <= Max */
   static int32 RandRange(int32 Min, int32 Max);
   static int64 RandRange(int64 Min, int64 Max);

   /** Util to generate a random number in a range. Overloaded to distinguish from int32 version, where passing a float is typically a mistake. */
   static float RandRange(float InMin, float InMax);
   static double RandRange(double InMin, double InMax);

   /** Util to generate a random number in a range. */
   static float FRandRange(float InMin, float InMax);

   /** Util to generate a random number in a range. */
   static double FRandRange(double InMin, double InMax);

   /** Util to generate a random boolean. */
   static bool RandBool();

   /** Return a uniformly distributed random unit length vector = point on the unit sphere surface. */
   static FVector VRand();
   
   /**
    * Returns a random unit vector, uniformly distributed, within the specified cone
    * ConeHalfAngleRad is the half-angle of cone, in radians.  Returns a normalized vector. 
    */
   static FVector VRandCone(FVector const& Dir, float ConeHalfAngleRad);

   /** 
    * This is a version of VRandCone that handles "squished" cones, i.e. with different angle limits in the Y and Z axes.
    * Assumes world Y and Z, although this could be extended to handle arbitrary rotations.
    */
   static FVector VRandCone(FVector const& Dir, float HorizontalConeHalfAngleRad, float VerticalConeHalfAngleRad);

   /** Returns a random point, uniformly distributed, within the specified radius */
   static FVector2D RandPointInCircle(float CircleRadius);

   /** 
    * Given a direction vector and a surface normal, returns the vector reflected across the surface normal.
    * Produces a result like shining a laser at a mirror!
    *
    * @param Direction Direction vector the ray is coming from.
    * @param SurfaceNormal A normal of the surface the ray should be reflected on.
    *
    * @returns Reflected vector.
    */
   static FVector GetReflectionVector(const FVector& Direction, const FVector& SurfaceNormal);
   
   // Predicates

   /** Checks if value is within a range, exclusive on MaxValue) */
   template< class T, class U> 
   static bool IsWithin(const T& TestValue, const U& MinValue, const U& MaxValue);

   /** Checks if value is within a range, inclusive on MaxValue) */
   template< class T, class U> 
   static bool IsWithinInclusive(const T& TestValue, const U& MinValue, const U& MaxValue);
   
   /**
    * Checks if two floating point numbers are nearly equal.
    * @param A          First number to compare
    * @param B          Second number to compare
    * @param ErrorTolerance   Maximum allowed difference for considering them as 'nearly equal'
    * @return              true if A and B are nearly equal
    */
   static bool IsNearlyEqual(float A, float B, float ErrorTolerance = UE_SMALL_NUMBER);
   static bool IsNearlyEqual(double A, double B, double ErrorTolerance = UE_DOUBLE_SMALL_NUMBER);

   /**
    * Checks if a floating point number is nearly zero.
    * @param Value         Number to compare
    * @param ErrorTolerance   Maximum allowed difference for considering Value as 'nearly zero'
    * @return              true if Value is nearly zero
    */
   static bool IsNearlyZero(float Value, float ErrorTolerance = UE_SMALL_NUMBER);

   /**
    * Checks if a floating point number is nearly zero.
    * @param Value         Number to compare
    * @param ErrorTolerance   Maximum allowed difference for considering Value as 'nearly zero'
    * @return              true if Value is nearly zero
    */
   static bool IsNearlyZero(double Value, double ErrorTolerance = UE_DOUBLE_SMALL_NUMBER);

   /**
   *  Checks whether a number is a power of two.
   *  @param Value   Number to check
   *  @return        true if Value is a power of two
   */
   template <typename T>
   static bool IsPowerOfTwo( T Value );

   /** Converts a float to a nearest less or equal integer. */
   static float Floor(float F);

   /** Converts a double to a nearest less or equal integer. */
   static double Floor(double F);


   // Math Operations

   /** Returns highest of 3 values */
   template< class T > 
   static T Max3( const T A, const T B, const T C );

   /** Returns lowest of 3 values */
   template< class T > 
   static T Min3( const T A, const T B, const T C );

   template< class T > 
   static int32 Max3Index( const T A, const T B, const T C );

   /** Returns index of the lowest value */
   template< class T > 
   static int32 Min3Index( const T A, const T B, const T C );

   /** Multiples value by itself */
   template< class T > 
   static T Square( const T A );

   /** Cubes the value */
   template< class T > 
   static T Cube( const T A );

   /** Clamps X to be between Min and Max, inclusive */
   template< class T >
   static T Clamp(const T X, const T MinValue, const T MaxValue);

   /** Wraps X to be between Min and Max, inclusive. */
   /** When X can wrap to both Min and Max, it will wrap to Min if it lies below the range and wrap to Max if it is above the range. */
   template< class T >
   static T Wrap(const T X, const T Min, const T Max);

   /** Snaps a value to the nearest grid multiple */
   template< class T >
   static T GridSnap(T Location, T Grid);

   /** Divides two integers and rounds up */
   template <class T>
   static T DivideAndRoundUp(T Dividend, T Divisor);

   /** Divides two integers and rounds down */
   template <class T>
   static T DivideAndRoundDown(T Dividend, T Divisor);

   /** Divides two integers and rounds to nearest */
   template <class T>
   static T DivideAndRoundNearest(T Dividend, T Divisor);

   /**
    * Computes the base 2 logarithm of the specified value
    *
    * @param Value the value to perform the log on
    *
    * @return the base 2 log of the value
    */
   static float Log2(float Value);

   /**
    * Computes the base 2 logarithm of the specified value
    *
    * @param Value the value to perform the log on
    *
    * @return the base 2 log of the value
    */
   static double Log2(double Value);

   static void SinCos(double* ScalarSin, double* ScalarCos, double Value);

   // Note:  We use FASTASIN_HALF_PI instead of HALF_PI inside of FastASin(), since it was the value that accompanied the minimax coefficients below.
   // It is important to use exactly the same value in all places inside this function to ensure that FastASin(0.0f) == 0.0f.
   // For comparison:
   //    HALF_PI           == 1.57079632679f == 0x3fC90FDB
   //    FASTASIN_HALF_PI  == 1.5707963050f  == 0x3fC90FDA
   /**
   * Computes the ASin of a scalar value.
   *
   * @param Value  input angle
   * @return ASin of Value
   */
   static float FastAsin(float Value);
   static double FastAsin(double Value);

   // Conversion Functions

   /** 
    * Converts radians to degrees.
    * @param   RadVal         Value in radians.
    * @return              Value in degrees.
    */
   template<class T>
   static auto RadiansToDegrees(T const& RadVal);

   /** 
    * Converts degrees to radians.
    * @param   DegVal         Value in degrees.
    * @return              Value in radians.
    */
   template<class T>
   static auto DegreesToRadians(T const& DegVal);

   /** 
    * Clamps an arbitrary angle to be between the given angles.  Will clamp to nearest boundary.
    * 
    * @param MinAngleDegrees  "from" angle that defines the beginning of the range of valid angles (sweeping clockwise)
    * @param MaxAngleDegrees  "to" angle that defines the end of the range of valid angles
    * @return Returns clamped angle in the range -180..180.
    */
   template<typename T>
   static T ClampAngle(T AngleDegrees, T MinAngleDegrees, T MaxAngleDegrees);

   /** Find the smallest angle between two headings (in degrees) */
   template<typename T, typename T2>
   static auto FindDeltaAngleDegrees(T A1, T2 A2);

   /** Find the smallest angle between two headings (in radians) */
   template<typename T, typename T2>
   static auto FindDeltaAngleRadians(T A1, T2 A2);

   /** Given a heading which may be outside the +/- PI range, 'unwind' it back into that range. */
   template<typename T>
   static T UnwindRadians(T A);

   /** Utility to ensure angle is between +/- 180 degrees by unwinding. */
   template<typename T>
   static T UnwindDegrees(T A);

   /** 
    * Given two angles in degrees, 'wind' the rotation in Angle1 so that it avoids >180 degree flips.
    * Good for winding rotations previously expressed as quaternions into a euler-angle representation.
    * @param   InAngle0 The first angle that we wind relative to.
    * @param   InOutAngle1 The second angle that we may wind relative to the first.
    */
   static void WindRelativeAnglesDegrees(float InAngle0, float& InOutAngle1);
   static void WindRelativeAnglesDegrees(double InAngle0, double& InOutAngle1);

   /** Returns a new rotation component value
    *
    * @param InCurrent is the current rotation value
    * @param InDesired is the desired rotation value
    * @param InDeltaRate is the rotation amount to apply
    *
    * @return a new rotation component value
    */
   static float FixedTurn(float InCurrent, float InDesired, float InDeltaRate);

   /** Converts given Cartesian coordinate pair to Polar coordinate system. */
   template<typename T>
   static void CartesianToPolar(const T X, const T Y, T& OutRad, T& OutAng);
   /** Converts given Cartesian coordinate pair to Polar coordinate system. */
   static void CartesianToPolar(const FVector2D InCart, FVector2D& OutPolar);

   /** Converts given Polar coordinate pair to Cartesian coordinate system. */
   template<typename T>
   static void PolarToCartesian(const T Rad, const T Ang, T& OutX, T& OutY);
   /** Converts given Polar coordinate pair to Cartesian coordinate system. */
   static void PolarToCartesian(const FVector2D InPolar, FVector2D& OutCart);

   /**
    * Calculates the dotted distance of vector 'Direction' to coordinate system O(AxisX,AxisY,AxisZ).
    *
    * Orientation: (consider 'O' the first person view of the player, and 'Direction' a vector pointing to an enemy)
    * - positive azimuth means enemy is on the right of crosshair. (negative means left).
    * - positive elevation means enemy is on top of crosshair, negative means below.
    *
    * @Note: 'Azimuth' (.X) sign is changed to represent left/right and not front/behind. front/behind is the funtion's return value.
    *
    * @param   OutDotDist  .X = 'Direction' dot AxisX relative to plane (AxisX,AxisZ). (== Cos(Azimuth))
    *                .Y = 'Direction' dot AxisX relative to plane (AxisX,AxisY). (== Sin(Elevation))
    * @param   Direction   direction of target.
    * @param   AxisX    X component of reference system.
    * @param   AxisY    Y component of reference system.
    * @param   AxisZ    Z component of reference system.
    *
    * @return  true if 'Direction' is facing AxisX (Direction dot AxisX >= 0.f)
    */
   static bool GetDotDistance(FVector2D &OutDotDist, const FVector &Direction, const FVector &AxisX, const FVector &AxisY, const FVector &AxisZ);

   /**
    * Returns Azimuth and Elevation of vector 'Direction' in coordinate system O(AxisX,AxisY,AxisZ).
    *
    * Orientation: (consider 'O' the first person view of the player, and 'Direction' a vector pointing to an enemy)
    * - positive azimuth means enemy is on the right of crosshair. (negative means left).
    * - positive elevation means enemy is on top of crosshair, negative means below.
    *
    * @param   Direction      Direction of target.
    * @param   AxisX       X component of reference system.
    * @param   AxisY       Y component of reference system.
    * @param   AxisZ       Z component of reference system.
    *
    * @return  FVector2D   X = Azimuth angle (in radians) (-PI, +PI)
    *                Y = Elevation angle (in radians) (-PI/2, +PI/2)
    */
   static FVector2D GetAzimuthAndElevation(const FVector &Direction, const FVector &AxisX, const FVector &AxisY, const FVector &AxisZ);

   // Interpolation Functions

   /** Calculates the percentage along a line from MinValue to MaxValue that Value is. */
   template<typename T, typename T2>
   static auto GetRangePct(T MinValue, T MaxValue, T2 Value);

   /** Same as above, but taking a 2d vector as the range. */
   template<typename T, typename T2>
   static auto GetRangePct(FVector2D const& Range, T2 Value);
   
   /** Basically a Vector2d version of Lerp. */
   template<typename T, typename T2>
   static auto GetRangeValue(FVector2D const& Range, T2 Pct);

   /** For the given Value clamped to the [Input:Range] inclusive, returns the corresponding percentage in [Output:Range] Inclusive. */
   template<typename T, typename T2>
   static auto GetMappedRangeValueClamped(const FVector2D& InputRange, const FVector2D& OutputRange, const T2 Value);

   /** Transform the given Value relative to the input range to the Output Range. */
   template<typename T, typename T2>
   static auto GetMappedRangeValueUnclamped(const FVector2D& InputRange, const FVector2D& OutputRange, const T2 Value);

   /** Performs a linear interpolation between two values, Alpha ranges from 0-1 */
   template<typename T, typename U>
   static T Lerp( const T& A, const T& B, const U& Alpha );

   /** Performs a linear interpolation between two values, Alpha ranges from 0-1. Handles full numeric range of T */
   template< class T >
   static T LerpStable(const T& A, const T& B, float Alpha);
   
   /** Performs a 2D linear interpolation between four values values, FracX, FracY ranges from 0-1 */
   template<typename T, typename U>
   static T BiLerp(const T& P00,const T& P10,const T& P01,const T& P11, const U& FracX, const U& FracY);

   /**
    * Performs a cubic interpolation
    *
    * @param  P - end points
    * @param  T - tangent directions at end points
    * @param  A - distance along spline
    *
    * @return  Interpolated value
    */
   template<typename T, typename U>
   static T CubicInterp( const T& P0, const T& T0, const T& P1, const T& T1, const U& A );

   /**
    * Performs a first derivative cubic interpolation
    *
    * @param  P - end points
    * @param  T - tangent directions at end points
    * @param  A - distance along spline
    *
    * @return  Interpolated value
    */
   template<typename T, typename U>
   static T CubicInterpDerivative( const T& P0, const T& T0, const T& P1, const T& T1, const U& A );

   /**
    * Performs a second derivative cubic interpolation
    *
    * @param  P - end points
    * @param  T - tangent directions at end points
    * @param  A - distance along spline
    *
    * @return  Interpolated value
    */
   template<typename T, typename U>
   static T CubicInterpSecondDerivative( const T& P0, const T& T0, const T& P1, const T& T1, const U& A );

   /** Interpolate between A and B, applying an ease in function.  Exp controls the degree of the curve. */
   template< class T >
   static T InterpEaseIn(const T& A, const T& B, float Alpha, float Exp);

   /** Interpolate between A and B, applying an ease out function.  Exp controls the degree of the curve. */
   template< class T >
   static T InterpEaseOut(const T& A, const T& B, float Alpha, float Exp);

   /** Interpolate between A and B, applying an ease in/out function.  Exp controls the degree of the curve. */
   template< class T > 
   static T InterpEaseInOut( const T& A, const T& B, float Alpha, float Exp );

   /** Interpolation between A and B, applying a step function. */
   template< class T >
   static T InterpStep(const T& A, const T& B, float Alpha, int32 Steps);

   /** Interpolation between A and B, applying a sinusoidal in function. */
   template< class T >
   static T InterpSinIn(const T& A, const T& B, float Alpha);
   
   /** Interpolation between A and B, applying a sinusoidal out function. */
   template< class T >
   static T InterpSinOut(const T& A, const T& B, float Alpha);

   /** Interpolation between A and B, applying a sinusoidal in/out function. */
   template< class T >
   static T InterpSinInOut(const T& A, const T& B, float Alpha);

   /** Interpolation between A and B, applying an exponential in function. */
   template< class T >
   static T InterpExpoIn(const T& A, const T& B, float Alpha);

   /** Interpolation between A and B, applying an exponential out function. */
   template< class T >
   static T InterpExpoOut(const T& A, const T& B, float Alpha);

   /** Interpolation between A and B, applying an exponential in/out function. */
   template< class T >
   static T InterpExpoInOut(const T& A, const T& B, float Alpha);

   /** Interpolation between A and B, applying a circular in function. */
   template< class T >
   static T InterpCircularIn(const T& A, const T& B, float Alpha);

   /** Interpolation between A and B, applying a circular out function. */
   template< class T >
   static T InterpCircularOut(const T& A, const T& B, float Alpha);

   /** Interpolation between A and B, applying a circular in/out function. */
   template< class T >
   static T InterpCircularInOut(const T& A, const T& B, float Alpha);

   // Rotator specific interpolation
   // Similar to Lerp, but does not take the shortest path. Allows interpolation over more than 180 degrees.
   template< typename T, typename U >
   static FRotator LerpRange(const FRotator& A, const FRotator& B, U Alpha);


   // Special-case interpolation

   /** Interpolate a normal vector Current to Target, by interpolating the angle between those vectors with constant step. */
   static FVector VInterpNormalRotationTo(const FVector& Current, const FVector& Target, float DeltaTime, float RotationSpeedDegrees);

   /** Interpolate vector from Current to Target with constant step */
   static FVector VInterpConstantTo(const FVector& Current, const FVector& Target, float DeltaTime, float InterpSpeed);

   /** Interpolate vector from Current to Target. Scaled by distance to Target, so it has a strong start speed and ease out. */
   static FVector VInterpTo( const FVector& Current, const FVector& Target, float DeltaTime, float InterpSpeed );
   
   /** Interpolate vector2D from Current to Target with constant step */
   static FVector2D Vector2DInterpConstantTo( const FVector2D& Current, const FVector2D& Target, float DeltaTime, float InterpSpeed );

   /** Interpolate vector2D from Current to Target. Scaled by distance to Target, so it has a strong start speed and ease out. */
   static FVector2D Vector2DInterpTo( const FVector2D& Current, const FVector2D& Target, float DeltaTime, float InterpSpeed );

   /** Interpolate rotator from Current to Target with constant step */
   static FRotator RInterpConstantTo( const FRotator& Current, const FRotator& Target, float DeltaTime, float InterpSpeed);

   /** Interpolate rotator from Current to Target. Scaled by distance to Target, so it has a strong start speed and ease out. */
   static FRotator RInterpTo( const FRotator& Current, const FRotator& Target, float DeltaTime, float InterpSpeed);

   /** Interpolate float from Current to Target with constant step */
   template<typename T1, typename T2 = T1, typename T3 = T2, typename T4 = T3>
   static auto FInterpConstantTo( T1 Current, T2 Target, T3 DeltaTime, T4 InterpSpeed );

   /** Interpolate float from Current to Target. Scaled by distance to Target, so it has a strong start speed and ease out. */
   template<typename T1, typename T2 = T1, typename T3 = T2, typename T4 = T3>
   static auto FInterpTo( T1  Current, T2 Target, T3 DeltaTime, T4 InterpSpeed );

   /** Interpolate Linear Color from Current to Target. Scaled by distance to Target, so it has a strong start speed and ease out. */
   static FLinearColor CInterpTo(const FLinearColor& Current, const FLinearColor& Target, float DeltaTime, float InterpSpeed);

   /** Interpolate quaternion from Current to Target with constant step (in radians) */
   template< class T >
   static FQuat QInterpConstantTo(const FQuat& Current, const FQuat& Target, float DeltaTime, float InterpSpeed);

   /** Interpolate quaternion from Current to Target. Scaled by angle to Target, so it has a strong start speed and ease out. */
   template< class T >
   static FQuat QInterpTo(const FQuat& Current, const FQuat& Target, float DeltaTime, float InterpSpeed);

   /**
   * Returns an approximation of Exp(-X) based on a Taylor expansion that has had the coefficients adjusted (using
   * optimisation) to minimise the error in the range 0 < X < 1, which is below 0.1%. Note that it returns exactly 1
   * when X is 0, and the return value is greater than the real value for values of X > 1 (but it still tends
   * to zero for large X). 
   */
   template<class T>
   static T InvExpApprox(T X);
   
   /**
   * Smooths a value using exponential damping towards a target. Works for any type that supports basic arithmetic operations.
   * 
   * An approximation is used that is accurate so long as InDeltaTime < 0.5 * InSmoothingTime
   * 
   * @param  InOutValue      The value to be smoothed
   * @param  InTargetValue   The target to smooth towards
   * @param  InDeltaTime     Time interval
   * @param  InSmoothingTime Timescale over which to smooth. Larger values result in more smoothed behaviour. Can be zero.
   */
   template< class T >
    static void ExponentialSmoothingApprox(
        T&          InOutValue,
        const T&    InTargetValue,
        const float InDeltaTime,
        const float InSmoothingTime);

   /**
    * Smooths a value using a critically damped spring. Works for any type that supports basic arithmetic operations.
    * 
    * Note that InSmoothingTime is the time lag when tracking constant motion (so if you tracked a predicted position 
    * TargetVel * InSmoothingTime ahead, you'd match the target position)
    *
    * An approximation is used that is accurate so long as InDeltaTime < 0.5 * InSmoothingTime
    * 
    * When starting from zero velocity, the maximum velocity is reached after time InSmoothingTime * 0.5
    *
    * @param  InOutValue        The value to be smoothed
    * @param  InOutValueRate    The rate of change of the value
    * @param  InTargetValue     The target to smooth towards
    * @param  InTargetValueRate The target rate of change smooth towards. Note that if this is discontinuous, then the output will have discontinuous velocity too.
    * @param  InDeltaTime       Time interval
    * @param  InSmoothingTime   Timescale over which to smooth. Larger values result in more smoothed behaviour. Can be zero.
    */
   template< class T >
   static void CriticallyDampedSmoothing(
      T&          InOutValue,
      T&          InOutValueRate,
      const T&    InTargetValue,
       const T&    InTargetValueRate,
      const float InDeltaTime,
      const float InSmoothingTime);

   /**
   * Smooths a value using a spring damper towards a target.
   * 
   * The implementation uses approximations for Exp/Sin/Cos. These are accurate for all sensible values of 
   * InUndampedFrequency and DampingRatio so long as InDeltaTime < 1 / InUndampedFrequency (approximately), but
   * are generally well behaved even for larger timesteps etc. 
   * 
   * @param  InOutValue          The value to be smoothed
   * @param  InOutValueRate      The rate of change of the value
   * @param  InTargetValue       The target to smooth towards
   * @param  InTargetValueRate   The target rate of change smooth towards. Note that if this is discontinuous, then the output will have discontinuous velocity too.
   * @param  InDeltaTime         Time interval
   * @param  InUndampedFrequency Oscillation frequency when there is no damping. Proportional to the square root of the spring stiffness.
   * @param  InDampingRatio      1 is critical damping. <1 results in under-damped motion (i.e. with overshoot), and >1 results in over-damped motion. 
   */
   template< class T >
   static void SpringDamper(
       T&          InOutValue,
       T&          InOutValueRate,
       const T&    InTargetValue,
       const T&    InTargetValueRate,
       const float InDeltaTime,
       const float InUndampedFrequency,
       const float InDampingRatio);

   /**
   * Smooths a value using a spring damper towards a target.
   * 
   * The implementation uses approximations for Exp/Sin/Cos. These are accurate for all sensible values of 
   * DampingRatio and InSmoothingTime so long as InDeltaTime < 0.5 * InSmoothingTime, but are generally well behaved 
   * even for larger timesteps etc. 
   * 
   * @param  InOutValue        The value to be smoothed
   * @param  InOutValueRate    The rate of change of the value
   * @param  InTargetValue     The target to smooth towards
   * @param  InTargetValueRate The target rate of change smooth towards. Note that if this is discontinuous, then the output will have discontinuous velocity too.
   * @param  InDeltaTime       Time interval
   * @param  InSmoothingTime   Timescale over which to smooth. Larger values result in more smoothed behaviour. Can be zero.
   * @param  InDampingRatio    1 is critical damping. <1 results in under-damped motion (i.e. with overshoot), and >1 results in over-damped motion. 
   */
   template< class T >
   static void SpringDamperSmoothing(
      T&          InOutValue,
      T&          InOutValueRate,
      const T&    InTargetValue,
      const T&    InTargetValueRate,
      const float InDeltaTime,
      const float InSmoothingTime,
      const float InDampingRatio);

   /**
    * Simple function to create a pulsating scalar value
    *
    * @param  InCurrentTime  Current absolute time
    * @param  InPulsesPerSecond  How many full pulses per second?
    * @param  InPhase  Optional phase amount, between 0.0 and 1.0 (to synchronize pulses)
    *
    * @return  Pulsating value (0.0-1.0)
    */
   static float MakePulsatingValue( const double InCurrentTime, const float InPulsesPerSecond, const float InPhase = 0.0f );

   // Geometry intersection 

   /**
    * Find the intersection of a line and an offset plane. Assumes that the
    * line and plane do indeed intersect; you must make sure they're not
    * parallel before calling.
    *
    * @param Point1 the first point defining the line
    * @param Point2 the second point defining the line
    * @param PlaneOrigin the origin of the plane
    * @param PlaneNormal the normal of the plane
    *
    * @return The point of intersection between the line and the plane.
    */
   static FVector LinePlaneIntersection(const FVector& Point1, const FVector& Point2, const FVector& PlaneOrigin, const FVector& PlaneNormal);

   /** Determines whether a line intersects a sphere. */
   static bool LineSphereIntersection(const FVector& Start,const FVector& Dir, double Length,const FVector& Origin, double Radius);

   /**
    * Assumes the cone tip is at 0,0,0 (means the SphereCenter is relative to the cone tip)
    * @return true: cone and sphere do intersect, false otherwise
    */
   static bool SphereConeIntersection(const FVector& SphereCenter, float SphereRadius, const FVector& ConeAxis, float ConeAngleSin, float ConeAngleCos);

   /** Find the point on the line segment from LineStart to LineEnd which is closest to Point */
   static FVector ClosestPointOnLine(const FVector& LineStart, const FVector& LineEnd, const FVector& Point);

   /** Find the point on the infinite line between two points (LineStart, LineEnd) which is closest to Point */
   static FVector ClosestPointOnInfiniteLine(const FVector& LineStart, const FVector& LineEnd, const FVector& Point);

   /**
    * Calculates the distance of a given Point in world space to a given line,
    * defined by the vector couple (Origin, Direction).
    *
    * @param   Point          Point to check distance to line
    * @param   Direction         Vector indicating the direction of the line. Not required to be normalized.
    * @param   Origin            Point of reference used to calculate distance
    * @param   OutClosestPoint   optional point that represents the closest point projected onto Axis
    *
    * @return  distance of Point from line defined by (Origin, Direction)
    */
   static float PointDistToLine(const FVector &Point, const FVector &Direction, const FVector &Origin, FVector &OutClosestPoint);
   static float PointDistToLine(const FVector &Point, const FVector &Direction, const FVector &Origin);

   /**
    * Returns closest point on a segment to a given point.
    * The idea is to project point on line formed by segment.
    * Then we see if the closest point on the line is outside of segment or inside.
    *
    * @param   Point       point for which we find the closest point on the segment
    * @param   StartPoint     StartPoint of segment
    * @param   EndPoint    EndPoint of segment
    *
    * @return  point on the segment defined by (StartPoint, EndPoint) that is closest to Point.
    */
   static FVector ClosestPointOnSegment(const FVector &Point, const FVector &StartPoint, const FVector &EndPoint);

   /**
   * FVector2D version of ClosestPointOnSegment.
   * Returns closest point on a segment to a given 2D point.
   * The idea is to project point on line formed by segment.
   * Then we see if the closest point on the line is outside of segment or inside.
   *
   * @param Point       point for which we find the closest point on the segment
   * @param StartPoint     StartPoint of segment
   * @param EndPoint    EndPoint of segment
   *
   * @return   point on the segment defined by (StartPoint, EndPoint) that is closest to Point.
   */
   static FVector2D ClosestPointOnSegment2D(const FVector2D &Point, const FVector2D &StartPoint, const FVector2D &EndPoint);

   /**
    * Returns distance from a point to the closest point on a segment.
    *
    * @param   Point       point to check distance for
    * @param   StartPoint     StartPoint of segment
    * @param   EndPoint    EndPoint of segment
    *
    * @return  closest distance from Point to segment defined by (StartPoint, EndPoint).
    */
   static float PointDistToSegment(const FVector &Point, const FVector &StartPoint, const FVector &EndPoint);

   /**
    * Returns square of the distance from a point to the closest point on a segment.
    *
    * @param   Point       point to check distance for
    * @param   StartPoint     StartPoint of segment
    * @param   EndPoint    EndPoint of segment
    *
    * @return  square of the closest distance from Point to segment defined by (StartPoint, EndPoint).
    */
   static float PointDistToSegmentSquared(const FVector &Point, const FVector &StartPoint, const FVector &EndPoint);

   /** 
    * Find closest points between 2 segments.
    *
    * If either segment may have a length of 0, use SegmentDistToSegmentSafe instance.
    *
    * @param   (A1, B1) defines the first segment.
    * @param   (A2, B2) defines the second segment.
    * @param   OutP1    Closest point on segment 1 to segment 2.
    * @param   OutP2    Closest point on segment 2 to segment 1.
    */
   static void SegmentDistToSegment(FVector A1, FVector B1, FVector A2, FVector B2, FVector& OutP1, FVector& OutP2);

   /** 
    * Find closest points between 2 segments.
    *
    * This is the safe version, and will check both segments' lengths.
    * Use this if either (or both) of the segments lengths may be 0.
    *
    * @param   (A1, B1) defines the first segment.
    * @param   (A2, B2) defines the second segment.
    * @param   OutP1    Closest point on segment 1 to segment 2.
    * @param   OutP2    Closest point on segment 2 to segment 1.
    */
   static void SegmentDistToSegmentSafe(FVector A1, FVector B1, FVector A2, FVector B2, FVector& OutP1, FVector& OutP2);

   /**
   * Returns true if there is an intersection between the segment specified by StartPoint and Endpoint, and
   * the Triangle defined by A, B and C. If there is an intersection, the point is placed in out_IntersectionPoint
   * @param StartPoint - start point of segment
   * @param EndPoint   - end point of segment
   * @param A, B, C  - points defining the triangle 
   * @param OutIntersectPoint - out var for the point on the segment that intersects the triangle (if any)
   * @param OutTriangleNormal - out var for the triangle normal
   * @return true if intersection occurred
   */
   static bool SegmentTriangleIntersection(const FVector& StartPoint, const FVector& EndPoint, const FVector& A, const FVector& B, const FVector& C, FVector& OutIntersectPoint, FVector& OutTriangleNormal);

   /**
    * Returns true if there is an intersection between the segment specified by SegmentStartA and SegmentEndA, and
    * the segment specified by SegmentStartB and SegmentEndB, in 2D space. If there is an intersection, the point is placed in out_IntersectionPoint
    * @param SegmentStartA - start point of first segment
    * @param SegmentEndA   - end point of first segment
    * @param SegmentStartB - start point of second segment
    * @param SegmentEndB   - end point of second segment
    * @param out_IntersectionPoint - out var for the intersection point (if any)
    * @return true if intersection occurred
    */
   static bool SegmentIntersection2D(const FVector& SegmentStartA, const FVector& SegmentEndA, const FVector& SegmentStartB, const FVector& SegmentEndB, FVector& out_IntersectionPoint);


   /**
    * Returns closest point on a triangle to a point.
    * The idea is to identify the halfplanes that the point is
    * in relative to each triangle segment "plane"
    *
    * @param   Point       point to check distance for
    * @param   A,B,C       counter clockwise ordering of points defining a triangle
    *
    * @return  Point on triangle ABC closest to given point
    */
   static FVector ClosestPointOnTriangleToPoint(const FVector& Point, const FVector& A, const FVector& B, const FVector& C);

   /**
    * Returns closest point on a tetrahedron to a point.
    * The idea is to identify the halfplanes that the point is
    * in relative to each face of the tetrahedron
    *
    * @param   Point       point to check distance for
    * @param   A,B,C,D        four points defining a tetrahedron
    *
    * @return  Point on tetrahedron ABCD closest to given point
    */
   static FVector ClosestPointOnTetrahedronToPoint(const FVector& Point, const FVector& A, const FVector& B, const FVector& C, const FVector& D);

   /** 
    * Find closest point on a Sphere to a Line.
    * When line intersects    Sphere, then closest point to LineOrigin is returned.
    * @param SphereOrigin     Origin of Sphere
    * @param SphereRadius     Radius of Sphere
    * @param LineOrigin    Origin of line
    * @param LineDir       Direction of line. Needs to be normalized!!
    * @param OutClosestPoint  Closest point on sphere to given line.
    */
   static void SphereDistToLine(FVector SphereOrigin, float SphereRadius, FVector LineOrigin, FVector LineDir, FVector& OutClosestPoint);

   /**
    * Calculates whether a Point is within a cone segment, and also what percentage within the cone (100% is along the center line, whereas 0% is along the edge)
    *
    * @param Point - The Point in question
    * @param ConeStartPoint - the beginning of the cone (with the smallest radius)
    * @param ConeLine - the line out from the start point that ends at the largest radius point of the cone
    * @param RadiusAtStart - the radius at the ConeStartPoint (0 for a 'proper' cone)
    * @param RadiusAtEnd - the largest radius of the cone
    * @param PercentageOut - output variable the holds how much within the cone the point is (1 = on center line, 0 = on exact edge or outside cone).
    *
    * @return true if the point is within the cone, false otherwise.
    */
   static bool GetDistanceWithinConeSegment(FVector Point, FVector ConeStartPoint, FVector ConeLine, float RadiusAtStart, float RadiusAtEnd, float &PercentageOut);

   /**
    * Determines whether a given set of points are coplanar, with a tolerance. Any three points or less are always coplanar.
    *
    * @param Points - The set of points to determine coplanarity for.
    * @param Tolerance - Larger numbers means more variance is allowed.
    *
    * @return Whether the points are relatively coplanar, based on the tolerance
    */
   static bool PointsAreCoplanar(const TArray<FVector>& Points, const float Tolerance = 0.1f);

   /**
    * Truncates a floating point number to half if closer than the given tolerance.
    * @param F             Floating point number to truncate
    * @param Tolerance        Maximum allowed difference to 0.5 in order to truncate
    * @return              The truncated value
    */
   static float TruncateToHalfIfClose(float F, float Tolerance = UE_SMALL_NUMBER);
   static double TruncateToHalfIfClose(double F, double Tolerance = UE_SMALL_NUMBER);

   /**
   * Converts a floating point number to the nearest integer, equidistant ties go to the value which is closest to an even value: 1.5 becomes 2, 0.5 becomes 0
   * @param F     Floating point value to convert
   * @return      The rounded integer
   */
   static float RoundHalfToEven(float F);
   static double RoundHalfToEven(double F);

   /**
   * Converts a floating point number to the nearest integer, equidistant ties go to the value which is further from zero: -0.5 becomes -1.0, 0.5 becomes 1.0
   * @param F     Floating point value to convert
   * @return      The rounded integer
   */
   static float RoundHalfFromZero(float F);
   static double RoundHalfFromZero(double F);

   /**
   * Converts a floating point number to the nearest integer, equidistant ties go to the value which is closer to zero: -0.5 becomes 0, 0.5 becomes 0
   * @param F     Floating point value to convert
   * @return      The rounded integer
   */
   static float RoundHalfToZero(float F);
   static double RoundHalfToZero(double F);

   /**
   * Converts a floating point number to an integer which is further from zero, "larger" in absolute value: 0.1 becomes 1, -0.1 becomes -1
   * @param F     Floating point value to convert
   * @return      The rounded integer
   */
   static float RoundFromZero(float F);
   static double RoundFromZero(double F);

   /**
   * Converts a floating point number to an integer which is closer to zero, "smaller" in absolute value: 0.1 becomes 0, -0.1 becomes 0
   * @param F     Floating point value to convert
   * @return      The rounded integer
   */
   static float RoundToZero(float F);
   static double RoundToZero(double F);

   /**
   * Converts a floating point number to an integer which is more negative: 0.1 becomes 0, -0.1 becomes -1
   * @param F     Floating point value to convert
   * @return      The rounded integer
   */
   static float RoundToNegativeInfinity(float F);
   static double RoundToNegativeInfinity(double F);

   /**
   * Converts a floating point number to an integer which is more positive: 0.1 becomes 1, -0.1 becomes 0
   * @param F     Floating point value to convert
   * @return      The rounded integer
   */
   static float RoundToPositiveInfinity(float F);
   static double RoundToPositiveInfinity(double F);


   // Formatting functions

   /**
    * Formats an integer value into a human readable string (i.e. 12345 becomes "12,345")
    *
    * @param   Val      The value to use
    * @return  FString  The human readable string
    */
   static FString FormatIntToHumanReadable(int32 Val);


   // Utilities

   /**
    * Evaluates a numerical equation.
    *
    * Operators and precedence: 1:+- 2:/% 3:* 4:^ 5:&|
    * Unary: -
    * Types: Numbers (0-9.), Hex ($0-$f)
    * Grouping: ( )
    *
    * @param   Str         String containing the equation.
    * @param   OutValue    Pointer to storage for the result.
    * @return           1 if successful, 0 if equation fails.
    */
   static bool Eval( FString Str, float& OutValue );

   /**
    * Computes the barycentric coordinates for a given point in a triangle, only considering the XY coordinates - simpler than the Compute versions
    *
    * @param   Point       point to convert to barycentric coordinates (in plane of ABC)
    * @param   A,B,C       three non-collinear points defining a triangle in CCW
    * 
    * @return Vector containing the three weights a,b,c such that Point = a*A + b*B + c*C
    *                                                   or Point = A + b*(B-A) + c*(C-A) = (1-b-c)*A + b*B + c*C
    */
   static FVector GetBaryCentric2D(const FVector& Point, const FVector& A, const FVector& B, const FVector& C);

   /**
    * Computes the barycentric coordinates for a given point in a triangle - simpler than the Compute versions
    *
    * @param   Point       point to convert to barycentric coordinates (in plane of ABC)
    * @param   A,B,C       three non-collinear points defining a triangle in CCW
    *
    * @return Vector containing the three weights a,b,c such that Point = a*A + b*B + c*C
    *                                                   or Point = A + b*(B-A) + c*(C-A) = (1-b-c)*A + b*B + c*C
    */
   static FVector GetBaryCentric2D(const FVector2D& Point, const FVector2D& A, const FVector2D& B, const FVector2D& C);

   /**
    * Computes the barycentric coordinates for a given point in a triangle
    *
    * @param   Point       point to convert to barycentric coordinates (in plane of ABC)
    * @param   A,B,C       three non-collinear points defining a triangle in CCW
    * 
    * @return Vector containing the three weights a,b,c such that Point = a*A + b*B + c*C
    *                                                  or Point = A + b*(B-A) + c*(C-A) = (1-b-c)*A + b*B + c*C
    */
   static FVector ComputeBaryCentric2D(const FVector& Point, const FVector& A, const FVector& B, const FVector& C);

   /** 
    * Returns a smooth Hermite interpolation between 0 and 1 for the value X (where X ranges between A and B)
    * Clamped to 0 for X <= A and 1 for X >= B.
    *
    * @param A Minimum value of X
    * @param B Maximum value of X
    * @param X Parameter
    *
    * @return Smoothed value between 0 and 1
    */
   template<typename T>
   static T SmoothStep(T A, T B, T X);

   /**
    * Get a bit in memory created from bitflags (uint32 Value:1), used for EngineShowFlags,
    * TestBitFieldFunctions() tests the implementation
    */
   static bool ExtractBoolFromBitfield(const uint8* Ptr, uint32 Index);

   /**
    * Set a bit in memory created from bitflags (uint32 Value:1), used for EngineShowFlags,
    * TestBitFieldFunctions() tests the implementation
    */
   static void SetBoolInBitField(uint8* Ptr, uint32 Index, bool bSet);

   /**
    * Handy to apply scaling in the editor
    * @param Dst in and out
    */
   static void ApplyScaleToFloat(float& Dst, const FVector& DeltaScale, float Magnitude = 1.0f);

   // @param x assumed to be in this range: 0..1
   // @return 0..255
   static uint8 Quantize8UnsignedByte(float x);
   
   // @param x assumed to be in this range: -1..1
   // @return 0..255
   static uint8 Quantize8SignedByte(float x);

   // Use the Euclidean method to find the GCD
   static int32 GreatestCommonDivisor(int32 a, int32 b);

   // LCM = a/gcd * b
   // a and b are the number we want to find the lcm
   static int32 LeastCommonMultiplier(int32 a, int32 b);

   /**
    * Generates a 1D Perlin noise from the given value.  Returns a continuous random value between -1.0 and 1.0.
    *
    * @param   Value The input value that Perlin noise will be generated from.  This is usually a steadily incrementing time value.
    *
    * @return  Perlin noise in the range of -1.0 to 1.0
    */
   static float PerlinNoise1D(float Value);

   /**
   * Generates a 2D Perlin noise sample at the given location.  Returns a continuous random value between -1.0 and 1.0.
   *
   * @param Location Where to sample
   *
   * @return   Perlin noise in the range of -1.0 to 1.0
   */
   static float PerlinNoise2D(const FVector2D& Location);
    

   /**
   * Generates a 3D Perlin noise sample at the given location.  Returns a continuous random value between -1.0 and 1.0.
   *
   * @param Location Where to sample
   *
   * @return   Perlin noise in the range of -1.0 to 1.0
   */
   static float PerlinNoise3D(const FVector& Location);

   /**
    * Calculates the new value in a weighted moving average series using the previous value and the weight
    *
    * @param CurrentSample - The value to blend with the previous sample to get a new weighted value
    * @param PreviousSample - The last value from the series
    * @param Weight - The weight to blend with
    *
    * @return the next value in the series
    */
   template<typename T>
   static inline T WeightedMovingAverage(T CurrentSample, T PreviousSample, T Weight);

   /**
    * Calculates the new value in a weighted moving average series using the previous value and a weight range.
    * The weight range is used to dynamically adjust based upon distance between the samples
    * This allows you to smooth a value more aggressively for small noise and let large movements be smoothed less (or vice versa)
    *
    * @param CurrentSample - The value to blend with the previous sample to get a new weighted value
    * @param PreviousSample - The last value from the series
    * @param MaxDistance - Distance to use as the blend between min weight or max weight
    * @param MinWeight - The weight use when the distance is small
    * @param MaxWeight - The weight use when the distance is large
    *
    * @return the next value in the series
    */
   template<typename T>
   static inline T DynamicWeightedMovingAverage(T CurrentSample, T PreviousSample, T MaxDistance, T MinWeight, T MaxWeight);
};

enum LOG_CATEGORY {LogTemp, LogText};
enum LOG_VERBOSITY {NoLogging, Fatal, Error, Warning, Display, Log, Verbose, VeryVerbose, All, BreakOnLog};
void UE_LOG(LOG_CATEGORY CategoryName, LOG_VERBOSITY Verbosity, const TCHAR* Format, ...);

class FScriptDelegate 
{
};

class FTimerDynamicDelegate : FScriptDelegate
{
};

#endif
