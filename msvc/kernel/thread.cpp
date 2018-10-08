#include "thread.h"

namespace kiv_os {
	namespace thread {

		CThread_Manager * CThread_Manager::instance = NULL;

		CThread_Manager::CThread_Manager() {
		}

		CThread_Manager::~CThread_Manager() {
		}

		CThread_Manager & CThread_Manager::Get_Instance() {

			if (instance == NULL) {
				instance = new CThread_Manager();
			}

			return *instance;

		}
	}
}