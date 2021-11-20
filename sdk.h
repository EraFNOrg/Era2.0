#pragma once

#include <windows.h>
#include <iostream>
#include <tuple>
#include <vector>
#include <string>
#include "globals.h"
#include "memory.h"

using namespace std;

//COREUOBJECT classes

template<class T>
class TArray
{
	friend struct FString;

public:
	void Add(T InputData)
	{
		Data = (T*)Realloc(Data, sizeof(T) * (count + 1), 0);
		Data[count++] = InputData;
		max = count;
	};

	inline T& operator[](int i)
	{
		return Data[i];
	};

	inline const T& operator[](int i) const
	{
		return Data[i];
	};

	inline int MaxIndex()
	{
		return count - 1;
	}

	inline T* begin()
	{
		return(T*)(count, Data);
	}

	inline T* end()
	{
		return(T*)(count, Data + count);
	}

	T* Data;
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
			Data = const_cast<wchar_t*>(other);
		}
	};

	inline bool IsValid() const
	{
		return Data != nullptr;
	}

	const wchar_t* ToWString() const
	{
		return Data;
	}

	std::string ToString() const
	{
		auto length = std::wcslen(Data);

		std::string str(length, '\0');

		std::use_facet<std::ctype<wchar_t>>(std::locale()).narrow(Data, Data + length, '?', &str[0]);

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
};

class FFieldClass
{
public:
	FName NamePrivate;
	char Id;
	void* CastFlags;
	char ClassFlags;
	FFieldClass* Super;
	void* DefaultObj;

	string GetName()
	{
		string name = NamePrivate.ToString();
		auto pos = name.rfind('/');
		if (pos == std::string::npos)
		{
			return name;
		}

		return name.substr(pos + 1);
	}
};

class FField
{
public:
	void* VTablePointer;
	FFieldClass* ClassPointer;
	char pad[0x10];
	FField* Next;
	FName Name;
	char Flags;

	bool IsValid()
	{
		return !IsBadReadPtr(&Name);
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
};

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
		return Name.ToString();
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
	T& Child(string name, bool bIsFunction = false)
	{
		UObject* Class = this->Class;

		//Scan UProperties
		while (Class)
		{
			UObject* Children = *(UObject**)(int64(Class) + offsets::Children);

			if (!Children) {
				Class = *(UObject**)(int64(Class) + offsets::SuperClass);
				continue;
			}

			while (Children)
			{
				if (Children->Class->GetName() == _("StructProperty"))
				{
					auto StructChildren = *(UObject**)(int64(*(UObject**)(int64(Children) + offsets::Class)) + offsets::Children);

					while (StructChildren)
					{
						if (StructChildren->GetName() == name) return *(T*)(int64(this) + *(uint32*)(int64(Children) + offsets::Offset) + *(uint32*)(int64(StructChildren) + offsets::Offset));

						StructChildren = *(UObject**)(int64(StructChildren) + offsets::Next);
					}
				}

				if (Children->GetName() == name)
				{
					if (bIsFunction) return *(T*)(&*Children);

					return *(T*)(int64(this) + *(int32*)(int64(Children) + offsets::Offset));
				}

				Children = *(UObject**)(int64(Children) + offsets::Next);
			}

			Class = *(UObject**)(int64(Class) + offsets::SuperClass);
		}

		//Scan FFields
		Class = this->Class;

		while (Class)
		{	
			FField* ChildrenProperties = *(FField**)(int64(Class) + 0x50);
			
			if (!ChildrenProperties->IsValid()) {
				Class = *(UObject**)(int64(Class) + offsets::SuperClass);
				continue;
			}

			while (ChildrenProperties->IsValid())
			{
				if (ChildrenProperties->ClassPointer->GetName() == _("StructProperty"))
				{
					auto StructChildren = *(FField**)(int64(*(UObject**)(int64(ChildrenProperties) + offsets::Class)) + 0x50);

					while (StructChildren)
					{
						if (StructChildren->GetName() == name) return *(T*)(int64(this) + *(uint32*)(int64(ChildrenProperties) + 0x4C) + *(uint32*)(int64(StructChildren) + 0x4C));

						StructChildren = StructChildren->Next;
					}
				}

				if (ChildrenProperties->GetName() == name) return *(T*)(int64(this) + *(int32*)(int64(ChildrenProperties) + 0x4C));

				ChildrenProperties = ChildrenProperties->Next;
			}

			Class = *(UObject**)(int64(Class) + offsets::SuperClass);
		}

		return *(T*)(0);
	}

	vector<int> GetFunctionChildrenOffset()
	{
		auto ReturnVector = vector<int32>();

		auto Children = *(UObject**)(int64(this) + offsets::Children);
		auto ChildrenProperties = *(FField**)(int64(this) + 0x50);

		//Scan UProperties
		while (true)
		{
			if (!Children) break;

			ReturnVector.push_back(*(int*)(int64(Children) + offsets::Offset));

			Children = *(UObject**)(int64(Children) + offsets::Next);
		}

		//Scan FFields
		while (true)
		{
			if (!ChildrenProperties->IsValid()) break;

			ReturnVector.push_back(*(int*)(int64(ChildrenProperties) + 0x4C));

			ChildrenProperties = ChildrenProperties->Next;
		}

		return ReturnVector;
	}

	template< typename T = int, int16 ReturnOffset = -1, typename ...Params >
	T Call(string name, Params... args)
	{
		auto Function = &(this->Child<UObject>(name, true));

		if (!IsBadReadPtr(Function)) {
			auto ParamsSize = *(int16*)(int64(Function) + offsets::ParamsSize);
			auto ReturnValueOffset = *(int16*)(int64(Function) + offsets::ReturnValueOffset);

			PBYTE params = (PBYTE)malloc(ParamsSize);

			int i = 0;

			auto Offsets = Function->GetFunctionChildrenOffset();

			apply([&](auto... argument) {((
				memcpy(params + Offsets[i++], argument, sizeof(*argument))
				), ...); }, make_tuple(&args...));

			PE(this, Function, params);

			if (ReturnValueOffset == -1) ReturnValueOffset = ReturnOffset;

			auto ret = *reinterpret_cast<T*>(int64(params) + ReturnValueOffset);
			free(params);
			return ret;
		}
	}
};

//STRUCTS

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

struct FQuat
{
	float X;
	float Y;
	float Z;
	float W;
};

struct FTransform
{
	FQuat Rotation;
	FVector Translation;
	char pad1[0x4];
	FVector Scale3D;
	char pad2[0x4];
};

struct FKey
{
	FName KeyName;
	char pad[0x10];

	FKey() {}
	FKey(FName Name) {
		KeyName = Name;
	}
};

struct KeyMap
{
	FName Action;
	char pad[0x8];
	FKey Key;
};

struct FGameplayTagContainer
{
	TArray<FName> GameplayTags;
	TArray<FName> ParentTags;
};

class FUObjectItem
{
public:
	UObject* Object;
	int32_t Flags;
	int32_t ClusterIndex;
	int32_t SerialNumber;
};

struct TUObjectArrayNew
{
	FUObjectItem* Objects[9];
};

struct GObjects
{
	TUObjectArrayNew* ObjectArray;
	BYTE _padding_0[0xC];
	DWORD ObjectCount;

	inline void NumChunks(int* start, int* end)
	{
		int cStart = 0, cEnd = 0;

		if (!cEnd)
		{
			while (1)
			{
				if (this->ObjectArray->Objects[cStart] == 0)
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
				if (this->ObjectArray->Objects[cEnd] == 0)
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

	UObject* FindObjectById(uint32_t Id)
	{
		// we are on ue 4.21+
		int cStart = 0, cEnd = 0;
		int chunkIndex = 0, chunkSize = 0xFFFF, chunkPos;
		FUObjectItem* Object;

		this->NumChunks(&cStart, &cEnd);

		chunkIndex = Id / chunkSize;
		if (chunkSize * chunkIndex != 0 &&
			chunkSize * chunkIndex == Id)
		{
			chunkIndex--;
		}

		chunkPos = cStart + chunkIndex;
		if (chunkPos < cEnd)
		{
			Object = this->ObjectArray->Objects[chunkPos] + (Id - chunkSize * chunkIndex);

			if (!Object) { return NULL; }

			return Object->Object;
		}

		return nullptr;
	}
};

//Functions
inline UObject* FindObject(const wchar_t* Name)
{
	return StaticFindObject(nullptr, nullptr, Name, false);
}

inline UObject* FindObjectFromGObj(string Name)
{
	if (!GObjectArray) return nullptr;

	for (int i = 0; i < GObjectArray->ObjectCount; ++i)
	{
		UObject* object = GObjectArray->FindObjectById(i);
		if (object == nullptr)
		{
			continue;
		}
		std::string objectName = object->GetFullName();
		if (objectName.find(Name) != std::string::npos)
		{
			return object;
		}
	}
	return nullptr;
}

inline FKey GetKeyFromAction(string ActionName)
{
	static TArray<KeyMap> Settings = TArray<KeyMap>();

	if (auto TempSettings = &(PlayerController->Child(_("PlayerInput"))->Child(_("DesiredPlayerInputSettings"))); !IsBadReadPtr(TempSettings)) Settings = (*TempSettings)->Child<TArray<KeyMap>>(_("ActionMappings"));
	else if (auto TempSettings = PlayerController->Call<bool>(_("IsUsingGamepad")) ? &(PlayerController->Child(_("PlayerInput"))->Child(_("DesiredGamepadPlayerInputSettings"))) : &(PlayerController->Child(_("PlayerInput"))->Child(_("DesiredKBMPlayerInputSettings"))); !IsBadReadPtr(TempSettings)) Settings = (*TempSettings)->Child<TArray<KeyMap>>(_("ActionMappings"));

	for (KeyMap Setting : Settings)
	{
		if (Setting.Action.ToString() == ActionName)
			return Setting.Key;
	}
}