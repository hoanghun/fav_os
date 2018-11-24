#include "pipe.h"
#include <iostream>

CPipe::CPipe() : mRead_Index(0), mWrite_Index(0), mReader_Closed(false), mWriter_Closed(false)
	,mEmptyCount(BUFFER_SIZE), mFillCount(0) {

}


size_t CPipe::Write(const char *buffer, size_t buffer_size, size_t position) {
	size_t bytes_written = 0;
	
	for (size_t i = 0; i < buffer_size; i++) {
		mEmptyCount.Wait(); // is buffer empty?
		
		if (mReader_Closed) {
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

	if (mWriter_Closed && mRead_Index == mWrite_Index) {
		return 0;
	}

	for (size_t i = 0; i < buffer_size; i++) {
		mFillCount.Wait(); // is there a byte to read?

		if (mWriter_Closed) { // writer end closed need to check if there is anything to read
			if (mRead_Index == mWrite_Index) { // nothing to read
				buffer[i] = EOF; // mozna chyba
				return bytes_read;
			}
		}

		buffer[i] = mBuffer[mRead_Index];

		mRead_Index = (mRead_Index + 1) % BUFFER_SIZE;	
		bytes_read++;
		
		mEmptyCount.Signal();
	}
	
	return bytes_read;
}

bool CPipe::Is_Available_For_Write() {
	return true;
}

void CPipe::Close(const kiv_vfs::TFD_Attributes attrs) {
	switch (attrs) {
	case kiv_vfs::FD_ATTR_READ:
		mReader_Closed = true;
		mEmptyCount.Signal();
		break;
	case kiv_vfs::FD_ATTR_RW: // closing pipe end for writing
		mWriter_Closed = true;
		mFillCount.Signal(); // waking reader up, lets him pass through the semaphore to check condition
		break;
	default:
		break;
	}
}