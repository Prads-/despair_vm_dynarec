/*
	Written By Pradipna Nepal
	www.pradsprojects.com

	Copyright (C) 2013 Pradipna Nepal
	Please read COPYING.txt included along with this source code for more detail.
	If not included, see http://www.gnu.org/licenses/
*/

#include "fileManager.h"
#include "despairHeader.h"
using namespace std;
using namespace DespairHeader;

uint64 makeFileObject();
void destroyFileObject(uint64 ptr);
uint64 getFileSize(uint64 ptr);
void setFileOffset(uint64 fileObjPtr, uint64 offset);
bool openFile(uint64 fileObjPtr, uint64 pathPtr, bool asBinary, const string *exeFolderPath);
void closeFile(uint64 ptr);
void readFile(uint64 fileObjPtr, uint64 bufferPtr, uint64 size, bool asBinary);
void writeFile(uint64 fileObjPtr, uint64 bufferPtr, uint64 size);
void writeFile(uint64 fileObjPtr, uint64 stringPtr);

void FileManager::decodeFileCommands(uint8 cmd, uint8 *ports, const string *exeFolderPath) {
	switch (cmd) {
		case FILE_MANAGER_CREATE_OBJ:
			*(uint64*)&ports[PORT_FILE_STREAM_OBJ] = makeFileObject();
			break;
		case FILE_MANAGER_DESTROY_OBJ:
			destroyFileObject(*(uint64*)&ports[PORT_FILE_STREAM_OBJ]);
			break;
		case FILE_MANAGER_GET_SIZE:
			*(uint64*)&ports[PORT_FILE_IO_1] = getFileSize(*(uint64*)&ports[PORT_FILE_STREAM_OBJ]);
			break;
		case FILE_MANAGER_SET_OFFSET:
			setFileOffset(*(uint64*)&ports[PORT_FILE_STREAM_OBJ], *(uint64*)&ports[PORT_FILE_IO_1]);
			break;
		case FILE_MANAGER_OPEN_TEXT:
			if (openFile(*(uint64*)&ports[PORT_FILE_STREAM_OBJ], *(uint64*)&ports[PORT_FILE_IO_1], false, exeFolderPath)) {
				*(uint64*)&ports[PORT_FILE_IO_2] = 1;
			} else {
				*(uint64*)&ports[PORT_FILE_IO_2] = 0;
			}
			break;
		case FILE_MANAGER_OPEN_BINARY:
			if (openFile(*(uint64*)&ports[PORT_FILE_STREAM_OBJ], *(uint64*)&ports[PORT_FILE_IO_1], true, exeFolderPath)) {
				*(uint64*)&ports[PORT_FILE_IO_2] = 1;
			} else {
				*(uint64*)&ports[PORT_FILE_IO_2] = 0;
			}
			break;
		case FILE_MANAGER_CLOSE:
			closeFile(*(uint64*)&ports[PORT_FILE_STREAM_OBJ]);
			break;
		case FILE_MANAGER_READ_BINARY:
			readFile(*(uint64*)&ports[PORT_FILE_STREAM_OBJ], *(uint64*)&ports[PORT_FILE_IO_1], *(uint64*)&ports[PORT_FILE_IO_2], true);
			break;
		case FILE_MANAGER_READ_TEXT:
			readFile(*(uint64*)&ports[PORT_FILE_STREAM_OBJ], *(uint64*)&ports[PORT_FILE_IO_1], *(uint64*)&ports[PORT_FILE_IO_2], false);
			break;
		case FILE_MANAGER_WRITE_BINARY:
			writeFile(*(uint64*)&ports[PORT_FILE_STREAM_OBJ], *(uint64*)&ports[PORT_FILE_IO_1], *(uint64*)&ports[PORT_FILE_IO_2]);
			break;
		case FILE_MANAGER_WRITE_TEXT:
			writeFile(*(uint64*)&ports[PORT_FILE_STREAM_OBJ], *(uint64*)&ports[PORT_FILE_IO_1]);
	}
}

uint64 makeFileObject() {
	fstream *fileObject = new (nothrow) fstream;
	return (uint64)fileObject;
}

void destroyFileObject(uint64 ptr) {
	delete (fstream*)ptr;
}

uint64 getFileSize(uint64 ptr) {
	fstream *fileObject = (fstream*)ptr;
	uint64 currentPos = fileObject->tellg();
	fileObject->seekg(0, ios::end);
	uint64 size = fileObject->tellg();
	fileObject->seekg(currentPos, ios::beg);
	return size;
}

void setFileOffset(uint64 fileObjPtr, uint64 offset) {
	((fstream*)fileObjPtr)->seekg(offset, ios::beg);
}
bool openFile(uint64 fileObjPtr, uint64 pathPtr, bool asBinary, const string *exeFolderPath) {
	fstream *fileObject = (fstream*)fileObjPtr;
	string path = *(string*)pathPtr;
	
	if (path.size() == 0) return false;
	if ((path)[0] == '/') {
		path.erase(path.begin());
		path = *exeFolderPath + path;
	}
	if (asBinary) {
		fileObject->open(path.c_str(), fstream::in | fstream::binary | fstream::out);
	} else {
		fileObject->open(path.c_str(), fstream::in | fstream::out);
	}

	return fileObject->is_open();
}

void closeFile(uint64 ptr) {
	((fstream*)ptr)->close();
}

void readFile(uint64 fileObjPtr, uint64 bufferPtr, uint64 size, bool asBinary) {
	fstream *fileObject = (fstream*)fileObjPtr;
	
	if (asBinary) {
		if (size == 0) return;
		uint8 *buffer = (uint8*)bufferPtr;
		fileObject->read((char*)buffer, size);
	} else {
		string *buffer = (string*)bufferPtr;
		string tLine;

		if (size == 0) {
			while (getline(*fileObject, tLine)) {
				*buffer += tLine;
			}
		} else {
			while (size-- != 0) {
				if (!getline(*fileObject, tLine)) break;
				*buffer += tLine;
			}
		}
	}

}

void writeFile(uint64 fileObjPtr, uint64 bufferPtr, uint64 size) {
	fstream *fileObject = (fstream*)fileObjPtr;
	uint8 *buffer = (uint8*)bufferPtr;
	fileObject->write((char*)buffer, size);
}

void writeFile(uint64 fileObjPtr, uint64 stringPtr) {
	fstream *fileObject = (fstream*)fileObjPtr;
	string *buffer = (string*)stringPtr;
	*fileObject << *buffer;
}