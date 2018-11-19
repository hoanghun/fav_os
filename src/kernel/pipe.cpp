#include "pipe.h"

CPipe::CPipe() : mRead_Index(0), mWrite_Index(0), mEmptyCount(BUFFER_SIZE), mFillCount(0) {

}

size_t CPipe::Write(const char *buffer, size_t buffer_size, size_t position) {
	size_t bytes_written = 0;
	
	for (size_t i = 0; i < buffer_size; i++) {
		mEmptyCount.Wait(); // is buffer empty?
		
		if (Get_Read_Count() == 0) {
			return bytes_written;
		}

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

		if (Get_Write_Count() == 0 && mRead_Index == mWrite_Index) {
			buffer[mRead_Index] = EOF;
			return ++bytes_read;
		}

		mEmptyCount.Signal();
	}
	
	return bytes_read;
}

bool CPipe::Is_Available_For_Write() {
	return true;
}

void CPipe::Close(const kiv_vfs::TFD_Attributes attrs) {

}