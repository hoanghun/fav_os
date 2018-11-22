#include "pipe.h"
#include <iostream>

CPipe::CPipe() : mRead_Index(0), mWrite_Index(0), mEmptyCount(BUFFER_SIZE), mFillCount(0) {

}


size_t CPipe::Write(const char *buffer, size_t buffer_size, size_t position) {
	size_t bytes_written = 0;
	
	for (size_t i = 0; i < buffer_size; i++) {
		mEmptyCount.Wait(); // is buffer empty?

		mBuffer[mWrite_Index] = buffer[i];
		
		mWrite_Index = (mWrite_Index + 1) % BUFFER_SIZE;
		bytes_written++;
		mFillCount.Signal(); // signaling we wrote one byte
	}

	return bytes_written;
}

size_t CPipe::Read(char *buffer, size_t buffer_size, size_t position) {
	size_t bytes_read = 0;

	for (size_t i = 0; i < buffer_size; i++) {
		mFillCount.Wait(); // is there a byte to read?
		
		buffer[i] = mBuffer[mRead_Index];

		mRead_Index = (mRead_Index + 1) % BUFFER_SIZE;	
		bytes_read++;
		
		mEmptyCount.Signal();
		
		if (mRead_Index == mWrite_Index) {
			return bytes_read;
		}
	}
	
	return bytes_read;
}

bool CPipe::Is_Available_For_Write() {
	return true;
}

void CPipe::Close(const kiv_vfs::TFD_Attributes attrs) {

}