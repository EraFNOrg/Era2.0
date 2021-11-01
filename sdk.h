#pragma once

#include <windows.h>
#include <iostream>
#include <tuple>
#include <vector>
#include <string>
#include "globals.h"
#include "memory.h"

using namespace std;

//Base classes

template<class T>
class TArray
{
	friend struct FString;

public:
	inline int Num() const
	{
		return count;
	};

	void Add(T InputData)
	{
		data = (T*)realloc(data, sizeof(T) * (count + 1));
		data[count++] = InputData;
		max = count;
	};

	inline T& operator[](int i)
	{
		return data[i];
	};

	inline const T& operator[](int i) const
	{
		return data[i];
	};

	inline int MaxIndex()
	{
		return count - 1;
	}

	T* data;
	int32_t count;
	int32_t max;
private:
};

struct FString : private TArray<wchar_t>
{
	inline FString()
	{
	};

	FString(const wchar_t* other)
	{
		max = count = *other ? std::wcslen(other) + 1 : 0;

		if (count)
		{
			data = const_cast<wchar_t*>(other);
		}
	};

	inline bool IsValid() const
	{
		return data != nullptr;
	}

	const wchar_t* ToWString() const
	{
		return data;
	}

	std::string ToString() const
	{
		auto length = std::wcslen(data);

		std::string str(length, '\0');

		std::use_facet<std::ctype<wchar_t>>(std::locale()).narrow(data, data + length, '?', &str[0]);

		return str;
	}
};

inline void (*FreeInternal)(void*);
inline void (*FNameToString)(struct FName* pThis, FString& out);

struct FName
{
	uint32_t ComparisonIndex;
	uint32_t DisplayIndex;

	string ToString()
	{
		FString temp;

		FNameToString(this, temp);

		string ret(temp.ToString());

		FreeInternal((void*)temp.ToWString());

		return ret;
	}

	inline bool operator==(FName other)
	{
		return ComparisonIndex == other.ComparisonIndex;
	}
	inline bool operator!=(FName other)
	{
		return ComparisonIndex != other.ComparisonIndex;
	}
};

class UObject;

class UObject
{
public:
	void** Vtable;
	int32_t ObjectFlags;
	int32_t InternalIndex;
	class UObject* Class;
	FName Name;
	UObject* Outer;

	bool IsA(UObject* _Class)
	{
		if (_Class == this->Class) return true;
		else return false;
	}

	string GetName()
	{
		string name = Name.ToString();
		auto pos = name.rfind('/');
		if (pos == std::string::npos)
		{
			return name;
		}

		return name.substr(pos + 1);
	}

	string GetFullName()
	{
		std::string temp;

		for (auto outer = this->Outer; outer; outer = outer->Outer)
		{
			temp = outer->GetName() + "." + temp;
		}

		temp = reinterpret_cast<UObject*>(Class)->GetName() + " " + temp + this->GetName();

		return temp;
	}

	template<typename T = UObject*>
	T& Child(string name)
	{
		auto Class = this->Class;
		
		while (true)
		{
			if (!Class) break;

			auto Children = *(UObject**)(int64(Class) + 0x38);

			if (!Children) {
				Class = *(UObject**)(int64(Class) + 0x30);
				continue;
			}

			while (true)
			{
				if (Children->Class->GetName() == _("StructProperty"))
				{
					auto StructChildren = *(UObject**)(int64(*(UObject**)(int64(Children) + 0x70)) + 0x38);

					while (true)
					{
						if (!StructChildren) break;

						if (StructChildren->GetName() == name) return  *(T*)(int64(this) + *(uint32*)(int64(Children) + 0x44) + *(uint32*)(int64(StructChildren) + 0x44));

						StructChildren = *(UObject**)(int64(StructChildren) + 0x28);
					}
				}

				if (Children->GetName() == name && Children->Class->GetName() == _("Function")) return *(T*)(&*Children);
			
				if (Children->GetName() == name) return *(T*)(int64(this) + *(uint32*)(int64(Children) + 0x44));

				Children = *(UObject**)(int64(Children) + 0x28);

				if (!Children) break;
			}

			Class = *(UObject**)(int64(Class) + 0x30);
		}

		return *(T*)(0);
	}

	vector<int> GetFunctionChildrenOffset()
	{
		auto ReturnVector = vector<int32>();

		auto Children = *(UObject**)(int64(this) + 0x38);

		while (true)
		{
			if (!Children) break;

			ReturnVector.push_back(*(int*)(int64(Children) + 0x44));

			Children = *(UObject**)(int64(Children) + 0x28);
		}

		return ReturnVector;
	}

	template< typename T = int, typename ...Params>
	T Call(string name, Params... args)
	{
		auto Function = &(this->Child<UObject>(name));

		if (!IsBadReadPtr(Function)) {
			auto ParamsSize = *(int16*)(int64(Function) + 0x8E);
			auto ReturnValueOffset = *(int16*)(int64(Function) + 0x90);

			unsigned char* params = (unsigned char*)malloc(ParamsSize);

			int i = 0;

			auto Offsets = Function->GetFunctionChildrenOffset();

			apply([&](auto... argument) {((
				memcpy(params + Offsets[i++], argument, sizeof(*argument))
				), ...); }, make_tuple(&args...));

			PE(this, Function, params);

			auto ret = *reinterpret_cast<T*>(int64(params) + ReturnValueOffset);
			free(params);
			return ret;
		}
	}
};

class FUObjectItem
{
public:
	UObject* Object;
	int32_t Flags;
	int32_t ClusterIndex;
	int32_t SerialNumber;

	enum class ObjectFlags : int32_t
	{
		None = 0,
		Native = 1 << 25,
		Async = 1 << 26,
		AsyncLoading = 1 << 27,
		Unreachable = 1 << 28,
		PendingKill = 1 << 29,
		RootSet = 1 << 30,
		NoStrongReference = 1 << 31
	};

	inline bool IsUnreachable() const
	{
		return !!(Flags & static_cast<std::underlying_type_t<ObjectFlags>>(ObjectFlags::Unreachable));
	}
	inline bool IsPendingKill() const
	{
		return !!(Flags & static_cast<std::underlying_type_t<ObjectFlags>>(ObjectFlags::PendingKill));
	}
};

class TUObjectArray
{
public:

	inline int32_t Num() const
	{
		return NumElements;
	}

	inline UObject* GetByIndex(int32_t index) const
	{
		return Objects[index].Object;
	}

	inline FUObjectItem* GetItemByIndex(int32_t index) const
	{
		if (index < NumElements)
		{
			return &Objects[index];
		}
		return nullptr;
	}

	FUObjectItem* Objects;
	int32_t MaxElements;
	int32_t NumElements;

};

// This struct is used on 4.21 and above.
struct TUObjectArrayNew
{
	FUObjectItem* Objects[9];
};

struct GObjects
{
	TUObjectArrayNew* ObjectArray;
	BYTE _padding_0[0xC];
	DWORD ObjectCount;
};

class FUObjectArray
{
public:
	TUObjectArray ObjObjects;
};

inline FUObjectArray* OldGobjects;
inline GObjects* NewGobjects;

inline void NumChunks(int* start, int* end)
{
	int cStart = 0, cEnd = 0;

	if (!cEnd)
	{
		while (1)
		{
			if (NewGobjects->ObjectArray->Objects[cStart] == 0)
			{
				cStart++;
			}
			else
			{
				break;
			}
		}

		cEnd = cStart;
		while (1)
		{
			if (NewGobjects->ObjectArray->Objects[cEnd] == 0)
			{
				break;
			}
			else
			{
				cEnd++;
			}
		}
	}

	*start = cStart;
	*end = cEnd;
}

static UObject* FindObjectById(uint32_t Id)
{
	if (NewGobjects)
	{
		int cStart = 0, cEnd = 0;
		int chunkIndex = 0, chunkSize = 0xFFFF, chunkPos;
		FUObjectItem* Object;

		NumChunks(&cStart, &cEnd);

		chunkIndex = Id / chunkSize;
		if (chunkSize * chunkIndex != 0 &&
			chunkSize * chunkIndex == Id)
		{
			chunkIndex--;
		}

		chunkPos = cStart + chunkIndex;
		if (chunkPos < cEnd)
		{
			Object = NewGobjects->ObjectArray->Objects[chunkPos] + (Id - chunkSize * chunkIndex);

			if (!Object) { return NULL; }

			return Object->Object;
		}
	}
	else
	{
		return OldGobjects->ObjObjects.GetByIndex(Id);
	}

	return nullptr;
}

struct FVector
{
	float X;
	float Y;
	float Z;
public:
	inline FVector() {}

	FVector(float _X, float _Y, float _Z)
	{
		X = _X;
		Y = _Y;
		Z = _Z;
	}
};

struct FRotator
{
	float Pitch;
	float Yaw;
	float Roll;

public:
	inline FRotator() {}

	FRotator(float _Pitch, float _Yaw, float _Roll)
	{
		Pitch = _Pitch;
		Yaw = _Yaw;
		Roll = _Roll;
	}
};

struct FItemAndCount {
	int Count;
	char pad[0x4];
	class UObject* Item; 
};

struct FGuid
{
	int A;
	int B;
	int C;
	int D;
};

struct FVector2D
{
	float X;
	float Y;
};

struct FMargin
{
	float Left;
	float Top;
	float Right;
	float Bottom;
};

enum class ESlateColorStylingMode : uint8_t
{
	UseColor_Specified = 0,
	UseColor_Specified_Link = 1,
	UseColor_Foreground = 2,
	UseColor_Foreground_Subdued = 3,
	UseColor_MAX = 4
};

struct FLinearColor
{
	float R;
	float G;
	float B;
	float A;
};

struct FSlateColor
{
	FLinearColor SpecifiedColor;
	ESlateColorStylingMode ColorUseRule;
	unsigned char UnknownData00[0x17];
};

struct FSlateBrush
{
	unsigned char UnknownData00[0x8];
	FVector2D ImageSize;
	FMargin Margin;
	FSlateColor TintColor;
	UObject* ObjectResource;
};

struct FGameplayAbilitySpecDef
{
	UObject* Ability;
	unsigned char Unk00[0x90];
};

struct FGameplayEffectContextHandle
{
	char UnknownData_0[0x18]; 
};

struct FActiveGameplayEffectHandle
{
	int Handle; 
	bool bPassedFiltersAndWasExecuted; 
	char UnknownData_5[0x3]; 
};

inline UObject* SpawnActorEasy(UObject* Class, FVector loc, FRotator rot)
{
	return SpawnActor(World, Class, &loc, &rot, SpawnActorParams{});
}

//Functions
inline UObject* FindObject(const wchar_t* Name)
{
	auto ReturnValue = StaticFindObject(nullptr, nullptr, Name, false);
	
	if (ReturnValue) return ReturnValue;

	for (int i = 0; i < 10; i++) {
		ReturnValue = StaticFindObject(nullptr, nullptr, wstring(Name).substr(0, wcslen(Name) - 1).append(L"214748259").append(to_wstring(i)).c_str(), false);
		if (ReturnValue) return ReturnValue;
	}

	return nullptr;

}

