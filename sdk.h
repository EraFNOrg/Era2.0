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
	inline int Num() const
	{
		return count;
	};

	void Add(T InputData)
	{
		Data = (T*)Realloc(Data, sizeof(T) * (count + 1), 0);
		Data[count++] = InputData;
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

			auto Children = *(UObject**)(int64(Class) + offsets::Children);

			if (!Children) {
				Class = *(UObject**)(int64(Class) + offsets::SuperClass);
				continue;
			}

			while (true)
			{
				if (Children->Class->GetName() == _("StructProperty"))
				{
					auto StructChildren = *(UObject**)(int64(*(UObject**)(int64(Children) + 0x70)) + offsets::Children);

					while (true)
					{
						if (!StructChildren) break;

						if (StructChildren->GetName() == name) return  *(T*)(int64(this) + *(uint32*)(int64(Children) + 0x44) + *(uint32*)(int64(StructChildren) + 0x44));

						StructChildren = *(UObject**)(int64(StructChildren) + offsets::Next);
					}
				}

				if (Children->GetName() == name && Children->Class->GetName() == _("Function")) return *(T*)(&*Children);
			
				if (Children->GetName() == name) return *(T*)(int64(this) + *(uint32*)(int64(Children) + 0x44));

				Children = *(UObject**)(int64(Children) + offsets::Next);

				if (!Children) break;
			}

			Class = *(UObject**)(int64(Class) + offsets::SuperClass);
		}

		return *(T*)(0);
	}

	vector<int> GetFunctionChildrenOffset()
	{
		auto ReturnVector = vector<int32>();

		auto Children = *(UObject**)(int64(this) + offsets::Children);

		while (true)
		{
			if (!Children) break;

			ReturnVector.push_back(*(int*)(int64(Children) + 0x44));

			Children = *(UObject**)(int64(Children) + offsets::Next);
		}

		return ReturnVector;
	}

	template< typename T = int, int16 ReturnOffset = -1, typename ...Params >
	T Call(string name, Params... args)
	{
		auto Function = &(this->Child<UObject>(name));

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

//Functions
inline UObject* FindObject(const wchar_t* Name)
{
	auto ReturnValue = StaticFindObject(nullptr, nullptr, Name, false);
	
	if (ReturnValue) return ReturnValue;

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