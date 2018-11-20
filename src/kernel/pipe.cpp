#include "pipe.h"
#include <iostream>

CPipe::CPipe() : mRead_Index(0), mWrite_Index(0), mEmptyCount(BUFFER_SIZE), mFillCount(0) {

}


size_t CPipe::Write(const char *buffer, size_t buffer_size, size_t position) {
	size_t bytes_written = 0;
	
	for (size_t i = 0; i < buffer_size; i++) {
		mEmptyCount.Wait("WRITE WAIT"); // is buffer empty?
		
		//if (Get_Read_Count() == 0) {
		//	return bytes_written;
		//}

		mBuffer[mWrite_Index] = buffer[i];
		
		mWrite_Index = (mWrite_Index + 1) % BUFFER_SIZE;
		bytes_written++;
		mFillCount.Signal("WRITE SIGNAL"); // signaling we wrote one byte
	}

	return bytes_written;
}

size_t CPipe::Read(char *buffer, size_t buffer_size, size_t position) {
	size_t bytes_read = 0;

	for (size_t i = 0; i < buffer_size; i++) {
		mFillCount.Wait("WAIT READ"); // is there a byte to read?
		
		buffer[i] = mBuffer[mRead_Index];

		mRead_Index = (mRead_Index + 1) % BUFFER_SIZE;	
		bytes_read++;
		//std::cout << "MREAD: " << mRead_Index << ", MWRITE: " << mWrite_Index << std::endl;
		
		if (mRead_Index == mWrite_Index - 1) {
			return bytes_read;
		}
		mEmptyCount.Signal("SIGNAL READ");
	}
	std::cout << "penis?" << std::endl;
	return bytes_read;
}

bool CPipe::Is_Available_For_Write() {
	return true;
}

void CPipe::Close(const kiv_vfs::TFD_Attributes attrs) {

}