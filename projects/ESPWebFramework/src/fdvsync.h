/*
# Created by Fabrizio Di Vittorio (fdivitto@gmail.com)
# Copyright (c) 2015 Fabrizio Di Vittorio.
# All rights reserved.

# GNU GPL LICENSE
#
# This module is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; latest version thereof,
# available at: <http://www.gnu.org/licenses/gpl.txt>.
#
# This module is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this module; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA
*/

#ifndef _FDVSYNC_H
#define _FDVSYNC_H


extern "C"
{
#include "esp_common.h"
#include "freertos/FreeRTOS.h"	
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "freertos/queue.h"
}



namespace fdv
{
	
	
	uint32_t millisISR();		
	uint32_t millis();
	uint32_t millisDiff(uint32_t time1, uint32_t time2);
	void DisableWatchDog();
	void EnableWatchDog();
	void EnableInterrupts();
	void DisableInterrupts();
	

	/////////////////////////////////////////////////////////////////////+
	/////////////////////////////////////////////////////////////////////+
	// Mutex
	// An FreeRTOS semaphore wrapper	
	//
	// Example:
	//
	// fdv::Mutex mutex;
	//
	// {
	//   fdv::MutexLock lock(mutex);
	// } <- mutex unlocked
	
	class Mutex
	{
		public:		
			Mutex()
				: m_handle(NULL)
			{
				vSemaphoreCreateBinary(m_handle);
			}
			
			~Mutex()
			{
				vSemaphoreDelete(m_handle);
			}
			
			bool lock(uint32_t msTimeOut = portMAX_DELAY)
			{
				return xSemaphoreTake(m_handle, msTimeOut / portTICK_RATE_MS);
			}
			
			bool lockFromISR()
			{
				signed portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
				return xSemaphoreTakeFromISR(m_handle, &xHigherPriorityTaskWoken);
			}
			
			void unlock()
			{
				xSemaphoreGive(m_handle);
			}
			
			void unlockFromISR()
			{
				signed portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
				xSemaphoreGiveFromISR(m_handle, &xHigherPriorityTaskWoken);				
			}
					
		private:		
			xSemaphoreHandle m_handle;
	};

	
	/////////////////////////////////////////////////////////////////////+
	/////////////////////////////////////////////////////////////////////+
	// MutexLock & MutexLockFromISR
	// Mutex automatic lock/unlock helper

	class MutexLock
	{
		public:
			MutexLock(Mutex* mutex, uint32_t msTimeOut = portMAX_DELAY)
			  : m_mutex(mutex)
			{
				m_acquired = m_mutex->lock(msTimeOut);
			}

			~MutexLock()
			{
				if (m_acquired)
					m_mutex->unlock();
			}
			
			operator bool()
			{
				return m_acquired;
			}

		private:
			Mutex* m_mutex;
			bool   m_acquired;
	};
	
	
	class MutexLockFromISR
	{
		public:
			MutexLockFromISR(Mutex* mutex)
			  : m_mutex(mutex)
			{
				m_acquired = m_mutex->lockFromISR();
			}

			~MutexLockFromISR()
			{
				if (m_acquired)
					m_mutex->unlockFromISR();
			}
			
			operator bool()
			{
				return m_acquired;
			}

		private:
			Mutex* m_mutex;
			bool   m_acquired;
	};

	
	/////////////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////
	// SoftTimeOut class
	// ex. SoftTimeOut(200)  <- after 200ms SoftTimeOut() returns true
	// note: not use inside ISR!!

	class SoftTimeOut
	{
		public:
			SoftTimeOut(uint32_t time)
				: m_timeOut(time), m_startTime(millis())
			{
			}

			operator bool()
			{
				return millisDiff(m_startTime, millis()) > m_timeOut;
			}
			
			void reset(uint32_t time)
			{
				m_timeOut   = time;
				m_startTime = millis();				
			}

		private:
			uint32_t m_timeOut;
			uint32_t m_startTime;
	};
	

	/////////////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////
	// Queue class
	// A wrapper over FreeRTOS queue
	
	template <typename T>
	class Queue
	{
		
		public:
		
			Queue(uint32_t queueLength)
			{
				m_handle = xQueueCreate(queueLength, sizeof(T));
			}
						
			~Queue()
			{
				vQueueDelete(m_handle);
			}
			
			bool send(T& item, uint32_t msTimeOut)
			{
				return xQueueSend(m_handle, &item, msTimeOut / portTICK_RATE_MS);
			}

			bool sendFromISR(T& item)
			{
				signed portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
				return xQueueSendFromISR(m_handle, &item, &xHigherPriorityTaskWoken);
			}
			
			bool receive(T* item, uint32_t msTimeOut)
			{
				return xQueueReceive(m_handle, item, msTimeOut / portTICK_RATE_MS);
			}
			
			bool peek(T* item, uint32_t msTimeOut)
			{
				return xQueuePeek(m_handle, item, msTimeOut / portTICK_RATE_MS);
			}			
			
			void clear()
			{
				xQueueReset(m_handle);
			}
			
			uint32_t available()
			{
				return uxQueueMessagesWaiting(m_handle);
			}
						
		private:
			
			xQueueHandle m_handle;
		
	};

	
}





#endif