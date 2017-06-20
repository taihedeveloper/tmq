/*
 * io_service_poll.cpp
 *
 *  Created on: Apr 27, 2015
 *      Author: root
 */

#include "io_service_poll.h"

io_service_poll::io_service_poll(std::size_t poll_size)
:next_io_service_(0)
{
	for(std::size_t i = 0; i < poll_size; i++)
	{
		io_service_sptr io_service(new boost::asio::io_service());
		work_sptr w(new boost::asio::io_service::work(*io_service));
		io_services_.push_back(io_service);
		works_.push_back(w);
	}
}

io_service_poll::~io_service_poll()
{
}

void io_service_poll::start()
{
	for(std::size_t i = 0; i < io_services_.size(); i++)
	{
		thread_sptr t(new boost::thread(boost::bind(&boost::asio::io_service::run, io_services_[i])));
		threads_.push_back(t);
	}
}

void io_service_poll::join()
{
	for(std::size_t i = 0; i < threads_.size(); i++)
	{
		threads_[i]->join();
	}
}

void io_service_poll::stop()
{
	for(std::size_t i = 0; i < threads_.size(); i++)
	{
		if(!io_services_[i]->stopped())
		{
			io_services_[i]->stop();
		}
	}
}

boost::asio::io_service& io_service_poll::get_io_service()
{
	boost::mutex::scoped_lock lock(mtx_);
	boost::asio::io_service& io_serivce = *io_services_[next_io_service_];
	next_io_service_++;

	if(next_io_service_ == io_services_.size())
	{
		next_io_service_ = 0;
	}
	return io_serivce;
}
