/*
	Written By Pradipna Nepal
	www.pradsprojects.com

	Copyright (C) 2013 Pradipna Nepal
	Please read COPYING.txt included along with this source code for more detail.
	If not included, see http://www.gnu.org/licenses/
*/

#include "stringManager.h"
using namespace std;

uint64 makeStringObject();
void destroyStringObject(uint64 ptr);
uint64 getStringSize(uint64 ptr);
uint64 getChar(uint64 ptr, uint64 index);
void setChar(uint64 ptr, uint64 c, uint64 index);
void appendCharArray(uint64 ptr, uint64 charArray);
void appendString(uint64 destPtr, uint64 srcPtr);
void appendInteger(uint64 ptr, uint64 integer);
void appendFloat(uint64 ptr, float32 f, uint32 p);
void clearString(uint64 ptr);
uint64 getCharArray(uint64 ptr);
uint64 compare(uint64 ptr1, uint64 ptr2);
void copyString(uint64 ptr1, uint64 ptr2);

string integerToString(uint64 i);
string floatToString(float32 f, uint32 precision);

void StringManager::decodeStringCommands(uint8 cmd, uint8 *ports) {
	switch (cmd) {
		case STRING_MANAGER_CREATE_OBJ:
			*(uint64*)&ports[PORT_STRING_OBJ] = makeStringObject();
			break;
		case STRING_MANAGER_DESTROY_OBJ:
			destroyStringObject(*(uint64*)&ports[PORT_STRING_OBJ]);
			break;
		case STRING_MANAGER_GET_SIZE:
			*(uint64*)&ports[PORT_STRING_IO_1] = getStringSize(*(uint64*)&ports[PORT_STRING_OBJ]);
			break;
		case STRING_MANAGER_GET_CHAR:
			*(uint64*)&ports[PORT_STRING_IO_1] = getChar(*(uint64*)&ports[PORT_STRING_OBJ], *(uint64*)&ports[PORT_STRING_IO_2]);
			break;
		case STRING_MANAGER_SET_CHAR:
			setChar(*(uint64*)&ports[PORT_STRING_OBJ], *(uint64*)&ports[PORT_STRING_IO_1], *(uint64*)&ports[PORT_STRING_IO_2]);
			break;
		case STRING_MANAGER_APPEND_CHAR_ARRAY:
			appendCharArray(*(uint64*)&ports[PORT_STRING_OBJ], *(uint64*)&ports[PORT_STRING_IO_1]);
			break;
		case STRING_MANAGER_APPEND_STRING:
			appendString(*(uint64*)&ports[PORT_STRING_OBJ], *(uint64*)&ports[PORT_STRING_IO_1]);
			break;
		case STRING_MANAGER_APPEND_INTEGER:
			appendInteger(*(uint64*)&ports[PORT_STRING_OBJ], *(uint64*)&ports[PORT_STRING_IO_1]);
			break;
		case STRING_MANAGER_APPEND_FLOAT:
			appendFloat(*(uint64*)&ports[PORT_STRING_OBJ], *(float32*)&ports[PORT_STRING_IO_1], ports[PORT_STRING_IO_2]);
			break;
		case STRING_MANAGER_CLEAR_STRING:
			clearString(*(uint64*)&ports[PORT_STRING_OBJ]);
			break;
		case STRING_MANAGER_GET_CHAR_ARRAY:
			*(uint64*)&ports[PORT_STRING_IO_1] = getCharArray(*(uint64*)&ports[PORT_STRING_OBJ]);
			break;
		case STRING_MANAGER_COMPARE:
			*(uint64*)&ports[PORT_STRING_IO_2] = compare(*(uint64*)&ports[PORT_STRING_OBJ], *(uint64*)&ports[PORT_STRING_IO_1]);
			break;
		case STRING_MANAGER_COPY_STR:
			copyString(*(uint64*)&ports[PORT_STRING_OBJ], *(uint64*)&ports[PORT_STRING_IO_1]);
	}
}

uint64 makeStringObject() {
	string *strObj = new (nothrow) string;
	*strObj = "";
	return (uint64)strObj;
}

void destroyStringObject(uint64 ptr) {
	delete (string*)ptr;
}

uint64 getStringSize(uint64 ptr) {
	return ((string*)ptr)->size();
}

uint64 getChar(uint64 ptr, uint64 index) {
	return (uint64)(*(string*)ptr)[index];
}

void setChar(uint64 ptr, uint64 c, uint64 index) {
	(*(string*)ptr)[index] = (char)c;
}

void appendCharArray(uint64 ptr, uint64 charArray) {
	*(string*)ptr += (char*)charArray;
}

void appendString(uint64 destPtr, uint64 srcPtr) {
	*(string*)destPtr += *(string*)srcPtr;
}

void appendInteger(uint64 ptr, uint64 integer) {
	*(string*)ptr += integerToString(integer);
}

void appendFloat(uint64 ptr, float32 f, uint32 p) {
	*(string*)ptr += floatToString(f, p);
}

void clearString(uint64 ptr) {
	*(string*)ptr = "";
}

string integerToString(uint64 i) {
	if (i == 0) {
		return "0";
	}

	string retStr = "";
	int digit;
	bool neg = false;
	
	if (i < 0) {
		neg = true;
		i = ~i + 1;
	}
	while (i != 0) {
		digit = i % 10;
		retStr.push_back((char)(digit + 48));
		i /= 10;
	}
	for (int start = 0, end = retStr.size() - 1; start < retStr.size() / 2; ++start, --end) {
		retStr[start] ^= retStr[end];
		retStr[end] ^= retStr[start];
		retStr[start] ^= retStr[end];
	}
	if (neg) retStr.insert(retStr.begin(), '-');

	return retStr;
}

string floatToString(float32 f, uint32 precision) {
	if (f == 0) {
		return "0";
	}
	
	string retStr = "";
	string intStr = "", fracStr = "", zeros = "";
	bool neg = false;
	if (precision > 6) {
		precision = 6;
	} else if (precision == 0) {
		precision = 1;
	}
	precision = pow(10.0, (double)precision); 
	int integral = f, fractional = (f - integral) * precision, digit;
	
	if (integral < 0) {
		integral = ~integral + 1;
		fractional = ~fractional + 1;
		neg = true;
	}
	
	while (integral != 0) {
		digit = integral % 10;
		intStr += (char)(digit + 48);
		integral /= 10;
	}
	precision /= 10;
	while (precision > fractional) {
		zeros += '0';
		precision /= 10;
	}
	while (fractional != 0) {
		digit = fractional % 10;
		fracStr += (char)(digit + 48);
		fractional /= 10;
	}
	for (int start = 0, end = intStr.size() - 1; start < intStr.size() / 2; ++start, --end) {
		intStr[start] ^= intStr[end];
		intStr[end] ^= intStr[start];
		intStr[start] ^= intStr[end];
	}
	for (int start = 0, end = fracStr.size() - 1; start < fracStr.size() / 2; ++start, --end) {
		fracStr[start] ^= fracStr[end];
		fracStr[end] ^= fracStr[start];
		fracStr[start] ^= fracStr[end];
	}
	if (neg) retStr = '-';
	retStr.append(intStr);
	if (fracStr.size() > 0) {
		retStr.push_back('.');
		retStr.append(zeros);
		retStr.append(fracStr);
	}

	return retStr;
}

uint64 getCharArray(uint64 ptr) {
	string *str = (string*)ptr;
	return (uint64)((string*)ptr)->c_str();
}

uint64 compare(uint64 ptr1, uint64 ptr2) {
	if ((*(string*)ptr1) == (*(string*)ptr2))
		return 1;
	else
		return 0;
}

void copyString(uint64 ptr1, uint64 ptr2) {
	*(string*)ptr1 = *(string*)ptr2;
}