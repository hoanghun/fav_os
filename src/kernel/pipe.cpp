#include "pipe.h"
#include <iostream>

CPipe::CPipe() : mRead_Index(0), mWrite_Index(0), mReader_Closed(false), mWriter_Closed(false)
	,mEmptyCount(BUFFER_SIZE), mFillCount(0) {

}


kiv_os::NOS_Error CPipe::Write(const char *buffer, size_t buffer_size, size_t position, size_t &written) {
	written = 0;
	
	if (mReader_Closed) {
		return kiv_os::NOS_Error::Unknown_Error; // TODO spravnej error?
	}

	for (size_t i = 0; i < buffer_size; i++) {
		mEmptyCount.Wait(); // is buffer empty?
		
		if (mReader_Closed) {
			return kiv_os::NOS_Error::Success;
		}

		mBuffer[mWrite_Index] = buffer[i];
		
		mWrite_Index = (mWrite_Index + 1) % BUFFER_SIZE;
		written++;
		mFillCount.Signal(); // signaling we wrote one byte
	}

	return kiv_os::NOS_Error::Success;
}

kiv_os::NOS_Error CPipe::Read(char *buffer, size_t buffer_size, size_t position, size_t &read) {
	read = 0;

	if (mWriter_Closed && mRead_Index == mWrite_Index) {
		return kiv_os::NOS_Error::Success;
	}

	for (size_t i = 0; i < buffer_size; i++) {
		mFillCount.Wait(); // is there a byte to read?

		if (mWriter_Closed) { // writer end closed need to check if there is anything to read
			if (mRead_Index == mWrite_Index) { // nothing to read
				buffer[i] = EOF; // mozna chyba
				return kiv_os::NOS_Error::Success;
			}
		}

		buffer[i] = mBuffer[mRead_Index];

		mRead_Index = (mRead_Index + 1) % BUFFER_SIZE;	
		read++;
		
		mEmptyCount.Signal();
	}
	
	return kiv_os::NOS_Error::Success;
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
#ifdef DEBUG_
		printf("debug: closing pipe defaultly\n");
#endif
		mReader_Closed = true;
		mEmptyCount.Signal();
		mWriter_Closed = true;
		mFillCount.Signal(); // waking reader up, lets him pass through the semaphore to check condition
		break;
	}
}