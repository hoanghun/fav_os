#include "process.h"

namespace kiv_os {
	namespace process {

		CProcess_Manager * CProcess_Manager::instance = NULL;

		CProcess_Manager::CProcess_Manager() {
		}

		CProcess_Manager::~CProcess_Manager() {

		}

		CProcess_Manager & CProcess_Manager::Get_Instance() {

			if (instance == NULL) {
				instance = new CProcess_Manager();
			}

			return *instance;

		}

	}
}