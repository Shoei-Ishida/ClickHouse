#pragma once

#include <Poco/Event.h>
#include <Yandex/logger_useful.h>
#include <DB/Core/Types.h>
#include <thread>


namespace DB
{

class StorageReplicatedMergeTree;


/** Инициализирует сессию в ZK.
  * Выставляет эфемерные ноды. Выставляет нужные для обнаружения реплики значения нод.
  * Запускает участие в выборе лидера. Запускает все фоновые потоки.
  * Затем следит за тем, не истекла ли сессия. И если истекла - переинициализирует её.
  */
class ReplicatedMergeTreeRestartingThread
{
public:
	ReplicatedMergeTreeRestartingThread(StorageReplicatedMergeTree & storage_);

	~ReplicatedMergeTreeRestartingThread()
	{
		if (thread.joinable())
			thread.join();
	}

	void wakeup()
	{
		wakeup_event.set();
	}

	Poco::Event & getWakeupEvent()
	{
		return wakeup_event;
	}

	void stop()
	{
		need_stop = true;
		wakeup();
	}

private:
	StorageReplicatedMergeTree & storage;
	Logger * log;
	std::thread thread;
	Poco::Event wakeup_event;
	volatile bool need_stop = false;

	/// Случайные данные, которые мы записали в /replicas/me/is_active.
	String active_node_identifier;

	void run();

	/// Запустить или остановить фоновые потоки. Используется для частичной переинициализации при пересоздании сессии в ZooKeeper.
	bool tryStartup(); /// Возвращает false, если недоступен ZooKeeper.

	/// Отметить в ZooKeeper, что эта реплика сейчас активна.
	void activateReplica();

	void partialShutdown();

	/// Запретить запись в таблицу и завершить все фоновые потоки.
	void goReadOnlyPermanently();
};


}
