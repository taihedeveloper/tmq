/*
 * io_service_poll.h
 *
 *  Created on: Apr 27, 2015
 *      Author: root
 */

#ifndef IO_SERVICE_POLL_H_
#define IO_SERVICE_POLL_H_

#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/bind.hpp>

#include <boost/asio/ip/tcp.hpp>

#include <vector>

using boost::asio::ip::tcp;


class io_service_poll :public boost::noncopyable
{
public:
	explicit io_service_poll(std::size_t poll_size);
	virtual ~io_service_poll();
public:
	void start();
	void stop();
	void join();
	boost::asio::io_service& get_io_service();
private:
	typedef boost::shared_ptr<boost::asio::io_service>       io_service_sptr;
	typedef boost::shared_ptr<boost::asio::io_service::work> work_sptr;
	typedef boost::shared_ptr<boost::thread>                 thread_sptr;
private:
	std::vector<thread_sptr>     threads_;
	std::vector<io_service_sptr> io_services_;
	std::vector<work_sptr>       works_;

	std::size_t                  next_io_service_;
	boost::mutex                 mtx_;

};

#endif /* IO_SERVICE_POLL_H_ */
